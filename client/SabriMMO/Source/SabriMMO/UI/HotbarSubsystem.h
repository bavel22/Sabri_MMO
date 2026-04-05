// HotbarSubsystem.h — UWorldSubsystem that owns all hotbar slot data (4 rows x 9 slots),
// keybind configuration, and SHotbarRowWidget lifecycle.
// Registers Socket.io event handlers via the persistent EventRouter.
// F5 key toggle handled by SabriMMOCharacter via Enhanced Input.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "HotbarSubsystem.generated.h"

class UGameViewportClient;
class SHotbarRowWidget;
class SHotbarKeybindWidget;

// ============================================================
// Hotbar slot — single entry in the 4x9 grid
// ============================================================

USTRUCT()
struct FHotbarSlot
{
	GENERATED_BODY()

	int32 RowIndex = 0;
	int32 SlotIndex = 0;        // 0-8
	FString SlotType;           // "item" or "skill" or "" (empty)

	// Item fields (valid when SlotType == "item")
	int32 InventoryId = 0;
	int32 ItemId = 0;
	FString ItemName;
	FString ItemIcon;
	int32 Quantity = 0;

	// Skill fields (valid when SlotType == "skill")
	int32 SkillId = 0;
	int32 SkillLevel = 0;       // RO Classic: selected use level (1 to max learned, 0 = max)
	FString SkillName;
	FString SkillIcon;

	bool IsEmpty() const { return SlotType.IsEmpty() || (InventoryId <= 0 && SkillId <= 0); }
	bool IsItem() const { return SlotType == TEXT("item") && InventoryId > 0; }
	bool IsSkill() const { return SlotType == TEXT("skill") && SkillId > 0; }

	void Clear()
	{
		SlotType.Empty();
		InventoryId = 0; ItemId = 0; ItemName.Empty(); ItemIcon.Empty(); Quantity = 0;
		SkillId = 0; SkillLevel = 0; SkillName.Empty(); SkillIcon.Empty();
	}
};

// ============================================================
// Keybind for a single hotbar slot
// ============================================================

USTRUCT()
struct FHotbarKeybind
{
	GENERATED_BODY()

	FKey PrimaryKey = EKeys::Invalid;
	bool bRequiresAlt = false;
	bool bRequiresCtrl = false;
	bool bRequiresShift = false;

	bool IsValid() const { return PrimaryKey != EKeys::Invalid; }

	FString GetDisplayString() const;
};

// ============================================================
// Visibility cycling state
// ============================================================

enum class EHotbarVisibility : uint8
{
	OneRow,
	TwoRows,
	ThreeRows,
	FourRows,
	Hidden
};

// ============================================================
// HotbarSubsystem
// ============================================================

UCLASS()
class SABRIMMO_API UHotbarSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 NUM_ROWS = 4;
	static constexpr int32 SLOTS_PER_ROW = 9;

	// ---- hotbar data (read by widgets via lambdas) ----
	FHotbarSlot Slots[NUM_ROWS][SLOTS_PER_ROW];
	uint32 DataVersion = 0;

	// ---- visibility ----
	EHotbarVisibility Visibility = EHotbarVisibility::OneRow;
	void CycleVisibility();
	int32 GetVisibleRowCount() const;
	bool IsVisible() const;

	// ---- slot operations ----
	void AssignItem(int32 RowIndex, int32 SlotIndex, const FInventoryItem& Item);
	void AssignSkill(int32 RowIndex, int32 SlotIndex, int32 SkillId, const FString& SkillName, const FString& SkillIcon = TEXT(""), int32 SkillLevel = 0);
	void ClearSlot(int32 RowIndex, int32 SlotIndex);
	void ActivateSlot(int32 RowIndex, int32 SlotIndex);
	const FHotbarSlot& GetSlot(int32 RowIndex, int32 SlotIndex) const;

	// ---- keybind management ----
	FHotbarKeybind GetKeybind(int32 RowIndex, int32 SlotIndex) const;
	void SetKeybind(int32 RowIndex, int32 SlotIndex, const FHotbarKeybind& Keybind);
	FString GetKeybindDisplayString(int32 RowIndex, int32 SlotIndex) const;
	void LoadKeybinds();
	void SaveKeybinds();
	void ResetKeybindsToDefaults();

	// ---- keybind config panel ----
	void ToggleKeybindWidget();
	void HideKeybindWidget();
	bool IsKeybindWidgetVisible() const;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- icon utilities (delegate to InventorySubsystem/SkillTreeSubsystem) ----
	FSlateBrush* GetItemIconBrush(const FString& IconName);
	FSlateBrush* GetSkillIconBrush(const FString& IconName);

	// ---- delegate for widget refresh ----
	DECLARE_MULTICAST_DELEGATE(FOnHotbarDataUpdated);
	FOnHotbarDataUpdated OnHotbarDataUpdated;

	// ---- process key input from number keys (called from SabriMMOCharacter) ----
	void HandleNumberKey(int32 KeyNumber, bool bAlt, bool bCtrl, bool bShift);

	// ---- refresh item quantities from inventory data ----
	void RefreshItemQuantities();

private:
	// ---- event handlers ----
	void HandleHotbarAllData(const TSharedPtr<FJsonValue>& Data);
	void HandleHotbarData(const TSharedPtr<FJsonValue>& Data);

	// ---- server emit helpers ----
	void EmitSaveItem(int32 RowIndex, int32 SlotIndex, int32 InventoryId, int32 ItemId, const FString& ItemName);
	void EmitSaveSkill(int32 RowIndex, int32 SlotIndex, int32 SkillId, const FString& SkillName, int32 SkillLevel = 0);
	void EmitClearSlot(int32 RowIndex, int32 SlotIndex);

	// ---- state ----
	bool bInitialRowsShown = false;
	int32 LocalCharacterId = 0;
	FTimerHandle HotbarRequestTimer;
	FTimerHandle ViewportCheckTimer;

	// ---- keybind storage (4 rows x 9 slots) ----
	FHotbarKeybind Keybinds[NUM_ROWS][SLOTS_PER_ROW];
	void InitializeDefaultKeybinds();

	// ---- row widget state ----
	struct FHotbarRowWidgetState
	{
		TSharedPtr<SHotbarRowWidget> Widget;
		TSharedPtr<SWidget> AlignmentWrapper;
		TSharedPtr<SWidget> ViewportOverlay;
		bool bIsAdded = false;
	};
	FHotbarRowWidgetState RowWidgets[NUM_ROWS];
	void ShowRows();
	void HideAllRows();

	// ---- keybind config widget state ----
	TSharedPtr<SHotbarKeybindWidget> KeybindWidget;
	TSharedPtr<SWidget> KeybindAlignmentWrapper;
	TSharedPtr<SWidget> KeybindViewportOverlay;
	bool bKeybindWidgetVisible = false;

	// ---- cached viewport client (PIE-safe: World->GetGameViewport() returns the global
	//      GEngine->GameViewport which changes when other PIE instances load, so we cache
	//      the VC we first added widgets to and always use that for add/remove) ----
	TWeakObjectPtr<UGameViewportClient> CachedViewportClient;

	// ---- static empty slot for out-of-bounds access ----
	static const FHotbarSlot EmptySlot;
};
