#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterData.h"
#include "ItemInspectSubsystem.generated.h"

class SItemInspectWidget;

/**
 * Manages the item inspect popup window (right-click inspect).
 * Any widget can call ShowInspect(Item) to open the detailed view.
 * Only one inspect window is shown at a time (Z=22).
 */
UCLASS()
class SABRIMMO_API UItemInspectSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Show inspect window for an inventory/equipment item */
	void ShowInspect(const FInventoryItem& Item);

	/** Hide the inspect window */
	void HideInspect();

	/** Is the inspect window currently visible? */
	bool IsInspectVisible() const { return bWidgetVisible; }

	/** The currently inspected item (for widget to read) */
	FInventoryItem InspectedItem;

private:
	void EnsureWidgetCreated();

	bool bWidgetVisible = false;
	bool bWidgetAdded = false;

	TSharedPtr<SItemInspectWidget> Widget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
};
