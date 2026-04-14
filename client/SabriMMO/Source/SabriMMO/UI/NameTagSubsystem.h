// NameTagSubsystem.h — Renders entity name tags via Slate OnPaint overlay.
// Phase 5 of Blueprint-to-C++ migration. Replaces per-actor WBP_PlayerNameTag WidgetComponents.
// RO Classic behavior: player names always visible, monster/NPC names hover-only.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NameTagSubsystem.generated.h"

UENUM()
enum class ENameTagEntityType : uint8
{
	LocalPlayer,
	Player,
	Monster,
	NPC
};

struct FNameTagEntry
{
	TWeakObjectPtr<AActor> Actor;
	FString DisplayName;
	FString SubText;           // "(PartyName)" line or "GuildName [Title]" line
	int32 Level = 0;           // For monsters: level-based name coloring
	ENameTagEntityType Type = ENameTagEntityType::Player;
	bool bVisible = true;
	float VerticalOffset = 120.f; // Units above actor origin (above head in 3D)
	float SpriteHeight = 0.f;     // If >0, use projected sprite height for positioning (scales with zoom)
	FString VendingTitle;          // Non-empty = player is vending, show shop sign above name
};

UCLASS()
class SABRIMMO_API UNameTagSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Registration API ----
	void RegisterEntity(AActor* Actor, const FString& Name, ENameTagEntityType Type, int32 Level = 0, float VertOffset = 120.f, float InSpriteHeight = 0.f);
	void UnregisterEntity(AActor* Actor);
	void SetVisible(AActor* Actor, bool bVisible);
	void UpdateName(AActor* Actor, const FString& NewName);
	void SetVendingTitle(AActor* Actor, const FString& Title);

	// ---- Read by SNameTagOverlay in OnPaint ----
	const TArray<FNameTagEntry>& GetEntries() const { return Entries; }
	int32 GetLocalPlayerLevel() const { return LocalPlayerLevel; }

	// ---- options flags (set by OptionsSubsystem) ----
	bool bShowPlayerNames = true;
	bool bShowEnemyNames = true;
	bool bShowNPCNames = true;

private:
	TArray<FNameTagEntry> Entries;
	int32 LocalPlayerLevel = 1;

	// Widget state
	bool bWidgetAdded = false;
	TSharedPtr<SWidget> OverlayWidget;
	TSharedPtr<SWidget> ViewportOverlay;

	void ShowOverlay();
	void HideOverlay();

	// Find entry by actor (returns nullptr if not found)
	FNameTagEntry* FindEntry(AActor* Actor);
};
