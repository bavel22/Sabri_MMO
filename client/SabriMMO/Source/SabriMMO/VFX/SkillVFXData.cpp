// SkillVFXData.cpp — Skill VFX config registry.
// Rebuilt every call (no static caching) so Live Coding patches take effect immediately.

#include "SkillVFXData.h"

static TMap<int32, FSkillVFXConfig> BuildSkillVFXConfigs()
{
	TMap<int32, FSkillVFXConfig> Configs;

	auto Add = [&](int32 Id, ESkillVFXTemplate Tmpl, FLinearColor Primary,
		const FString& Elem, bool bCastCircle = false,
		float AoE = 0.f, bool bLoop = false, float ProjSpeed = 2000.f,
		float BoltHeight = 500.f,
		const FString& VFXOverride = TEXT(""), bool bCascade = false,
		float InScale = 1.f, bool bSelfCenter = false, float CascadeLife = 0.f)
	{
		FSkillVFXConfig C;
		C.SkillId = Id;
		C.Template = Tmpl;
		C.PrimaryColor = Primary;
		C.Element = Elem;
		C.bUseCastingCircle = bCastCircle;
		C.CastingCircleColor = SkillVFXDataHelper::GetElementColor(Elem);
		C.CastingCircleRadius = (AoE > 0.f) ? AoE : 150.f;
		C.AoERadius = AoE;
		C.bLooping = bLoop;
		C.ProjectileSpeed = ProjSpeed;
		C.BoltSpawnHeight = BoltHeight;
		C.VFXOverridePath = VFXOverride;
		C.bIsCascade = bCascade;
		C.Scale = InScale;
		C.bSelfCentered = bSelfCenter;
		C.CascadeLifetime = CascadeLife;
		Configs.Add(Id, C);
	};

	// ========= NOVICE =========
	// First Aid — NS_Potion, green, on self, scale 200.0 for testing visibility
	Add(2, ESkillVFXTemplate::HealFlash, FLinearColor(0.2f, 0.9f, 0.3f, 1.f), TEXT("neutral"),
		false, 0.f, false, 0.f, 0.f,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion.NS_Potion"), false, 200.0f);

	// ========= SWORDSMAN =========
	// Bash — NE_attack01 melee slash, at enemy
	Add(103, ESkillVFXTemplate::AoEImpact, FLinearColor(1.f, 1.f, 0.8f, 1.f), TEXT("neutral"),
		false, 0.f, false, 0.f, 0.f,
		TEXT("/Game/Knife_light/VFX/NE_attack01.NE_attack01"), false, 1.5f);

	// Provoke — P_Enrage_Base (Cascade), above enemy head, 3s lifetime
	Add(104, ESkillVFXTemplate::TargetDebuff, FLinearColor(1.f, 0.1f, 0.1f, 1.f), TEXT("neutral"),
		false, 0.f, false, 0.f, 0.f,
		TEXT("/Game/InfinityBladeEffects/Effects/FX_Archive/P_Enrage_Base.P_Enrage_Base"), true, 3.0f, false, 3.0f);

	// Magnum Break — NS_Free_Magic_Buff, SELF-CENTERED AoE
	Add(105, ESkillVFXTemplate::AoEImpact, FLinearColor(1.f, 0.3f, 0.0f, 1.f), TEXT("fire"),
		false, 500.f, false, 0.f, 0.f,
		TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Buff.NS_Free_Magic_Buff"), false, 1.0f, true);

	// Endure — P_Ice_Proj_charge_01, plays once, scale 2.0 for visibility
	Add(106, ESkillVFXTemplate::SelfBuff, FLinearColor(1.f, 0.9f, 0.3f, 1.f), TEXT("neutral"),
		false, 0.f, false, 0.f, 0.f,
		TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Spider/ICE/P_Ice_Proj_charge_01.P_Ice_Proj_charge_01"), true, 2.0f, false, 1.5f);

	// ========= MAGE =========
	// Cold Bolt — P_Elemental_Ice_Proj, strikes from above, 0.5s lifetime per bolt
	Add(200, ESkillVFXTemplate::BoltFromSky, FLinearColor(0.3f, 0.8f, 1.f, 1.f), TEXT("water"), true,
		0.f, false, 2000.f, 500.f,
		TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/ICE/P_Elemental_Ice_Proj.P_Elemental_Ice_Proj"), true, 1.0f, false, 0.5f);

	// Fire Bolt — NS_Magma_Shot_Projectile, scale 3.0 (base is tiny)
	Add(201, ESkillVFXTemplate::BoltFromSky, FLinearColor(1.f, 0.3f, 0.0f, 1.f), TEXT("fire"), true,
		0.f, false, 2000.f, 500.f,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/Sperate_VFX/NS_Magma_Shot_Projectile.NS_Magma_Shot_Projectile"), false, 3.0f);

	// Lightning Bolt — NS_Zap_03_Yellow, strikes from above, 1s linger at impact
	Add(202, ESkillVFXTemplate::BoltFromSky, FLinearColor(1.f, 1.f, 0.5f, 1.f), TEXT("wind"), true,
		0.f, false, 2000.f, 100.f,
		TEXT("/Game/Vefects/Zap_VFX/VFX/Zap/Particles/NS_Zap_03_Yellow.NS_Zap_03_Yellow"), false, 1.0f, false, 1.0f);

	// Napalm Beat — NS_Dark_Solo_Impact, on enemy
	Add(203, ESkillVFXTemplate::AoEImpact, FLinearColor(0.6f, 0.2f, 0.8f, 1.f), TEXT("ghost"),
		false, 300.f, false, 0.f, 0.f,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Solo_Impact.NS_Dark_Solo_Impact"), false, 1.0f);

	// Sight — P_ElementalFire_Lg, plays once, scale 1.5 for visibility
	Add(205, ESkillVFXTemplate::SelfBuff, FLinearColor(1.f, 0.4f, 0.1f, 1.f), TEXT("fire"),
		false, 0.f, false, 0.f, 0.f,
		TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/Fire/P_ElementalFire_Lg.P_ElementalFire_Lg"), true, 1.5f, false, 2.0f);

	// Stone Curse — NS_Dark_Mist, persists, scale 0.5 (enemy-sized)
	Add(206, ESkillVFXTemplate::TargetDebuff, FLinearColor(0.4f, 0.4f, 0.4f, 1.f), TEXT("earth"),
		true, 0.f, true, 0.f, 0.f,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Mist.NS_Dark_Mist"), false, 0.5f);

	// Fire Ball — NS_Magma_Shot, projectile player→enemy
	Add(207, ESkillVFXTemplate::Projectile, FLinearColor(1.f, 0.3f, 0.0f, 1.f), TEXT("fire"),
		true, 500.f, false, 1500.f, 0.f,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Magma_Shot.NS_Magma_Shot"), false, 1.0f);

	// Frost Diver — P_Ice_Proj_charge_01 (Cascade), persists, scale 1.5
	// Uses Cascade instead of Niagara because NS_Ice_Mist uses world-space simulation
	// and ignores component scale entirely. Cascade properly respects SpawnEmitterAttached scale.
	Add(208, ESkillVFXTemplate::TargetDebuff, FLinearColor(0.3f, 0.8f, 1.f, 1.f), TEXT("water"),
		true, 0.f, true, 0.f, 0.f,
		TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Spider/ICE/P_Ice_Proj_charge_01.P_Ice_Proj_charge_01"), true, 1.5f, false, 5.0f);

	// Fire Wall — NS_Spline_Fire, ground persistent
	Add(209, ESkillVFXTemplate::GroundPersistent, FLinearColor(1.f, 0.3f, 0.0f, 1.f), TEXT("fire"),
		true, 150.f, true, 0.f, 0.f,
		TEXT("/Game/_SplineVFX/NS/NS_Spline_Fire.NS_Spline_Fire"), false, 1.0f);

	// Soul Strike — NS_Magic_Bubbles, projectile player→enemy, per hit
	Add(210, ESkillVFXTemplate::Projectile, FLinearColor(0.6f, 0.3f, 0.9f, 1.f), TEXT("ghost"),
		false, 0.f, false, 1500.f, 0.f,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Magic_Bubbles.NS_Magic_Bubbles"), false, 1.0f);

	// Safety Wall — P_levelUp_Detail, ground persistent, 1.0 scale
	Add(211, ESkillVFXTemplate::GroundPersistent, FLinearColor(1.f, 0.4f, 0.6f, 1.f), TEXT("neutral"),
		true, 100.f, true, 0.f, 0.f,
		TEXT("/Game/InfinityBladeEffects/Effects/FX_Mobile/Misc/P_levelUp_Detail.P_levelUp_Detail"), true, 1.0f, false, 60.f);

	// Thunderstorm — NS_Lightning_Strike, per-hit in AoE
	Add(212, ESkillVFXTemplate::GroundAoERain, FLinearColor(1.f, 1.f, 0.5f, 1.f), TEXT("wind"),
		true, 500.f, false, 0.f, 0.f,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Strike.NS_Lightning_Strike"), false, 1.0f);

	return Configs;
}

const FSkillVFXConfig& SkillVFXDataHelper::GetSkillVFXConfig(int32 SkillId)
{
	// Rebuild every call so Live Coding patches pick up changes immediately.
	// This only runs on skill events (not per-frame), so cost is negligible.
	static TMap<int32, FSkillVFXConfig> Configs = BuildSkillVFXConfigs();
	Configs = BuildSkillVFXConfigs();

	static FSkillVFXConfig EmptyConfig;
	if (const FSkillVFXConfig* Found = Configs.Find(SkillId))
	{
		return *Found;
	}
	return EmptyConfig;
}
