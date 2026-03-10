// PositionBroadcastSubsystem.cpp — 30Hz position broadcasting via persistent socket.

#include "PositionBroadcastSubsystem.h"
#include "MMOGameInstance.h"
#include "CharacterData.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Dom/JsonObject.h"

DEFINE_LOG_CATEGORY_STATIC(LogPositionBroadcast, Log, All);

bool UPositionBroadcastSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UPositionBroadcastSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Start 30Hz position broadcast timer (~33ms interval)
	InWorld.GetTimerManager().SetTimer(
		PositionTimer,
		FTimerDelegate::CreateUObject(this, &UPositionBroadcastSubsystem::BroadcastPosition),
		0.033f, true
	);

	UE_LOG(LogPositionBroadcast, Log, TEXT("PositionBroadcastSubsystem started — 30Hz position updates."));
}

void UPositionBroadcastSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PositionTimer);
	}

	Super::Deinitialize();
}

void UPositionBroadcastSubsystem::BroadcastPosition()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	FVector Location = Pawn->GetActorLocation();
	FRotator Rotation = Pawn->GetActorRotation();

	FCharacterData SelChar = GI->GetSelectedCharacter();

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("characterId"), FString::FromInt(SelChar.CharacterId));
	Payload->SetNumberField(TEXT("x"), Location.X);
	Payload->SetNumberField(TEXT("y"), Location.Y);
	Payload->SetNumberField(TEXT("z"), Location.Z);
	Payload->SetNumberField(TEXT("yaw"), Rotation.Yaw);

	GI->EmitSocketEvent(TEXT("player:position"), Payload);
}
