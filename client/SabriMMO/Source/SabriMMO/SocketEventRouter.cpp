// SocketEventRouter.cpp — Multi-handler dispatch for Socket.io events.

#include "SocketEventRouter.h"
#include "SocketIONative.h"

DEFINE_LOG_CATEGORY_STATIC(LogSocketRouter, Log, All);

uint32 USocketEventRouter::RegisterHandler(
	const FString& EventName,
	UObject* Owner,
	TFunction<void(const TSharedPtr<FJsonValue>&)> Handler)
{
	uint32 Id = NextHandleId++;

	TSharedPtr<FEntry>& EntryRef = HandlerMap.FindOrAdd(EventName);
	bool bFirstHandler = false;

	if (!EntryRef.IsValid())
	{
		EntryRef = MakeShared<FEntry>();
		bFirstHandler = true;
	}

	FHandler H;
	H.HandleId = Id;
	H.Owner = Owner;
	H.Callback = MoveTemp(Handler);
	EntryRef->Handlers.Add(MoveTemp(H));

	// If this is the first handler for this event and we have a native client,
	// bind the native listener now
	if (bFirstHandler && CachedNative.IsValid())
	{
		BindNativeEvent(EventName, EntryRef);
	}

	UE_LOG(LogSocketRouter, Verbose, TEXT("RegisterHandler: %s (handle=%u, owner=%s, total=%d)"),
		*EventName, Id, Owner ? *Owner->GetName() : TEXT("null"), EntryRef->Handlers.Num());

	return Id;
}

void USocketEventRouter::UnregisterAllForOwner(UObject* Owner)
{
	if (!Owner) return;

	int32 RemovedCount = 0;

	for (auto& Pair : HandlerMap)
	{
		TSharedPtr<FEntry>& Entry = Pair.Value;
		if (!Entry.IsValid()) continue;

		int32 Before = Entry->Handlers.Num();
		Entry->Handlers.RemoveAll([Owner](const FHandler& H)
		{
			return !H.Owner.IsValid() || H.Owner.Get() == Owner;
		});
		RemovedCount += Before - Entry->Handlers.Num();
	}

	if (RemovedCount > 0)
	{
		UE_LOG(LogSocketRouter, Log, TEXT("UnregisterAllForOwner: %s — removed %d handlers"),
			*Owner->GetName(), RemovedCount);
	}
}

void USocketEventRouter::UnregisterHandler(uint32 HandleId)
{
	for (auto& Pair : HandlerMap)
	{
		TSharedPtr<FEntry>& Entry = Pair.Value;
		if (!Entry.IsValid()) continue;

		int32 Idx = Entry->Handlers.IndexOfByPredicate([HandleId](const FHandler& H)
		{
			return H.HandleId == HandleId;
		});

		if (Idx != INDEX_NONE)
		{
			Entry->Handlers.RemoveAt(Idx);
			UE_LOG(LogSocketRouter, Verbose, TEXT("UnregisterHandler: handle=%u from event %s"), HandleId, *Pair.Key);
			return;
		}
	}
}

void USocketEventRouter::BindToNativeClient(TSharedPtr<FSocketIONative> InNativeClient)
{
	CachedNative = InNativeClient;

	if (!CachedNative.IsValid())
	{
		UE_LOG(LogSocketRouter, Warning, TEXT("BindToNativeClient called with null client"));
		return;
	}

	// Bind native listeners for all already-registered events
	RebindAllEvents();

	UE_LOG(LogSocketRouter, Log, TEXT("BindToNativeClient — bound %d event names"), HandlerMap.Num());
}

void USocketEventRouter::UnbindFromNativeClient()
{
	CachedNative.Reset();
	UE_LOG(LogSocketRouter, Log, TEXT("UnbindFromNativeClient — detached from native client"));
}

void USocketEventRouter::RebindAllEvents()
{
	if (!CachedNative.IsValid()) return;

	for (auto& Pair : HandlerMap)
	{
		if (Pair.Value.IsValid() && Pair.Value->Handlers.Num() > 0)
		{
			BindNativeEvent(Pair.Key, Pair.Value);
		}
	}
}

bool USocketEventRouter::HasHandlersFor(const FString& EventName) const
{
	const TSharedPtr<FEntry>* Found = HandlerMap.Find(EventName);
	return Found && Found->IsValid() && (*Found)->Handlers.Num() > 0;
}

void USocketEventRouter::BindNativeEvent(const FString& EventName, TSharedPtr<FEntry> Entry)
{
	if (!CachedNative.IsValid() || !Entry.IsValid()) return;

	// Capture the TSharedPtr<FEntry> — this remains stable even if the TArray reallocates
	TSharedPtr<FEntry> CapturedEntry = Entry;

	CachedNative->OnEvent(EventName,
		[CapturedEntry](const FString& Event, const TSharedPtr<FJsonValue>& Message)
		{
			if (!CapturedEntry.IsValid()) return;

			// Iterate a copy of handlers in case callbacks modify the list
			TArray<FHandler> HandlersCopy = CapturedEntry->Handlers;
			for (const FHandler& H : HandlersCopy)
			{
				// Skip handlers whose owner has been destroyed
				if (!H.Owner.IsValid()) continue;

				if (H.Callback)
				{
					H.Callback(Message);
				}
			}
		},
		TEXT("/"),
		ESIOThreadOverrideOption::USE_GAME_THREAD
	);

	UE_LOG(LogSocketRouter, Verbose, TEXT("BindNativeEvent: %s (%d handlers)"),
		*EventName, Entry->Handlers.Num());
}
