// PartySubsystem.cpp — Manages party state and socket events for the RO Classic party window.
// Registers 8 socket event handlers via persistent EventRouter. Provides emit helpers
// for party actions (create, invite, kick, leave, delegate, exp share, chat).

#include "PartySubsystem.h"
#include "SPartyWidget.h"
#include "ChatSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogParty, Log, All);

// ============================================================================
// Helper: Parse a single FPartyMember from a JSON object
// ============================================================================

static FPartyMember ParseMemberFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FPartyMember M;
	if (!Obj.IsValid()) return M;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	M.CharacterId = (int32)CharIdD;

	Obj->TryGetStringField(TEXT("characterName"), M.CharacterName);
	Obj->TryGetStringField(TEXT("jobClass"), M.JobClass);

	double LevelD = 0;
	Obj->TryGetNumberField(TEXT("level"), LevelD);
	M.Level = (int32)LevelD;

	Obj->TryGetStringField(TEXT("map"), M.MapName);

	double HPD = 0, MaxHPD = 1;
	Obj->TryGetNumberField(TEXT("hp"), HPD);
	Obj->TryGetNumberField(TEXT("maxHp"), MaxHPD);
	M.HP = (int32)HPD;
	M.MaxHP = FMath::Max((int32)MaxHPD, 1);

	double SPD = 0, MaxSPD = 1;
	Obj->TryGetNumberField(TEXT("sp"), SPD);
	Obj->TryGetNumberField(TEXT("maxSp"), MaxSPD);
	M.SP = (int32)SPD;
	M.MaxSP = FMath::Max((int32)MaxSPD, 1);

	Obj->TryGetBoolField(TEXT("online"), M.bIsOnline);
	Obj->TryGetBoolField(TEXT("isLeader"), M.bIsLeader);

	return M;
}

// ============================================================================
// Lifecycle
// ============================================================================

bool UPartySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UPartySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("party:update"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePartyUpdate(D); });
		Router->RegisterHandler(TEXT("party:member_joined"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleMemberJoined(D); });
		Router->RegisterHandler(TEXT("party:member_left"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleMemberLeft(D); });
		Router->RegisterHandler(TEXT("party:member_update"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleMemberUpdate(D); });
		Router->RegisterHandler(TEXT("party:member_offline"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleMemberOffline(D); });
		Router->RegisterHandler(TEXT("party:dissolved"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePartyDissolved(D); });
		Router->RegisterHandler(TEXT("party:invite_received"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleInviteReceived(D); });
		Router->RegisterHandler(TEXT("party:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePartyError(D); });
	}

	// Only show widget when socket is connected (game level, not login screen)
	if (GI->IsSocketConnected())
	{
		ShowWidget();

		// Re-request party data — the party:update from player:join may have
		// arrived before this subsystem registered its handlers
		GI->EmitSocketEvent(TEXT("party:load"), MakeShared<FJsonObject>());
	}

	UE_LOG(LogParty, Log, TEXT("[Party] Events registered via EventRouter. CharId=%d"), LocalCharacterId);
}

void UPartySubsystem::Deinitialize()
{
	if (bWidgetAdded)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UGameViewportClient* ViewportClient = World->GetGameViewport();
			if (ViewportClient && ViewportOverlay.IsValid())
			{
				ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
			}
		}
		bWidgetAdded = false;
	}

	Widget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	Members.Empty();

	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}
	}

	Super::Deinitialize();
}

// ============================================================================
// Widget Management
// ============================================================================

void UPartySubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	Widget = SNew(SPartyWidget).Subsystem(this);

	// Upper-right area of screen (RO Classic party window position)
	// Must use HAlign_Left so the widget's render transform positions from top-left origin
	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			Widget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 12); // Z=12
	bWidgetAdded = true;

	UE_LOG(LogParty, Log, TEXT("[Party] Widget added to viewport (Z=12)."));
}

void UPartySubsystem::HideWidget()
{
	if (!bWidgetAdded) return;
	UWorld* World = GetWorld();
	UGameViewportClient* ViewportClient = World ? World->GetGameViewport() : nullptr;
	if (ViewportClient && ViewportOverlay.IsValid())
	{
		ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
	}
	bWidgetAdded = false;
}

void UPartySubsystem::ToggleWidget()
{
	if (bWidgetAdded)
	{
		HideWidget();
	}
	else
	{
		ShowWidget();
	}
}

// ============================================================================
// Event Handlers
// ============================================================================

void UPartySubsystem::HandlePartyUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse party metadata
	double PartyIdD = 0;
	Obj->TryGetNumberField(TEXT("partyId"), PartyIdD);
	PartyId = (int32)PartyIdD;

	Obj->TryGetStringField(TEXT("name"), PartyName);

	double LeaderIdD = 0;
	Obj->TryGetNumberField(TEXT("leaderId"), LeaderIdD);
	LeaderId = (int32)LeaderIdD;

	Obj->TryGetStringField(TEXT("expShare"), ExpShare);

	// Parse members array
	Members.Empty();
	const TArray<TSharedPtr<FJsonValue>>* MembersArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("members"), MembersArr) && MembersArr)
	{
		for (const TSharedPtr<FJsonValue>& MemberVal : *MembersArr)
		{
			const TSharedPtr<FJsonObject>* MemberObjPtr = nullptr;
			if (MemberVal.IsValid() && MemberVal->TryGetObject(MemberObjPtr) && MemberObjPtr)
			{
				FPartyMember M = ParseMemberFromJson(*MemberObjPtr);
				Members.Add(MoveTemp(M));
			}
		}
	}

	bInParty = true;
	++DataVersion;

	UE_LOG(LogParty, Log, TEXT("[Party] Update: id=%d name='%s' leader=%d members=%d expShare=%s"),
		PartyId, *PartyName, LeaderId, Members.Num(), *ExpShare);
}

void UPartySubsystem::HandleMemberJoined(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FPartyMember M = ParseMemberFromJson(Obj);

	// Avoid duplicates
	Members.RemoveAll([&M](const FPartyMember& Existing) {
		return Existing.CharacterId == M.CharacterId;
	});
	Members.Add(MoveTemp(M));
	++DataVersion;

	UE_LOG(LogParty, Log, TEXT("[Party] Member joined: %s (id=%d)"),
		*Members.Last().CharacterName, Members.Last().CharacterId);
}

void UPartySubsystem::HandleMemberLeft(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;

	int32 Removed = Members.RemoveAll([CharId](const FPartyMember& M) {
		return M.CharacterId == CharId;
	});
	++DataVersion;

	UE_LOG(LogParty, Log, TEXT("[Party] Member left: charId=%d (removed=%d)"), CharId, Removed);

	// If local player was removed, clear party state
	if (CharId == LocalCharacterId)
	{
		PartyId = 0;
		PartyName.Empty();
		LeaderId = 0;
		ExpShare.Empty();
		Members.Empty();
		bInParty = false;
		++DataVersion;
		UE_LOG(LogParty, Log, TEXT("[Party] Local player removed from party."));
	}
}

void UPartySubsystem::HandleMemberUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;

	for (FPartyMember& M : Members)
	{
		if (M.CharacterId == CharId)
		{
			double HPD = 0, MaxHPD = 0;
			if (Obj->TryGetNumberField(TEXT("hp"), HPD))
			{
				M.HP = (int32)HPD;
			}
			if (Obj->TryGetNumberField(TEXT("maxHp"), MaxHPD))
			{
				M.MaxHP = FMath::Max((int32)MaxHPD, 1);
			}

			double SPD = 0, MaxSPD = 0;
			if (Obj->TryGetNumberField(TEXT("sp"), SPD))
			{
				M.SP = (int32)SPD;
			}
			if (Obj->TryGetNumberField(TEXT("maxSp"), MaxSPD))
			{
				M.MaxSP = FMath::Max((int32)MaxSPD, 1);
			}

			FString MapStr;
			if (Obj->TryGetStringField(TEXT("map"), MapStr))
			{
				M.MapName = MapStr;
			}

			bool bOnline = true;
			if (Obj->TryGetBoolField(TEXT("online"), bOnline))
			{
				M.bIsOnline = bOnline;
			}

			++DataVersion;
			break;
		}
	}
}

void UPartySubsystem::HandleMemberOffline(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;

	for (FPartyMember& M : Members)
	{
		if (M.CharacterId == CharId)
		{
			M.bIsOnline = false;
			++DataVersion;
			UE_LOG(LogParty, Log, TEXT("[Party] Member offline: %s (id=%d)"), *M.CharacterName, CharId);
			break;
		}
	}
}

void UPartySubsystem::HandlePartyDissolved(const TSharedPtr<FJsonValue>& Data)
{
	UE_LOG(LogParty, Log, TEXT("[Party] Party dissolved (id=%d, name='%s')."), PartyId, *PartyName);

	PartyId = 0;
	PartyName.Empty();
	LeaderId = 0;
	ExpShare.Empty();
	Members.Empty();
	bInParty = false;
	++DataVersion;
}

void UPartySubsystem::HandleInviteReceived(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double InvPartyIdD = 0;
	Obj->TryGetNumberField(TEXT("partyId"), InvPartyIdD);
	PendingInvitePartyId = (int32)InvPartyIdD;

	Obj->TryGetStringField(TEXT("partyName"), PendingInvitePartyName);
	Obj->TryGetStringField(TEXT("inviterName"), PendingInviterName);

	bHasPendingInvite = true;
	++DataVersion;

	UE_LOG(LogParty, Log, TEXT("[Party] Invite received: party='%s' from '%s' (partyId=%d)"),
		*PendingInvitePartyName, *PendingInviterName, PendingInvitePartyId);
}

void UPartySubsystem::HandlePartyError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString ErrorMsg;
	Obj->TryGetStringField(TEXT("message"), ErrorMsg);

	UE_LOG(LogParty, Warning, TEXT("[Party] Error from server: %s"), *ErrorMsg);

	// Show error in chat so the player can see it
	if (UWorld* World = GetWorld())
	{
		if (UChatSubsystem* Chat = World->GetSubsystem<UChatSubsystem>())
		{
			Chat->AddCombatLogMessage(FString::Printf(TEXT("[Party] %s"), *ErrorMsg));
		}
	}
}

// ============================================================================
// Emit Helpers (called by SPartyWidget / context menus)
// ============================================================================

void UPartySubsystem::CreateParty(const FString& Name, bool bPartyShare)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("name"), Name);
	Payload->SetStringField(TEXT("expShare"), bPartyShare ? TEXT("even_share") : TEXT("each_take"));

	GI->EmitSocketEvent(TEXT("party:create"), Payload);
	UE_LOG(LogParty, Log, TEXT("[Party] Create request: name='%s' share=%s"), *Name, bPartyShare ? TEXT("even") : TEXT("each"));
}

void UPartySubsystem::InvitePlayer(const FString& TargetName)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("targetName"), TargetName);

	GI->EmitSocketEvent(TEXT("party:invite"), Payload);
	UE_LOG(LogParty, Log, TEXT("[Party] Invite sent to: %s"), *TargetName);
}

void UPartySubsystem::RespondToInvite(bool bAccept)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("partyId"), PendingInvitePartyId);
	Payload->SetBoolField(TEXT("accept"), bAccept);

	GI->EmitSocketEvent(TEXT("party:invite_respond"), Payload);

	UE_LOG(LogParty, Log, TEXT("[Party] Invite response: %s (partyId=%d)"),
		bAccept ? TEXT("accepted") : TEXT("declined"), PendingInvitePartyId);

	// Clear pending invite state
	PendingInvitePartyId = 0;
	PendingInvitePartyName.Empty();
	PendingInviterName.Empty();
	bHasPendingInvite = false;
	++DataVersion;
}

void UPartySubsystem::LeaveParty()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	GI->EmitSocketEvent(TEXT("party:leave"), TEXT("{}"));
	UE_LOG(LogParty, Log, TEXT("[Party] Leave request sent."));
}

void UPartySubsystem::KickMember(int32 TargetCharId)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("targetCharacterId"), TargetCharId);

	GI->EmitSocketEvent(TEXT("party:kick"), Payload);
	UE_LOG(LogParty, Log, TEXT("[Party] Kick request: charId=%d"), TargetCharId);
}

void UPartySubsystem::DelegateLeader(int32 TargetCharId)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("targetCharacterId"), TargetCharId);

	GI->EmitSocketEvent(TEXT("party:delegate_leader"), Payload);
	UE_LOG(LogParty, Log, TEXT("[Party] Delegate leader request: charId=%d"), TargetCharId);
}

void UPartySubsystem::ChangeExpShare(const FString& Mode)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("mode"), Mode);

	GI->EmitSocketEvent(TEXT("party:change_exp_share"), Payload);
	UE_LOG(LogParty, Log, TEXT("[Party] Change exp share: %s"), *Mode);
}

void UPartySubsystem::SendPartyChat(const FString& Message)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("message"), Message);

	GI->EmitSocketEvent(TEXT("party:chat"), Payload);
	UE_LOG(LogParty, Log, TEXT("[Party] Chat: %s"), *Message);
}

// ============================================================================
// Query
// ============================================================================

bool UPartySubsystem::IsLeader() const
{
	return bInParty && LeaderId == LocalCharacterId;
}
