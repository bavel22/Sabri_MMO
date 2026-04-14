// CraftingSubsystem.cpp — Handles Arrow Crafting + Pharmacy crafting UI.
// Listens for arrow_crafting:recipes/result and pharmacy:recipes/result events.

#include "CraftingSubsystem.h"
#include "SCraftingPopup.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogCrafting, Log, All);

bool UCraftingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCraftingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	Router->RegisterHandler(TEXT("arrow_crafting:recipes"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleArrowCraftingRecipes(D); });
	Router->RegisterHandler(TEXT("arrow_crafting:result"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleArrowCraftingResult(D); });
	Router->RegisterHandler(TEXT("pharmacy:recipes"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePharmacyRecipes(D); });
	Router->RegisterHandler(TEXT("pharmacy:result"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePharmacyResult(D); });
	Router->RegisterHandler(TEXT("crafting:converter_recipes"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleConverterRecipes(D); });
	Router->RegisterHandler(TEXT("crafting:converter_result"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleConverterResult(D); });

	UE_LOG(LogCrafting, Log, TEXT("CraftingSubsystem initialized — 6 event handlers registered"));
}

void UCraftingSubsystem::Deinitialize()
{
	HideCraftingPopup();

	UWorld* World = GetWorld();
	if (World)
	{
		UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
		if (GI)
		{
			USocketEventRouter* Router = GI->GetEventRouter();
			if (Router) Router->UnregisterAllForOwner(this);
		}
	}

	Super::Deinitialize();
}

// ── Socket Handlers ──────────────────────────────────────────────

void UCraftingSubsystem::HandleArrowCraftingRecipes(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	const TArray<TSharedPtr<FJsonValue>>* CraftableArr = nullptr;
	if (!Obj->TryGetArrayField(TEXT("craftable"), CraftableArr)) return;

	TArray<FCraftingRecipe> Recipes;
	for (const auto& Val : *CraftableArr)
	{
		const TSharedPtr<FJsonObject>* ItemObj = nullptr;
		if (!Val->TryGetObject(ItemObj) || !ItemObj) continue;
		const TSharedPtr<FJsonObject>& It = *ItemObj;

		FCraftingRecipe R;
		double d;
		FString s;
		if (It->TryGetNumberField(TEXT("arrowId"), d)) R.OutputItemId = (int32)d;
		if (It->TryGetStringField(TEXT("arrowName"), s)) R.OutputName = s;
		if (It->TryGetNumberField(TEXT("arrowQty"), d)) R.OutputQuantity = (int32)d;
		if (It->TryGetStringField(TEXT("itemName"), s)) R.SourceItemName = s;
		if (It->TryGetNumberField(TEXT("inventoryId"), d)) R.SourceInventoryId = (int32)d;
		if (It->TryGetNumberField(TEXT("quantity"), d)) R.SourceQuantity = (int32)d;
		R.SuccessRate = 100; // Arrow Crafting always succeeds
		Recipes.Add(R);
	}

	if (Recipes.Num() > 0)
	{
		CurrentCraftType = ECraftType::ArrowCrafting;
		ShowCraftingPopup(TEXT("Arrow Crafting"), Recipes, false, false);
	}
	else
	{
		UE_LOG(LogCrafting, Log, TEXT("Arrow Crafting: no craftable recipes available"));
	}
}

void UCraftingSubsystem::HandleArrowCraftingResult(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid() || !PopupWidget.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool bSuccess = false;
	Obj->TryGetBoolField(TEXT("success"), bSuccess);
	FString Message;
	Obj->TryGetStringField(TEXT("message"), Message);

	if (bSuccess)
	{
		PopupWidget->SetStatusMessage(Message, false);
		UE_LOG(LogCrafting, Log, TEXT("Arrow Crafting success: %s"), *Message);
	}
	else
	{
		PopupWidget->SetStatusMessage(Message, true);
		UE_LOG(LogCrafting, Warning, TEXT("Arrow Crafting failed: %s"), *Message);
	}
}

void UCraftingSubsystem::HandlePharmacyRecipes(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	const TArray<TSharedPtr<FJsonValue>>* CraftableArr = nullptr;
	if (!Obj->TryGetArrayField(TEXT("craftable"), CraftableArr)) return;

	TArray<FCraftingRecipe> Recipes;
	for (const auto& Val : *CraftableArr)
	{
		const TSharedPtr<FJsonObject>* ItemObj = nullptr;
		if (!Val->TryGetObject(ItemObj) || !ItemObj) continue;
		const TSharedPtr<FJsonObject>& It = *ItemObj;

		FCraftingRecipe R;
		double d;
		FString s;
		if (It->TryGetNumberField(TEXT("outputId"), d)) R.OutputItemId = (int32)d;
		if (It->TryGetStringField(TEXT("outputName"), s)) R.OutputName = s;
		if (It->TryGetStringField(TEXT("outputIcon"), s)) R.OutputIcon = s;
		if (It->TryGetNumberField(TEXT("outputQty"), d)) R.OutputQuantity = (int32)d;
		if (It->TryGetNumberField(TEXT("successRate"), d)) R.SuccessRate = (int32)d;
		if (It->TryGetStringField(TEXT("ingredients"), s)) R.SourceItemName = s;
		R.SourceInventoryId = 0; // Pharmacy doesn't use inventory_id selection
		Recipes.Add(R);
	}

	if (Recipes.Num() > 0)
	{
		CurrentCraftType = ECraftType::Pharmacy;
		ShowCraftingPopup(TEXT("Pharmacy"), Recipes, true, true);
	}
	else
	{
		UE_LOG(LogCrafting, Log, TEXT("Pharmacy: no craftable recipes (missing ingredients/guide)"));
	}
}

void UCraftingSubsystem::HandlePharmacyResult(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid() || !PopupWidget.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool bSuccess = false;
	Obj->TryGetBoolField(TEXT("success"), bSuccess);
	FString Message;
	Obj->TryGetStringField(TEXT("message"), Message);

	// Pharmacy chime — 2D non-spatial, distinct success/fail tones
	if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
	{
		if (bSuccess) Audio->PlayPharmacySuccessSound();
		else          Audio->PlayPharmacyFailSound();
	}

	if (bSuccess)
	{
		PopupWidget->SetStatusMessage(Message, false);
		UE_LOG(LogCrafting, Log, TEXT("Pharmacy success: %s"), *Message);
	}
	else
	{
		PopupWidget->SetStatusMessage(Message, true);
		UE_LOG(LogCrafting, Warning, TEXT("Pharmacy failed: %s"), *Message);
	}
}

void UCraftingSubsystem::HandleConverterRecipes(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	const TArray<TSharedPtr<FJsonValue>>* RecipeArr = nullptr;
	if (!Obj->TryGetArrayField(TEXT("recipes"), RecipeArr)) return;

	TArray<FCraftingRecipe> Recipes;
	for (const auto& Val : *RecipeArr)
	{
		const TSharedPtr<FJsonObject>* ItemObj = nullptr;
		if (!Val->TryGetObject(ItemObj) || !ItemObj) continue;
		const TSharedPtr<FJsonObject>& It = *ItemObj;

		FCraftingRecipe R;
		double d;
		FString s;
		if (It->TryGetNumberField(TEXT("outputId"), d)) R.OutputItemId = (int32)d;
		if (It->TryGetStringField(TEXT("outputName"), s)) R.OutputName = s;
		if (It->TryGetStringField(TEXT("outputIcon"), s)) R.OutputIcon = s;
		if (It->TryGetNumberField(TEXT("outputQty"), d)) R.OutputQuantity = (int32)d;
		if (It->TryGetNumberField(TEXT("successRate"), d)) R.SuccessRate = (int32)d;
		if (It->TryGetStringField(TEXT("ingredients"), s)) R.SourceItemName = s;
		bool bCanCraft = false;
		It->TryGetBoolField(TEXT("canCraft"), bCanCraft);
		R.SourceInventoryId = 0;
		Recipes.Add(R);
	}

	if (Recipes.Num() > 0)
	{
		CurrentCraftType = ECraftType::Converter;
		ShowCraftingPopup(TEXT("Create Elemental Converter"), Recipes, true, false);
	}
	else
	{
		UE_LOG(LogCrafting, Log, TEXT("Converter: no recipes received"));
	}
}

void UCraftingSubsystem::HandleConverterResult(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid() || !PopupWidget.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool bSuccess = false;
	Obj->TryGetBoolField(TEXT("success"), bSuccess);
	FString Message;
	Obj->TryGetStringField(TEXT("message"), Message);

	if (bSuccess)
	{
		PopupWidget->SetStatusMessage(Message, false);
		UE_LOG(LogCrafting, Log, TEXT("Converter craft success: %s"), *Message);
	}
	else
	{
		PopupWidget->SetStatusMessage(Message, true);
		UE_LOG(LogCrafting, Warning, TEXT("Converter craft failed: %s"), *Message);
	}
}

// ── Popup Lifecycle ──────────────────────────────────────────────

void UCraftingSubsystem::ShowCraftingPopup(const FString& Title, const TArray<FCraftingRecipe>& Recipes, bool bShowRate, bool bIsPharmacy)
{
	HideCraftingPopup();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	PopupWidget = SNew(SCraftingPopup)
		.Subsystem(this)
		.Title(Title)
		.Recipes(Recipes)
		.ShowSuccessRate(bShowRate);

	PopupAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::Visible)
		[
			PopupWidget.ToSharedRef()
		];

	PopupOverlay = SNew(SWeakWidget).PossiblyNullContent(PopupAlignWrapper);
	VC->AddViewportWidgetContent(PopupOverlay.ToSharedRef(), 24); // Z=24 above card compound (23)
	bPopupVisible = true;

	FSlateApplication::Get().SetKeyboardFocus(PopupWidget);

	UE_LOG(LogCrafting, Log, TEXT("Crafting popup shown: %s — %d recipes (Z=24)"), *Title, Recipes.Num());
}

void UCraftingSubsystem::HideCraftingPopup()
{
	if (!bPopupVisible) return;

	UAudioSubsystem::PlayUICancelStatic(GetWorld());


	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* VC = World->GetGameViewport();
		if (VC && PopupOverlay.IsValid())
		{
			VC->RemoveViewportWidgetContent(PopupOverlay.ToSharedRef());
		}
	}

	PopupWidget.Reset();
	PopupAlignWrapper.Reset();
	PopupOverlay.Reset();
	bPopupVisible = false;

	UE_LOG(LogCrafting, Log, TEXT("Crafting popup hidden"));
}

// ── Emit Helpers ─────────────────────────────────────────────────

void UCraftingSubsystem::EmitArrowCraft(int32 SourceInventoryId)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UAudioSubsystem::PlayUIClickStatic(World);
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("skillId"), 305);
	Payload->SetNumberField(TEXT("sourceInventoryId"), SourceInventoryId);
	Payload->SetNumberField(TEXT("targetId"), SourceInventoryId);
	Payload->SetBoolField(TEXT("isEnemy"), false);
	GI->EmitSocketEvent(TEXT("skill:use"), Payload);

	UE_LOG(LogCrafting, Log, TEXT("Sent skill:use (arrow_crafting) sourceInventoryId=%d"), SourceInventoryId);
}

void UCraftingSubsystem::EmitPharmacyCraft(int32 RecipeOutputId)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UAudioSubsystem::PlayUIClickStatic(World);
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("recipeOutputId"), RecipeOutputId);
	GI->EmitSocketEvent(TEXT("pharmacy:craft"), Payload);

	UE_LOG(LogCrafting, Log, TEXT("Sent pharmacy:craft recipeOutputId=%d"), RecipeOutputId);
}

void UCraftingSubsystem::EmitConverterCraft(int32 ProductId)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UAudioSubsystem::PlayUIClickStatic(World);
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("productId"), ProductId);
	GI->EmitSocketEvent(TEXT("crafting:craft_converter"), Payload);

	UE_LOG(LogCrafting, Log, TEXT("Sent crafting:craft_converter productId=%d"), ProductId);
}
