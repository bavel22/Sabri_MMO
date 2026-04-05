// TradeSubsystem.h — UWorldSubsystem managing player-to-player trading.
// Handles trade:* socket events, trade state machine, and trade widget lifecycle.
// Two-step confirmation: OK (lock) -> Trade (finalize). Both must complete both steps.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "TradeSubsystem.generated.h"

class STradeWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogTrade, Log, All);

// ============================================================
// Trade item data (mirrors server-side trade item)
// ============================================================

struct FTradeItem
{
	int32 TradeSlot = -1;
	int32 InventoryId = 0;
	int32 ItemId = 0;
	FString Name;
	FString Icon;
	int32 Quantity = 1;
	int32 RefineLevel = 0;
	int32 Slots = 0;
	FString CompoundedCards;
	FString ItemType;
	FString EquipSlot;
	int32 Weight = 0;
	bool bIdentified = true;
	FString CardPrefix;
	FString CardSuffix;
	int32 WeaponLevel = 0;

	bool IsValid() const { return ItemId > 0; }
};

// ============================================================
// Trade state machine
// ============================================================

enum class ETradeState : uint8
{
	None,            // No active trade
	Requesting,      // Sent request, waiting for response
	RequestReceived, // Received request from another player
	Open,            // Trade window open, can modify items/zeny
	Locked,          // We clicked OK (locked our side)
	BothLocked,      // Both locked, Trade button available
	Confirmed,       // We clicked Trade, waiting for partner
	Completing       // Both confirmed, transfer in progress
};

// ============================================================
// TradeSubsystem
// ============================================================

UCLASS()
class SABRIMMO_API UTradeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 MAX_TRADE_ITEMS = 10;

	// ---- trade state (read by STradeWidget via lambdas) ----
	ETradeState State = ETradeState::None;

	int32 PartnerCharacterId = 0;
	FString PartnerName;

	// Our offer
	TArray<FTradeItem> MyItems;
	int64 MyZeny = 0;
	bool bMyLocked = false;

	// Partner's offer
	TArray<FTradeItem> PartnerItems;
	int64 PartnerZeny = 0;
	bool bPartnerLocked = false;

	// Request popup data
	int32 RequestFromCharId = 0;
	FString RequestFromName;

	// Data version for widget polling
	uint32 DataVersion = 0;

	// ---- public API (emit to server) ----
	void RequestTrade(int32 TargetCharacterId);
	void AcceptRequest();
	void DeclineRequest();
	void AddItem(int32 InventoryId, int32 Quantity = 1);
	void RemoveItem(int32 TradeSlot);
	void SetZeny(int64 Amount);
	void Lock();     // OK button
	void Confirm();  // Trade button
	void Cancel();

	bool IsTrading() const;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void HandleRequestReceived(const TSharedPtr<FJsonValue>& Data);
	void HandleOpened(const TSharedPtr<FJsonValue>& Data);
	void HandleItemAdded(const TSharedPtr<FJsonValue>& Data);
	void HandleItemRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleZenyUpdated(const TSharedPtr<FJsonValue>& Data);
	void HandlePartnerLocked(const TSharedPtr<FJsonValue>& Data);
	void HandleLocksReset(const TSharedPtr<FJsonValue>& Data);
	void HandleBothLocked(const TSharedPtr<FJsonValue>& Data);
	void HandleCompleted(const TSharedPtr<FJsonValue>& Data);
	void HandleCancelled(const TSharedPtr<FJsonValue>& Data);
	void HandleError(const TSharedPtr<FJsonValue>& Data);

	void ShowTradeWidget();
	void HideTradeWidget();
	void ShowRequestPopup();
	void HideRequestPopup();
	void ResetTradeState();
	void UnlockMovement();

	int32 LocalCharacterId = 0;
	FString LocalCharacterName;
	FTimerHandle RequestTimeoutHandle;

	// Trade widget (Z=22)
	TSharedPtr<STradeWidget> TradeWidget;
	TSharedPtr<SWidget> TradeAlignWrapper;
	TSharedPtr<SWidget> TradeOverlay;
	bool bTradeWidgetVisible = false;

	// Request popup (Z=30)
	TSharedPtr<SCompoundWidget> RequestPopupWidget;
	TSharedPtr<SWidget> RequestAlignWrapper;
	TSharedPtr<SWidget> RequestOverlay;
	bool bRequestPopupVisible = false;
};
