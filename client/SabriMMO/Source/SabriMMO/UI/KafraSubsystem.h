// KafraSubsystem.h — UWorldSubsystem managing Kafra NPC service state,
// Socket.io event wrapping (kafra:data/saved/teleported/error), and SKafraWidget lifecycle.
// RO Classic: Save point + teleport service with zeny costs.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "KafraSubsystem.generated.h"

class USocketIOClientComponent;
class SKafraWidget;

USTRUCT()
struct FKafraDestination
{
	GENERATED_BODY()

	FString ZoneName;
	FString DisplayName;
	int32 Cost = 0;
};

UCLASS()
class SABRIMMO_API UKafraSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- kafra state (read by widget) ----
	FString CurrentKafraId;
	FString KafraName;
	TArray<FKafraDestination> Destinations;
	int32 PlayerZuzucoin = 0;
	FString CurrentSaveMap;
	FString StatusMessage;
	double StatusExpireTime = 0.0;

	// ---- public API ----
	void RequestOpenKafra(const FString& KafraId);
	void RequestSave();
	void RequestTeleport(const FString& DestZone);
	void CloseKafra();

	// ---- widget lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ShowWidget();
	void HideWidget();
	bool IsWidgetVisible() const;

private:
	// ---- socket event wrapping ----
	void TryWrapSocketEvents();
	void WrapSingleEvent(const FString& EventName,
		TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
	USocketIOClientComponent* FindSocketIOComponent() const;

	// ---- event handlers ----
	void HandleKafraData(const TSharedPtr<FJsonValue>& Data);
	void HandleKafraSaved(const TSharedPtr<FJsonValue>& Data);
	void HandleKafraTeleported(const TSharedPtr<FJsonValue>& Data);
	void HandleKafraError(const TSharedPtr<FJsonValue>& Data);

	// ---- state ----
	bool bEventsWrapped = false;
	bool bWidgetAdded = false;
	FTimerHandle BindCheckTimer;

	TSharedPtr<SKafraWidget> KafraWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;

	TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
