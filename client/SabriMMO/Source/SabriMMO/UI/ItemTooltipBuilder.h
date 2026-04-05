#pragma once

#include "CoreMinimal.h"
#include "CharacterData.h"

class SWidget;

/**
 * Shared item tooltip builder — produces consistent tooltips across all widgets.
 * Used by: SInventoryWidget, SEquipmentWidget, SShopWidget, SHotbarRowWidget, SItemInspectWidget (card slots)
 */
namespace ItemTooltipBuilder
{
	/** Build a simple hover tooltip for an item. Shows: formatted name, type, description, key stats, weight. */
	TSharedRef<SWidget> Build(const FInventoryItem& Item);

	/** Format item type for display: "weapon" → "One-Handed Sword", "armor" → "Body Armor", etc. */
	FString FormatItemType(const FInventoryItem& Item);
}
