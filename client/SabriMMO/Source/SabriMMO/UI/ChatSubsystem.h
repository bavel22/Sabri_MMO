// ChatSubsystem.h — UWorldSubsystem that owns the chat message log,
// handles chat:receive events, and manages the SChatWidget lifecycle.
// Phase E of BP_SocketManager bridge migration — replaces OnChatReceived BP bridge.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "ChatSubsystem.generated.h"

class SChatWidget;

// ============================================================
// Chat channel enum
// ============================================================

enum class EChatChannel : uint8
{
	Normal,
	Party,
	Guild,
	Whisper,
	System,
	Combat
};

// ============================================================
// Single chat message
// ============================================================

struct FChatMessage
{
	FString SenderName;
	FString Message;
	EChatChannel Channel = EChatChannel::Normal;
	double Timestamp = 0.0;
};

// ============================================================
// ChatSubsystem
// ============================================================

UCLASS()
class SABRIMMO_API UChatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 MAX_MESSAGES = 100;

	// ---- message storage (read by SChatWidget) ----
	TArray<FChatMessage> Messages;
	uint32 MessageVersion = 0;

	// ---- options flags (set by OptionsSubsystem) ----
	bool bShowTimestamps = false;
	float ChatPanelOpacity = 0.90f;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- public API ----
	UFUNCTION(BlueprintCallable, Category = "MMO Chat")
	void SendChatMessage(const FString& Message, const FString& Channel = TEXT("normal"));

	void FocusChatInput();
	void StartWhisperTo(const FString& PlayerName);
	void AddCombatLogMessage(const FString& Message);

	// ---- helpers ----
	static FLinearColor GetChannelColor(EChatChannel Channel);
	static EChatChannel ParseChannel(const FString& ChannelStr);

private:
	void HandleChatReceive(const TSharedPtr<FJsonValue>& Data);
	void HandleChatError(const TSharedPtr<FJsonValue>& Data);

	// ---- combat log handlers ----
	void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillEffectDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleStatusApplied(const TSharedPtr<FJsonValue>& Data);
	void HandleStatusRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffApplied(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data);
	void HandleExpGain(const TSharedPtr<FJsonValue>& Data);

	void ShowWidget();
	void HideWidget();

	TSharedPtr<SChatWidget> ChatWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
	bool bWidgetAdded = false;
	int32 LocalCharacterId = 0;
	FString LocalCharacterName;
	FString LastWhisperer;  // For /r reply shortcut
};
