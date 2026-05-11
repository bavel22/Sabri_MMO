// JobChangeSubsystem.cpp — Job Master class change dialog state + sprite respawn.

#include "JobChangeSubsystem.h"
#include "SJobChangeWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "SkillTreeSubsystem.h"
#include "NameTagSubsystem.h"
#include "../SabriMMOCharacter.h"
#include "../Sprite/SpriteCharacterActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogJobChange, Log, All);

// ============================================================
// Static class data — mirrors server/src/ro_exp_tables.js
// ============================================================

namespace JobChangeData
{
	static const TArray<FString> FirstClasses = {
		TEXT("swordsman"), TEXT("mage"), TEXT("archer"),
		TEXT("acolyte"), TEXT("thief"), TEXT("merchant"),
	};

	struct FUpgradePath
	{
		FString From;
		TArray<FString> To;
	};

	static const TArray<FUpgradePath>& GetSecondClassUpgrades()
	{
		static const TArray<FUpgradePath> Map = {
			{ TEXT("swordsman"), { TEXT("knight"),     TEXT("crusader") } },
			{ TEXT("mage"),      { TEXT("wizard"),     TEXT("sage") } },
			{ TEXT("archer"),    { TEXT("hunter"),     TEXT("bard"), TEXT("dancer") } },
			{ TEXT("acolyte"),   { TEXT("priest"),     TEXT("monk") } },
			{ TEXT("thief"),     { TEXT("assassin"),   TEXT("rogue") } },
			{ TEXT("merchant"),  { TEXT("blacksmith"), TEXT("alchemist") } },
		};
		return Map;
	}

	static const TArray<FString> SecondClasses = {
		TEXT("knight"), TEXT("crusader"),
		TEXT("wizard"), TEXT("sage"),
		TEXT("hunter"), TEXT("bard"), TEXT("dancer"),
		TEXT("priest"), TEXT("monk"),
		TEXT("assassin"), TEXT("rogue"),
		TEXT("blacksmith"), TEXT("alchemist"),
	};

	static const TArray<FString> TranscendentClasses = {
		TEXT("lord_knight"), TEXT("paladin"),
		TEXT("high_wizard"), TEXT("scholar"),
		TEXT("sniper"), TEXT("minstrel"), TEXT("gypsy"),
		TEXT("high_priest"), TEXT("champion"),
		TEXT("assassin_cross"), TEXT("stalker"),
		TEXT("whitesmith"), TEXT("biochemist"),
	};

	static int32 GetTier(const FString& JobClass)
	{
		const FString Lower = JobClass.ToLower();
		if (Lower == TEXT("novice")) return 0;
		if (FirstClasses.Contains(Lower)) return 1;
		if (SecondClasses.Contains(Lower)) return 2;
		if (TranscendentClasses.Contains(Lower)) return 3;
		return -1;
	}
}

// ============================================================
// Static helper — class display name (mirrors JOB_CLASS_CONFIG.displayName)
// ============================================================

FString UJobChangeSubsystem::GetClassDisplayName(const FString& ClassId)
{
	// Capitalize first letter; underscores → spaces with each word capitalized.
	if (ClassId.IsEmpty()) return ClassId;
	FString Lower = ClassId.ToLower();
	FString Out;
	bool bCapNext = true;
	for (TCHAR Ch : Lower)
	{
		if (Ch == TEXT('_'))
		{
			Out += TEXT(' ');
			bCapNext = true;
		}
		else if (bCapNext)
		{
			Out += FChar::ToUpper(Ch);
			bCapNext = false;
		}
		else
		{
			Out += Ch;
		}
	}
	return Out;
}

// ============================================================
// Lifecycle
// ============================================================

bool UJobChangeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UJobChangeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	if (USocketEventRouter* Router = GI->GetEventRouter())
	{
		Router->RegisterHandler(TEXT("job:changed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleJobChanged(D); });
		Router->RegisterHandler(TEXT("job:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleJobError(D); });
	}

	UE_LOG(LogJobChange, Log, TEXT("JobChangeSubsystem started — events registered."));
}

void UJobChangeSubsystem::Deinitialize()
{
	HideWidget();

	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}
	}

	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

void UJobChangeSubsystem::OpenDialog()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	const FCharacterData SelChar = GI->GetSelectedCharacter();
	CurrentClassDisplayName = GetClassDisplayName(SelChar.JobClass);
	CurrentJobLevel = SelChar.JobLevel;

	// Read basic skill level from SkillTreeSubsystem (skill ID 1).
	BasicSkillLevel = 0;
	if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
	{
		const int32* Lv = SkillSub->LearnedSkills.Find(1);
		if (Lv) BasicSkillLevel = *Lv;
	}

	RecomputeEligibility();
	ServerErrorMessage.Empty();
	bRequestInflight = false;
	CurrentPage = EJobChangePage::Greeting;
	ShowWidget();
}

void UJobChangeSubsystem::CloseDialog()
{
	HideWidget();
	bRequestInflight = false;
	ServerErrorMessage.Empty();
}

void UJobChangeSubsystem::GoToGreetingPage()
{
	CurrentPage = EJobChangePage::Greeting;
	ServerErrorMessage.Empty();
}

void UJobChangeSubsystem::GoToSelectionPage()
{
	CurrentPage = EJobChangePage::Selection;
	ServerErrorMessage.Empty();
}

void UJobChangeSubsystem::RequestChangeJob(const FString& TargetClass)
{
	if (bRequestInflight) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("targetClass"), TargetClass);

	bRequestInflight = true;
	ServerErrorMessage.Empty();
	GI->EmitSocketEvent(TEXT("job:change"), Payload);

	UE_LOG(LogJobChange, Log, TEXT("Requesting job:change → %s"), *TargetClass);
}

// ============================================================
// Eligibility
// ============================================================

void UJobChangeSubsystem::RecomputeEligibility()
{
	EligibleTargets.Reset();
	RequirementMessage.Empty();

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI)
	{
		Eligibility = EJobEligibility::AlreadySecondClass;
		return;
	}

	const FCharacterData SelChar = GI->GetSelectedCharacter();
	const FString CurrentClass = SelChar.JobClass.ToLower();
	const int32 Tier = JobChangeData::GetTier(CurrentClass);

	if (Tier == 0)
	{
		// Novice
		if (CurrentJobLevel < 10)
		{
			Eligibility = EJobEligibility::NoviceJobLevelTooLow;
			RequirementMessage = FString::Printf(
				TEXT("You must reach Job Level 10 first. (current: %d/10)"),
				CurrentJobLevel);
			return;
		}
		if (BasicSkillLevel < 9)
		{
			Eligibility = EJobEligibility::NoviceBasicSkillTooLow;
			RequirementMessage = FString::Printf(
				TEXT("Master your Basic Skill first. (current: %d/9)"),
				BasicSkillLevel);
			return;
		}
		Eligibility = EJobEligibility::Ready;
		for (const FString& First : JobChangeData::FirstClasses)
		{
			FJobChangeTarget T;
			T.ClassId = First;
			T.DisplayName = GetClassDisplayName(First);
			EligibleTargets.Add(T);
		}
		return;
	}

	if (Tier == 1)
	{
		// First class — needs job 40+
		if (CurrentJobLevel < 40)
		{
			Eligibility = EJobEligibility::FirstClassJobLevelTooLow;
			RequirementMessage = FString::Printf(
				TEXT("You must reach Job Level 40 first. (current: %d/40)"),
				CurrentJobLevel);
			return;
		}
		Eligibility = EJobEligibility::Ready;
		for (const auto& Path : JobChangeData::GetSecondClassUpgrades())
		{
			if (Path.From == CurrentClass)
			{
				for (const FString& To : Path.To)
				{
					FJobChangeTarget T;
					T.ClassId = To;
					T.DisplayName = GetClassDisplayName(To);
					EligibleTargets.Add(T);
				}
				break;
			}
		}
		return;
	}

	if (Tier == 2)
	{
		Eligibility = EJobEligibility::AlreadySecondClass;
		RequirementMessage = TEXT("You have already mastered your craft.");
		return;
	}

	// Tier 3 (transcendent) or unknown — placeholder.
	Eligibility = EJobEligibility::Transcendent;
	RequirementMessage = TEXT("Speak with the Valkyrie in Juno.");
}

// ============================================================
// Event handlers
// ============================================================

void UJobChangeSubsystem::HandleJobChanged(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	int32 EventCharId = 0;
	if (Obj->TryGetNumberField(TEXT("characterId"), CharIdD))
	{
		EventCharId = (int32)CharIdD;
	}

	UWorld* World = GetWorld();
	UMMOGameInstance* GI = World ? Cast<UMMOGameInstance>(World->GetGameInstance()) : nullptr;
	if (!GI) return;

	const FCharacterData SelChar = GI->GetSelectedCharacter();

	// Only handle our own job:changed here. Remote players are handled by
	// UOtherPlayerSubsystem's separate job:changed handler.
	if (EventCharId != 0 && EventCharId != SelChar.CharacterId)
	{
		return;
	}

	FString NewClass;
	Obj->TryGetStringField(TEXT("newClass"), NewClass);
	FString NewClassDisplay;
	Obj->TryGetStringField(TEXT("newClassDisplayName"), NewClassDisplay);

	if (NewClass.IsEmpty()) return;

	bRequestInflight = false;
	NewClassDisplayName = NewClassDisplay.IsEmpty() ? GetClassDisplayName(NewClass) : NewClassDisplay;

	// Update GameInstance so future zone transitions / ASabriMMOCharacter::BeginPlay
	// spawn the correct sprite.
	GI->SetSelectedCharacterJobClass(NewClass);

	// Respawn the local player sprite to reflect the new class.
	RespawnLocalPlayerSprite(NewClass);

	// Refresh skill tree — server doesn't auto-emit skill:data after job change,
	// so the new-class tree stays unloaded without this poke.
	GI->EmitSocketEvent(TEXT("skill:data"), TEXT("{}"));

	// Switch dialog to congrats page.
	CurrentPage = EJobChangePage::Congrats;

	UE_LOG(LogJobChange, Log,
		TEXT("Local job change confirmed: → %s. Sprite respawned, skill tree refresh requested."),
		*NewClass);
}

void UJobChangeSubsystem::HandleJobError(const TSharedPtr<FJsonValue>& Data)
{
	bRequestInflight = false;

	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString Msg;
	if ((*ObjPtr)->TryGetStringField(TEXT("message"), Msg))
	{
		ServerErrorMessage = Msg;
		UE_LOG(LogJobChange, Warning, TEXT("Job change rejected: %s"), *Msg);
	}
}

// ============================================================
// Local sprite respawn
// ============================================================

void UJobChangeSubsystem::RespawnLocalPlayerSprite(const FString& NewJobClass)
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	ASabriMMOCharacter* MyChar = Cast<ASabriMMOCharacter>(PC->GetPawn());
	if (!MyChar) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	const FCharacterData SelChar = GI->GetSelectedCharacter();
	const int32 SpriteClassId = ASpriteCharacterActor::JobClassToId(NewJobClass);
	const int32 SpriteGender = SelChar.Gender.ToLower() == TEXT("female") ? 1 : 0;

	// Unregister old name tag + destroy old sprite.
	UNameTagSubsystem* NTS = World->GetSubsystem<UNameTagSubsystem>();
	if (MyChar->LocalSprite.IsValid())
	{
		ASpriteCharacterActor* OldSprite = MyChar->LocalSprite.Get();
		if (NTS && OldSprite)
		{
			NTS->UnregisterEntity(OldSprite);
		}
		if (OldSprite)
		{
			OldSprite->Destroy();
		}
		MyChar->LocalSprite.Reset();
	}

	// Spawn new sprite at the character's current location.
	const FVector SpawnLoc = MyChar->GetActorLocation();
	ASpriteCharacterActor* NewSprite = ASpriteCharacterActor::SpawnSpriteForClass(
		World, SpawnLoc, SpriteClassId, SpriteGender);
	if (!NewSprite)
	{
		UE_LOG(LogJobChange, Warning,
			TEXT("RespawnLocalPlayerSprite: SpawnSpriteForClass returned null for %s"),
			*NewJobClass);
		return;
	}

	NewSprite->AttachToOwnerActor(MyChar, true);
	MyChar->LocalSprite = NewSprite;

	// Reapply hair if set.
	if (SelChar.HairStyle > 0)
	{
		NewSprite->SetHairStyle(SelChar.HairStyle, SelChar.HairColor);
	}

	// Re-register name tag against the new sprite.
	if (NTS && !SelChar.Name.IsEmpty())
	{
		NTS->RegisterEntity(NewSprite, SelChar.Name,
			ENameTagEntityType::LocalPlayer, 0, 120.f, 150.f);
	}
}

// ============================================================
// Widget lifecycle
// ============================================================

void UJobChangeSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	Widget = SNew(SJobChangeWidget).Subsystem(this);

	// Center the popup naturally via alignment; drag offset is applied through SetRenderTransform.
	// SelfHitTestInvisible lets clicks pass through the empty viewport area outside the popup.
	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			Widget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 26);
	bWidgetAdded = true;

	// Lock player movement while dialog is open.
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (ACharacter* Char = PC->GetCharacter())
		{
			if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
			{
				MovComp->DisableMovement();
			}
		}
	}

	UE_LOG(LogJobChange, Log, TEXT("Job change widget shown (Z=26)."));
}

void UJobChangeSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* ViewportClient = World->GetGameViewport();
		if (ViewportClient && ViewportOverlay.IsValid())
		{
			ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
	}

	Widget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;

	// Unlock player movement.
	if (World)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ACharacter* Char = PC->GetCharacter())
			{
				if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
				{
					MovComp->SetMovementMode(MOVE_Walking);
				}
			}
		}
	}
}

bool UJobChangeSubsystem::IsWidgetVisible() const
{
	return bWidgetAdded;
}
