#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterData.h"
#include "LoginFlowSubsystem.generated.h"

class SLoginWidget;
class SServerSelectWidget;
class SCharacterSelectWidget;
class SCharacterCreateWidget;
class SLoadingOverlayWidget;
class UMMOGameInstance;

/**
 * Login flow states — each state corresponds to a visible screen.
 */
UENUM()
enum class ELoginFlowState : uint8
{
	Login,
	ServerSelect,
	CharacterSelect,
	CharacterCreate,
	EnteringWorld
};

/**
 * ULoginFlowSubsystem
 *
 * Replaces BP_GameFlow. Manages the entire login → server select → character select/create → enter world flow.
 * Pure C++ — no Blueprints needed.
 *
 * Only creates in the login level (L_Startup or L_Login). Does NOT create in the game level.
 */
UCLASS()
class SABRIMMO_API ULoginFlowSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- Lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Called by SLoginWidget ----
	void OnLoginSubmitted(const FString& Username, const FString& Password, bool bRememberUsername);
	void OnRegisterSubmitted(const FString& Username, const FString& Email, const FString& Password);
	void OnExitRequested();

	// ---- Called by SServerSelectWidget ----
	void OnServerSelected(const FServerInfo& Server);
	void OnBackToLogin();

	// ---- Called by SCharacterSelectWidget ----
	void OnPlayCharacter(const FCharacterData& Character);
	void OnDeleteCharacterConfirmed(int32 CharacterId, const FString& Password);
	void OnCreateCharacterRequested();
	void OnBackToServerSelect();

	// ---- Called by SCharacterCreateWidget ----
	void OnCreateCharacterSubmitted(const FString& Name, const FString& Class,
		int32 HairStyle, int32 HairColor, const FString& Gender);
	void OnCreateCharacterCancelled();

	ELoginFlowState GetCurrentState() const { return CurrentState; }

private:
	void TransitionTo(ELoginFlowState NewState);
	void ShowLoadingOverlay(const FString& StatusText);
	void HideLoadingOverlay();
	void HideAllWidgets();

	// GameInstance delegate handlers (UFUNCTION required for AddDynamic)
	UFUNCTION()
	void HandleLoginSuccess();
	UFUNCTION()
	void HandleLoginFailedWithReason(const FString& ErrorMessage);
	UFUNCTION()
	void HandleServerListReceived(const TArray<FServerInfo>& Servers);
	UFUNCTION()
	void HandleCharacterListReceived();
	UFUNCTION()
	void HandleCharacterCreated();
	UFUNCTION()
	void HandleCharacterCreateFailed(const FString& ErrorMessage);
	UFUNCTION()
	void HandleCharacterDeleteSuccess(const FString& CharacterName);
	UFUNCTION()
	void HandleCharacterDeleteFailed(const FString& ErrorMessage);

	UMMOGameInstance* GetGI() const;

	// ---- State ----
	ELoginFlowState CurrentState = ELoginFlowState::Login;
	bool bWidgetsCreated = false;

	// ---- Widgets (all owned by the subsystem via TSharedPtr) ----
	TSharedPtr<SWidget> BackgroundWidget;
	TSharedPtr<SLoginWidget> LoginWidget;
	TSharedPtr<SServerSelectWidget> ServerSelectWidget;
	TSharedPtr<SCharacterSelectWidget> CharacterSelectWidget;
	TSharedPtr<SCharacterCreateWidget> CharacterCreateWidget;
	TSharedPtr<SLoadingOverlayWidget> LoadingOverlayWidget;

	// Viewport wrappers (prevents GC, required for AddViewportWidgetContent)
	TSharedPtr<SWidget> BackgroundViewportWidget;
	TSharedPtr<SWidget> LoginViewportWidget;
	TSharedPtr<SWidget> ServerSelectViewportWidget;
	TSharedPtr<SWidget> CharacterSelectViewportWidget;
	TSharedPtr<SWidget> CharacterCreateViewportWidget;
	TSharedPtr<SWidget> LoadingOverlayViewportWidget;

	// Background image brush (plain FSlateBrush — avoids deprecated FSlateDynamicImageBrush)
	FSlateBrush BackgroundBrush;

	UPROPERTY()
	TObjectPtr<UTexture2D> BackgroundTexture;

	void CreateBackgroundWidget(UGameViewportClient* VC);
	void TryLoadBackgroundTexture(UGameViewportClient* VC, int32 RetriesLeft);
	FTimerHandle BackgroundRetryTimer;

	// Timer for entering world
	FTimerHandle EnterWorldTimer;

	// Z-orders: Background covers all game widgets (Z 5-50), login widgets sit above it.
	static constexpr int32 BackgroundZOrder = 200;
	static constexpr int32 LoginWidgetZOrder = 201;
	static constexpr int32 OverlayWidgetZOrder = 250;
};
