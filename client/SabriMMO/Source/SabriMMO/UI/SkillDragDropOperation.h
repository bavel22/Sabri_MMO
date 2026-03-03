// SkillDragDropOperation.h — UDragDropOperation subclass for dragging skills
// from the Slate Skill Tree to Blueprint UMG hotbar slots.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "SkillDragDropOperation.generated.h"

UCLASS(BlueprintType)
class SABRIMMO_API USkillDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "SkillDrag")
	int32 SkillId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SkillDrag")
	FString SkillName;

	UPROPERTY(BlueprintReadOnly, Category = "SkillDrag")
	FString SkillDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "SkillDrag")
	int32 SkillLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SkillDrag")
	FString SkillType; // active, passive, toggle

	UPROPERTY(BlueprintReadOnly, Category = "SkillDrag")
	int32 SpCost = 0;
};
