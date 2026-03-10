// SocketEventRouter.h — Multi-handler dispatch layer for Socket.io events.
// FSocketIONative::OnEvent() replaces the previous handler for the same event name.
// This router allows multiple subsystems to register handlers for the same event.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonValue.h"
#include "SocketEventRouter.generated.h"

class FSocketIONative;

UCLASS()
class SABRIMMO_API USocketEventRouter : public UObject
{
	GENERATED_BODY()

public:
	// Register a handler for an event. Returns a unique handle ID for optional targeted removal.
	// Owner is tracked so all handlers for a destroyed subsystem can be bulk-removed.
	uint32 RegisterHandler(
		const FString& EventName,
		UObject* Owner,
		TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);

	// Remove all handlers registered by Owner (call in subsystem Deinitialize).
	void UnregisterAllForOwner(UObject* Owner);

	// Remove a single handler by its handle ID.
	void UnregisterHandler(uint32 HandleId);

	// Bind this router to a live FSocketIONative client — sets up native listeners.
	void BindToNativeClient(TSharedPtr<FSocketIONative> InNativeClient);

	// Unbind from the native client (called on disconnect).
	void UnbindFromNativeClient();

	// Re-bind all registered events to the native client (e.g. after reconnect).
	void RebindAllEvents();

	// Check if any handlers are registered for an event.
	bool HasHandlersFor(const FString& EventName) const;

private:
	struct FHandler
	{
		uint32 HandleId;
		TWeakObjectPtr<UObject> Owner;
		TFunction<void(const TSharedPtr<FJsonValue>&)> Callback;
	};

	// Shared entry per event — TSharedPtr so lambda captures remain stable
	struct FEntry
	{
		TArray<FHandler> Handlers;
	};

	// Map from event name to handler list
	TMap<FString, TSharedPtr<FEntry>> HandlerMap;

	// The native Socket.io client we dispatch from
	TSharedPtr<FSocketIONative> CachedNative;

	// Monotonic counter for unique handle IDs
	uint32 NextHandleId = 1;

	// Bind a single event name to the native client's OnEvent
	void BindNativeEvent(const FString& EventName, TSharedPtr<FEntry> Entry);
};
