// CraftingSubsystem.h — UWorldSubsystem that handles crafting UI for
// Arrow Crafting (305), Pharmacy (1800), and Create Elemental Converter (1420).
// Registers socket handlers via EventRouter for recipe lists and craft results.
// Shows SCraftingPopup overlay.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CraftingSubsystem.generated.h"

class SCraftingPopup;

// Data for a single craftable recipe (sent by server)
struct FCraftingRecipe
{
	int32 OutputItemId = 0;
	FString OutputName;
	FString OutputIcon;
	int32 OutputQuantity = 1;
	int32 SuccessRate = 100;        // 0-100 (Pharmacy uses this, Arrow Crafting = 100)
	FString SourceItemName;          // Arrow Crafting: source item name
	int32 SourceInventoryId = 0;     // Arrow Crafting: inventory_id to pass back
	int32 SourceQuantity = 0;        // quantity of source item
};

UCLASS()
class SABRIMMO_API UCraftingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	enum class ECraftType : uint8 { ArrowCrafting, Pharmacy, Converter };

	// Crafting emit helpers (called by SCraftingPopup on row click)
	void EmitArrowCraft(int32 SourceInventoryId);
	void EmitPharmacyCraft(int32 RecipeOutputId);
	void EmitConverterCraft(int32 ProductId);

	// Popup lifecycle
	void HideCraftingPopup();
	bool IsCraftingPopupVisible() const { return bPopupVisible; }
	ECraftType GetCurrentCraftType() const { return CurrentCraftType; }

private:
	// Socket event handlers
	void HandleArrowCraftingRecipes(const TSharedPtr<FJsonValue>& Data);
	void HandleArrowCraftingResult(const TSharedPtr<FJsonValue>& Data);
	void HandlePharmacyRecipes(const TSharedPtr<FJsonValue>& Data);
	void HandlePharmacyResult(const TSharedPtr<FJsonValue>& Data);
	void HandleConverterRecipes(const TSharedPtr<FJsonValue>& Data);
	void HandleConverterResult(const TSharedPtr<FJsonValue>& Data);

	// Show popup with recipes
	void ShowCraftingPopup(const FString& Title, const TArray<FCraftingRecipe>& Recipes, bool bShowRate, bool bIsPharmacy);

	// State
	bool bPopupVisible = false;
	ECraftType CurrentCraftType = ECraftType::ArrowCrafting;

	// Popup widgets (kept alive as members)
	TSharedPtr<SCraftingPopup> PopupWidget;
	TSharedPtr<SWidget> PopupAlignWrapper;
	TSharedPtr<SWidget> PopupOverlay;
};
