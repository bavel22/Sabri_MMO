// SkillTreeSubsystem.h — UWorldSubsystem that manages the Skill Tree Slate widget
// and wraps Socket.io events for skill data, learning, and resetting.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"
#include "SkillTreeSubsystem.generated.h"

class USocketIOClientComponent;
class SSkillTreeWidget;
class SSkillTargetingOverlay;
struct FSIOBoundEvent;

// Targeting mode for skills that require target/ground selection before casting
enum class ESkillTargetingMode : uint8
{
	None,
	SingleTarget,   // Click an enemy to cast (e.g. Bash)
	GroundTarget    // Click ground to confirm (e.g. Magnum Break self-centered AoE)
};

// Prerequisite entry for a skill
USTRUCT(BlueprintType)
struct FSkillPrerequisite
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 RequiredSkillId = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 RequiredLevel = 0;
};

// Single skill entry parsed from server data
USTRUCT(BlueprintType)
struct FSkillEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SkillId = 0;

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY(BlueprintReadOnly)
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxLevel = 1;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentLevel = 0;

	UPROPERTY(BlueprintReadOnly)
	FString Type;         // active, passive, toggle

	UPROPERTY(BlueprintReadOnly)
	FString TargetType;   // none, self, single, ground, aoe

	UPROPERTY(BlueprintReadOnly)
	FString Element;      // neutral, fire, water, etc.

	UPROPERTY(BlueprintReadOnly)
	int32 Range = 0;

	UPROPERTY(BlueprintReadOnly)
	FString Description;

	UPROPERTY(BlueprintReadOnly)
	FString Icon;

	UPROPERTY(BlueprintReadOnly)
	FString IconPath;

	UPROPERTY(BlueprintReadOnly)
	int32 TreeRow = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TreeCol = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SpCost = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 NextSpCost = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CastTime = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Cooldown = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 EffectValue = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bCanLearn = false;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSkillPrerequisite> Prerequisites;
};

// A group of skills belonging to one class
USTRUCT(BlueprintType)
struct FSkillClassGroup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString ClassId;

	UPROPERTY(BlueprintReadOnly)
	FString ClassDisplayName;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSkillEntry> Skills;
};

UCLASS(BlueprintType)
class SABRIMMO_API USkillTreeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- public data fields (read by the Slate widget and Blueprint) ----
	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	FString JobClass;

	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	int32 SkillPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
	TArray<FSkillClassGroup> SkillGroups;

	TMap<int32, int32> LearnedSkills; // skillId -> level (TMap not supported by UPROPERTY)

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- widget visibility ----
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void ToggleWidget();

	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void ShowWidget();

	/** Actually creates and adds the widget to the viewport (called internally). */
	void ShowWidgetInternal();

	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void HideWidget();

	UFUNCTION(BlueprintPure, Category = "SkillTree")
	bool IsWidgetVisible() const;

	// ---- actions (called from widget or Blueprint) ----
	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void RequestSkillData();

	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void LearnSkill(int32 SkillId);

	UFUNCTION(BlueprintCallable, Category = "SkillTree")
	void ResetAllSkills();

	// ---- hotbar skill assignment (called from Slate quick-assign buttons) ----
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Hotbar")
	void AssignSkillToHotbar(int32 SkillId, const FString& SkillDisplayName, int32 SlotIndex);

	/** Called by Blueprint UseHotbarSlot. Returns true if the slot had a skill and was handled.
	 *  SlotIndex is 0-based (0 = key 1, 8 = key 9). */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Hotbar")
	bool TryUseHotbarSkill(int32 SlotIndex);

	// ---- skill usage ----
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Combat")
	void UseSkill(int32 SkillId);

	UFUNCTION(BlueprintCallable, Category = "SkillTree|Combat")
	void UseSkillOnTarget(int32 SkillId, int32 TargetId, bool bIsEnemy);

	/** Use a ground-targeted skill at a specific world position. */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Combat")
	void UseSkillOnGround(int32 SkillId, FVector GroundPosition);

	// ---- cooldown queries ----
	UFUNCTION(BlueprintPure, Category = "SkillTree|Combat")
	bool IsSkillOnCooldown(int32 SkillId) const;

	UFUNCTION(BlueprintPure, Category = "SkillTree|Combat")
	float GetSkillCooldownRemaining(int32 SkillId) const;

	// ---- active buff tracking (read by widgets) ----
	struct FActiveBuff
	{
		int32 SkillId = 0;
		FString BuffName;
		double ExpiresAt = 0;
		int32 Duration = 0;
	};
	TArray<FActiveBuff> ActiveBuffs;

	// ---- icon utilities ----
	FString ResolveIconContentPath(const FString& IconName) const;
	FSlateBrush* GetOrCreateIconBrush(const FString& ContentPath);

	// ---- delegate for widget refresh ----
	DECLARE_MULTICAST_DELEGATE(FOnSkillDataUpdated);
	FOnSkillDataUpdated OnSkillDataUpdated;

	// ---- skill targeting mode (RO-style click-to-cast) ----

	/** Enter targeting mode for a skill. Cursor changes, overlay appears, awaiting click. */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Targeting")
	void BeginTargeting(int32 SkillId);

	/** Cancel targeting mode and restore normal cursor. */
	UFUNCTION(BlueprintCallable, Category = "SkillTree|Targeting")
	void CancelTargeting();

	UFUNCTION(BlueprintPure, Category = "SkillTree|Targeting")
	bool IsInTargetingMode() const { return bIsInTargetingMode; }

	UFUNCTION(BlueprintPure, Category = "SkillTree|Targeting")
	int32 GetPendingSkillId() const { return PendingSkillId; }

	/** Look up a cached skill entry by ID. Returns nullptr if not found. */
	const FSkillEntry* FindSkillEntry(int32 SkillId) const;

	// ---- skill drag state (for drag-to-hotbar) ----
	bool bSkillDragging = false;
	int32 DraggedSkillId = 0;
	FString DraggedSkillName;
	FString DraggedSkillIcon;

	void StartSkillDrag(int32 SkillId, const FString& Name, const FString& Icon);
	void CancelSkillDrag();
	void UpdateSkillDragCursorPosition();  // Call from widget Tick to follow cursor

private:
	// ---- socket event wrapping ----
	void TryWrapSocketEvents();
	void BindNewEvent(const FString& EventName,
		TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
	void WrapExistingEvent(const FString& EventName,
		TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);

	USocketIOClientComponent* FindSocketIOComponent() const;

	// ---- event handlers ----
	void HandleSkillData(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillLearned(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillRefresh(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillResetComplete(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillError(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillUsed(const TSharedPtr<FJsonValue>& Data);
	void HandleHotbarData(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillEffectDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillBuffApplied(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillBuffRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillCooldownStarted(const TSharedPtr<FJsonValue>& Data);

	// ---- state ----
	bool bEventsWrapped = false;
	bool bWidgetAdded   = false;
	int32 LocalCharacterId = 0;

	// ---- cooldown tracking ----
	TMap<int32, double> SkillCooldownExpiry;  // skillId -> platform seconds when cooldown expires

	FTimerHandle BindCheckTimer;
	FTimerHandle HotbarRequestTimer;

	TSharedPtr<SSkillTreeWidget> SkillTreeWidget;
	TSharedPtr<SWidget>          AlignmentWrapper;
	TSharedPtr<SWidget>          ViewportOverlay;

	TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;

	// ---- icon brush cache (TSharedPtr so raw FSlateBrush* survives TMap rehash) ----
	TMap<FString, TSharedPtr<FSlateBrush>> IconBrushCache;

	// ---- GC root for loaded textures (CRITICAL: prevents UTexture2D garbage collection) ----
	// Without this UPROPERTY, textures stored only in FSlateBrush (via non-UPROPERTY TSharedPtr)
	// are invisible to GC and get collected → dangling pointer → crash during Paint.
	UPROPERTY()
	TMap<FString, TObjectPtr<UTexture2D>> IconTextureCache;

	// ---- hotbar skill tracking (0-based slotIndex → skillId) ----
	TMap<int32, int32> HotbarSkillMap;
	void HandleHotbarAllData(const TSharedPtr<FJsonValue>& Data);

	// ---- skill drag cursor overlay ----
	void ShowSkillDragCursor(const FString& IconPath);
	void HideSkillDragCursor();
	TSharedPtr<SBox> SkillDragCursorBox;
	TSharedPtr<SWidget> SkillDragCursorAlignWrapper;
	TSharedPtr<SWidget> SkillDragCursorOverlay;

	// ---- targeting state ----
	bool bIsInTargetingMode = false;
	int32 PendingSkillId = 0;
	ESkillTargetingMode PendingTargetingMode = ESkillTargetingMode::None;
	FString PendingSkillName;

	TSharedPtr<SSkillTargetingOverlay> TargetingOverlay;
	TSharedPtr<SWidget> TargetingOverlayWrapper;
	TSharedPtr<SWidget> TargetingOverlayViewport;

	void ShowTargetingOverlay();
	void HideTargetingOverlay();
	void HandleTargetingClick();
	void HandleTargetingCancel();
	int32 GetEnemyIdFromActor(AActor* Actor) const;
};
