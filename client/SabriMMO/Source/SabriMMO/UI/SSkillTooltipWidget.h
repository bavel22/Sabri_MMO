// SSkillTooltipWidget.h — RO Classic-style skill tooltip with per-level breakdown
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

struct FSkillEntry;
class USkillTreeSubsystem;

class SSkillTooltipWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSkillTooltipWidget) {}
		SLATE_ARGUMENT(const FSkillEntry*, SkillData)
		SLATE_ARGUMENT(USkillTreeSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildHeader(const FSkillEntry& Skill, USkillTreeSubsystem* Sub);
	TSharedRef<SWidget> BuildSkillInfo(const FSkillEntry& Skill);
	TSharedRef<SWidget> BuildPrerequisites(const FSkillEntry& Skill, USkillTreeSubsystem* Sub);
	TSharedRef<SWidget> BuildDescription(const FSkillEntry& Skill);
	TSharedRef<SWidget> BuildLevelTable(const FSkillEntry& Skill);

	static FLinearColor GetElementColor(const FString& Element);
	static FString FormatTime(int32 TimeMs);
	static FString GetSkillTypeLabel(const FString& Type);
	static FString GetTargetLabel(const FString& TargetType);
};
