// PartySubsystem.h — UWorldSubsystem that manages party state and socket events
// for the RO Classic party window. Tracks party membership, handles invites,
// and provides emit helpers for party actions (create/invite/kick/leave/etc.).
// Registers Socket.io event handlers via the persistent EventRouter:
// party:update, party:member_joined, party:member_left, party:member_update,
// party:member_offline, party:dissolved, party:invite_received, party:error.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "PartySubsystem.generated.h"

class SPartyWidget;

// Party member data (matches server party:update payload)
struct FPartyMember
{
	int32 CharacterId = 0;
	FString CharacterName;
	FString JobClass;
	int32 Level = 0;
	FString MapName;
	int32 HP = 0;
	int32 MaxHP = 1;
	int32 SP = 0;
	int32 MaxSP = 1;
	bool bIsOnline = true;
	bool bIsLeader = false;
};

UCLASS()
class SABRIMMO_API UPartySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- public state (read by SPartyWidget via Tick version check) ----
	int32 PartyId = 0;
	FString PartyName;
	int32 LeaderId = 0;
	FString ExpShare;          // "each_take" or "even_share"
	TArray<FPartyMember> Members;
	uint32 DataVersion = 0;
	bool bInParty = false;

	// Pending invite state
	int32 PendingInvitePartyId = 0;
	FString PendingInvitePartyName;
	FString PendingInviterName;
	bool bHasPendingInvite = false;

	// ---- public API (called by widget button/context menu actions) ----
	void CreateParty(const FString& Name, bool bPartyShare = false);
	void InvitePlayer(const FString& TargetName);
	void RespondToInvite(bool bAccept);
	void LeaveParty();
	void KickMember(int32 TargetCharId);
	void DelegateLeader(int32 TargetCharId);
	void ChangeExpShare(const FString& Mode);
	void SendPartyChat(const FString& Message);
	void ShowWidget();
	void HideWidget();
	void ToggleWidget();

	// Helper: check if local player is the party leader
	bool IsLeader() const;

private:
	// ---- event handlers ----
	void HandlePartyUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleMemberJoined(const TSharedPtr<FJsonValue>& Data);
	void HandleMemberLeft(const TSharedPtr<FJsonValue>& Data);
	void HandleMemberUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleMemberOffline(const TSharedPtr<FJsonValue>& Data);
	void HandlePartyDissolved(const TSharedPtr<FJsonValue>& Data);
	void HandleInviteReceived(const TSharedPtr<FJsonValue>& Data);
	void HandlePartyError(const TSharedPtr<FJsonValue>& Data);

	// ---- state ----
	TSharedPtr<SPartyWidget> Widget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
	bool bWidgetAdded = false;
	int32 LocalCharacterId = 0;
};
