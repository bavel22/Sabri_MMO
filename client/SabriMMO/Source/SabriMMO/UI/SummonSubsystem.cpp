// SummonSubsystem.cpp — Tracks summoned plants and marine spheres via socket events.
// Manages SSummonOverlay for visual rendering. No Blueprint actors required.

#include "SummonSubsystem.h"
#include "SSummonOverlay.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Widgets/SWeakWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogSummon, Log, All);

bool USummonSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void USummonSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	Router->RegisterHandler(TEXT("summon:plant_spawned"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePlantSpawned(D); });
	Router->RegisterHandler(TEXT("summon:plant_removed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePlantRemoved(D); });
	Router->RegisterHandler(TEXT("summon:plant_attack"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePlantAttack(D); });
	Router->RegisterHandler(TEXT("summon:sphere_spawned"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleSphereSpawned(D); });
	Router->RegisterHandler(TEXT("summon:sphere_exploded"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleSphereExploded(D); });
	Router->RegisterHandler(TEXT("summon:sphere_removed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleSphereRemoved(D); });

	ShowOverlay();
	UE_LOG(LogSummon, Log, TEXT("SummonSubsystem initialized — 6 event handlers registered"));
}

void USummonSubsystem::Deinitialize()
{
	if (bOverlayAdded)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UGameViewportClient* VC = World->GetGameViewport();
			if (VC && ViewportOverlay.IsValid())
				VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
	}
	OverlayWidget.Reset();
	ViewportOverlay.Reset();
	Plants.Empty();
	Spheres.Empty();

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

// ── Overlay ──────────────────────────────────────────────────────

void USummonSubsystem::ShowOverlay()
{
	if (bOverlayAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	OverlayWidget = SNew(SSummonOverlay).Subsystem(this);
	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(OverlayWidget);
	VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 6); // Z=6, below name tags (7)
	bOverlayAdded = true;
}

// ── Projection ───────────────────────────────────────────────────

bool USummonSubsystem::ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const
{
	UWorld* World = GetWorld();
	if (!World) return false;
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return false;
	return PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, false);
}

// ── Emit ─────────────────────────────────────────────────────────

void USummonSubsystem::DetonateSphere(int32 SphereId)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("sphereId"), SphereId);
	GI->EmitSocketEvent(TEXT("summon:detonate"), Payload);
}

// ── Plant Events ─────────────────────────────────────────────────

void USummonSubsystem::HandlePlantSpawned(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FSummonedPlant Plant;
	double d;
	FString s;
	if (Obj->TryGetNumberField(TEXT("plantId"), d)) Plant.PlantId = (int32)d;
	if (Obj->TryGetNumberField(TEXT("ownerId"), d)) Plant.OwnerId = (int32)d;
	if (Obj->TryGetStringField(TEXT("ownerName"), s)) Plant.OwnerName = s;
	if (Obj->TryGetStringField(TEXT("type"), s)) Plant.Type = s;
	if (Obj->TryGetNumberField(TEXT("monsterId"), d)) Plant.MonsterId = (int32)d;
	if (Obj->TryGetNumberField(TEXT("x"), d)) Plant.Position.X = d;
	if (Obj->TryGetNumberField(TEXT("y"), d)) Plant.Position.Y = d;
	if (Obj->TryGetNumberField(TEXT("z"), d)) Plant.Position.Z = d;
	if (Obj->TryGetNumberField(TEXT("hp"), d)) Plant.HP = (int32)d;
	if (Obj->TryGetNumberField(TEXT("maxHp"), d)) Plant.MaxHP = (int32)d;
	if (Obj->TryGetNumberField(TEXT("duration"), d)) Plant.Duration = (float)(d / 1000.0);
	Plant.SpawnTime = FPlatformTime::Seconds();

	Plants.Add(Plant.PlantId, Plant);
	UE_LOG(LogSummon, Log, TEXT("Plant spawned: %s (id=%d) by %s"), *Plant.Type, Plant.PlantId, *Plant.OwnerName);
}

void USummonSubsystem::HandlePlantRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double d;
	if (Obj->TryGetNumberField(TEXT("plantId"), d))
	{
		int32 PlantId = (int32)d;
		Plants.Remove(PlantId);
		UE_LOG(LogSummon, Log, TEXT("Plant removed: id=%d"), PlantId);
	}
}

void USummonSubsystem::HandlePlantAttack(const TSharedPtr<FJsonValue>& Data)
{
	// Plant attacks are shown via enemy:health_update and skill:effect_damage
	// which are handled by other subsystems. Nothing to track here.
}

// ── Sphere Events ────────────────────────────────────────────────

void USummonSubsystem::HandleSphereSpawned(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FSummonedSphere Sphere;
	double d;
	FString s;
	if (Obj->TryGetNumberField(TEXT("sphereId"), d)) Sphere.SphereId = (int32)d;
	if (Obj->TryGetNumberField(TEXT("ownerId"), d)) Sphere.OwnerId = (int32)d;
	if (Obj->TryGetStringField(TEXT("ownerName"), s)) Sphere.OwnerName = s;
	if (Obj->TryGetNumberField(TEXT("x"), d)) Sphere.Position.X = d;
	if (Obj->TryGetNumberField(TEXT("y"), d)) Sphere.Position.Y = d;
	if (Obj->TryGetNumberField(TEXT("z"), d)) Sphere.Position.Z = d;
	if (Obj->TryGetNumberField(TEXT("hp"), d)) Sphere.HP = (int32)d;
	if (Obj->TryGetNumberField(TEXT("maxHp"), d)) Sphere.MaxHP = (int32)d;
	Sphere.SpawnTime = FPlatformTime::Seconds();

	Spheres.Add(Sphere.SphereId, Sphere);
	UE_LOG(LogSummon, Log, TEXT("Marine Sphere spawned: id=%d by %s"), Sphere.SphereId, *Sphere.OwnerName);
}

void USummonSubsystem::HandleSphereExploded(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double d;
	if (Obj->TryGetNumberField(TEXT("sphereId"), d))
	{
		int32 SphereId = (int32)d;
		Spheres.Remove(SphereId);
		UE_LOG(LogSummon, Log, TEXT("Marine Sphere exploded: id=%d"), SphereId);
	}
}

void USummonSubsystem::HandleSphereRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double d;
	if (Obj->TryGetNumberField(TEXT("sphereId"), d))
	{
		Spheres.Remove((int32)d);
	}
}
