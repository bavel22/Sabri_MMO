#include "GroundItemActor.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Texture2D.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/SlateBrush.h"
#include "EngineUtils.h"
#include "InventorySubsystem.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogGroundItem, Log, All);

AGroundItemActor::AGroundItemActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // Only tick during arc/fade

	// Click/hover detection box
	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	ClickBox->SetBoxExtent(FVector(30.f, 30.f, 25.f));
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ClickBox->SetCanEverAffectNavigation(false);
	ClickBox->SetHiddenInGame(true);
	RootComponent = ClickBox;

	// Icon widget — Screen space so it always faces camera and scales naturally
	IconWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("IconWidget"));
	IconWidget->SetupAttachment(RootComponent);
	IconWidget->SetWidgetSpace(EWidgetSpace::Screen);
	IconWidget->SetDrawSize(FVector2D(32.f, 32.f));
	IconWidget->SetPivot(FVector2D(0.5f, 0.5f));
	IconWidget->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	IconWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IconWidget->SetCastShadow(false);
	IconWidget->SetCanEverAffectNavigation(false);

	// Name label widget — Screen space, attached to icon so distance is constant
	NameWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameWidget"));
	NameWidget->SetupAttachment(IconWidget); // Attached to icon, not root — stays relative
	NameWidget->SetWidgetSpace(EWidgetSpace::Screen);
	NameWidget->SetDrawSize(FVector2D(200.f, 20.f));
	NameWidget->SetPivot(FVector2D(0.5f, 0.f));
	NameWidget->SetRelativeLocation(FVector(5.f, 0.f, -20.f)); // Below icon, slightly forward so it draws in front
	NameWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NameWidget->SetCastShadow(false);
	NameWidget->SetCanEverAffectNavigation(false);
	NameWidget->SetVisibility(false); // Hidden until hover
	NameWidget->SetTranslucentSortPriority(100); // Draw in front of icon
}

void AGroundItemActor::BeginPlay()
{
	Super::BeginPlay();
	// Ground snap is handled by PlayDropArc (for all items) or SnapToGround (deferred)
}

void AGroundItemActor::SnapToGround()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector Loc = GetActorLocation();
	FVector Start = FVector(Loc.X, Loc.Y, Loc.Z + 500.f);
	FVector End   = FVector(Loc.X, Loc.Y, Loc.Z - 2000.f);
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// Ignore all pawns, sprites, and ground items — only hit terrain
	for (TActorIterator<APawn> PIt(World); PIt; ++PIt)
		Params.AddIgnoredActor(*PIt);
	for (TActorIterator<AGroundItemActor> GIt(World); GIt; ++GIt)
		Params.AddIgnoredActor(*GIt);

	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		SetActorLocation(FVector(Loc.X, Loc.Y, Hit.ImpactPoint.Z));
	}
}

void AGroundItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Drop arc animation
	if (bAnimatingArc)
	{
		ArcElapsed += DeltaTime;
		float Alpha = FMath::Clamp(ArcElapsed / ArcDuration, 0.f, 1.f);
		float EasedAlpha = 1.f - FMath::Pow(1.f - Alpha, 2.f);

		FVector Pos = FMath::Lerp(ArcStart, ArcEnd, EasedAlpha);
		float ParabolaT = Alpha * 2.f - 1.f;
		float ZOffset = ArcHeight * (1.f - ParabolaT * ParabolaT);
		Pos.Z += ZOffset;
		SetActorLocation(Pos);

		if (Alpha >= 1.f)
		{
			bAnimatingArc = false;
			SetActorLocation(ArcEnd);
			SetClickable(true);
			if (!bFadingOut) SetActorTickEnabled(false);
		}
	}

	// Fade-out animation
	if (bFadingOut)
	{
		FadeElapsed += DeltaTime;
		float Alpha = FMath::Clamp(FadeElapsed / FadeDuration, 0.f, 1.f);
		float Scale = FMath::Lerp(1.f, 0.f, Alpha);
		SetActorScale3D(FVector(Scale, Scale, Scale));

		if (Alpha >= 1.f)
		{
			Destroy();
		}
	}
}

void AGroundItemActor::InitGroundItem(int32 InGroundItemId, const FString& InItemName,
                                       const FString& InIcon, const FString& InTierColor, int32 InQuantity)
{
	GroundItemId = InGroundItemId;

	// Tier color
	FColor LabelColor = FColor::White;
	if (InTierColor == TEXT("red"))         LabelColor = FColor(255, 80, 80);
	else if (InTierColor == TEXT("purple")) LabelColor = FColor(200, 120, 255);
	else if (InTierColor == TEXT("blue"))   LabelColor = FColor(100, 180, 255);
	else if (InTierColor == TEXT("green"))  LabelColor = FColor(100, 255, 100);
	else if (InTierColor == TEXT("yellow")) LabelColor = FColor(255, 255, 100);
	else if (InTierColor == TEXT("pink"))   LabelColor = FColor(255, 180, 200);

	// Name label text (shown on hover) — always include quantity
	FString LabelText = FString::Printf(TEXT("%s x%d"), *InItemName, InQuantity);

	if (NameWidget)
	{
		FLinearColor LC = LabelColor.ReinterpretAsLinear();
		NameWidget->SetSlateWidget(
			SNew(STextBlock)
			.Text(FText::FromString(LabelText))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FSlateColor(LC))
			.Justification(ETextJustify::Center)
		);
	}

	// Get icon texture from InventorySubsystem (single source of truth for icon loading)
	UWorld* World = GetWorld();
	if (World)
	{
		if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
		{
			FSlateBrush* Brush = InvSub->GetOrCreateItemIconBrush(InIcon);
			if (Brush)
			{
				IconTexture = Cast<UTexture2D>(Brush->GetResourceObject());
			}
		}
	}

	// Set up icon widget
	if (IconTexture)
	{

		IconBrush = MakeShared<FSlateBrush>();
		IconBrush->SetResourceObject(IconTexture);
		IconBrush->ImageSize = FVector2D(32.f, 32.f);
		IconBrush->DrawAs = ESlateBrushDrawType::Image;

		if (IconWidget)
		{
			IconWidget->SetSlateWidget(
				SNew(SImage).Image(IconBrush.Get())
			);
		}
	}
	else
	{
		UE_LOG(LogGroundItem, Warning, TEXT("Ground item %d: no icon for '%s'"), GroundItemId, *InIcon);
		if (IconWidget)
		{
			IconWidget->SetSlateWidget(
				SNew(SImage).ColorAndOpacity(LabelColor.ReinterpretAsLinear())
			);
		}
	}
}

void AGroundItemActor::PlayDropArc(const FVector& SourcePos, const FVector& FinalPos, float Duration)
{
	ArcStart = SourcePos;
	ArcEnd = FinalPos;

	// Ground-snap both endpoints — ignore all pawns and ground items, only hit terrain
	UWorld* World = GetWorld();
	if (World)
	{
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		for (TActorIterator<APawn> PIt(World); PIt; ++PIt)
			Params.AddIgnoredActor(*PIt);
		for (TActorIterator<AGroundItemActor> GIt(World); GIt; ++GIt)
			Params.AddIgnoredActor(*GIt);

		auto SnapZ = [&](FVector& Pos)
		{
			FVector Start = FVector(Pos.X, Pos.Y, Pos.Z + 500.f);
			FVector End   = FVector(Pos.X, Pos.Y, Pos.Z - 2000.f);
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
			{
				Pos.Z = Hit.ImpactPoint.Z;
			}
		};

		SnapZ(ArcStart);
		SnapZ(ArcEnd);
	}

	ArcDuration = FMath::Max(Duration, 0.1f);
	ArcElapsed = 0.f;
	ArcHeight = 80.f;
	bAnimatingArc = true;

	SetActorLocation(ArcStart);
	SetClickable(false);
	SetActorTickEnabled(true);
}

void AGroundItemActor::FadeOutAndDestroy(float Duration)
{
	bFadingOut = true;
	FadeDuration = FMath::Max(Duration, 0.1f);
	FadeElapsed = 0.f;
	SetClickable(false);
	SetActorTickEnabled(true);
}

void AGroundItemActor::SetClickable(bool bClickable)
{
	if (ClickBox)
	{
		ClickBox->SetCollisionResponseToChannel(ECC_Visibility,
			bClickable ? ECR_Block : ECR_Ignore);
	}
}

void AGroundItemActor::SetNameVisible(bool bVisible)
{
	if (NameWidget)
	{
		NameWidget->SetVisibility(bVisible);
	}
}
