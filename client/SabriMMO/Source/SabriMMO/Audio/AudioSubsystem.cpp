// AudioSubsystem.cpp — Monster sound playback for the enemy SFX layer.

#include "AudioSubsystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Dom/JsonObject.h"
#include "Misc/CoreDelegates.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "UI/EnemySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMOAudio, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// ---- Build the monster sound lookup table ----
	// Asset paths assume wavs were imported under Content/SabriMMO/Audio/SFX/Monsters/
	const FString Root = TEXT("/Game/SabriMMO/Audio/SFX/Monsters/");

	// ---- Poring family ----
	// Per Divine Pride: all four primary sounds + monster_poring as body material.
	// All of poring/poporing/drops/marin/mastering share this set in classic.
	// Single-variant arrays — Poring's classic sounds are not multi-take.
	{
		FMonsterSoundConfig Poring;
		Poring.AttackPaths        = { Root + TEXT("poring_attack") };
		Poring.DamagePaths        = { Root + TEXT("poring_damage") };
		Poring.DiePaths           = { Root + TEXT("poring_die") };
		Poring.MovePaths          = { Root + TEXT("poring_move") };
		Poring.StandPaths         = {};                                  // no stand sound in classic
		Poring.BodyMaterialPath   = Root + TEXT("monster_poring");       // layered with damage + die
		Poring.StandIntervalSeconds = 0.f;

		MonsterSoundMap.Add(TEXT("poring"),    Poring);
		MonsterSoundMap.Add(TEXT("poporing"),  Poring);
		MonsterSoundMap.Add(TEXT("drops"),     Poring);
		MonsterSoundMap.Add(TEXT("marin"),     Poring);
		MonsterSoundMap.Add(TEXT("mastering"), Poring);
	}

	// ---- Skeleton (basic, mob 1076) ----
	// Per Divine Pride canonical mapping:
	//   attack -> skel_soldier_attack.wav
	//   damage -> skel_damage.wav
	//   die    -> skel_archer_die.wav
	//   body   -> monster_skelton.wav (sic — original Korean misspelling preserved)
	// Skeletons walk silently in classic (no move sound, no idle ambient).
	{
		FMonsterSoundConfig Skeleton;
		Skeleton.AttackPaths        = { Root + TEXT("skel_soldier_attack") };
		Skeleton.DamagePaths        = { Root + TEXT("skel_damage") };
		Skeleton.DiePaths           = { Root + TEXT("skel_archer_die") };
		Skeleton.MovePaths          = {};
		Skeleton.StandPaths         = {};
		Skeleton.BodyMaterialPath   = Root + TEXT("monster_skelton");
		Skeleton.StandIntervalSeconds = 0.f;

		MonsterSoundMap.Add(TEXT("skeleton"), Skeleton);
	}

	// ============================================================
	// Pre-wired monster sound configs — data entered ahead of
	// sprite implementation. These entries sit dormant until the
	// server template in ro_monster_templates.js gets
	//     spriteClass: '<key>', weaponMode: 0
	// added. Once the server sends spriteClass in enemy:spawn,
	// the audio resolves automatically with zero code changes.
	//
	// Per Divine Pride canonical mapping where verifiable. Body
	// material layering follows the RO Classic 2-layer "primary
	// impact + body crumple" model. Empty path fields mean "no
	// sound for this event" (silently skipped — e.g. a monster
	// with no _damage wav stays silent on hit reactions).
	//
	// Original RO filenames preserve Korean misspellings —
	// chocho (Chonchon), elder_wilow (Elder Willow, single L),
	// monster_skelton (Skeleton body, missing E) — do not "fix".
	// ============================================================
	{
		// Body material full paths (layered with Damage and Die).
		const FString Insect     = Root + TEXT("monster_insect");
		const FString Shell      = Root + TEXT("monster_shell");
		const FString Feather    = Root + TEXT("monster_feather");
		const FString Flesh      = Root + TEXT("monster_flesh");
		const FString Clothes    = Root + TEXT("monster_clothes");
		const FString Woodenmail = Root + TEXT("monster_woodenmail");
		const FString Plant      = Root + TEXT("monster_plant");
		const FString Reptiles   = Root + TEXT("monster_reptiles");
		const FString Zombie     = Root + TEXT("monster_zombie");
		const FString Stone      = Root + TEXT("monster_stone");
		const FString Metal      = Root + TEXT("monster_metal");
		const FString Poring     = Root + TEXT("monster_poring");
		const FString Skelton    = Root + TEXT("monster_skelton");

		// Variant-aware helper — accepts initializer lists for each event type.
		// Pass {} for "no sound", {single} for one wav, or {a, b, c} for multiple
		// variants randomly picked at playback time. Variant arrays prevent
		// robotic repetition for monsters with multi-take sound sets in classic
		// RO (ghoul_attack1..4, hornet_stand + hornet_stand2, vadon_die1..3, etc.).
		auto AddMonster = [this, &Root](
			const TCHAR* Key,
			std::initializer_list<const TCHAR*> Atks,
			std::initializer_list<const TCHAR*> Dmgs,
			std::initializer_list<const TCHAR*> Dies,
			std::initializer_list<const TCHAR*> Movs,
			std::initializer_list<const TCHAR*> Stands,
			const FString& Body,
			float StandSec = 5.f)
		{
			auto Build = [&Root](std::initializer_list<const TCHAR*> Names) -> TArray<FString>
			{
				TArray<FString> Out;
				Out.Reserve(static_cast<int32>(Names.size()));
				for (const TCHAR* Name : Names)
				{
					if (Name && *Name)
					{
						Out.Add(Root + Name);
					}
				}
				return Out;
			};

			FMonsterSoundConfig Cfg;
			Cfg.AttackPaths          = Build(Atks);
			Cfg.DamagePaths          = Build(Dmgs);
			Cfg.DiePaths             = Build(Dies);
			Cfg.MovePaths            = Build(Movs);
			Cfg.StandPaths           = Build(Stands);
			Cfg.BodyMaterialPath     = Body;
			Cfg.StandIntervalSeconds = (Cfg.StandPaths.Num() > 0) ? StandSec : 0.f;
			MonsterSoundMap.Add(FString(Key), Cfg);
		};

		// ---- Prontera Field family (prt_fild01-12) ----
		AddMonster(TEXT("fabre"),     {TEXT("fabre_attack")},     {TEXT("fabre_damage")},  {TEXT("fabre_die")},  {},                                                          {},                                                                         Insect);
		AddMonster(TEXT("creamy"),    {TEXT("creamy_attack")},    {TEXT("creamy_damage")}, {TEXT("creamy_die")}, {TEXT("creamy_move")},                                       {},                                                                         Insect);
		AddMonster(TEXT("hornet"),    {TEXT("hornet_attack"), TEXT("hornet_attack1"), TEXT("hornet_attack2")}, {TEXT("hornet_damage")}, {TEXT("hornet_die")}, {TEXT("hornet_move")}, {TEXT("hornet_stand"), TEXT("hornet_stand2")}, Insect, 8.f);
		AddMonster(TEXT("lunatic"),   {TEXT("lunatic_attack")},   {},                       {TEXT("lunatic_die")}, {},                                                         {},                                                                         Feather);
		AddMonster(TEXT("picky"),     {},                          {TEXT("picky_damage")},  {TEXT("picky_die")},  {TEXT("picky_hop")},                                         {},                                                                         Shell);
		AddMonster(TEXT("spore"),     {TEXT("spore_attack")},     {TEXT("spore_damage")},  {TEXT("spore_die")},  {TEXT("spore_move1"), TEXT("spore_move2")},                  {},                                                                         Flesh);
		AddMonster(TEXT("roda_frog"), {TEXT("roda_frog_attack1"), TEXT("roda_frog_attack2")}, {TEXT("roda_frog_damage")}, {TEXT("roda_frog_die")}, {},                       {TEXT("roda_frog_stand")},                                                  Reptiles, 6.f);
		AddMonster(TEXT("chonchon"),  {TEXT("chocho_attack")},    {TEXT("chocho_damage")}, {TEXT("chocho_die")}, {},                                                          {TEXT("chocho_stand"), TEXT("chocho_stand1"), TEXT("chocho_stand2")},      Insect, 5.f);
		AddMonster(TEXT("coco"),      {TEXT("coco_attack")},      {},                       {TEXT("coco_die"), TEXT("coco_die2")}, {TEXT("coco_move")},                       {},                                                                         Feather);
		AddMonster(TEXT("caramel"),   {TEXT("caramel_attack")},   {},                       {TEXT("caramel_die")}, {TEXT("caramel_move")},                                    {},                                                                         Feather);
		AddMonster(TEXT("argos"),     {TEXT("argos_attack")},     {TEXT("argos_damage")},  {},                    {TEXT("argos_move")},                                       {},                                                                         Insect);
		AddMonster(TEXT("wild_rose"), {TEXT("wild_rose_attack")}, {},                       {TEXT("wild_rose_die")}, {},                                                       {},                                                                         Plant);

		// ---- Geffen Field / Dungeon family (gef_fild01-14, gef_dun) ----
		AddMonster(TEXT("stainer"),      {TEXT("stainer_attack")},     {},                                                              {TEXT("stainer_die")},      {TEXT("stainer_move")},   {},                                  Insect);
		AddMonster(TEXT("wolf"),         {TEXT("wolf_attack")},        {TEXT("wolf_damage")},                                          {TEXT("wolf_die")},         {},                       {TEXT("wolf_stand")},                Feather, 7.f);
		AddMonster(TEXT("goblin"),       {TEXT("goblin_attack")},      {TEXT("goblin_damage")},                                        {TEXT("goblin_die")},       {},                       {},                                  Clothes);
		AddMonster(TEXT("mantis"),       {TEXT("mantis_attack")},      {},                                                              {TEXT("mantis_die")},       {TEXT("mantis_move")},    {},                                  Insect);
		AddMonster(TEXT("argiope"),      {TEXT("argiope_attack")},     {},                                                              {TEXT("argiope_die")},      {TEXT("argiope_move")},   {},                                  Insect);
		AddMonster(TEXT("elder_willow"), {TEXT("elder_wilow_attack")}, {TEXT("elder_wilow_damage"), TEXT("elder_wilow_damage2")},      {TEXT("elder_wilow_die"), TEXT("elder_wilow_die2")}, {}, {},                                  Woodenmail);  // wav filenames keep single-L misspelling
		AddMonster(TEXT("dustiness"),    {TEXT("dustiness_attack")},   {},                                                              {},                          {TEXT("dustiness_move")}, {TEXT("dustiness_stand")},           Insect, 6.f);
		AddMonster(TEXT("giearth"),      {TEXT("giearth_attack")},     {},                                                              {TEXT("giearth_die")},      {TEXT("giearth_move")},   {},                                  Clothes);
		AddMonster(TEXT("deviruchi"),    {TEXT("deviruchi_attack")},   {TEXT("deviruchi_damage")},                                     {TEXT("deviruchi_die")},    {TEXT("deviruchi_move")}, {TEXT("deviruchi_stand")},           Clothes, 6.f);
		AddMonster(TEXT("golem"),        {TEXT("golem_attack1"), TEXT("golem_attack2")}, {TEXT("golem_damage")},                       {TEXT("golem_die")},        {TEXT("golem_step1"), TEXT("golem_step2")},                  {},                                  Stone);

		// ---- Payon Field / Cave / Ant Hell family (pay_fild, pay_dun, anthell) ----
		AddMonster(TEXT("yoyo"),         {TEXT("yoyo_attack")},         {TEXT("yoyo_damage")},        {TEXT("yoyo_die")},          {TEXT("yoyo_hop1"), TEXT("yoyo_hop2")},                            {},                                                            Feather);
		AddMonster(TEXT("worm_tail"),    {TEXT("worm_tail_attack")},    {TEXT("worm_tail_damage")},   {TEXT("worm_tail_die")},     {},                                                                {},                                                            Reptiles);
		AddMonster(TEXT("andre"),        {TEXT("andre_attack")},        {},                            {TEXT("andre_die")},         {TEXT("andre_move")},                                              {TEXT("andre_stand")},                                         Shell, 6.f);
		AddMonster(TEXT("vitata"),       {TEXT("vitata_attack")},       {},                            {TEXT("vitata_die")},        {},                                                                {},                                                            Shell);
		AddMonster(TEXT("savage"),       {TEXT("savage_attack")},       {},                            {TEXT("savage_die")},        {TEXT("savage_move")},                                             {TEXT("savage_stand")},                                        Feather, 7.f);
		AddMonster(TEXT("sohee"),        {TEXT("sohee_attack")},        {},                            {TEXT("sohee_die")},         {},                                                                {TEXT("sohee_stand")},                                         Clothes, 6.f);
		AddMonster(TEXT("nine_tail"),    {TEXT("nine_tail_attack")},    {},                            {TEXT("nine_tail_die")},     {TEXT("nine_tail_move")},                                          {},                                                            Feather);
		AddMonster(TEXT("horong"),       {TEXT("horong_attack")},       {TEXT("horong_damage")},      {TEXT("horong_die")},        {TEXT("horong_move")},                                             {TEXT("horong_stand")},                                        Clothes, 6.f);
		AddMonster(TEXT("baby_leopard"), {TEXT("baby_leopard_attack")}, {TEXT("baby_leopard_damage")},{TEXT("baby_leopard_die")},  {},                                                                {TEXT("baby_leopard_stand")},                                  Feather, 6.f);
		AddMonster(TEXT("bigfoot"),      {TEXT("bigfoot_attack")},      {TEXT("bigfoot_damage")},     {TEXT("bigfoot_die"), TEXT("bigfoot_die2")},                                                     {TEXT("bigfoot_step1"), TEXT("bigfoot_step2")},                {TEXT("bigfoot_stand1"), TEXT("bigfoot_stand2")},          Feather, 7.f);

		// ---- Morroc / Sograt Desert / Pyramid / Sphinx family ----
		AddMonster(TEXT("frilldora"), {TEXT("frilldora_attack")},  {TEXT("frilldora_damage")},  {TEXT("frilldora_die")},  {TEXT("frilldora_move")}, {},                                                Reptiles);
		AddMonster(TEXT("scorpion"),  {TEXT("scorpion_attack")},   {TEXT("scorpion_damage")},   {TEXT("scorpion_die")},   {},                       {},                                                Insect);
		AddMonster(TEXT("mummy"),     {TEXT("mummy_attack")},      {TEXT("mummy_damage")},      {TEXT("mummy_die")},      {TEXT("mummy_move1"), TEXT("mummy_move2")},                                  {}, Clothes);
		AddMonster(TEXT("isis"),      {TEXT("isis_attack")},       {TEXT("isis_damage")},       {TEXT("isis_die")},       {},                       {},                                                Clothes);
		AddMonster(TEXT("marduk"),    {TEXT("marduk_attack")},     {TEXT("marduk_damage")},     {TEXT("marduk_die")},     {TEXT("marduk_move")},    {TEXT("marduk_stand")},                            Clothes, 6.f);
		AddMonster(TEXT("osiris"),    {TEXT("osiris_attack1"), TEXT("osiris_attack2")}, {TEXT("osiris_damage")},          {TEXT("osiris_die")},     {},                       {},                                                Clothes);
		AddMonster(TEXT("maya"),      {TEXT("maya_attack1"), TEXT("maya_attack2")},      {},                              {TEXT("maya_die1"), TEXT("maya_die2")},                                       {TEXT("maya_move")},      {},                                                Clothes);

		// ---- Coastal / Izlude / Byalan family ----
		AddMonster(TEXT("vadon"),     {TEXT("vadon_attack")},     {TEXT("vadon_damage")},   {TEXT("vadon_die1"), TEXT("vadon_die2"), TEXT("vadon_die3")}, {},                          {TEXT("vadon_stand")},     Shell, 6.f);
		AddMonster(TEXT("crab"),      {TEXT("crab_attack")},      {},                        {TEXT("crab_die")},      {},                                                            {TEXT("crab_stand")},      Shell, 6.f);
		AddMonster(TEXT("shellfish"), {TEXT("shellfish_attack")}, {},                        {TEXT("shellfish_die")}, {TEXT("shellfish_move")},                                       {},                        Shell);
		AddMonster(TEXT("ambernite"), {TEXT("ambernite_attack")}, {},                        {TEXT("ambernite_die")}, {},                                                            {TEXT("ambernite_stand")}, Shell, 6.f);
		AddMonster(TEXT("marc"),      {TEXT("marc_attack")},      {TEXT("marc_damage")},    {TEXT("marc_die")},      {},                                                            {},                        Reptiles);
		AddMonster(TEXT("obeaune"),   {TEXT("obeaune_attack")},   {TEXT("obeaune_damage")}, {TEXT("obeaune_die")},   {TEXT("obeaune_move1"), TEXT("obeaune_move2")},                {},                        Clothes);
		AddMonster(TEXT("kobold"),    {TEXT("kobold_attack")},    {TEXT("kobold_damage")},  {TEXT("kobold_die")},    {},                                                            {},                        Feather);

		// ---- Glast Heim family (gl_*) ----
		AddMonster(TEXT("ghoul"),           {TEXT("ghoul_attack1"), TEXT("ghoul_attack2"), TEXT("ghoul_attack3"), TEXT("ghoul_attack4")}, {TEXT("ghoul_damage")},     {TEXT("ghoul_die1"), TEXT("ghoul_die2"), TEXT("ghoul_die3")}, {TEXT("ghoul_step1"), TEXT("ghoul_step2")},                  {},                            Zombie);
		AddMonster(TEXT("zombie_prisoner"), {TEXT("zombie_prisoner_attack")}, {},                                                                                                                                          {TEXT("zombie_prisoner_die")}, {TEXT("zombie_prisoner_move1"), TEXT("zombie_prisoner_move2")}, {TEXT("zombie_prisoner_stand")}, Zombie, 7.f);
		AddMonster(TEXT("evil_druid"),      {TEXT("evil_druid_attack")},      {TEXT("evil_druid_damage")},                                                                                                                {TEXT("evil_druid_die"), TEXT("evil_druid_die2")},              {TEXT("evil_druid_move")},                                  {},                            Clothes);
		AddMonster(TEXT("raydric"),         {TEXT("raydric_attack")},         {TEXT("raydric_damage")},                                                                                                                   {TEXT("raydric_die")},          {TEXT("raydric_move")},                                     {},                            Metal);
		AddMonster(TEXT("khalitzburg"),     {TEXT("khalitzburg_attack")},     {},                                                                                                                                          {TEXT("khalitzburg_die")},      {TEXT("khalitzburg_move")},                                 {},                            Metal);
		AddMonster(TEXT("injustice"),       {TEXT("injustice_attack")},       {},                                                                                                                                          {TEXT("injustice_die")},        {TEXT("injustice_move")},                                   {},                            Metal);
		AddMonster(TEXT("wraith"),          {TEXT("wraith_attack")},          {},                                                                                                                                          {TEXT("wraith_die")},           {},                                                          {},                            Clothes);
		AddMonster(TEXT("skel_worker"),     {TEXT("skel_worker_attack")},     {},                                                                                                                                          {TEXT("skel_worker_die")},      {TEXT("skel_worker_move")},                                  {},                            Skelton);

		// ---- MVP / Boss family ----
		AddMonster(TEXT("eddga"),    {TEXT("eddga_attack"), TEXT("eddga_attack2")}, {},                       {TEXT("eddga_die")},    {TEXT("eddga_move")}, {}, Feather);
		AddMonster(TEXT("baphomet"), {TEXT("baphomet_attack")},                     {TEXT("baphomet_damage")}, {TEXT("baphomet_die")}, {},                   {}, Clothes);

		// ---- Lutie / Christmas family ----
		AddMonster(TEXT("cookie"), {TEXT("cookie_attack")}, {TEXT("cookie_damage")}, {TEXT("cookie_die")}, {TEXT("cookie_move")}, {TEXT("cookie_stand")}, Flesh, 6.f);

		// ---- Poring family variants (own sound set, not an alias) ----
		AddMonster(TEXT("botaring"), {TEXT("botaring_attack")}, {TEXT("botaring_damage")}, {TEXT("botaring_die")}, {TEXT("botaring_move")}, {}, Poring);
	}

	// ---- Programmatic 3D attenuation ----
	// Sphere falloff so spatialization works regardless of per-asset import settings.
	MonsterAttenuation = NewObject<USoundAttenuation>(this, TEXT("MonsterAttenuation"));
	if (MonsterAttenuation)
	{
		FSoundAttenuationSettings& A = MonsterAttenuation->Attenuation;
		A.bAttenuate              = true;
		A.bSpatialize             = true;
		A.AttenuationShape        = EAttenuationShape::Sphere;
		A.AttenuationShapeExtents = FVector(300.f);
		A.FalloffDistance         = 3000.f;
		A.DistanceAlgorithm       = EAttenuationDistanceModel::Linear;
	}

	// ---- Concurrency limits per event type ----
	// Caps prevent audio chaos when many entities are active. Lifetime is the
	// duration each play counts toward the cap (approx avg sound length).
	auto AddCap = [this](EMonsterSoundType Type, int32 Max, double Lifetime)
	{
		FConcurrencyState State;
		State.MaxConcurrent = Max;
		State.Lifetime = Lifetime;
		ConcurrencyTracking.Add(Type, State);
	};
	AddCap(EMonsterSoundType::Attack, 8, 1.5);
	AddCap(EMonsterSoundType::Damage, 8, 1.0);
	AddCap(EMonsterSoundType::Die,    6, 2.5);
	AddCap(EMonsterSoundType::Move,   6, 0.6);
	AddCap(EMonsterSoundType::Stand,  4, 2.0);

	// ---- Player sound maps (per-class swing/hit + body materials + singletons) ----
	// Map KEYS use the server's job class strings ("swordsman", "mage", etc. — see
	// server/src/ro_exp_tables.js HP_SP_COEFFICIENTS for the canonical list).
	// VALUES point to the legacy RO Korean wav file names ("swordman_attack",
	// "magician_attack" — preserved as-is for authenticity).
	// Only 7 base classes are stored; second/transcendent classes (knight, wizard,
	// lord_knight, etc.) fall back via ResolveBaseClassFor().
	{
		const FString PlayerRoot = TEXT("/Game/SabriMMO/Audio/SFX/Player/");

		ClassAttackSoundMap.Add(TEXT("novice"),    PlayerRoot + TEXT("novice_attack"));
		ClassAttackSoundMap.Add(TEXT("swordsman"), PlayerRoot + TEXT("swordman_attack"));
		ClassAttackSoundMap.Add(TEXT("mage"),      PlayerRoot + TEXT("magician_attack"));
		ClassAttackSoundMap.Add(TEXT("acolyte"),   PlayerRoot + TEXT("acolyte_attack"));
		ClassAttackSoundMap.Add(TEXT("merchant"),  PlayerRoot + TEXT("merchant_attack"));
		ClassAttackSoundMap.Add(TEXT("archer"),    PlayerRoot + TEXT("archer_attack"));
		ClassAttackSoundMap.Add(TEXT("thief"),     PlayerRoot + TEXT("thief_attack"));

		ClassHitSoundMap.Add(TEXT("novice"),    PlayerRoot + TEXT("novice_hit"));
		ClassHitSoundMap.Add(TEXT("swordsman"), PlayerRoot + TEXT("swordman_hit"));
		ClassHitSoundMap.Add(TEXT("mage"),      PlayerRoot + TEXT("magician_hit"));
		ClassHitSoundMap.Add(TEXT("acolyte"),   PlayerRoot + TEXT("acolyte_hit"));
		ClassHitSoundMap.Add(TEXT("merchant"),  PlayerRoot + TEXT("merchant_hit"));
		ClassHitSoundMap.Add(TEXT("archer"),    PlayerRoot + TEXT("archer_hit"));
		ClassHitSoundMap.Add(TEXT("thief"),     PlayerRoot + TEXT("thief_hit"));

		BodyMaterialSoundMap.Add(EPlayerBodyMaterial::Cloth,       PlayerRoot + TEXT("player_clothes"));
		BodyMaterialSoundMap.Add(EPlayerBodyMaterial::WoodLeather, PlayerRoot + TEXT("player_wooden_male"));
		BodyMaterialSoundMap.Add(EPlayerBodyMaterial::Metal,       PlayerRoot + TEXT("player_metal"));

		LevelUpSoundPath = PlayerRoot + TEXT("levelup");
		HealSoundPath    = PlayerRoot + TEXT("heal_effect");
	}

	// ---- Weapon-type sound maps (RO Classic primary path) ----
	// Server weapon type strings (from ro_exp_tables.js ASPD_BASE_DELAYS) map to wav
	// files in /Game/SabriMMO/Audio/SFX/Weapons/. Several weapon types share wavs
	// (1H+2H sword both use sword, katar/whip/instrument/book reuse mace per RO Classic).
	{
		const FString WepRoot = TEXT("/Game/SabriMMO/Audio/SFX/Weapons/");

		// Swing sounds (attack frame)
		WeaponAttackSoundMap.Add(TEXT("dagger"),         WepRoot + TEXT("attack_dagger"));
		WeaponAttackSoundMap.Add(TEXT("one_hand_sword"), WepRoot + TEXT("attack_sword"));
		WeaponAttackSoundMap.Add(TEXT("two_hand_sword"), WepRoot + TEXT("attack_sword"));
		WeaponAttackSoundMap.Add(TEXT("spear"),          WepRoot + TEXT("attack_spear"));
		WeaponAttackSoundMap.Add(TEXT("two_hand_spear"), WepRoot + TEXT("attack_spear"));
		WeaponAttackSoundMap.Add(TEXT("one_hand_axe"),   WepRoot + TEXT("attack_axe"));
		WeaponAttackSoundMap.Add(TEXT("two_hand_axe"),   WepRoot + TEXT("attack_axe"));
		WeaponAttackSoundMap.Add(TEXT("axe"),            WepRoot + TEXT("attack_axe"));
		WeaponAttackSoundMap.Add(TEXT("mace"),           WepRoot + TEXT("attack_mace"));
		WeaponAttackSoundMap.Add(TEXT("staff"),          WepRoot + TEXT("attack_rod"));
		WeaponAttackSoundMap.Add(TEXT("rod"),            WepRoot + TEXT("attack_rod"));
		WeaponAttackSoundMap.Add(TEXT("bow"),            WepRoot + TEXT("attack_bow"));
		WeaponAttackSoundMap.Add(TEXT("katar"),          WepRoot + TEXT("attack_katar"));
		WeaponAttackSoundMap.Add(TEXT("book"),           WepRoot + TEXT("attack_book"));
		// knuckle / whip / instrument intentionally have no swing wav — they fall back
		// to per-class swing via PlayWeaponAttackSound returning false.

		// Hit sounds (impact frame)
		WeaponHitSoundMap.Add(TEXT("dagger"),         WepRoot + TEXT("hit_dagger"));
		WeaponHitSoundMap.Add(TEXT("one_hand_sword"), WepRoot + TEXT("hit_sword"));
		WeaponHitSoundMap.Add(TEXT("two_hand_sword"), WepRoot + TEXT("hit_sword"));
		WeaponHitSoundMap.Add(TEXT("spear"),          WepRoot + TEXT("hit_spear"));
		WeaponHitSoundMap.Add(TEXT("two_hand_spear"), WepRoot + TEXT("hit_spear"));
		WeaponHitSoundMap.Add(TEXT("one_hand_axe"),   WepRoot + TEXT("hit_axe"));
		WeaponHitSoundMap.Add(TEXT("two_hand_axe"),   WepRoot + TEXT("hit_axe"));
		WeaponHitSoundMap.Add(TEXT("axe"),            WepRoot + TEXT("hit_axe"));
		WeaponHitSoundMap.Add(TEXT("mace"),           WepRoot + TEXT("hit_mace"));
		WeaponHitSoundMap.Add(TEXT("staff"),          WepRoot + TEXT("hit_rod"));
		WeaponHitSoundMap.Add(TEXT("rod"),            WepRoot + TEXT("hit_rod"));
		WeaponHitSoundMap.Add(TEXT("bow"),            WepRoot + TEXT("hit_arrow"));
		// RO Classic re-uses mace hit for katar / book / whip / instrument
		WeaponHitSoundMap.Add(TEXT("katar"),          WepRoot + TEXT("hit_mace"));
		WeaponHitSoundMap.Add(TEXT("book"),           WepRoot + TEXT("hit_mace"));
		WeaponHitSoundMap.Add(TEXT("whip"),           WepRoot + TEXT("hit_mace"));
		WeaponHitSoundMap.Add(TEXT("instrument"),     WepRoot + TEXT("hit_mace"));

		// Fist hit variants — randomized when weapon type is knuckle/bare_hand/unarmed
		FistHitSoundPaths.Add(WepRoot + TEXT("hit_fist1"));
		FistHitSoundPaths.Add(WepRoot + TEXT("hit_fist2"));
		FistHitSoundPaths.Add(WepRoot + TEXT("hit_fist3"));
		FistHitSoundPaths.Add(WepRoot + TEXT("hit_fist4"));
	}

	// ---- Skill SFX maps (per-skill cast/impact sounds) ----
	// Files in /Game/SabriMMO/Audio/SFX/Skills/ named skill_<name> (one per skill).
	// Most RO Classic skills only have an impact sound; the cast map is for the few
	// that also need a windup sound. Auto-attack sounds are NOT here — those live
	// in WeaponAttackSoundMap / ClassAttackSoundMap.
	{
		const FString SkillRoot = TEXT("/Game/SabriMMO/Audio/SFX/Skills/");

		// ---- COMPLETE skill SFX map for ALL active first/second-class skills.
		//      226 entries spanning 18 classes. Tags in comments:
		//        [unique]   = canonical RO Classic wav for this skill
		//        [shared]   = intentionally shares a sound with a related skill
		//        [fallback] = best-effort substitute (no canonical wav exists)
		//        [missing]  = canonical wav not extracted from data.grf yet (2)
		//      ALL active/toggle skills now have an entry. No active skill is silent.
		//      (Verified 2026-04-08 by cross-checking against ro_skill_data.js +
		//      ro_skill_data_2nd.js — passive skills are intentionally excluded.)

		// ---- NOVICE ----
		SkillImpactSoundMap.Add(   2, TEXT("/Game/SabriMMO/Audio/SFX/Player/heal_effect"));  // first_aid [unique]
		SkillImpactSoundMap.Add(   3, SkillRoot + TEXT("skill_hiding"));                // play_dead [fallback - shares the "go silent" stealth-cue]

		// ---- SWORDSMAN ----
		SkillImpactSoundMap.Add( 103, SkillRoot + TEXT("skill_bash"));                  // bash [unique]
		SkillImpactSoundMap.Add( 104, SkillRoot + TEXT("skill_provoke"));               // provoke [unique]
		SkillImpactSoundMap.Add( 105, SkillRoot + TEXT("skill_magnum_break"));          // magnum_break [unique]
		SkillImpactSoundMap.Add( 106, SkillRoot + TEXT("skill_endure"));                // endure [unique]
		// CRITICAL FIX (2026-04-08): server skill ID for auto_berserk is 108, NOT 109.
		// Old code mapped 109 (which is fatal_blow, a passive) and Auto Berserk (108)
		// played nothing. Verified against ro_skill_data.js: id 108 = auto_berserk.
		SkillImpactSoundMap.Add( 108, SkillRoot + TEXT("skill_auto_berserk"));          // auto_berserk [fallback]

		// ---- MAGE ----
		SkillImpactSoundMap.Add( 200, SkillRoot + TEXT("skill_cold_bolt"));             // cold_bolt [unique]
		SkillImpactSoundMap.Add( 201, SkillRoot + TEXT("skill_fire_bolt"));             // fire_bolt [unique]
		SkillImpactSoundMap.Add( 202, SkillRoot + TEXT("skill_lightning_bolt"));        // lightning_bolt [unique]
		SkillImpactSoundMap.Add( 203, SkillRoot + TEXT("skill_napalm_beat"));           // napalm_beat [unique]
		SkillImpactSoundMap.Add( 205, SkillRoot + TEXT("skill_sight"));                 // sight [unique]
		SkillImpactSoundMap.Add( 206, SkillRoot + TEXT("skill_stone_curse"));           // stone_curse [unique]
		SkillImpactSoundMap.Add( 207, SkillRoot + TEXT("skill_fire_ball"));             // fire_ball [unique]
		SkillImpactSoundMap.Add( 208, SkillRoot + TEXT("skill_frost_diver"));           // frost_diver [unique]
		SkillImpactSoundMap.Add( 209, SkillRoot + TEXT("skill_fire_wall"));             // fire_wall [unique]
		SkillImpactSoundMap.Add( 210, SkillRoot + TEXT("skill_soul_strike"));           // soul_strike [unique]
		SkillImpactSoundMap.Add( 211, SkillRoot + TEXT("skill_safety_wall"));           // safety_wall [unique]
		SkillImpactSoundMap.Add( 212, SkillRoot + TEXT("skill_thunderstorm"));          // thunderstorm [unique]
		SkillImpactSoundMap.Add( 213, SkillRoot + TEXT("skill_energy_coat"));           // energy_coat [missing]

		// ---- ARCHER ----
		SkillImpactSoundMap.Add( 302, SkillRoot + TEXT("skill_improve_concentration"));  // improve_concentration [unique]
		SkillImpactSoundMap.Add( 303, SkillRoot + TEXT("skill_double_strafe"));          // double_strafe [fallback]
		SkillImpactSoundMap.Add( 304, SkillRoot + TEXT("skill_arrow_shower"));           // arrow_shower [fallback]
		SkillImpactSoundMap.Add( 305, SkillRoot + TEXT("skill_arrow_crafting"));         // arrow_crafting [fallback]
		SkillImpactSoundMap.Add( 306, SkillRoot + TEXT("skill_arrow_repel"));            // arrow_repel [fallback]

		// ---- ACOLYTE ----
		SkillImpactSoundMap.Add( 400, TEXT("/Game/SabriMMO/Audio/SFX/Player/heal_effect")); // heal [unique]
		SkillImpactSoundMap.Add( 402, SkillRoot + TEXT("skill_blessing"));               // blessing [unique]
		SkillImpactSoundMap.Add( 403, SkillRoot + TEXT("skill_increase_agi"));           // increase_agi [unique]
		SkillImpactSoundMap.Add( 404, SkillRoot + TEXT("skill_decrease_agi"));           // decrease_agi [unique]
		SkillImpactSoundMap.Add( 405, SkillRoot + TEXT("skill_cure"));                   // cure [unique]
		SkillImpactSoundMap.Add( 406, SkillRoot + TEXT("skill_angelus"));                // angelus [unique]
		SkillImpactSoundMap.Add( 407, SkillRoot + TEXT("skill_signum_crucis"));          // signum_crucis [unique]
		SkillImpactSoundMap.Add( 408, SkillRoot + TEXT("skill_ruwach"));                 // ruwach [unique]
		SkillImpactSoundMap.Add( 409, SkillRoot + TEXT("skill_teleport"));               // teleport [unique]
		SkillImpactSoundMap.Add( 410, SkillRoot + TEXT("skill_warp_portal"));            // warp_portal [unique]
		SkillImpactSoundMap.Add( 411, SkillRoot + TEXT("skill_pneuma"));                 // pneuma [missing]
		SkillImpactSoundMap.Add( 412, SkillRoot + TEXT("skill_aqua_benedicta"));         // aqua_benedicta [unique]
		SkillImpactSoundMap.Add( 414, SkillRoot + TEXT("skill_holy_light"));             // holy_light [unique]

		// ---- THIEF ----
		SkillImpactSoundMap.Add( 502, SkillRoot + TEXT("skill_steal"));                  // steal [unique]
		SkillImpactSoundMap.Add( 503, SkillRoot + TEXT("skill_hiding"));                 // hiding [unique]
		SkillImpactSoundMap.Add( 504, SkillRoot + TEXT("skill_envenom"));                // envenom [unique]
		SkillImpactSoundMap.Add( 505, SkillRoot + TEXT("skill_detoxify"));               // detoxify [unique]
		SkillImpactSoundMap.Add( 506, SkillRoot + TEXT("skill_sand_attack"));            // sand_attack [fallback]
		SkillImpactSoundMap.Add( 507, SkillRoot + TEXT("skill_backslide"));              // backslide [fallback]
		SkillImpactSoundMap.Add( 508, SkillRoot + TEXT("skill_throw_stone"));            // throw_stone [fallback]
		SkillImpactSoundMap.Add( 509, SkillRoot + TEXT("skill_pick_stone"));             // pick_stone [fallback]

		// ---- MERCHANT ----
		SkillImpactSoundMap.Add( 603, SkillRoot + TEXT("skill_mammonite"));              // mammonite [unique]
		SkillImpactSoundMap.Add( 605, SkillRoot + TEXT("skill_vending"));                // vending [fallback]
		SkillImpactSoundMap.Add( 606, SkillRoot + TEXT("skill_item_appraisal"));         // item_appraisal [fallback]
		SkillImpactSoundMap.Add( 607, SkillRoot + TEXT("skill_cart_revolution"));        // change_cart [fallback - shares the cart-themed thunk]
		SkillImpactSoundMap.Add( 608, SkillRoot + TEXT("skill_cart_revolution"));        // cart_revolution [fallback]
		SkillImpactSoundMap.Add( 609, SkillRoot + TEXT("skill_loud_exclamation"));       // loud_exclamation [fallback]

		// ---- KNIGHT ----
		SkillImpactSoundMap.Add( 701, SkillRoot + TEXT("skill_pierce"));                 // pierce [fallback] — multi-hit (1-3 vs size)
		SkillImpactSoundMap.Add( 702, SkillRoot + TEXT("skill_spear_stab"));             // spear_stab [fallback]
		SkillImpactSoundMap.Add( 703, SkillRoot + TEXT("skill_brandish_spear"));         // brandish_spear [unique]
		SkillImpactSoundMap.Add( 704, SkillRoot + TEXT("skill_spear_boomerang"));        // spear_boomerang [unique]
		SkillImpactSoundMap.Add( 705, SkillRoot + TEXT("skill_two_hand_quicken"));       // two_hand_quicken [unique]
		SkillImpactSoundMap.Add( 706, SkillRoot + TEXT("skill_auto_counter"));           // auto_counter [unique]
		SkillImpactSoundMap.Add( 707, SkillRoot + TEXT("skill_bowling_bash"));           // bowling_bash [unique]
		SkillImpactSoundMap.Add( 710, SkillRoot + TEXT("skill_charge_attack"));          // charge_attack [fallback]

		// ---- WIZARD ----
		SkillImpactSoundMap.Add( 800, SkillRoot + TEXT("skill_jupitel_thunder"));        // jupitel_thunder [fallback] — multi-hit
		SkillImpactSoundMap.Add( 801, SkillRoot + TEXT("skill_lord_of_vermilion"));      // lord_of_vermilion [fallback]
		SkillImpactSoundMap.Add( 802, SkillRoot + TEXT("skill_meteor_storm"));           // meteor_storm [unique]
		SkillImpactSoundMap.Add( 803, SkillRoot + TEXT("skill_storm_gust"));             // storm_gust [unique]
		SkillImpactSoundMap.Add( 804, SkillRoot + TEXT("skill_earth_spike"));            // earth_spike [unique] — multi-hit
		SkillImpactSoundMap.Add( 805, SkillRoot + TEXT("skill_heavens_drive"));          // heavens_drive [shared]
		SkillImpactSoundMap.Add( 806, SkillRoot + TEXT("skill_quagmire"));               // quagmire [unique]
		SkillImpactSoundMap.Add( 807, SkillRoot + TEXT("skill_water_ball"));             // water_ball [unique]
		SkillImpactSoundMap.Add( 808, SkillRoot + TEXT("skill_ice_wall"));               // ice_wall [unique]
		SkillImpactSoundMap.Add( 809, SkillRoot + TEXT("skill_sight_rasher"));           // sight_rasher [unique]
		SkillImpactSoundMap.Add( 810, SkillRoot + TEXT("skill_fire_pillar"));            // fire_pillar [unique]
		SkillImpactSoundMap.Add( 811, SkillRoot + TEXT("skill_frost_nova"));             // frost_nova [fallback]
		SkillImpactSoundMap.Add( 812, SkillRoot + TEXT("skill_sense"));                  // sense [fallback]
		SkillImpactSoundMap.Add( 813, SkillRoot + TEXT("skill_sight_blaster"));          // sight_blaster [shared]

		// ---- HUNTER ----
		SkillImpactSoundMap.Add( 900, SkillRoot + TEXT("skill_blitz_beat"));             // blitz_beat [unique] — multi-hit
		SkillImpactSoundMap.Add( 902, SkillRoot + TEXT("skill_detect"));                 // detect [unique]
		SkillImpactSoundMap.Add( 903, SkillRoot + TEXT("skill_ankle_snare"));            // ankle_snare [unique]
		SkillImpactSoundMap.Add( 904, SkillRoot + TEXT("skill_land_mine"));              // land_mine [unique]
		SkillImpactSoundMap.Add( 905, SkillRoot + TEXT("skill_remove_trap"));            // remove_trap [unique]
		SkillImpactSoundMap.Add( 906, SkillRoot + TEXT("skill_shockwave_trap"));         // shockwave_trap [unique]
		SkillImpactSoundMap.Add( 907, SkillRoot + TEXT("skill_claymore_trap"));          // claymore_trap [unique]
		SkillImpactSoundMap.Add( 908, SkillRoot + TEXT("skill_skid_trap"));              // skid_trap [unique]
		SkillImpactSoundMap.Add( 909, SkillRoot + TEXT("skill_sandman"));                // sandman [unique]
		SkillImpactSoundMap.Add( 910, SkillRoot + TEXT("skill_flasher"));                // flasher [unique]
		SkillImpactSoundMap.Add( 911, SkillRoot + TEXT("skill_freezing_trap"));          // freezing_trap [unique]
		SkillImpactSoundMap.Add( 912, SkillRoot + TEXT("skill_blast_mine"));             // blast_mine [unique]
		SkillImpactSoundMap.Add( 913, SkillRoot + TEXT("skill_spring_trap"));            // spring_trap [unique]
		SkillImpactSoundMap.Add( 914, SkillRoot + TEXT("skill_talkie_box"));             // talkie_box [unique]
		SkillImpactSoundMap.Add( 917, SkillRoot + TEXT("skill_phantasmic_arrow"));       // phantasmic_arrow [fallback]

		// ---- PRIEST ----
		SkillImpactSoundMap.Add(1000, SkillRoot + TEXT("skill_sanctuary"));              // sanctuary [unique]
		SkillImpactSoundMap.Add(1001, SkillRoot + TEXT("skill_kyrie_eleison"));          // kyrie_eleison [unique]
		SkillImpactSoundMap.Add(1002, SkillRoot + TEXT("skill_magnificat"));             // magnificat [unique]
		SkillImpactSoundMap.Add(1003, SkillRoot + TEXT("skill_gloria"));                 // gloria [unique]
		SkillImpactSoundMap.Add(1004, SkillRoot + TEXT("skill_resurrection"));           // resurrection [unique]
		SkillImpactSoundMap.Add(1005, SkillRoot + TEXT("skill_magnus_exorcismus"));      // magnus_exorcismus [unique]
		SkillImpactSoundMap.Add(1006, SkillRoot + TEXT("skill_turn_undead"));            // turn_undead [unique]
		SkillImpactSoundMap.Add(1007, SkillRoot + TEXT("skill_lex_aeterna"));            // lex_aeterna [unique]
		SkillImpactSoundMap.Add(1009, SkillRoot + TEXT("skill_impositio_manus"));        // impositio_manus [unique]
		SkillImpactSoundMap.Add(1010, SkillRoot + TEXT("skill_suffragium"));             // suffragium [unique]
		SkillImpactSoundMap.Add(1011, SkillRoot + TEXT("skill_aspersio"));               // aspersio [unique]
		SkillImpactSoundMap.Add(1012, SkillRoot + TEXT("skill_bs_sacramenti"));          // bs_sacramenti [unique]
		SkillImpactSoundMap.Add(1013, SkillRoot + TEXT("skill_slow_poison"));            // slow_poison [unique]
		SkillImpactSoundMap.Add(1014, SkillRoot + TEXT("skill_status_recovery"));        // status_recovery [unique]
		SkillImpactSoundMap.Add(1015, SkillRoot + TEXT("skill_lex_divina"));             // lex_divina [unique]
		SkillImpactSoundMap.Add(1017, SkillRoot + TEXT("skill_safety_wall_priest"));     // safety_wall_priest [shared]
		SkillImpactSoundMap.Add(1018, SkillRoot + TEXT("skill_redemptio"));              // redemptio [fallback]

		// ---- ASSASSIN ----
		SkillImpactSoundMap.Add(1101, SkillRoot + TEXT("skill_sonic_blow"));             // sonic_blow [unique] — multi-hit (8)
		SkillImpactSoundMap.Add(1102, SkillRoot + TEXT("skill_grimtooth"));              // grimtooth [fallback]
		SkillImpactSoundMap.Add(1103, SkillRoot + TEXT("skill_cloaking"));               // cloaking [unique]
		SkillImpactSoundMap.Add(1104, SkillRoot + TEXT("skill_envenom"));                // poison_react [fallback - shares the poison cue with envenom]
		SkillImpactSoundMap.Add(1105, SkillRoot + TEXT("skill_venom_dust"));             // venom_dust [unique]
		SkillImpactSoundMap.Add(1109, SkillRoot + TEXT("skill_enchant_poison"));         // enchant_poison [unique]
		SkillImpactSoundMap.Add(1110, SkillRoot + TEXT("skill_venom_splasher"));         // venom_splasher [unique]
		SkillImpactSoundMap.Add(1111, SkillRoot + TEXT("skill_throw_venom_knife"));      // throw_venom_knife [fallback]

		// ---- BLACKSMITH ----
		SkillImpactSoundMap.Add(1200, SkillRoot + TEXT("skill_adrenaline_rush"));        // adrenaline_rush [unique]
		SkillImpactSoundMap.Add(1201, SkillRoot + TEXT("skill_weapon_perfection"));      // weapon_perfection [unique]
		SkillImpactSoundMap.Add(1202, SkillRoot + TEXT("skill_power_thrust"));           // power_thrust [unique]
		SkillImpactSoundMap.Add(1203, SkillRoot + TEXT("skill_power_thrust"));           // maximize_power [fallback - shares the strength-buff cue]
		SkillImpactSoundMap.Add(1206, SkillRoot + TEXT("skill_hammer_fall"));            // hammer_fall [unique]
		SkillImpactSoundMap.Add(1209, SkillRoot + TEXT("skill_weapon_repair"));          // weapon_repair [unique]
		SkillImpactSoundMap.Add(1210, SkillRoot + TEXT("skill_greed"));                  // greed [fallback]

		// ---- CRUSADER ----
		SkillImpactSoundMap.Add(1301, SkillRoot + TEXT("skill_auto_guard"));             // auto_guard [fallback]
		SkillImpactSoundMap.Add(1302, SkillRoot + TEXT("skill_holy_cross"));             // holy_cross [unique]
		SkillImpactSoundMap.Add(1303, SkillRoot + TEXT("skill_grand_cross"));            // grand_cross [unique]
		SkillImpactSoundMap.Add(1304, SkillRoot + TEXT("skill_shield_charge"));          // shield_charge [fallback]
		SkillImpactSoundMap.Add(1305, SkillRoot + TEXT("skill_shield_boomerang"));       // shield_boomerang [unique]
		SkillImpactSoundMap.Add(1306, SkillRoot + TEXT("skill_devotion"));               // devotion [fallback]
		SkillImpactSoundMap.Add(1307, SkillRoot + TEXT("skill_reflect_shield"));         // reflect_shield [fallback]
		SkillImpactSoundMap.Add(1308, SkillRoot + TEXT("skill_providence"));             // providence [fallback]
		SkillImpactSoundMap.Add(1309, SkillRoot + TEXT("skill_defender"));               // defender [fallback]
		SkillImpactSoundMap.Add(1310, SkillRoot + TEXT("skill_two_hand_quicken"));       // spear_quicken [fallback - shares the weapon-quicken cue]
		SkillImpactSoundMap.Add(1311, TEXT("/Game/SabriMMO/Audio/SFX/Player/heal_effect")); // heal_crusader [shared]
		SkillImpactSoundMap.Add(1312, SkillRoot + TEXT("skill_cure_crusader"));          // cure_crusader [shared]
		SkillImpactSoundMap.Add(1313, SkillRoot + TEXT("skill_shrink"));                 // shrink [fallback]

		// ---- SAGE ----
		SkillImpactSoundMap.Add(1401, SkillRoot + TEXT("skill_cast_cancel"));            // cast_cancel [fallback]
		SkillImpactSoundMap.Add(1402, SkillRoot + TEXT("skill_hindsight"));              // hindsight [fallback]
		SkillImpactSoundMap.Add(1403, SkillRoot + TEXT("skill_dispell"));                // dispell [fallback]
		SkillImpactSoundMap.Add(1404, SkillRoot + TEXT("skill_magic_rod"));              // magic_rod [unique]
		SkillImpactSoundMap.Add(1406, SkillRoot + TEXT("skill_spell_breaker"));          // spell_breaker [unique]
		SkillImpactSoundMap.Add(1408, SkillRoot + TEXT("skill_endow_blaze"));            // endow_blaze [fallback]
		SkillImpactSoundMap.Add(1409, SkillRoot + TEXT("skill_endow_tsunami"));          // endow_tsunami [fallback]
		SkillImpactSoundMap.Add(1410, SkillRoot + TEXT("skill_endow_tornado"));          // endow_tornado [fallback]
		SkillImpactSoundMap.Add(1411, SkillRoot + TEXT("skill_endow_quake"));            // endow_quake [fallback]
		SkillImpactSoundMap.Add(1412, SkillRoot + TEXT("skill_volcano"));                // volcano [fallback]
		SkillImpactSoundMap.Add(1413, SkillRoot + TEXT("skill_deluge"));                 // deluge [fallback]
		SkillImpactSoundMap.Add(1414, SkillRoot + TEXT("skill_violent_gale"));           // violent_gale [fallback]
		SkillImpactSoundMap.Add(1415, SkillRoot + TEXT("skill_land_protector"));         // land_protector [fallback]
		SkillImpactSoundMap.Add(1416, SkillRoot + TEXT("skill_abracadabra"));            // abracadabra [fallback]
		SkillImpactSoundMap.Add(1417, SkillRoot + TEXT("skill_earth_spike_sage"));       // earth_spike_sage [shared]
		SkillImpactSoundMap.Add(1418, SkillRoot + TEXT("skill_heavens_drive_sage"));     // heavens_drive_sage [shared]
		SkillImpactSoundMap.Add(1419, SkillRoot + TEXT("skill_sense_sage"));             // sense_sage [shared]
		SkillImpactSoundMap.Add(1420, SkillRoot + TEXT("skill_create_converter"));       // create_elemental_converter [fallback]
		SkillImpactSoundMap.Add(1421, SkillRoot + TEXT("skill_elemental_change"));       // elemental_change [fallback]

		// ---- BARD ----
		SkillImpactSoundMap.Add(1501, SkillRoot + TEXT("skill_poem_of_bragi"));          // poem_of_bragi [unique]
		SkillImpactSoundMap.Add(1502, SkillRoot + TEXT("skill_assassin_cross_sunset"));  // assassin_cross_of_sunset [unique]
		SkillImpactSoundMap.Add(1503, SkillRoot + TEXT("skill_adaptation"));             // adaptation [fallback]
		SkillImpactSoundMap.Add(1504, SkillRoot + TEXT("skill_encore"));                 // encore [fallback]
		SkillImpactSoundMap.Add(1505, SkillRoot + TEXT("skill_dissonance"));             // dissonance [unique]
		SkillImpactSoundMap.Add(1506, SkillRoot + TEXT("skill_frost_joker"));            // frost_joker [fallback]
		SkillImpactSoundMap.Add(1507, SkillRoot + TEXT("skill_a_whistle"));              // a_whistle [unique]
		SkillImpactSoundMap.Add(1508, SkillRoot + TEXT("skill_apple_of_idun"));          // apple_of_idun [unique]
		SkillImpactSoundMap.Add(1509, SkillRoot + TEXT("skill_pang_voice"));             // pang_voice [fallback]
		SkillImpactSoundMap.Add(1511, SkillRoot + TEXT("skill_musical_strike"));         // musical_strike [fallback]
		SkillImpactSoundMap.Add(1530, SkillRoot + TEXT("skill_lullaby"));                // lullaby [unique]
		SkillImpactSoundMap.Add(1531, SkillRoot + TEXT("skill_mr_kim_rich"));            // mr_kim_a_rich_man [unique]
		SkillImpactSoundMap.Add(1532, SkillRoot + TEXT("skill_eternal_chaos"));          // eternal_chaos [unique]
		SkillImpactSoundMap.Add(1533, SkillRoot + TEXT("skill_drum_battlefield"));       // drum_on_battlefield [unique]
		SkillImpactSoundMap.Add(1534, SkillRoot + TEXT("skill_nibelungen_ring"));        // ring_of_nibelungen [unique]
		SkillImpactSoundMap.Add(1535, SkillRoot + TEXT("skill_lokis_veil"));             // lokis_veil [unique]
		SkillImpactSoundMap.Add(1536, SkillRoot + TEXT("skill_into_the_abyss"));         // into_the_abyss [unique]
		SkillImpactSoundMap.Add(1537, SkillRoot + TEXT("skill_siegfried"));              // invulnerable_siegfried [unique]

		// ---- DANCER ----
		SkillImpactSoundMap.Add(1521, SkillRoot + TEXT("skill_service_for_you"));        // service_for_you [unique]
		SkillImpactSoundMap.Add(1522, SkillRoot + TEXT("skill_humming"));                // humming [unique]
		SkillImpactSoundMap.Add(1523, SkillRoot + TEXT("skill_adaptation_dancer"));      // adaptation_dancer [fallback]
		SkillImpactSoundMap.Add(1524, SkillRoot + TEXT("skill_encore_dancer"));          // encore_dancer [fallback]
		SkillImpactSoundMap.Add(1525, SkillRoot + TEXT("skill_ugly_dance"));             // ugly_dance [unique]
		SkillImpactSoundMap.Add(1526, SkillRoot + TEXT("skill_scream"));                 // scream [unique]
		SkillImpactSoundMap.Add(1527, SkillRoot + TEXT("skill_dont_forget_me"));         // please_dont_forget_me [unique]
		SkillImpactSoundMap.Add(1528, SkillRoot + TEXT("skill_fortunes_kiss"));          // fortunes_kiss [unique]
		SkillImpactSoundMap.Add(1529, SkillRoot + TEXT("skill_charming_wink"));          // charming_wink [fallback]
		SkillImpactSoundMap.Add(1541, SkillRoot + TEXT("skill_slinging_arrow"));         // slinging_arrow [fallback]
		SkillImpactSoundMap.Add(1550, SkillRoot + TEXT("skill_lullaby_dancer"));         // lullaby_dancer [unique]
		SkillImpactSoundMap.Add(1551, SkillRoot + TEXT("skill_mr_kim_dancer"));          // mr_kim_dancer [shared]
		SkillImpactSoundMap.Add(1552, SkillRoot + TEXT("skill_eternal_chaos_dancer"));   // eternal_chaos_dancer [shared]
		SkillImpactSoundMap.Add(1553, SkillRoot + TEXT("skill_drum_battlefield_dancer"));// drum_battlefield_dancer [shared]
		SkillImpactSoundMap.Add(1554, SkillRoot + TEXT("skill_nibelungen_dancer"));      // nibelungen_dancer [shared]
		SkillImpactSoundMap.Add(1555, SkillRoot + TEXT("skill_lokis_veil_dancer"));      // lokis_veil_dancer [shared]
		SkillImpactSoundMap.Add(1556, SkillRoot + TEXT("skill_into_abyss_dancer"));      // into_abyss_dancer [shared]
		SkillImpactSoundMap.Add(1557, SkillRoot + TEXT("skill_siegfried_dancer"));       // siegfried_dancer [shared]

		// ---- MONK ----
		SkillImpactSoundMap.Add(1601, SkillRoot + TEXT("skill_summon_spirit_sphere"));   // summon_spirit_sphere [fallback]
		SkillImpactSoundMap.Add(1602, SkillRoot + TEXT("skill_investigate"));            // investigate [fallback]
		SkillImpactSoundMap.Add(1604, SkillRoot + TEXT("skill_finger_offensive"));       // finger_offensive [unique] — multi-hit
		SkillImpactSoundMap.Add(1605, SkillRoot + TEXT("skill_asura_strike"));           // asura_strike [unique]
		SkillImpactSoundMap.Add(1607, SkillRoot + TEXT("skill_absorb_spirit_sphere"));   // absorb_spirit_sphere [unique]
		SkillImpactSoundMap.Add(1609, SkillRoot + TEXT("skill_blade_stop"));             // blade_stop [fallback]
		SkillImpactSoundMap.Add(1610, SkillRoot + TEXT("skill_chain_combo"));            // chain_combo [unique] — multi-hit
		SkillImpactSoundMap.Add(1611, SkillRoot + TEXT("skill_critical_explosion"));     // critical_explosion [unique]
		SkillImpactSoundMap.Add(1612, SkillRoot + TEXT("skill_steel_body"));             // steel_body [fallback]
		SkillImpactSoundMap.Add(1613, SkillRoot + TEXT("skill_combo_finish"));           // combo_finish [unique]
		SkillImpactSoundMap.Add(1614, SkillRoot + TEXT("skill_ki_translation"));         // ki_translation [fallback]
		SkillImpactSoundMap.Add(1615, SkillRoot + TEXT("skill_ki_explosion"));           // ki_explosion [unique]

		// ---- ROGUE ----
		SkillImpactSoundMap.Add(1701, SkillRoot + TEXT("skill_back_stab"));              // back_stab [unique]
		SkillImpactSoundMap.Add(1703, SkillRoot + TEXT("skill_raid"));                   // raid [fallback]
		SkillImpactSoundMap.Add(1704, SkillRoot + TEXT("skill_intimidate"));             // intimidate [unique]
		SkillImpactSoundMap.Add(1707, SkillRoot + TEXT("skill_double_strafe_rogue"));    // double_strafe_rogue [shared]
		SkillImpactSoundMap.Add(1708, SkillRoot + TEXT("skill_remove_trap_rogue"));      // remove_trap_rogue [shared]
		SkillImpactSoundMap.Add(1709, SkillRoot + TEXT("skill_steal_coin"));             // steal_coin [unique]
		SkillImpactSoundMap.Add(1710, SkillRoot + TEXT("skill_divest_helm"));            // divest_helm [fallback]
		SkillImpactSoundMap.Add(1711, SkillRoot + TEXT("skill_divest_shield"));          // divest_shield [fallback]
		SkillImpactSoundMap.Add(1712, SkillRoot + TEXT("skill_divest_armor"));           // divest_armor [fallback]
		SkillImpactSoundMap.Add(1713, SkillRoot + TEXT("skill_divest_weapon"));          // divest_weapon [fallback]
		SkillImpactSoundMap.Add(1717, SkillRoot + TEXT("skill_scribble"));               // scribble [fallback]
		SkillImpactSoundMap.Add(1718, SkillRoot + TEXT("skill_close_confine"));          // close_confine [fallback]

		// ---- ALCHEMIST ----
		SkillImpactSoundMap.Add(1800, SkillRoot + TEXT("skill_pharmacy"));               // pharmacy [unique] — uses p_success.wav
		SkillImpactSoundMap.Add(1801, SkillRoot + TEXT("skill_acid_terror"));            // acid_terror [fallback]
		SkillImpactSoundMap.Add(1802, SkillRoot + TEXT("skill_demonstration"));          // demonstration [fallback]
		SkillImpactSoundMap.Add(1803, SkillRoot + TEXT("skill_summon_flora"));           // summon_flora [fallback]
		SkillImpactSoundMap.Add(1806, TEXT("/Game/SabriMMO/Audio/SFX/Player/heal_effect")); // potion_pitcher [fallback]
		SkillImpactSoundMap.Add(1807, SkillRoot + TEXT("skill_summon_marine_sphere"));   // summon_marine_sphere [fallback]
		SkillImpactSoundMap.Add(1808, SkillRoot + TEXT("skill_cp_helm"));                // chemical_protection_helm [fallback]
		SkillImpactSoundMap.Add(1809, SkillRoot + TEXT("skill_cp_shield"));              // chemical_protection_shield [fallback]
		SkillImpactSoundMap.Add(1810, SkillRoot + TEXT("skill_cp_armor"));               // chemical_protection_armor [fallback]
		SkillImpactSoundMap.Add(1811, SkillRoot + TEXT("skill_cp_weapon"));              // chemical_protection_weapon [fallback]
		SkillImpactSoundMap.Add(1813, SkillRoot + TEXT("skill_call_homunculus"));        // call_homunculus [fallback]
		SkillImpactSoundMap.Add(1814, SkillRoot + TEXT("skill_homun_rest"));             // rest [fallback]
		SkillImpactSoundMap.Add(1815, SkillRoot + TEXT("skill_resurrect_homunculus"));   // resurrect_homunculus [fallback]

		// ---- Default cast windup sound ----
		// RO Classic plays a short "spell beginning" sound (ef_beginspell.wav) when
		// ANY skill with a cast time begins. This default is used for every cast
		// skill unless a more specific wav is added to SkillCastSoundMap below.
		DefaultSkillCastSoundPath = SkillRoot + TEXT("skill_cast_spell");

		// SkillCastSoundMap intentionally left empty — every cast skill gets the
		// default. Add per-skill overrides here only when a skill needs a unique
		// windup (e.g. a specialty cast voice), e.g.:
		//   SkillCastSoundMap.Add(SOME_SKILL_ID, SkillRoot + TEXT("skill_cast_unique"));
	}

	// ---- UI / Event sound paths ----
	{
		const FString UIRoot     = TEXT("/Game/SabriMMO/Audio/SFX/UI/");
		const FString EquipRoot  = TEXT("/Game/SabriMMO/Audio/SFX/Equip/");
		const FString DropRoot   = TEXT("/Game/SabriMMO/Audio/SFX/Drops/");
		const FString EventRoot  = TEXT("/Game/SabriMMO/Audio/SFX/Events/");
		const FString VoiceRoot  = TEXT("/Game/SabriMMO/Audio/SFX/PlayerVoices/");

		// Universal UI clicks (Korean source files renamed to button_click / ui_cancel)
		UIClickSoundPath  = UIRoot + TEXT("button_click");
		UICancelSoundPath = UIRoot + TEXT("ui_cancel");

		// 7 equip kachunk variants — random pick per call
		EquipSoundPaths.Add(EquipRoot + TEXT("se_equip_item_01"));
		EquipSoundPaths.Add(EquipRoot + TEXT("se_equip_item_02"));
		EquipSoundPaths.Add(EquipRoot + TEXT("se_equip_item_03"));
		EquipSoundPaths.Add(EquipRoot + TEXT("se_equip_item_04"));
		EquipSoundPaths.Add(EquipRoot + TEXT("se_equip_item_05"));
		EquipSoundPaths.Add(EquipRoot + TEXT("se_equip_item_06"));
		EquipSoundPaths.Add(EquipRoot + TEXT("se_equip_item_07"));

		// Item drop tier color -> chime
		DropSoundPaths.Add(TEXT("pink"),   DropRoot + TEXT("drop_pink"));
		DropSoundPaths.Add(TEXT("yellow"), DropRoot + TEXT("drop_yellow"));
		DropSoundPaths.Add(TEXT("purple"), DropRoot + TEXT("drop_purple"));
		DropSoundPaths.Add(TEXT("blue"),   DropRoot + TEXT("drop_blue"));
		DropSoundPaths.Add(TEXT("green"),  DropRoot + TEXT("drop_green"));
		DropSoundPaths.Add(TEXT("red"),    DropRoot + TEXT("drop_red"));

		// Singleton event sounds
		CoinPickupSoundPath      = EventRoot + TEXT("ef_coin1");
		JobLevelUpSoundPath      = EventRoot + TEXT("st_job_level_up");
		RefineSuccessSoundPath   = EventRoot + TEXT("bs_refinesuccess");
		RefineFailSoundPath      = EventRoot + TEXT("bs_refinefailed");
		PharmacySuccessSoundPath = EventRoot + TEXT("p_success");
		PharmacyFailSoundPath    = EventRoot + TEXT("p_failed");
		MvpFanfareSoundPath      = EventRoot + TEXT("st_mvp");
		WarpPortalSoundPath      = EventRoot + TEXT("ef_portal");
		TeleportSoundPath        = EventRoot + TEXT("ef_teleportation");

		// Player damage voices — female has per-class variants, male only has generic
		// Key format: "<baseclass>_male" or "<baseclass>_female"
		// Source filenames preserve legacy "swordman" / "magician" spellings.
		PlayerDamageVoiceMap.Add(TEXT("novice_female"),    VoiceRoot + TEXT("damage_novice_female"));
		PlayerDamageVoiceMap.Add(TEXT("swordsman_female"), VoiceRoot + TEXT("damage_swordman_female"));
		PlayerDamageVoiceMap.Add(TEXT("mage_female"),      VoiceRoot + TEXT("damage_magician_female"));
		PlayerDamageVoiceMap.Add(TEXT("acolyte_female"),   VoiceRoot + TEXT("damage_acolyte_female"));
		PlayerDamageVoiceMap.Add(TEXT("merchant_female"),  VoiceRoot + TEXT("damage_merchant_female"));
		PlayerDamageVoiceMap.Add(TEXT("archer_female"),    VoiceRoot + TEXT("damage_archer_female"));
		PlayerDamageVoiceMap.Add(TEXT("thief_female"),     VoiceRoot + TEXT("damage_thief_female"));
		// Generic female fallback (used if class lookup misses)
		PlayerDamageVoiceMap.Add(TEXT("__female"), VoiceRoot + TEXT("damage_female"));
		// All male classes share one generic damage_male.wav
		PlayerDamageVoiceMap.Add(TEXT("__male"),   VoiceRoot + TEXT("damage_male"));

		PlayerDeathVoiceMap.Add(TEXT("novice_female"),    VoiceRoot + TEXT("die_novice_female"));
		PlayerDeathVoiceMap.Add(TEXT("swordsman_female"), VoiceRoot + TEXT("die_swordman_female"));
		PlayerDeathVoiceMap.Add(TEXT("mage_female"),      VoiceRoot + TEXT("die_magician_female"));
		PlayerDeathVoiceMap.Add(TEXT("acolyte_female"),   VoiceRoot + TEXT("die_acolyte_female"));
		PlayerDeathVoiceMap.Add(TEXT("merchant_female"),  VoiceRoot + TEXT("die_merchant_female"));
		PlayerDeathVoiceMap.Add(TEXT("archer_female"),    VoiceRoot + TEXT("die_archer_female"));
		PlayerDeathVoiceMap.Add(TEXT("thief_female"),     VoiceRoot + TEXT("die_thief_female"));
		// No generic die_female.wav — fall back to novice_female if class lookup misses
		PlayerDeathVoiceMap.Add(TEXT("__female"), VoiceRoot + TEXT("die_novice_female"));
		PlayerDeathVoiceMap.Add(TEXT("__male"),   VoiceRoot + TEXT("die_male"));
	}

	// ---- Zone ambient bed map ----
	// Each zone gets one or more 2D looped layers (wind + birds + bells = stacked).
	// Multiple zones share the same layer set via aliasing.
	{
		const FString AmbientRoot = TEXT("/Game/SabriMMO/Audio/SFX/Ambient/");

		// Layer paths
		const FString PrtMarket  = AmbientRoot + TEXT("se_prtmarket01");
		const FString PrtBirds   = AmbientRoot + TEXT("se_prtbird_01");
		const FString PrtBell    = AmbientRoot + TEXT("se_prtchbell_01");
		const FString FieldWind1 = AmbientRoot + TEXT("se_field_wind_01");
		const FString FieldWind2 = AmbientRoot + TEXT("se_field_wind_02");
		const FString MocWind    = AmbientRoot + TEXT("se_moc_wind_strong");
		const FString MocWindMid = AmbientRoot + TEXT("se_moc_wind_middle");
		const FString DunWind    = AmbientRoot + TEXT("se_dun_wind01");
		const FString CaveDrip   = AmbientRoot + TEXT("se_subterranean_waterdrop_01");
		const FString CaveFall   = AmbientRoot + TEXT("se_subterranean_waterfall_01");
		const FString PyramidA   = AmbientRoot + TEXT("se_pyramid_a");
		const FString PayGrass   = AmbientRoot + TEXT("se_pay_grass_01");
		const FString PayAnimal  = AmbientRoot + TEXT("se_pay_animal01");

		// ---- Towns ----
		// Prontera (3 layers: market chatter + birds + church bell)
		ZoneAmbientMap.Add(TEXT("prontera"),       { PrtMarket, PrtBirds, PrtBell });
		ZoneAmbientMap.Add(TEXT("prontera_south"), { PrtMarket, PrtBirds, PrtBell });
		ZoneAmbientMap.Add(TEXT("prontera_north"), { PrtMarket, PrtBirds, PrtBell });
		ZoneAmbientMap.Add(TEXT("prontera_east"),  { PrtMarket, PrtBirds, PrtBell });
		ZoneAmbientMap.Add(TEXT("prontera_west"),  { PrtMarket, PrtBirds, PrtBell });

		// Payon (grass + animals — quieter forest village)
		ZoneAmbientMap.Add(TEXT("payon"),       { PayGrass, PayAnimal });
		ZoneAmbientMap.Add(TEXT("payon_south"), { PayGrass, PayAnimal });
		ZoneAmbientMap.Add(TEXT("payon_north"), { PayGrass, PayAnimal });

		// Geffen (wind only — magic city, eerie quiet)
		ZoneAmbientMap.Add(TEXT("geffen"),       { FieldWind2 });
		ZoneAmbientMap.Add(TEXT("geffen_south"), { FieldWind2 });

		// Morroc (desert wind, no birds)
		ZoneAmbientMap.Add(TEXT("morroc"),       { MocWindMid });
		ZoneAmbientMap.Add(TEXT("morroc_south"), { MocWindMid });

		// Alberta / Izlude (port towns — could add seagulls; using field wind for now)
		ZoneAmbientMap.Add(TEXT("alberta"), { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("izlude"),  { FieldWind1, PrtBirds });

		// ---- Field maps (any prt_fild* / pay_fild* / gef_fild* / moc_fild*) ----
		// Generic field wind + birds for grasslands; wind only for deserts
		ZoneAmbientMap.Add(TEXT("prt_fild01"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild02"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild03"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild04"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild05"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild06"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild07"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild08"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild09"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild10"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild11"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("prt_fild12"),  { FieldWind1, PrtBirds });
		ZoneAmbientMap.Add(TEXT("pay_fild01"),  { PayGrass,   PayAnimal });
		ZoneAmbientMap.Add(TEXT("pay_fild02"),  { PayGrass,   PayAnimal });
		ZoneAmbientMap.Add(TEXT("pay_fild03"),  { PayGrass,   PayAnimal });
		ZoneAmbientMap.Add(TEXT("pay_fild04"),  { PayGrass,   PayAnimal });
		ZoneAmbientMap.Add(TEXT("gef_fild01"),  { FieldWind2 });
		ZoneAmbientMap.Add(TEXT("gef_fild02"),  { FieldWind2 });
		ZoneAmbientMap.Add(TEXT("gef_fild03"),  { FieldWind2 });
		ZoneAmbientMap.Add(TEXT("moc_fild01"),  { MocWind });
		ZoneAmbientMap.Add(TEXT("moc_fild02"),  { MocWind });
		ZoneAmbientMap.Add(TEXT("moc_fild03"),  { MocWind });

		// ---- Dungeons ----
		// Generic dungeon wind + drips for cave-like environments
		ZoneAmbientMap.Add(TEXT("ant_hell"),    { DunWind, CaveDrip });
		ZoneAmbientMap.Add(TEXT("ant_hell_01"), { DunWind, CaveDrip });
		ZoneAmbientMap.Add(TEXT("ant_hell_02"), { DunWind, CaveDrip });
		ZoneAmbientMap.Add(TEXT("byalan"),      { CaveFall, CaveDrip });
		ZoneAmbientMap.Add(TEXT("byalan_01"),   { CaveFall, CaveDrip });
		ZoneAmbientMap.Add(TEXT("byalan_02"),   { CaveFall, CaveDrip });
		ZoneAmbientMap.Add(TEXT("payon_dun"),   { DunWind, CaveDrip });
		ZoneAmbientMap.Add(TEXT("pay_dun01"),   { DunWind, CaveDrip });
		ZoneAmbientMap.Add(TEXT("gef_dun01"),   { DunWind });
		ZoneAmbientMap.Add(TEXT("gef_dun02"),   { DunWind });
		ZoneAmbientMap.Add(TEXT("gef_dun03"),   { DunWind });
		ZoneAmbientMap.Add(TEXT("gl_church"),   { DunWind });
		ZoneAmbientMap.Add(TEXT("gl_chyard"),   { DunWind });
		ZoneAmbientMap.Add(TEXT("gl_dun01"),    { DunWind });
		ZoneAmbientMap.Add(TEXT("gl_dun02"),    { DunWind });

		// ---- Pyramids / Sphinx ----
		ZoneAmbientMap.Add(TEXT("moc_pryd01"), { PyramidA, MocWindMid });
		ZoneAmbientMap.Add(TEXT("moc_pryd02"), { PyramidA, MocWindMid });
		ZoneAmbientMap.Add(TEXT("moc_pryd03"), { PyramidA, MocWindMid });
		ZoneAmbientMap.Add(TEXT("moc_pryd04"), { PyramidA, MocWindMid });
		ZoneAmbientMap.Add(TEXT("in_sphinx1"), { MocWind });
		ZoneAmbientMap.Add(TEXT("in_sphinx2"), { MocWind });
		ZoneAmbientMap.Add(TEXT("in_sphinx3"), { MocWind });
	}

	// ---- Status effect sound map ----
	// Played when server emits status:applied with the matching statusType.
	// Files mirror RO Classic's data\wav\_<status>.wav set, with leading underscore
	// stripped (UE5 import compatibility).
	{
		const FString StatusRoot = TEXT("/Game/SabriMMO/Audio/SFX/Status/");
		StatusSoundMap.Add(TEXT("stun"),        StatusRoot + TEXT("stun"));
		StatusSoundMap.Add(TEXT("curse"),       StatusRoot + TEXT("curse"));
		StatusSoundMap.Add(TEXT("poison"),      StatusRoot + TEXT("poison"));
		StatusSoundMap.Add(TEXT("blind"),       StatusRoot + TEXT("blind"));
		StatusSoundMap.Add(TEXT("silence"),     StatusRoot + TEXT("silence"));
		StatusSoundMap.Add(TEXT("freeze"),      StatusRoot + TEXT("freeze"));
		StatusSoundMap.Add(TEXT("frozen"),      StatusRoot + TEXT("freeze"));   // alias
		StatusSoundMap.Add(TEXT("stone"),       StatusRoot + TEXT("stone"));
		StatusSoundMap.Add(TEXT("stone curse"), StatusRoot + TEXT("stone"));    // alias
		StatusSoundMap.Add(TEXT("sleep"),       StatusRoot + TEXT("sleep"));
		StatusSoundMap.Add(TEXT("confusion"),   StatusRoot + TEXT("confusion"));
		StatusSoundMap.Add(TEXT("confused"),    StatusRoot + TEXT("confusion"));  // alias
	}

	// ---- BGM zone → track map (mp3nametable-style assignment) ----
	// Asset paths: /Game/SabriMMO/Audio/BGM/bgm_NN where NN matches RO Classic
	// pre-renewal track numbers. Drawn from the research catalog (§2.1–2.3).
	{
		const FString BgmRoot = TEXT("/Game/SabriMMO/Audio/BGM/");

		// ---- Towns ----
		ZoneToBgmMap.Add(TEXT("prontera"),       BgmRoot + TEXT("bgm_08")); // Theme of Prontera
		ZoneToBgmMap.Add(TEXT("prontera_south"), BgmRoot + TEXT("bgm_08"));
		ZoneToBgmMap.Add(TEXT("prontera_north"), BgmRoot + TEXT("bgm_08"));
		ZoneToBgmMap.Add(TEXT("prontera_east"),  BgmRoot + TEXT("bgm_08"));
		ZoneToBgmMap.Add(TEXT("prontera_west"),  BgmRoot + TEXT("bgm_08"));
		ZoneToBgmMap.Add(TEXT("morroc"),         BgmRoot + TEXT("bgm_11")); // Theme of Morroc
		ZoneToBgmMap.Add(TEXT("morroc_south"),   BgmRoot + TEXT("bgm_11"));
		ZoneToBgmMap.Add(TEXT("geffen"),         BgmRoot + TEXT("bgm_13")); // Theme of Geffen
		ZoneToBgmMap.Add(TEXT("geffen_south"),   BgmRoot + TEXT("bgm_13"));
		ZoneToBgmMap.Add(TEXT("payon"),          BgmRoot + TEXT("bgm_14")); // Theme of Payon
		ZoneToBgmMap.Add(TEXT("payon_south"),    BgmRoot + TEXT("bgm_14"));
		ZoneToBgmMap.Add(TEXT("payon_north"),    BgmRoot + TEXT("bgm_14"));
		ZoneToBgmMap.Add(TEXT("alberta"),        BgmRoot + TEXT("bgm_15")); // Theme of Alberta
		ZoneToBgmMap.Add(TEXT("izlude"),         BgmRoot + TEXT("bgm_26")); // Everlasting Wanderers
		ZoneToBgmMap.Add(TEXT("aldebaran"),      BgmRoot + TEXT("bgm_39")); // Theme of Al de Baran
		ZoneToBgmMap.Add(TEXT("comodo"),         BgmRoot + TEXT("bgm_62")); // High Roller Coaster
		ZoneToBgmMap.Add(TEXT("yuno"),           BgmRoot + TEXT("bgm_70")); // Theme of Juno
		ZoneToBgmMap.Add(TEXT("umbala"),         BgmRoot + TEXT("bgm_68")); // Jazzy Funky Sweety
		ZoneToBgmMap.Add(TEXT("amatsu"),         BgmRoot + TEXT("bgm_76")); // Purity of Your Smile
		ZoneToBgmMap.Add(TEXT("gonryun"),        BgmRoot + TEXT("bgm_74")); // Not So Far Away
		ZoneToBgmMap.Add(TEXT("louyang"),        BgmRoot + TEXT("bgm_79")); // The Great
		ZoneToBgmMap.Add(TEXT("ayothaya"),       BgmRoot + TEXT("bgm_81")); // Thai Orchid
		ZoneToBgmMap.Add(TEXT("xmas"),           BgmRoot + TEXT("bgm_59")); // Theme of Lutie
		ZoneToBgmMap.Add(TEXT("niflheim"),       BgmRoot + TEXT("bgm_84")); // Christmas in 13th Month
		ZoneToBgmMap.Add(TEXT("einbroch"),       BgmRoot + TEXT("bgm_86")); // Steel Me
		ZoneToBgmMap.Add(TEXT("einbech"),        BgmRoot + TEXT("bgm_86"));
		ZoneToBgmMap.Add(TEXT("lighthalzen"),    BgmRoot + TEXT("bgm_90")); // Noblesse Oblige
		ZoneToBgmMap.Add(TEXT("hugel"),          BgmRoot + TEXT("bgm_93")); // Latinnova
		ZoneToBgmMap.Add(TEXT("rachel"),         BgmRoot + TEXT("bgm_94")); // Theme of Rachel
		ZoneToBgmMap.Add(TEXT("veins"),          BgmRoot + TEXT("bgm_104")); // On Your Way Back
		ZoneToBgmMap.Add(TEXT("moscovia"),       BgmRoot + TEXT("bgm_114")); // Theme of Moscovia

		// ---- Field maps ----
		// Prontera fields (Streamside / Tread on the Ground / I Miss You — alternating)
		ZoneToBgmMap.Add(TEXT("prt_fild01"), BgmRoot + TEXT("bgm_12")); // Streamside
		ZoneToBgmMap.Add(TEXT("prt_fild02"), BgmRoot + TEXT("bgm_05")); // Tread on the Ground
		ZoneToBgmMap.Add(TEXT("prt_fild03"), BgmRoot + TEXT("bgm_05"));
		ZoneToBgmMap.Add(TEXT("prt_fild04"), BgmRoot + TEXT("bgm_05"));
		ZoneToBgmMap.Add(TEXT("prt_fild05"), BgmRoot + TEXT("bgm_12"));
		ZoneToBgmMap.Add(TEXT("prt_fild06"), BgmRoot + TEXT("bgm_12"));
		ZoneToBgmMap.Add(TEXT("prt_fild07"), BgmRoot + TEXT("bgm_05"));
		ZoneToBgmMap.Add(TEXT("prt_fild08"), BgmRoot + TEXT("bgm_12"));
		ZoneToBgmMap.Add(TEXT("prt_fild09"), BgmRoot + TEXT("bgm_04")); // I Miss You
		ZoneToBgmMap.Add(TEXT("prt_fild10"), BgmRoot + TEXT("bgm_04"));
		ZoneToBgmMap.Add(TEXT("prt_fild11"), BgmRoot + TEXT("bgm_04"));
		ZoneToBgmMap.Add(TEXT("prt_fild12"), BgmRoot + TEXT("bgm_12"));

		// Geffen fields (Plateau / Travel / Nano East alternating)
		ZoneToBgmMap.Add(TEXT("gef_fild01"), BgmRoot + TEXT("bgm_23")); // Travel
		ZoneToBgmMap.Add(TEXT("gef_fild02"), BgmRoot + TEXT("bgm_35")); // Nano East
		ZoneToBgmMap.Add(TEXT("gef_fild03"), BgmRoot + TEXT("bgm_35"));
		ZoneToBgmMap.Add(TEXT("gef_fild04"), BgmRoot + TEXT("bgm_25")); // Plateau
		ZoneToBgmMap.Add(TEXT("gef_fild07"), BgmRoot + TEXT("bgm_25"));
		ZoneToBgmMap.Add(TEXT("gef_fild05"), BgmRoot + TEXT("bgm_23"));
		ZoneToBgmMap.Add(TEXT("gef_fild06"), BgmRoot + TEXT("bgm_23"));
		ZoneToBgmMap.Add(TEXT("gef_fild08"), BgmRoot + TEXT("bgm_23"));
		ZoneToBgmMap.Add(TEXT("gef_fild09"), BgmRoot + TEXT("bgm_23"));

		// Morroc fields (Desert)
		ZoneToBgmMap.Add(TEXT("moc_fild01"), BgmRoot + TEXT("bgm_24")); // Desert
		ZoneToBgmMap.Add(TEXT("moc_fild04"), BgmRoot + TEXT("bgm_24"));
		ZoneToBgmMap.Add(TEXT("moc_fild05"), BgmRoot + TEXT("bgm_24"));
		ZoneToBgmMap.Add(TEXT("moc_fild06"), BgmRoot + TEXT("bgm_24"));
		ZoneToBgmMap.Add(TEXT("moc_fild07"), BgmRoot + TEXT("bgm_24"));
		ZoneToBgmMap.Add(TEXT("moc_fild02"), BgmRoot + TEXT("bgm_03")); // Peaceful Forest
		ZoneToBgmMap.Add(TEXT("moc_fild03"), BgmRoot + TEXT("bgm_03"));

		// Payon fields (Peaceful Forest)
		ZoneToBgmMap.Add(TEXT("pay_fild01"), BgmRoot + TEXT("bgm_03"));
		ZoneToBgmMap.Add(TEXT("pay_fild02"), BgmRoot + TEXT("bgm_03"));
		ZoneToBgmMap.Add(TEXT("pay_fild03"), BgmRoot + TEXT("bgm_03"));
		ZoneToBgmMap.Add(TEXT("pay_fild04"), BgmRoot + TEXT("bgm_03"));

		// ---- Dungeons ----
		ZoneToBgmMap.Add(TEXT("ant_hell"),    BgmRoot + TEXT("bgm_46")); // An Ant-Lion's Pit
		ZoneToBgmMap.Add(TEXT("ant_hell_01"), BgmRoot + TEXT("bgm_46"));
		ZoneToBgmMap.Add(TEXT("ant_hell_02"), BgmRoot + TEXT("bgm_46"));
		ZoneToBgmMap.Add(TEXT("anthell01"),   BgmRoot + TEXT("bgm_46"));
		ZoneToBgmMap.Add(TEXT("anthell02"),   BgmRoot + TEXT("bgm_46"));
		ZoneToBgmMap.Add(TEXT("byalan"),      BgmRoot + TEXT("bgm_29")); // Be Nice 'n Easy (upper)
		ZoneToBgmMap.Add(TEXT("byalan_01"),   BgmRoot + TEXT("bgm_29"));
		ZoneToBgmMap.Add(TEXT("byalan_02"),   BgmRoot + TEXT("bgm_29"));
		ZoneToBgmMap.Add(TEXT("iz_dun00"),    BgmRoot + TEXT("bgm_29"));
		ZoneToBgmMap.Add(TEXT("iz_dun01"),    BgmRoot + TEXT("bgm_29"));
		ZoneToBgmMap.Add(TEXT("iz_dun02"),    BgmRoot + TEXT("bgm_29"));
		ZoneToBgmMap.Add(TEXT("iz_dun03"),    BgmRoot + TEXT("bgm_49")); // Watery Grave (deep)
		ZoneToBgmMap.Add(TEXT("iz_dun04"),    BgmRoot + TEXT("bgm_49"));
		ZoneToBgmMap.Add(TEXT("iz_dun05"),    BgmRoot + TEXT("bgm_49"));
		ZoneToBgmMap.Add(TEXT("payon_dun"),   BgmRoot + TEXT("bgm_20")); // Ancient Groover (upper)
		ZoneToBgmMap.Add(TEXT("pay_dun00"),   BgmRoot + TEXT("bgm_20"));
		ZoneToBgmMap.Add(TEXT("pay_dun01"),   BgmRoot + TEXT("bgm_20"));
		ZoneToBgmMap.Add(TEXT("pay_dun02"),   BgmRoot + TEXT("bgm_20"));
		ZoneToBgmMap.Add(TEXT("pay_dun03"),   BgmRoot + TEXT("bgm_47")); // Welcome Mr. Hwang (deep)
		ZoneToBgmMap.Add(TEXT("pay_dun04"),   BgmRoot + TEXT("bgm_47"));
		ZoneToBgmMap.Add(TEXT("gef_dun00"),   BgmRoot + TEXT("bgm_21")); // Through the Tower (upper)
		ZoneToBgmMap.Add(TEXT("gef_dun01"),   BgmRoot + TEXT("bgm_21"));
		ZoneToBgmMap.Add(TEXT("gef_dun02"),   BgmRoot + TEXT("bgm_50")); // Out of Curiosity (deep)
		ZoneToBgmMap.Add(TEXT("gef_dun03"),   BgmRoot + TEXT("bgm_50"));
		ZoneToBgmMap.Add(TEXT("orcsdun01"),   BgmRoot + TEXT("bgm_48")); // Help Yourself
		ZoneToBgmMap.Add(TEXT("orcsdun02"),   BgmRoot + TEXT("bgm_48"));
		ZoneToBgmMap.Add(TEXT("prt_sewb1"),   BgmRoot + TEXT("bgm_19")); // Under the Ground (Culvert)
		ZoneToBgmMap.Add(TEXT("prt_sewb2"),   BgmRoot + TEXT("bgm_19"));
		ZoneToBgmMap.Add(TEXT("prt_sewb3"),   BgmRoot + TEXT("bgm_19"));
		ZoneToBgmMap.Add(TEXT("prt_sewb4"),   BgmRoot + TEXT("bgm_19"));
		ZoneToBgmMap.Add(TEXT("prt_maze01"),  BgmRoot + TEXT("bgm_16")); // Labyrinth
		ZoneToBgmMap.Add(TEXT("prt_maze02"),  BgmRoot + TEXT("bgm_16"));
		ZoneToBgmMap.Add(TEXT("prt_maze03"),  BgmRoot + TEXT("bgm_16"));

		// ---- Glast Heim family ----
		ZoneToBgmMap.Add(TEXT("glast_01"),  BgmRoot + TEXT("bgm_42")); // Curse'n Pain
		ZoneToBgmMap.Add(TEXT("gl_dun01"),  BgmRoot + TEXT("bgm_42"));
		ZoneToBgmMap.Add(TEXT("gl_dun02"),  BgmRoot + TEXT("bgm_42"));
		ZoneToBgmMap.Add(TEXT("gl_in01"),   BgmRoot + TEXT("bgm_42"));
		ZoneToBgmMap.Add(TEXT("gl_chyard"), BgmRoot + TEXT("bgm_40")); // Monk Zonk
		ZoneToBgmMap.Add(TEXT("gl_church"), BgmRoot + TEXT("bgm_40"));
		ZoneToBgmMap.Add(TEXT("gl_prison"), BgmRoot + TEXT("bgm_40"));
		ZoneToBgmMap.Add(TEXT("gl_cas01"),  BgmRoot + TEXT("bgm_43")); // Morning Gloomy
		ZoneToBgmMap.Add(TEXT("gl_cas02"),  BgmRoot + TEXT("bgm_43"));
		ZoneToBgmMap.Add(TEXT("gl_knt01"),  BgmRoot + TEXT("bgm_44")); // TeMP it Up
		ZoneToBgmMap.Add(TEXT("gl_knt02"),  BgmRoot + TEXT("bgm_44"));
		ZoneToBgmMap.Add(TEXT("gl_step"),   BgmRoot + TEXT("bgm_42"));

		// ---- Pyramids / Sphinx ----
		ZoneToBgmMap.Add(TEXT("moc_pryd01"), BgmRoot + TEXT("bgm_22")); // Backattack!!
		ZoneToBgmMap.Add(TEXT("moc_pryd02"), BgmRoot + TEXT("bgm_22"));
		ZoneToBgmMap.Add(TEXT("moc_pryd03"), BgmRoot + TEXT("bgm_22"));
		ZoneToBgmMap.Add(TEXT("moc_pryd04"), BgmRoot + TEXT("bgm_22"));
		ZoneToBgmMap.Add(TEXT("moc_pryd05"), BgmRoot + TEXT("bgm_22"));
		ZoneToBgmMap.Add(TEXT("moc_pryd06"), BgmRoot + TEXT("bgm_22"));
		ZoneToBgmMap.Add(TEXT("in_sphinx1"), BgmRoot + TEXT("bgm_38")); // Sphinx theme
		ZoneToBgmMap.Add(TEXT("in_sphinx2"), BgmRoot + TEXT("bgm_38"));
		ZoneToBgmMap.Add(TEXT("in_sphinx3"), BgmRoot + TEXT("bgm_38"));
		ZoneToBgmMap.Add(TEXT("in_sphinx4"), BgmRoot + TEXT("bgm_38"));
		ZoneToBgmMap.Add(TEXT("in_sphinx5"), BgmRoot + TEXT("bgm_38"));

		// ---- Special / event ----
		ZoneToBgmMap.Add(TEXT("xmas_dun01"), BgmRoot + TEXT("bgm_58")); // Jingle Bell on Ragnarok
		ZoneToBgmMap.Add(TEXT("xmas_dun02"), BgmRoot + TEXT("bgm_58"));
	}

	// ---- 3D positional ambient sources (RSW-style) ----
	// Coordinate-based ambient emitters for water/fountains/fires/etc. Spawned as
	// 3D-attenuated audio components when their zone is loaded. World coordinates
	// match the level's actor positions — adjust per zone as level art evolves.
	{
		const FString AmbientRoot = TEXT("/Game/SabriMMO/Audio/SFX/Ambient/");
		const FString WaterFall = AmbientRoot + TEXT("se_subterranean_waterfall_01");
		const FString WaterDrip = AmbientRoot + TEXT("se_subterranean_waterdrop_01");
		const FString PrtBell   = AmbientRoot + TEXT("se_prtchbell_01");

		auto AddPoint = [this](const TCHAR* Zone, const FString& Path, const FVector& Loc, float Radius)
		{
			TArray<FAmbientPoint>& Points = Zone3DAmbientMap.FindOrAdd(FString(Zone));
			FAmbientPoint P;
			P.SoundPath = Path;
			P.Location = Loc;
			P.AttenuationRadius = Radius;
			Points.Add(P);
		};

		// Prontera — fountain in central plaza + bell in church
		AddPoint(TEXT("prontera"),       WaterFall, FVector(0.f,    0.f,    100.f), 1500.f);
		AddPoint(TEXT("prontera"),       PrtBell,   FVector(0.f,    1500.f, 200.f), 2000.f);
		AddPoint(TEXT("prontera_south"), WaterFall, FVector(0.f,    0.f,    100.f), 1500.f);
		AddPoint(TEXT("prontera_south"), PrtBell,   FVector(0.f,    1500.f, 200.f), 2000.f);

		// Byalan / underwater dungeons — drips throughout
		AddPoint(TEXT("byalan"),    WaterDrip, FVector(0.f,    0.f,    100.f), 1500.f);
		AddPoint(TEXT("byalan_01"), WaterDrip, FVector(0.f,    0.f,    100.f), 1500.f);
		AddPoint(TEXT("iz_dun00"),  WaterDrip, FVector(0.f,    0.f,    100.f), 1500.f);
		AddPoint(TEXT("iz_dun01"),  WaterDrip, FVector(0.f,    0.f,    100.f), 1500.f);
		AddPoint(TEXT("iz_dun03"),  WaterFall, FVector(0.f,    0.f,    100.f), 2000.f);
		AddPoint(TEXT("iz_dun04"),  WaterFall, FVector(0.f,    0.f,    100.f), 2000.f);

		// Cave dungeons — water drips
		AddPoint(TEXT("payon_dun"),   WaterDrip, FVector(0.f, 0.f, 100.f), 1500.f);
		AddPoint(TEXT("pay_dun01"),   WaterDrip, FVector(0.f, 0.f, 100.f), 1500.f);
		AddPoint(TEXT("ant_hell_01"), WaterDrip, FVector(0.f, 0.f, 100.f), 1500.f);
		AddPoint(TEXT("ant_hell_02"), WaterDrip, FVector(0.f, 0.f, 100.f), 1500.f);

		// Build a 3D ambient attenuation profile (larger radius than monster sounds)
		AmbientAttenuation = NewObject<USoundAttenuation>(this, TEXT("AmbientAttenuation"));
		if (AmbientAttenuation)
		{
			FSoundAttenuationSettings& A = AmbientAttenuation->Attenuation;
			A.bAttenuate              = true;
			A.bSpatialize             = true;
			A.AttenuationShape        = EAttenuationShape::Sphere;
			A.AttenuationShapeExtents = FVector(500.f);
			A.FalloffDistance         = 4000.f;
			A.DistanceAlgorithm       = EAttenuationDistanceModel::Linear;
		}
	}

	// ---- Volume bus state (manual multiplier system) ----
	// We do NOT use USoundClass / USoundMix anymore. The previous implementation tried
	// to create runtime USoundClass instances and route every sound through them via
	// `Sound->SoundClassObject = SfxSoundClass`, but this approach has two unfixable
	// problems in UE5.7:
	//
	//   1) `Sound->SoundClassObject = X` MUTATES THE SHARED USoundBase ASSET. In PIE
	//      with multiple worlds, world A and world B each create their own SoundClass
	//      UObject instances; whichever world plays last overwrites the asset's
	//      SoundClassObject pointer. The other world's audio device then parses a
	//      sound that points to a class only registered with the OTHER device, so
	//      `SoundClasses.Find()` returns null and "Unable to find sound class
	//      properties for sound class AudioBgm" spams every audio tick (~80Hz).
	//      bPersistAcrossLevelTransition makes this much worse on zone changes.
	//
	//   2) Even calling `RegisterSoundClass` (which we tried) only fixes the first
	//      `SoundClasses.Find()` warning from `ParseSoundClasses`, NOT the second
	//      lookup from `SoundWave::ParseAttributes`. The class identity mismatch
	//      between worlds persists regardless.
	//
	// REPLACEMENT: store bus volumes as plain floats and bake them into the
	// VolumeMultiplier param at PlaySoundAtLocation / PlaySound2D / SpawnSound2D
	// time. For long-running audio (BGM + ambient layers) we keep the
	// AudioComponent pointer alive and call SetVolumeMultiplier() on slider drag
	// via ApplyVolumeToActiveComponents(). One-shot SFX use the volume current at
	// the moment they fire — that's fine because they're short.
	MasterSoundClass  = nullptr;
	BgmSoundClass     = nullptr;
	SfxSoundClass     = nullptr;
	AmbientSoundClass = nullptr;
	VolumeMix         = nullptr;

	// ---- Subscribe to status:applied so status sounds fire when the server reports
	//      stun / poison / blind / freeze / curse / silence / sleep / stone effects ----
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		if (USocketEventRouter* Router = GI->GetEventRouter())
		{
			Router->RegisterHandler(TEXT("status:applied"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleStatusApplied(D); });
		}
	}

	// ---- Mute on focus loss (windowed mute when minimized) ----
	// FCoreDelegates fires when the OS reports the application went to background
	// or returned to foreground. Each new world rebinds; Deinitialize unbinds.
	FocusGainedHandle = FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(
		this, &UAudioSubsystem::OnApplicationActivated);
	FocusLostHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(
		this, &UAudioSubsystem::OnApplicationDeactivated);

	UE_LOG(LogMMOAudio, Log, TEXT("AudioSubsystem ready: %d monster classes, %d status sounds, %d player base classes, %d weapon types, %d skill impacts, %d ambient zones, attenuation 300/3000, concurrency caps active."),
		MonsterSoundMap.Num(), StatusSoundMap.Num(), ClassAttackSoundMap.Num(), WeaponAttackSoundMap.Num(), SkillImpactSoundMap.Num(), ZoneAmbientMap.Num());

	// Kick off the initial zone ambient bed + BGM once everything is wired. Reads the
	// current zone from the GameInstance — covers the case where the player loaded
	// directly into a zone (login or character select → first level) without going
	// through ZoneTransitionSubsystem::CompleteTransition.
	//
	// IMPORTANT: only auto-start when the socket is connected (= we're in a game
	// world, not the login screen). The login screen explicitly calls PlayBgm(bgm_01)
	// from LoginFlowSubsystem::OnWorldBeginPlay; if we let the auto-start fire here
	// it would race against the login subsystem and could overwrite the Title music
	// with whatever CurrentZoneName defaults to.
	if (UMMOGameInstance* GIInit = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		const bool bIsGameWorld = GIInit->IsSocketConnected();
		const FString InitialZone = GIInit->CurrentZoneName;
		if (bIsGameWorld && !InitialZone.IsEmpty())
		{
			PlayZoneAmbient(InitialZone);
			PlayZoneBgm(InitialZone);
		}
	}
}

void UAudioSubsystem::Deinitialize()
{
	// Unsubscribe from socket events
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

	// Unbind focus delegates
	if (FocusGainedHandle.IsValid())
	{
		FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(FocusGainedHandle);
		FocusGainedHandle.Reset();
	}
	if (FocusLostHandle.IsValid())
	{
		FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(FocusLostHandle);
		FocusLostHandle.Reset();
	}

	SoundCache.Empty();
	MonsterSoundMap.Empty();
	StatusSoundMap.Empty();
	ClassAttackSoundMap.Empty();
	ClassHitSoundMap.Empty();
	BodyMaterialSoundMap.Empty();
	WeaponAttackSoundMap.Empty();
	WeaponHitSoundMap.Empty();
	FistHitSoundPaths.Empty();
	LevelUpSoundPath.Empty();
	HealSoundPath.Empty();
	DefaultSkillCastSoundPath.Empty();
	SkillCastSoundMap.Empty();
	SkillImpactSoundMap.Empty();
	RecentSkillImpactPlayTime.Empty();
	ConcurrencyTracking.Empty();
	MonsterAttenuation = nullptr;
	UIClickSoundPath.Empty();
	UICancelSoundPath.Empty();
	EquipSoundPaths.Empty();
	DropSoundPaths.Empty();
	CoinPickupSoundPath.Empty();
	JobLevelUpSoundPath.Empty();
	RefineSuccessSoundPath.Empty();
	RefineFailSoundPath.Empty();
	PharmacySuccessSoundPath.Empty();
	PharmacyFailSoundPath.Empty();
	MvpFanfareSoundPath.Empty();
	WarpPortalSoundPath.Empty();
	TeleportSoundPath.Empty();
	PlayerDamageVoiceMap.Empty();
	PlayerDeathVoiceMap.Empty();
	StopAllAmbient();
	ZoneAmbientMap.Empty();
	Zone3DAmbientMap.Empty();
	StopBgm();
	ZoneToBgmMap.Empty();
	ActiveBgmComponent = nullptr;
	AmbientAttenuation = nullptr;

	// Sound class fields are nullptr (we don't use USoundClass anymore — see
	// OnWorldBeginPlay comment). Reset just to be safe.
	MasterSoundClass  = nullptr;
	BgmSoundClass     = nullptr;
	SfxSoundClass     = nullptr;
	AmbientSoundClass = nullptr;
	VolumeMix         = nullptr;

	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

void UAudioSubsystem::PlayMonsterSound(const FString& SpriteClass, EMonsterSoundType Type, const FVector& Location)
{
	if (SpriteClass.IsEmpty()) return;

	const FMonsterSoundConfig* Config = ResolveConfig(SpriteClass);
	if (!Config)
	{
		UE_LOG(LogMMOAudio, Verbose, TEXT("No sound config for sprite class '%s'"), *SpriteClass);
		return;
	}

	// Play the primary event sound
	const FString& PrimaryPath = GetPathForType(*Config, Type);
	if (!PrimaryPath.IsEmpty())
	{
		PlaySoundInternal(PrimaryPath, Location, Type);
	}

	// RO Classic-style body material layering: Damage and Die both play the body
	// material sound (the crumple/impact reaction) simultaneously with the primary.
	// Other event types (Attack, Move, Stand) do not get layered.
	if ((Type == EMonsterSoundType::Damage || Type == EMonsterSoundType::Die)
		&& !Config->BodyMaterialPath.IsEmpty())
	{
		// Body material counts toward the same concurrency bucket as Damage so
		// big mob fights don't double the audio load.
		PlaySoundInternal(Config->BodyMaterialPath, Location, EMonsterSoundType::Damage);
	}
}

bool UAudioSubsystem::HasStandSound(const FString& SpriteClass) const
{
	const FMonsterSoundConfig* Config = ResolveConfig(SpriteClass);
	return Config && Config->StandPaths.Num() > 0;
}

float UAudioSubsystem::GetStandInterval(const FString& SpriteClass) const
{
	const FMonsterSoundConfig* Config = ResolveConfig(SpriteClass);
	if (!Config || Config->StandPaths.Num() == 0) return 0.f;
	return Config->StandIntervalSeconds;
}

void UAudioSubsystem::PlayStatusSound(const FString& StatusType, const FVector& Location)
{
	if (StatusType.IsEmpty()) return;

	// Try the status type as-is first, then lowercase fallback (server casing varies)
	const FString* PathPtr = StatusSoundMap.Find(StatusType);
	if (!PathPtr)
		PathPtr = StatusSoundMap.Find(StatusType.ToLower());
	if (!PathPtr) return;

	USoundBase* Sound = LoadSoundCached(*PathPtr);
	if (!Sound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Status sounds bypass concurrency caps — they're rare and important.
	UGameplayStatics::PlaySoundAtLocation(
		World,
		Sound,
		Location,
		GetEffectiveSfxVolume(),    // SFX bus baked in
		1.0f,
		0.0f,
		MonsterAttenuation,
		nullptr,
		nullptr
	);
}

// ============================================================
// Player sound API
// ============================================================

void UAudioSubsystem::PlayPlayerAttackSound(const FString& JobClass, const FVector& Location)
{
	if (JobClass.IsEmpty()) return;
	const FString BaseClass = ResolveBaseClassFor(JobClass);
	if (const FString* PathPtr = ClassAttackSoundMap.Find(BaseClass))
	{
		// Reuse the monster Attack concurrency bucket — combat audio shares the cap.
		PlaySoundInternal(*PathPtr, Location, EMonsterSoundType::Attack);
	}
}

void UAudioSubsystem::PlayPlayerHitSound(const FString& AttackerJobClass, const FVector& TargetLocation)
{
	if (AttackerJobClass.IsEmpty()) return;
	const FString BaseClass = ResolveBaseClassFor(AttackerJobClass);
	if (const FString* PathPtr = ClassHitSoundMap.Find(BaseClass))
	{
		// Reuse the monster Damage concurrency bucket.
		PlaySoundInternal(*PathPtr, TargetLocation, EMonsterSoundType::Damage);
	}
}

void UAudioSubsystem::PlayPlayerBodyReaction(const FString& JobClass, const FVector& Location)
{
	if (JobClass.IsEmpty()) return;
	const EPlayerBodyMaterial Material = ResolveBodyMaterialFor(JobClass);
	if (const FString* PathPtr = BodyMaterialSoundMap.Find(Material))
	{
		// Body reaction shares the Damage concurrency bucket too.
		PlaySoundInternal(*PathPtr, Location, EMonsterSoundType::Damage);
	}
}

void UAudioSubsystem::PlayLevelUpSound()
{
	if (LevelUpSoundPath.IsEmpty()) return;

	USoundBase* Sound = LoadSoundCached(LevelUpSoundPath);
	if (!Sound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 2D non-spatial — local player only. The caller filters for the local player
	// (BasicInfoSubsystem::HandleExpLevelUp already does this).
	UGameplayStatics::PlaySound2D(World, Sound, GetEffectiveSfxVolume(), 1.0f, 0.0f);
}

void UAudioSubsystem::PlayHealSound(const FVector& Location)
{
	if (HealSoundPath.IsEmpty()) return;

	USoundBase* Sound = LoadSoundCached(HealSoundPath);
	if (!Sound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayStatics::PlaySoundAtLocation(
		World, Sound, Location,
		GetEffectiveSfxVolume(), 1.0f, 0.0f,
		MonsterAttenuation, nullptr, nullptr
	);
}

// ============================================================
// Player damage / death voices
// ============================================================

void UAudioSubsystem::PlayPlayerDamageVoice(const FString& JobClass, bool bIsFemale, const FVector& Location)
{
	if (JobClass.IsEmpty()) return;

	const FString BaseClass = ResolveBaseClassFor(JobClass);
	const FString GenderSuffix = bIsFemale ? TEXT("_female") : TEXT("_male");
	const FString Key = BaseClass + GenderSuffix;

	const FString* PathPtr = PlayerDamageVoiceMap.Find(Key);
	if (!PathPtr)
	{
		// Generic fallback when class lookup misses
		PathPtr = PlayerDamageVoiceMap.Find(bIsFemale ? TEXT("__female") : TEXT("__male"));
	}
	if (!PathPtr || PathPtr->IsEmpty()) return;

	// Reuse Damage concurrency bucket — voices share with body reaction
	PlaySoundInternal(*PathPtr, Location, EMonsterSoundType::Damage);
}

void UAudioSubsystem::PlayPlayerDeathVoice(const FString& JobClass, bool bIsFemale, const FVector& Location)
{
	if (JobClass.IsEmpty()) return;

	const FString BaseClass = ResolveBaseClassFor(JobClass);
	const FString GenderSuffix = bIsFemale ? TEXT("_female") : TEXT("_male");
	const FString Key = BaseClass + GenderSuffix;

	const FString* PathPtr = PlayerDeathVoiceMap.Find(Key);
	if (!PathPtr)
	{
		PathPtr = PlayerDeathVoiceMap.Find(bIsFemale ? TEXT("__female") : TEXT("__male"));
	}
	if (!PathPtr || PathPtr->IsEmpty()) return;

	// Reuse Die bucket
	PlaySoundInternal(*PathPtr, Location, EMonsterSoundType::Die);
}

// ============================================================
// UI / Event sound API
// ============================================================

void UAudioSubsystem::PlayUIClick()
{
	PlaySound2DInternal(UIClickSoundPath);
}

void UAudioSubsystem::PlayUICancel()
{
	PlaySound2DInternal(UICancelSoundPath);
}

void UAudioSubsystem::PlayEquipSound()
{
	if (EquipSoundPaths.Num() == 0) return;
	const int32 Idx = FMath::RandRange(0, EquipSoundPaths.Num() - 1);
	PlaySound2DInternal(EquipSoundPaths[Idx]);
}

void UAudioSubsystem::PlayItemDropSound(const FString& TierColor, const FVector& Location)
{
	const FString Lower = TierColor.IsEmpty() ? TEXT("blue") : TierColor.ToLower();
	const FString* PathPtr = DropSoundPaths.Find(Lower);
	if (!PathPtr)
	{
		// Unknown tier — fall back to blue (mid rarity default)
		PathPtr = DropSoundPaths.Find(TEXT("blue"));
	}
	if (!PathPtr || PathPtr->IsEmpty()) return;

	USoundBase* Sound = LoadSoundCached(*PathPtr);
	if (!Sound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 3D positional, no concurrency cap (drops are infrequent + clustered)
	UGameplayStatics::PlaySoundAtLocation(
		World, Sound, Location,
		GetEffectiveSfxVolume(), 1.0f, 0.0f,
		MonsterAttenuation, nullptr, nullptr
	);
}

void UAudioSubsystem::PlayCoinPickupSound()
{
	PlaySound2DInternal(CoinPickupSoundPath);
}

void UAudioSubsystem::PlayJobLevelUpSound()
{
	PlaySound2DInternal(JobLevelUpSoundPath);
}

void UAudioSubsystem::PlayRefineSuccessSound()
{
	PlaySound2DInternal(RefineSuccessSoundPath);
}

void UAudioSubsystem::PlayRefineFailSound()
{
	PlaySound2DInternal(RefineFailSoundPath);
}

void UAudioSubsystem::PlayPharmacySuccessSound()
{
	PlaySound2DInternal(PharmacySuccessSoundPath);
}

void UAudioSubsystem::PlayPharmacyFailSound()
{
	PlaySound2DInternal(PharmacyFailSoundPath);
}

void UAudioSubsystem::PlayMvpFanfareSound()
{
	// Slightly louder than other 2D event sounds — this is a celebration moment
	PlaySound2DInternal(MvpFanfareSoundPath, 1.2f);
}

void UAudioSubsystem::PlayWarpPortalSound(const FVector& Location)
{
	if (WarpPortalSoundPath.IsEmpty()) return;
	USoundBase* Sound = LoadSoundCached(WarpPortalSoundPath);
	if (!Sound) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameplayStatics::PlaySoundAtLocation(
		World, Sound, Location,
		GetEffectiveSfxVolume(), 1.0f, 0.0f,
		MonsterAttenuation, nullptr, nullptr
	);
}

void UAudioSubsystem::PlayTeleportSound(const FVector& Location)
{
	if (TeleportSoundPath.IsEmpty()) return;
	USoundBase* Sound = LoadSoundCached(TeleportSoundPath);
	if (!Sound) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameplayStatics::PlaySoundAtLocation(
		World, Sound, Location,
		GetEffectiveSfxVolume(), 1.0f, 0.0f,
		MonsterAttenuation, nullptr, nullptr
	);
}

// ============================================================
// Static convenience helpers (UI / button-handler entry points)
// ============================================================

void UAudioSubsystem::PlayUIClickStatic(UWorld* World)
{
	if (!World) return;
	if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
	{
		Audio->PlayUIClick();
	}
}

void UAudioSubsystem::PlayUICancelStatic(UWorld* World)
{
	if (!World) return;
	if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
	{
		Audio->PlayUICancel();
	}
}

void UAudioSubsystem::PlayEquipStatic(UWorld* World)
{
	if (!World) return;
	if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
	{
		Audio->PlayEquipSound();
	}
}

void UAudioSubsystem::PlayCoinPickupStatic(UWorld* World)
{
	if (!World) return;
	if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
	{
		Audio->PlayCoinPickupSound();
	}
}

// ============================================================
// Zone ambient bed system
// ============================================================

void UAudioSubsystem::PlayZoneAmbient(const FString& ZoneName)
{
	// Idempotent — same zone => no-op
	if (ZoneName.Equals(CurrentAmbientZone, ESearchCase::IgnoreCase))
	{
		return;
	}

	// Stop existing layers (if any) before swapping
	StopAllAmbient();

	if (ZoneName.IsEmpty()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// ---- 2D ambient layers ----
	const TArray<FString>* Layers = ZoneAmbientMap.Find(ZoneName);
	if (!Layers)
	{
		Layers = ZoneAmbientMap.Find(ZoneName.ToLower());
	}
	int32 LayerCount = 0;
	const float AmbVol = GetEffectiveAmbientVolume();
	if (Layers && Layers->Num() > 0)
	{
		for (const FString& Path : *Layers)
		{
			USoundBase* Sound = LoadSoundCached(Path);
			if (!Sound) continue;

			if (USoundWave* Wave = Cast<USoundWave>(Sound))
			{
				Wave->bLooping = true;
			}

			// Spawn at the current effective ambient volume. We keep the component
			// alive in ActiveAmbientComponents so ApplyVolumeToActiveComponents can
			// update its volume live when the slider moves.
			UAudioComponent* AC = UGameplayStatics::SpawnSound2D(
				World, Sound,
				AmbVol, 1.0f, 0.0f,
				nullptr, true, false
			);
			if (AC)
			{
				AC->bIsUISound = true;
				AC->bAllowSpatialization = false;
				ActiveAmbientComponents.Add(AC);
				++LayerCount;
			}
		}
	}

	// ---- 3D positional ambient sources (water/fountains/fires/etc.) ----
	const TArray<FAmbientPoint>* Points = Zone3DAmbientMap.Find(ZoneName);
	if (!Points)
	{
		Points = Zone3DAmbientMap.Find(ZoneName.ToLower());
	}
	int32 PointCount = 0;
	if (Points && Points->Num() > 0)
	{
		for (const FAmbientPoint& Point : *Points)
		{
			USoundBase* Sound = LoadSoundCached(Point.SoundPath);
			if (!Sound) continue;

			if (USoundWave* Wave = Cast<USoundWave>(Sound))
			{
				Wave->bLooping = true;
			}

			// 3D ambient also uses the ambient bus volume. The 0.6 multiplier is the
			// per-source design balance (e.g. fountains shouldn't dominate the bed).
			UAudioComponent* AC = UGameplayStatics::SpawnSoundAtLocation(
				World, Sound, Point.Location,
				FRotator::ZeroRotator,
				AmbVol * 0.6f, 1.0f, 0.0f,
				AmbientAttenuation,
				nullptr, false  // bAutoDestroy=false
			);
			if (AC)
			{
				AC->bAllowSpatialization = true;
				Active3DAmbientComponents.Add(AC);
				++PointCount;
			}
		}
	}

	CurrentAmbientZone = ZoneName;
	UE_LOG(LogMMOAudio, Log, TEXT("Zone ambient started for %s (%d 2D layers, %d 3D points)"),
		*ZoneName, LayerCount, PointCount);
}

void UAudioSubsystem::StopAllAmbient()
{
	for (const TObjectPtr<UAudioComponent>& AC : ActiveAmbientComponents)
	{
		if (AC)
		{
			AC->Stop();
		}
	}
	ActiveAmbientComponents.Empty();

	for (const TObjectPtr<UAudioComponent>& AC : Active3DAmbientComponents)
	{
		if (AC)
		{
			AC->Stop();
		}
	}
	Active3DAmbientComponents.Empty();

	CurrentAmbientZone.Empty();
}

// ============================================================
// BGM (Background Music)
// ============================================================

void UAudioSubsystem::PlayZoneBgm(const FString& ZoneName)
{
	if (ZoneName.IsEmpty()) return;

	const FString* TrackPtr = ZoneToBgmMap.Find(ZoneName);
	if (!TrackPtr)
	{
		TrackPtr = ZoneToBgmMap.Find(ZoneName.ToLower());
	}
	if (!TrackPtr || TrackPtr->IsEmpty())
	{
		// Zone has no BGM mapping — leave silent (don't stop existing BGM either,
		// so a player walking into an unmapped zone keeps hearing the previous track).
		return;
	}

	PlayBgm(*TrackPtr);
}

void UAudioSubsystem::PlayBgm(const FString& AssetPath)
{
	// Idempotent — same track => no-op
	if (AssetPath.Equals(CurrentBgmPath, ESearchCase::IgnoreCase))
	{
		return;
	}

	// Stop the previous track (with fade-out if active)
	if (ActiveBgmComponent)
	{
		ActiveBgmComponent->FadeOut(1.5f, 0.0f);
		ActiveBgmComponent = nullptr;
	}

	if (AssetPath.IsEmpty())
	{
		CurrentBgmPath.Empty();
		return;
	}

	USoundBase* Sound = LoadSoundCached(AssetPath);
	if (!Sound)
	{
		UE_LOG(LogMMOAudio, Warning, TEXT("BGM track not found: %s"), *AssetPath);
		return;
	}

	// Force loop on the underlying SoundWave (BGM tracks should loop indefinitely)
	if (USoundWave* Wave = Cast<USoundWave>(Sound))
	{
		Wave->bLooping = true;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// Spawn at the current effective BGM volume — slider drag updates this live
	// via ApplyVolumeToActiveComponents -> SetVolumeMultiplier on ActiveBgmComponent.
	const float BgmVol = GetEffectiveBgmVolume();
	UAudioComponent* AC = UGameplayStatics::SpawnSound2D(
		World, Sound,
		BgmVol, // Volume baked in — live updates handled in ApplyVolumeToActiveComponents
		1.0f,   // Pitch
		0.0f,   // Start time
		nullptr,
		true,   // bPersistAcrossLevelTransition
		false   // bAutoDestroy — we keep alive until next BGM swap
	);
	if (AC)
	{
		AC->bIsUISound = true;
		AC->bAllowSpatialization = false;
		AC->FadeIn(1.5f, BgmVol);
		ActiveBgmComponent = AC;
	}

	CurrentBgmPath = AssetPath;
	UE_LOG(LogMMOAudio, Log, TEXT("BGM started: %s"), *AssetPath);
}

void UAudioSubsystem::StopBgm()
{
	if (ActiveBgmComponent)
	{
		ActiveBgmComponent->FadeOut(1.5f, 0.0f);
		ActiveBgmComponent = nullptr;
	}
	CurrentBgmPath.Empty();
}

// ============================================================
// Item use / element hit / cloaking sound helpers
// ============================================================

void UAudioSubsystem::PlayItemUseSound()
{
	// se_drink_potion.wav — universal item use sound
	const FString Path = TEXT("/Game/SabriMMO/Audio/SFX/Events/se_drink_potion");
	PlaySound2DInternal(Path);
}

void UAudioSubsystem::PlayElementHitSound(const FString& ElementType, const FVector& Location)
{
	const FString HitsRoot = TEXT("/Game/SabriMMO/Audio/SFX/ElementHits/");
	FString Path;

	const FString L = ElementType.ToLower();
	if (L == TEXT("fire"))
	{
		// 2 fire variants — random pick
		Path = HitsRoot + (FMath::RandBool() ? TEXT("hit_fire1") : TEXT("hit_fire2"));
	}
	else if (L == TEXT("wind") || L == TEXT("electric"))
	{
		Path = HitsRoot + (FMath::RandBool() ? TEXT("hit_wind1") : TEXT("hit_wind2"));
	}
	else
	{
		// Neutral / normal / fallback — 4 variants
		const int32 Idx = FMath::RandRange(1, 4);
		Path = HitsRoot + FString::Printf(TEXT("hit_normal%d"), Idx);
	}

	// Reuse Damage concurrency bucket
	PlaySoundInternal(Path, Location, EMonsterSoundType::Damage);
}

void UAudioSubsystem::PlayCloakingSound(const FVector& Location)
{
	const FString Path = TEXT("/Game/SabriMMO/Audio/SFX/Events/assasin_cloaking");
	// 3D positional — plays at the cloaking entity's position
	PlaySoundInternal(Path, Location, EMonsterSoundType::Attack);
}

// ============================================================
// Volume bus control
// ============================================================

void UAudioSubsystem::SetMasterVolume(float Volume)
{
	CurrentMasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolumeToActiveComponents();
}

void UAudioSubsystem::SetBgmVolume(float Volume)
{
	CurrentBgmVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolumeToActiveComponents();
}

void UAudioSubsystem::SetSfxVolume(float Volume)
{
	CurrentSfxVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	// SFX is one-shot — no live update needed. Future plays use the new value.
}

void UAudioSubsystem::SetAmbientVolume(float Volume)
{
	CurrentAmbientVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolumeToActiveComponents();
}

void UAudioSubsystem::SetMuted(bool bMuted)
{
	bAudioMuted = bMuted;
	ApplyVolumeToActiveComponents();
}

void UAudioSubsystem::SetMuteWhenMinimizedEnabled(bool bEnabled)
{
	bMuteWhenMinimizedEnabled = bEnabled;
	// If user just disabled the option AND we're currently muted-by-focus-loss,
	// unmute immediately so they hear audio again.
	if (!bEnabled && bAudioMuted)
	{
		SetMuted(false);
	}
}

void UAudioSubsystem::OnApplicationActivated()
{
	// Window regained focus — unmute if we previously auto-muted
	if (bMuteWhenMinimizedEnabled && bAudioMuted)
	{
		SetMuted(false);
	}
}

void UAudioSubsystem::OnApplicationDeactivated()
{
	// Window lost focus — auto-mute if option is enabled
	if (bMuteWhenMinimizedEnabled && !bAudioMuted)
	{
		SetMuted(true);
	}
}

void UAudioSubsystem::ApplyVolumeToActiveComponents()
{
	// Walk every long-running audio component and update its VolumeMultiplier in
	// place. UAudioComponent::SetVolumeMultiplier is real-time safe — UE5 dispatches
	// the change to the audio thread internally, so slider drags are instant.
	//
	// One-shot SFX (skill impacts, monster sounds, hit sounds, UI clicks) are NOT
	// listed here — they baked the volume in at PlaySoundAtLocation/PlaySound2D
	// time and are short enough that no live update is needed.
	const float BgmVol = GetEffectiveBgmVolume();
	const float AmbVol = GetEffectiveAmbientVolume();

	if (ActiveBgmComponent)
	{
		ActiveBgmComponent->SetVolumeMultiplier(BgmVol);
	}

	for (const TObjectPtr<UAudioComponent>& AC : ActiveAmbientComponents)
	{
		if (AC) AC->SetVolumeMultiplier(AmbVol);
	}

	// 3D ambient uses the per-source 0.6 design balance (matches PlayZoneAmbient).
	for (const TObjectPtr<UAudioComponent>& AC : Active3DAmbientComponents)
	{
		if (AC) AC->SetVolumeMultiplier(AmbVol * 0.6f);
	}

	UE_LOG(LogMMOAudio, Verbose,
		TEXT("ApplyVolumeToActiveComponents: Master=%.2f Bgm=%.2f Sfx=%.2f Ambient=%.2f Muted=%d"),
		CurrentMasterVolume, CurrentBgmVolume, CurrentSfxVolume, CurrentAmbientVolume,
		bAudioMuted ? 1 : 0);
}

bool UAudioSubsystem::PlayWeaponAttackSound(const FString& WeaponType, const FVector& Location)
{
	if (WeaponType.IsEmpty()) return false;

	// Unarmed / bare_hand has no swing wav — caller falls back to per-class sound.
	const FString L = WeaponType.ToLower();
	if (L == TEXT("bare_hand") || L == TEXT("knuckle") ||
		L == TEXT("whip") || L == TEXT("instrument"))
	{
		return false;
	}

	const FString* PathPtr = WeaponAttackSoundMap.Find(L);
	if (!PathPtr) return false;

	// Reuse the monster Attack concurrency bucket
	return PlaySoundInternal(*PathPtr, Location, EMonsterSoundType::Attack);
}

bool UAudioSubsystem::PlayWeaponHitSound(const FString& WeaponType, const FVector& TargetLocation)
{
	if (WeaponType.IsEmpty()) return false;

	const FString L = WeaponType.ToLower();

	// Knuckle / unarmed / bare_hand: random fist variant
	if (L == TEXT("bare_hand") || L == TEXT("knuckle"))
	{
		if (FistHitSoundPaths.Num() == 0) return false;
		const int32 Idx = FMath::RandRange(0, FistHitSoundPaths.Num() - 1);
		return PlaySoundInternal(FistHitSoundPaths[Idx], TargetLocation, EMonsterSoundType::Damage);
	}

	const FString* PathPtr = WeaponHitSoundMap.Find(L);
	if (!PathPtr) return false;

	return PlaySoundInternal(*PathPtr, TargetLocation, EMonsterSoundType::Damage);
}

// ============================================================
// Skill SFX API
// ============================================================

bool UAudioSubsystem::PlaySkillCastSound(int32 SkillId, const FVector& Location)
{
	if (SkillId <= 0) return false;

	// Per-skill override takes priority. If none, fall back to the universal
	// default cast windup (ef_beginspell.wav). Only return false if BOTH are
	// missing — that means there's nothing to play.
	const FString* PathPtr = SkillCastSoundMap.Find(SkillId);
	const FString& Path = (PathPtr && !PathPtr->IsEmpty()) ? *PathPtr : DefaultSkillCastSoundPath;
	if (Path.IsEmpty()) return false;

	// Reuse the Attack concurrency bucket — skill casts share the cap with auto-attacks.
	return PlaySoundInternal(Path, Location, EMonsterSoundType::Attack);
}

bool UAudioSubsystem::PlaySkillImpactSound(int32 SkillId, const FVector& Location, int32 HitNumber)
{
	if (SkillId <= 0)
	{
		UE_LOG(LogMMOAudio, Verbose, TEXT("PlaySkillImpactSound: invalid skill id %d"), SkillId);
		return false;
	}
	const FString* PathPtr = SkillImpactSoundMap.Find(SkillId);
	if (!PathPtr)
	{
		UE_LOG(LogMMOAudio, Warning, TEXT("PlaySkillImpactSound: no wav mapped for skill id %d"), SkillId);
		return false;
	}

	// Per-skill 300ms dedup — prevents double-fire when a skill emits both
	// skill:effect_damage and skill:buff_applied (e.g., Stone Curse, Frost Diver),
	// AND prevents double-fire when a damage skill emits both skill:effect_damage
	// (handled by SkillVFXSubsystem) and skill:used (handled by SkillTreeSubsystem
	// fallback hook). The first call wins, the second is suppressed.
	//
	// BYPASSED for per-hit events (HitNumber >= 1) — multi-hit bolt skills emit
	// per-hit events 200ms apart and we WANT each projectile to make a sound.
	// The server already serialized them, so there's no risk of accidental
	// double-fire from buff_applied vs effect_damage at this point.
	if (HitNumber == 0)
	{
		UWorld* World = GetWorld();
		const double Now = World ? World->GetTimeSeconds() : 0.0;
		if (const double* Last = RecentSkillImpactPlayTime.Find(SkillId))
		{
			if (Now - *Last < 0.3)
			{
				UE_LOG(LogMMOAudio, Verbose, TEXT("PlaySkillImpactSound: skill %d dedup'd (last %.3fs ago)"),
					SkillId, (float)(Now - *Last));
				return false;
			}
		}
		RecentSkillImpactPlayTime.Add(SkillId, Now);
	}

	// Reuse the Damage concurrency bucket — skill impacts share the cap with hits.
	const bool bResult = PlaySoundInternal(*PathPtr, Location, EMonsterSoundType::Damage);
	UE_LOG(LogMMOAudio, Verbose, TEXT("PlaySkillImpactSound: skill %d hitNum=%d path=%s result=%d"),
		SkillId, HitNumber, **PathPtr, bResult ? 1 : 0);
	return bResult;
}

// ============================================================
// status:applied handler — server tells us a status was just applied
// ============================================================

void UAudioSubsystem::HandleStatusApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString StatusType;
	Obj->TryGetStringField(TEXT("statusType"), StatusType);
	if (StatusType.IsEmpty()) return;

	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	const int32 TargetId = (int32)TargetIdD;

	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	UWorld* World = GetWorld();
	if (!World) return;

	// Resolve target world position
	FVector Location = FVector::ZeroVector;
	bool bResolved = false;

	if (bIsEnemy)
	{
		// Enemy targets — look up via EnemySubsystem
		if (UEnemySubsystem* ES = World->GetSubsystem<UEnemySubsystem>())
		{
			if (AActor* Enemy = ES->GetEnemy(TargetId))
			{
				Location = Enemy->GetActorLocation();
				bResolved = true;
			}
		}
	}
	else
	{
		// Player targets — for the local player use the controlled pawn directly.
		// Remote players currently fall back to the local pawn's location (good enough
		// for the audio test scope; full multiplayer status audio for remote players
		// requires an OtherPlayerSubsystem position lookup).
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				Location = Pawn->GetActorLocation();
				bResolved = true;
			}
		}
	}

	if (!bResolved) return;

	PlayStatusSound(StatusType, Location);
}

// ============================================================
// Internals
// ============================================================

FString UAudioSubsystem::ResolveBaseClassFor(const FString& JobClass) const
{
	const FString L = JobClass.ToLower();

	// ---- Base classes (server canonical names — used directly as map keys) ----
	if (L == TEXT("novice") || L == TEXT("swordsman") || L == TEXT("mage") ||
		L == TEXT("archer") || L == TEXT("acolyte") || L == TEXT("thief") || L == TEXT("merchant"))
		return L;

	// ---- Swordsman line -> swordsman ----
	if (L == TEXT("knight") || L == TEXT("crusader") ||
		L == TEXT("lord_knight") || L == TEXT("paladin") ||
		L == TEXT("high_swordsman") ||
		L == TEXT("rune_knight") || L == TEXT("royal_guard"))
		return TEXT("swordsman");

	// ---- Mage line -> mage ----
	if (L == TEXT("wizard") || L == TEXT("sage") ||
		L == TEXT("high_wizard") || L == TEXT("scholar") ||
		L == TEXT("high_mage") ||
		L == TEXT("warlock") || L == TEXT("sorcerer"))
		return TEXT("mage");

	// ---- Acolyte line -> acolyte ----
	if (L == TEXT("priest") || L == TEXT("monk") ||
		L == TEXT("high_priest") || L == TEXT("champion") ||
		L == TEXT("high_acolyte") ||
		L == TEXT("arch_bishop") || L == TEXT("sura"))
		return TEXT("acolyte");

	// ---- Archer line -> archer ----
	if (L == TEXT("hunter") || L == TEXT("bard") || L == TEXT("dancer") ||
		L == TEXT("sniper") || L == TEXT("minstrel") || L == TEXT("gypsy") ||
		L == TEXT("high_archer") ||
		L == TEXT("ranger") || L == TEXT("maestro") || L == TEXT("wanderer"))
		return TEXT("archer");

	// ---- Thief line -> thief ----
	if (L == TEXT("assassin") || L == TEXT("rogue") ||
		L == TEXT("assassin_cross") || L == TEXT("stalker") ||
		L == TEXT("high_thief") ||
		L == TEXT("guillotine_cross") || L == TEXT("shadow_chaser"))
		return TEXT("thief");

	// ---- Merchant line -> merchant ----
	if (L == TEXT("blacksmith") || L == TEXT("alchemist") ||
		L == TEXT("whitesmith") || L == TEXT("biochemist") ||
		L == TEXT("high_merchant") ||
		L == TEXT("mechanic") || L == TEXT("genetic"))
		return TEXT("merchant");

	// ---- Novice variants and misc / extended classes -> novice ----
	if (L == TEXT("super_novice") || L == TEXT("supernovice") ||
		L == TEXT("high_novice") ||
		L == TEXT("ninja") || L == TEXT("gunslinger") || L == TEXT("rebellion") ||
		L == TEXT("taekwon") || L == TEXT("star_gladiator") || L == TEXT("soul_linker") ||
		L == TEXT("doram") || L == TEXT("summoner"))
		return TEXT("novice");

	// Unknown — passthrough (will fail lookup, sound will not play)
	return L;
}

EPlayerBodyMaterial UAudioSubsystem::ResolveBodyMaterialFor(const FString& JobClass) const
{
	const FString L = JobClass.ToLower();

	// Metal/Plate — heavy armor classes (Knight/Crusader lines + Monk + Star Gladiator)
	if (L == TEXT("knight") || L == TEXT("lord_knight") || L == TEXT("rune_knight") ||
		L == TEXT("crusader") || L == TEXT("paladin") || L == TEXT("royal_guard") ||
		L == TEXT("monk") || L == TEXT("champion") || L == TEXT("sura") ||
		L == TEXT("star_gladiator"))
		return EPlayerBodyMaterial::Metal;

	// Wood/Leather — Archer/Thief lines + Ninja/Gunslinger/Taekwon (agile / ranged).
	// Server class names (note: minstrel not clown, biochemist line stays cloth via merchant).
	if (L == TEXT("archer") || L == TEXT("hunter") || L == TEXT("sniper") || L == TEXT("ranger") ||
		L == TEXT("bard") || L == TEXT("minstrel") || L == TEXT("maestro") ||
		L == TEXT("dancer") || L == TEXT("gypsy") || L == TEXT("wanderer") ||
		L == TEXT("high_archer") ||
		L == TEXT("thief") || L == TEXT("assassin") || L == TEXT("assassin_cross") ||
		L == TEXT("guillotine_cross") || L == TEXT("rogue") || L == TEXT("stalker") ||
		L == TEXT("shadow_chaser") ||
		L == TEXT("high_thief") ||
		L == TEXT("ninja") || L == TEXT("gunslinger") || L == TEXT("rebellion") ||
		L == TEXT("taekwon"))
		return EPlayerBodyMaterial::WoodLeather;

	// Default: Cloth — Novice/Swordsman/Mage/Acolyte/Merchant base + Wizard/Sage/Priest/
	// Blacksmith/Alchemist/Super Novice/Soul Linker/Summoner + their high/transcendent variants.
	return EPlayerBodyMaterial::Cloth;
}

const FMonsterSoundConfig* UAudioSubsystem::ResolveConfig(const FString& SpriteClass) const
{
	// Direct lookup first (fast path)
	if (const FMonsterSoundConfig* Direct = MonsterSoundMap.Find(SpriteClass))
		return Direct;

	// Case-insensitive fallback (server may send "Poring" vs "poring")
	const FString Lower = SpriteClass.ToLower();
	if (const FMonsterSoundConfig* LowerHit = MonsterSoundMap.Find(Lower))
		return LowerHit;

	return nullptr;
}

USoundBase* UAudioSubsystem::LoadSoundCached(const FString& AssetPath)
{
	if (AssetPath.IsEmpty()) return nullptr;

	if (TObjectPtr<USoundBase>* Cached = SoundCache.Find(AssetPath))
	{
		if (Cached->Get()) return Cached->Get();
	}

	USoundBase* Loaded = LoadObject<USoundBase>(nullptr, *AssetPath);
	if (Loaded)
	{
		SoundCache.Add(AssetPath, Loaded);
	}
	return Loaded;
}

bool UAudioSubsystem::PlaySoundInternal(const FString& AssetPath, const FVector& Location, EMonsterSoundType Type)
{
	if (!TryReserveConcurrencySlot(Type))
	{
		// Cap reached — drop this play silently. Logged at Verbose to keep production logs clean.
		UE_LOG(LogMMOAudio, Verbose, TEXT("Concurrency cap reached for type %d — dropping %s"),
			(int32)Type, *AssetPath);
		return false;
	}

	USoundBase* Sound = LoadSoundCached(AssetPath);
	if (!Sound)
	{
		UE_LOG(LogMMOAudio, Warning, TEXT("Failed to load sound: %s"), *AssetPath);
		return false;
	}

	// Defensive: if a previous code path or older build set SoundClassObject, clear
	// it so the audio device doesn't try to look up properties for an unregistered
	// runtime UObject and spam "Unable to find sound class properties" warnings.
	Sound->SoundClassObject = nullptr;

	UWorld* World = GetWorld();
	if (!World) return false;

	// Bake the effective SFX volume into VolumeMultiplier — this REPLACES the old
	// USoundClass routing. Slider changes affect future plays only (one-shots are
	// short, no live update needed).
	const float Vol = GetEffectiveSfxVolume();
	UGameplayStatics::PlaySoundAtLocation(
		World,
		Sound,
		Location,
		Vol,                        // VolumeMultiplier (bus×master×mute baked in)
		1.0f,                       // PitchMultiplier
		0.0f,                       // StartTime
		MonsterAttenuation,         // Attenuation override
		nullptr,                    // Concurrency override
		nullptr                     // Owning actor
	);
	return true;
}

void UAudioSubsystem::PlaySound2DInternal(const FString& AssetPath, float VolumeMultiplier)
{
	if (AssetPath.IsEmpty()) return;
	USoundBase* Sound = LoadSoundCached(AssetPath);
	if (!Sound)
	{
		UE_LOG(LogMMOAudio, Warning, TEXT("Failed to load 2D sound: %s"), *AssetPath);
		return;
	}

	// Defensive: clear SoundClassObject (see PlaySoundInternal comment).
	Sound->SoundClassObject = nullptr;

	UWorld* World = GetWorld();
	if (!World) return;
	// 2D non-spatial — no concurrency cap (UI/event sounds are infrequent and important).
	// Bake the effective SFX volume in (caller's VolumeMultiplier × bus×master×mute).
	UGameplayStatics::PlaySound2D(World, Sound, VolumeMultiplier * GetEffectiveSfxVolume(), 1.0f, 0.0f);
}

bool UAudioSubsystem::TryReserveConcurrencySlot(EMonsterSoundType Type)
{
	FConcurrencyState* State = ConcurrencyTracking.Find(Type);
	if (!State) return true; // No cap configured -> allow

	UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;

	// Reap expired entries (older than Lifetime)
	const double Cutoff = Now - State->Lifetime;
	State->RecentPlayTimes.RemoveAll([Cutoff](double T) { return T < Cutoff; });

	// Check cap
	if (State->RecentPlayTimes.Num() >= State->MaxConcurrent)
	{
		return false;
	}

	// Reserve a slot
	State->RecentPlayTimes.Add(Now);
	return true;
}

const FString& UAudioSubsystem::GetPathForType(const FMonsterSoundConfig& Config, EMonsterSoundType Type) const
{
	// Variant SFX support: each event holds an array of one or more wav paths.
	// Picks a random variant per call so monsters with multi-take sound sets
	// (ghoul_attack1..4, hornet_stand + hornet_stand2, vadon_die1..3, etc.)
	// don't sound robotic. Single-element arrays are returned directly.
	const TArray<FString>* Paths = nullptr;
	switch (Type)
	{
		case EMonsterSoundType::Attack: Paths = &Config.AttackPaths; break;
		case EMonsterSoundType::Damage: Paths = &Config.DamagePaths; break;
		case EMonsterSoundType::Die:    Paths = &Config.DiePaths;    break;
		case EMonsterSoundType::Move:   Paths = &Config.MovePaths;   break;
		case EMonsterSoundType::Stand:  Paths = &Config.StandPaths;  break;
	}
	static const FString Empty;
	if (!Paths || Paths->Num() == 0) return Empty;
	if (Paths->Num() == 1)            return (*Paths)[0];
	return (*Paths)[FMath::RandRange(0, Paths->Num() - 1)];
}
