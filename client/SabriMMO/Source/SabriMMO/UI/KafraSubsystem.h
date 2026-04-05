// KafraSubsystem.h — UWorldSubsystem managing Kafra NPC service state
// and SKafraWidget lifecycle. Registers Socket.io event handlers via the persistent EventRouter.
// RO Classic: Save point + teleport service with zeny costs.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "KafraSubsystem.generated.h"

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

	// Cart state (from kafra:data)
	bool bHasCart = false;
	FString JobClass;
	int32 PushcartLevel = 0;

	// ---- public API ----
	void RequestOpenKafra(const FString& KafraId);
	void RequestSave();
	void RequestTeleport(const FString& DestZone);
	void RequestRentCart();
	void RequestRemoveCart();
	void CloseKafra();

	/** Returns true if the player's job class can use a pushcart. */
	bool CanUseCart() const;

	// ---- widget lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ShowWidget();
	void HideWidget();
	bool IsWidgetVisible() const;

private:
	// ---- event handlers ----
	void HandleKafraData(const TSharedPtr<FJsonValue>& Data);
	void HandleKafraSaved(const TSharedPtr<FJsonValue>& Data);
	void HandleKafraTeleported(const TSharedPtr<FJsonValue>& Data);
	void HandleKafraError(const TSharedPtr<FJsonValue>& Data);
	void HandleCartData(const TSharedPtr<FJsonValue>& Data);
	void HandleCartEquipped(const TSharedPtr<FJsonValue>& Data);
	void HandleCartError(const TSharedPtr<FJsonValue>& Data);

	// ---- state ----
	bool bWidgetAdded = false;

	TSharedPtr<SKafraWidget> KafraWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;

};
