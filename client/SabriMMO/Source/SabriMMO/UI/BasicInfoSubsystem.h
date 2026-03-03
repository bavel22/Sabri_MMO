// BasicInfoSubsystem.h — UWorldSubsystem that manages the Basic Info Slate widget
// and wraps existing Socket.io event callbacks to feed data into it.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "BasicInfoSubsystem.generated.h"

class USocketIOClientComponent;
class SBasicInfoWidget;
struct FSIOBoundEvent;

UCLASS()
class SABRIMMO_API UBasicInfoSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- public data fields (read by the Slate widget via lambdas) ----
	FString PlayerName;
	FString JobClassDisplayName;

	int32 CurrentHP  = 0;
	int32 MaxHP      = 100;
	int32 CurrentSP  = 0;
	int32 MaxSP      = 100;

	int32 BaseLevel  = 1;
	int32 JobLevel   = 1;
	int64 BaseExp    = 0;
	int64 BaseExpNext = 100;
	int64 JobExp     = 0;
	int64 JobExpNext = 50;

	int32 CurrentWeight = 0;
	int32 MaxWeight     = 2000;
	int32 Zuzucoin      = 0;

	int32 STR = 1;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- widget visibility ----
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
	void HandleHealthUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data);
	void HandlePlayerStats(const TSharedPtr<FJsonValue>& Data);
	void HandleExpGain(const TSharedPtr<FJsonValue>& Data);
	void HandleExpLevelUp(const TSharedPtr<FJsonValue>& Data);
	void HandlePlayerJoined(const TSharedPtr<FJsonValue>& Data);
	void HandleShopTransaction(const TSharedPtr<FJsonValue>& Data);
	void HandleInventoryData(const TSharedPtr<FJsonValue>& Data);

	void ParseExpPayload(const TSharedPtr<FJsonObject>& ExpObj);
	void PopulateNameFromGameInstance();
	void RecalcMaxWeight();

	// ---- state ----
	bool bEventsWrapped = false;
	bool bWidgetAdded   = false;
	int32 LocalCharacterId = 0;

	FTimerHandle BindCheckTimer;

	TSharedPtr<SBasicInfoWidget> BasicInfoWidget;
	TSharedPtr<SWidget>          AlignmentWrapper;
	TSharedPtr<SWidget>          ViewportOverlay;

	TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
