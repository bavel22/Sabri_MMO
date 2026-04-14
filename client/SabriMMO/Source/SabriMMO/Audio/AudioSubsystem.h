// AudioSubsystem.h — Monster + player sound playback for the SFX layer.
//
// Monster sounds (RO Classic data\wav\ behavior):
//   <monster>_attack.wav   on attack frame
//   <monster>_damage.wav   on hit reaction (layered with body material)
//   <monster>_die.wav      on death (layered with body material)
//   <monster>_move.wav     per Walk animation cycle (frame-synced via SpriteCharacterActor delegate)
//   <monster>_stand.wav    cycling idle ambient at IdleInterval seconds (timer-driven by EnemySubsystem)
//
// Player sounds (per-class system, mirrors roBrowser's WeaponSoundTable fallback):
//   _<basejob>_attack.wav  swing on auto-attack (per-class, e.g. swordman_attack)
//   _<basejob>_hit.wav     hit reaction sound when this class lands a hit (per-class)
//   player_<material>.wav  body reaction when this player gets hit (cloth/wood/metal)
//   levelup.wav            local-only 2D ascending chime on level up
//   _heal_effect.wav       heal sound at the recipient location
//
// Status sounds: stun/curse/poison/blind/freeze/silence/sleep/stone played at the
// affected entity's location when server emits status:applied.
//
// Body material layering for monsters: monster_<material>.wav plays simultaneously
// with damage/die (RO Classic's "physical impact + body crumple" two-layer model).
//
// Concurrency: each event type has a max simultaneous slot count to prevent audio
// chaos when many entities are active at once.
//
// Sprite class -> sound config lookup is built once in OnWorldBeginPlay.
// Family aliases (poporing -> poring, drops -> poring, etc.) are first-class.
// Job class -> base class fallback handled too (knight -> swordman, wizard -> magician, etc.).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "AudioSubsystem.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundClass;
class USoundMix;
class UAudioComponent;

// ============================================================
// Sound type enum — which event triggered the sound
// ============================================================

UENUM(BlueprintType)
enum class EMonsterSoundType : uint8
{
	Attack    UMETA(DisplayName = "Attack"),
	Damage    UMETA(DisplayName = "Damage"),
	Die       UMETA(DisplayName = "Die"),
	Move      UMETA(DisplayName = "Move"),
	Stand     UMETA(DisplayName = "Stand")
};

// ============================================================
// Body material — drives the player_<material>.wav reaction sound
// when a player takes damage. Per RO Classic JobHitSoundTable.
// ============================================================

UENUM(BlueprintType)
enum class EPlayerBodyMaterial : uint8
{
	Cloth      UMETA(DisplayName = "Cloth"),       // Novice/Swordman/Magician/Acolyte/Merchant lines (most non-physical)
	WoodLeather UMETA(DisplayName = "Wood/Leather"), // Archer/Thief lines + Ninja/Gunslinger/Taekwon
	Metal      UMETA(DisplayName = "Metal/Plate")  // Knight/Crusader/Monk/Star Gladiator
};

// ============================================================
// Per-monster sound config
// ============================================================

struct FMonsterSoundConfig
{
	// Asset paths per event (e.g. "/Game/SabriMMO/Audio/SFX/Monsters/poring_attack").
	// Each is an array of one or more variant paths — at playback time a random
	// variant is picked. Many RO Classic monsters have multi-variant attack/die/
	// move/stand sounds (e.g. ghoul_attack1..4, hornet_stand + hornet_stand2,
	// vadon_die1..3) and using all of them prevents robotic repetition.
	// An empty array means "no sound for this event" (silently skipped).
	TArray<FString> AttackPaths;
	TArray<FString> DamagePaths;
	TArray<FString> DiePaths;
	TArray<FString> MovePaths;
	TArray<FString> StandPaths;

	// Body material sound — plays *layered* with Damage and Die events to mimic
	// RO Classic's "primary impact + body crumple" two-layer model. This is a
	// single FString (not a variant array) because in classic RO the body
	// material wav is a fixed family-wide reaction, not a per-monster variant.
	// Examples: monster_poring.wav (Poring family), monster_skelton.wav (Skeleton family),
	// monster_insect.wav, monster_clothes.wav, monster_metal.wav.
	FString BodyMaterialPath;

	// Stand sound interval in seconds (RO Classic default ~5s for monsters with idle ambient).
	// Used by EnemySubsystem timer; ignored if StandPaths is empty.
	float StandIntervalSeconds = 5.0f;
};

// ============================================================
// Concurrency tracking — keeps simultaneous sound counts under control
// ============================================================

struct FConcurrencyState
{
	// Recent play timestamps. Entries older than Lifetime are reaped on each access.
	TArray<double> RecentPlayTimes;

	// Max concurrent plays of this type. Calls beyond this are dropped.
	int32 MaxConcurrent = 8;

	// How long a sound counts toward the cap (seconds).
	// Approximation of typical monster sound length.
	double Lifetime = 2.0;
};

// ============================================================
// UAudioSubsystem
// ============================================================

UCLASS()
class SABRIMMO_API UAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Public API ----

	// Play a monster sound at a 3D world location.
	// SpriteClass is FEnemyEntry::SpriteClass (e.g. "poring", "skeleton").
	// Family aliases are resolved automatically (poporing/drops/marin/mastering -> poring).
	// For Damage and Die, the body material sound is automatically layered if configured.
	// Concurrency limits are enforced — calls that exceed the cap are silently dropped.
	void PlayMonsterSound(const FString& SpriteClass, EMonsterSoundType Type, const FVector& Location);

	// Returns true if the given sprite class has a stand sound configured.
	// Used by EnemySubsystem to decide whether to spawn a stand sound timer per entity.
	bool HasStandSound(const FString& SpriteClass) const;

	// Returns the configured stand interval (seconds) for the sprite class, or 0 if none.
	float GetStandInterval(const FString& SpriteClass) const;

	// Play a status effect sound (stun, curse, poison, blind, freeze, etc.) at a 3D location.
	// Bypasses concurrency caps (status events are rare and important — always play).
	void PlayStatusSound(const FString& StatusType, const FVector& Location);

	// ---- Player sound API ----

	// Play a player swing sound (per-class fallback, e.g. swordman_attack.wav).
	// JobClass is the FCharacterData/FPlayerEntry::JobClass string ("swordsman", "knight", etc.).
	// Second class jobs auto-fall back to their base class sound (knight -> swordsman).
	// This is the unarmed/fallback path used when weapon-type sound is unavailable.
	void PlayPlayerAttackSound(const FString& JobClass, const FVector& Location);

	// Play a player hit-reaction sound (per-class, e.g. swordman_hit.wav). Used for the
	// "I just hit something" feedback on the attacker side, distinct from the body
	// reaction on the target side.
	void PlayPlayerHitSound(const FString& AttackerJobClass, const FVector& TargetLocation);

	// Play a per-weapon-type swing sound at the attacker's location (RO Classic primary path).
	// WeaponType is the server's weapon type string: dagger, one_hand_sword, two_hand_sword,
	// spear, one_hand_axe, two_hand_axe, mace, staff, bow, katar, knuckle, book, whip, instrument.
	// Returns true if a sound was played; false if no wav is mapped for this weapon type
	// (caller should fall back to PlayPlayerAttackSound for the per-class unarmed sound).
	bool PlayWeaponAttackSound(const FString& WeaponType, const FVector& Location);

	// Play a per-weapon-type hit sound at the target's location.
	// Knuckle/unarmed picks a random fist variant 1-4. Returns false if no wav is mapped.
	bool PlayWeaponHitSound(const FString& WeaponType, const FVector& TargetLocation);

	// Play the body reaction sound when a player gets hit (player_<material>.wav).
	// Material is derived from the target's job class via the cloth/wood/metal lookup.
	void PlayPlayerBodyReaction(const FString& JobClass, const FVector& Location);

	// Play the iconic level-up chime. 2D non-spatial — local player only.
	void PlayLevelUpSound();

	// Play heal effect sound at a 3D location (heal_effect.wav).
	void PlayHealSound(const FVector& Location);

	// Play voice grunt when a player gets hit (damage_<class>_female.wav for female,
	// damage_male.wav for male). Layered with PlayPlayerBodyReaction for the third
	// audio layer of player damage feedback.
	void PlayPlayerDamageVoice(const FString& JobClass, bool bIsFemale, const FVector& Location);

	// Play voice cry when a player dies (die_<class>_female.wav for female,
	// die_male.wav for male).
	void PlayPlayerDeathVoice(const FString& JobClass, bool bIsFemale, const FVector& Location);

	// ---- UI / Event sound API ----

	// Play universal button click sound (2D non-spatial). Used by every Slate
	// OnClicked handler. Cheap — short cached load + 2D playback, no concurrency check.
	void PlayUIClick();

	// Play universal cancel/close sound (2D non-spatial). Used for cancel buttons,
	// ESC closing windows, etc.
	void PlayUICancel();

	// Play random equip/unequip metallic kachunk sound (2D non-spatial). Picks a
	// random variant from se_equip_item_01..07 each call to prevent repetition.
	void PlayEquipSound();

	// Play item drop tier chime at a 3D location. Tier color string maps to one of
	// drop_pink/yellow/purple/blue/green/red.wav. Unknown tier defaults to "blue".
	void PlayItemDropSound(const FString& TierColor, const FVector& Location);

	// Play coin pickup chime (2D non-spatial). Used for zeny gained, vending sale,
	// item sold to NPC.
	void PlayCoinPickupSound();

	// Play job level up chime (2D non-spatial). Distinct from base level up — uses
	// st_job_level_up.wav. Caller (BasicInfoSubsystem) decides which to play.
	void PlayJobLevelUpSound();

	// Play refining success chime (2D non-spatial). bs_refinesuccess.wav.
	void PlayRefineSuccessSound();

	// Play refining failure thud (2D non-spatial). bs_refinefailed.wav.
	void PlayRefineFailSound();

	// Play pharmacy/cooking success chime (2D non-spatial). p_success.wav.
	// Used for alchemist crafting and cooking results.
	void PlayPharmacySuccessSound();

	// Play pharmacy/cooking failure thud (2D non-spatial). p_failed.wav.
	void PlayPharmacyFailSound();

	// Play MVP defeated fanfare (2D non-spatial). st_mvp.wav. Iconic RO Classic
	// MVP kill stinger. Caller fires this on MVP death broadcast for the killer's
	// party (or all participants).
	void PlayMvpFanfareSound();

	// Play warp portal sound at a 3D location. ef_portal.wav. Used when a warp
	// portal is created or entered.
	void PlayWarpPortalSound(const FVector& Location);

	// Play instant teleport sound at a 3D location. ef_teleportation.wav. Used for
	// fly wing, butterfly wing, kafra teleport, /memo recall.
	void PlayTeleportSound(const FVector& Location);

	// ---- Static convenience helpers ----
	// Resolve the audio subsystem from a UWorld* and play the named UI sound.
	// Use these from Slate widget OnClicked handlers / subsystem button handlers
	// where you only have a UWorld* (or a UWorldSubsystem*::GetWorld()).
	// Safe to call with nullptr — will silently no-op if no world or subsystem.
	static void PlayUIClickStatic(UWorld* World);
	static void PlayUICancelStatic(UWorld* World);
	static void PlayEquipStatic(UWorld* World);
	static void PlayCoinPickupStatic(UWorld* World);

	// ---- Zone ambient API ----

	// Switch the active ambient bed for a zone. Stops any current ambient layers
	// and starts the new ones. Each zone can have multiple looped 2D layers (wind +
	// birds + bells, etc.) configured in ZoneAmbientMap. Calling with an empty or
	// unknown zone stops all ambient. Idempotent — calling with the same zone twice
	// does not restart the loops.
	void PlayZoneAmbient(const FString& ZoneName);

	// Stop all currently playing ambient layers (used on zone teardown / logout).
	void StopAllAmbient();

	// ---- BGM (Background Music) API ----

	// Switch the active BGM for a zone. Stops any current BGM (with fade-out) and
	// starts the new track (with fade-in). Idempotent — calling with the same zone
	// is a no-op. ZoneToBgmMap defines the per-zone track assignment.
	void PlayZoneBgm(const FString& ZoneName);

	// Play a specific BGM asset directly (used by login screen, special situations).
	// AssetPath should be a /Game/... path. Pass an empty string to stop BGM.
	void PlayBgm(const FString& AssetPath);

	// Stop the currently playing BGM with a fade-out.
	void StopBgm();

	// Item use sound (e.g., potion drink — se_drink_potion.wav). 2D non-spatial.
	void PlayItemUseSound();

	// Element-typed hit sound. ElementType is one of: neutral/normal, fire, wind,
	// water, earth, holy, dark, ghost, undead, poison. Plays an element-specific
	// hit variant when available, falls back to neutral hit otherwise.
	// Layered with weapon/body sounds in HandleCombatDamage for elemental flavor.
	void PlayElementHitSound(const FString& ElementType, const FVector& Location);

	// Cloaking on/off transition sound (assasin_cloaking.wav). Used by skill 503
	// (Hiding) + Cloaking (1103) on buff_applied/removed.
	void PlayCloakingSound(const FVector& Location);

	// ---- Volume bus control (USoundClass-based) ----

	// Set master volume (0.0 to 1.0). Multiplies all 4 buses.
	void SetMasterVolume(float Volume);

	// Set BGM bus volume (0.0 to 1.0).
	void SetBgmVolume(float Volume);

	// Set SFX bus volume (0.0 to 1.0). Affects monster/player/skill/UI sounds.
	void SetSfxVolume(float Volume);

	// Set ambient bus volume (0.0 to 1.0). Zone ambient layers + 3D positional ambient.
	void SetAmbientVolume(float Volume);

	// Mute/unmute all audio (used by "mute when minimized" option).
	void SetMuted(bool bMuted);

	// Toggle whether the auto mute-on-focus-loss behavior is enabled.
	// Wired by OptionsSubsystem::SetMuteWhenMinimized.
	void SetMuteWhenMinimizedEnabled(bool bEnabled);

	// ---- Skill SFX API ----
	// Returns true if a sound was played, false if skill ID has no mapping.
	// Cast sound fires at the caster when skill:cast_start arrives (typically the
	// "spell windup" sound). Impact sound fires at the target when damage lands.
	// Many RO Classic skills only have ONE sound (the impact); for those, the cast
	// map can be left empty and only the impact map is populated.
	//
	// HitNumber semantics for PlaySkillImpactSound (matches server's skill:effect_damage payload):
	//   0 = single-hit event (Bash, Magnum Break, Fire Ball, etc.) — dedup applies
	//   >=1 = per-hit event of a multi-hit skill (Cold Bolt #2, Soul Strike #5) — dedup
	//         is BYPASSED so each projectile gets its own audible thump.
	bool PlaySkillCastSound(int32 SkillId, const FVector& Location);
	bool PlaySkillImpactSound(int32 SkillId, const FVector& Location, int32 HitNumber = 0);

	// Effective volume helpers — combine bus, master, and mute into a single multiplier
	// that gets passed to PlaySoundAtLocation/PlaySound2D as VolumeMultiplier. These
	// REPLACE the broken USoundClass-based bus routing — the old approach mutated the
	// shared sound asset's SoundClassObject which doesn't survive PIE multi-world or
	// world transitions, leading to "Unable to find sound class properties" spam.
	// Public so external systems (CombatActionSubsystem legacy hit sounds, etc.) can
	// route their direct PlaySoundAtLocation calls through the SFX bus too.
	float GetEffectiveSfxVolume()     const { return bAudioMuted ? 0.f : CurrentSfxVolume     * CurrentMasterVolume; }
	float GetEffectiveBgmVolume()     const { return bAudioMuted ? 0.f : CurrentBgmVolume     * CurrentMasterVolume; }
	float GetEffectiveAmbientVolume() const { return bAudioMuted ? 0.f : CurrentAmbientVolume * CurrentMasterVolume; }

private:
	// Server status:applied event handler — resolves target position and plays the status sound.
	void HandleStatusApplied(const TSharedPtr<FJsonValue>& Data);

	// Resolve a sprite class to its sound config (handles family aliases).
	const FMonsterSoundConfig* ResolveConfig(const FString& SpriteClass) const;

	// Lazy-load a USoundBase by /Game/ path. Cached on first access.
	USoundBase* LoadSoundCached(const FString& AssetPath);

	// Play one specific sound at a location, with concurrency check + attenuation.
	// Returns true if the sound was actually played (not dropped by concurrency).
	bool PlaySoundInternal(const FString& AssetPath, const FVector& Location, EMonsterSoundType Type);

	// Play one specific sound 2D non-spatial (UI / event sounds, no concurrency check).
	void PlaySound2DInternal(const FString& AssetPath, float VolumeMultiplier = 1.0f);

	// Apply the current volume settings to all currently-playing long-running audio
	// components (BGM + 2D ambient + 3D ambient). Called by all SetXxxVolume setters
	// AND SetMuted. One-shot sounds are not affected — they bake the volume in at
	// playback time via the Get*EffectiveVolume() helpers.
	void ApplyVolumeToActiveComponents();

	// Pick the right path field from a config given the sound type.
	const FString& GetPathForType(const FMonsterSoundConfig& Config, EMonsterSoundType Type) const;

	// Concurrency: returns true if this type still has slots; updates timestamps as a side effect.
	bool TryReserveConcurrencySlot(EMonsterSoundType Type);

	// ---- State ----

	// Sprite class -> sound config (built once in OnWorldBeginPlay)
	TMap<FString, FMonsterSoundConfig> MonsterSoundMap;

	// Status type (e.g. "stun") -> wav asset path. Built once in OnWorldBeginPlay.
	TMap<FString, FString> StatusSoundMap;

	// Player sound maps (job class -> wav path). Built once in OnWorldBeginPlay.
	// Only the 7 base classes are stored; second classes fall back via ResolveBaseClassFor().
	TMap<FString, FString> ClassAttackSoundMap;
	TMap<FString, FString> ClassHitSoundMap;

	// Weapon-type sound maps (server weapon type string -> wav path). Primary path
	// for swing/hit sounds when the attacker has a weapon equipped. Built once in
	// OnWorldBeginPlay.
	TMap<FString, FString> WeaponAttackSoundMap;
	TMap<FString, FString> WeaponHitSoundMap;

	// Fist hit variants (4 random alternates) — picked randomly when weapon type is
	// knuckle / bare_hand / unarmed.
	TArray<FString> FistHitSoundPaths;

	// Body material wav paths — one entry per EPlayerBodyMaterial value.
	TMap<EPlayerBodyMaterial, FString> BodyMaterialSoundMap;

	// Singleton player sounds
	FString LevelUpSoundPath;
	FString HealSoundPath;

	// ---- UI / Event sound paths (built once in OnWorldBeginPlay) ----

	// Universal UI button click + cancel (Korean files renamed: 버튼소리/취소)
	FString UIClickSoundPath;
	FString UICancelSoundPath;

	// Equip/unequip metallic kachunk variants (se_equip_item_01..07).
	// Random pick on each PlayEquipSound() call.
	TArray<FString> EquipSoundPaths;

	// Item drop tier color -> wav path (drop_pink/yellow/purple/blue/green/red).
	// Tier color string passed by caller, default fallback "blue".
	TMap<FString, FString> DropSoundPaths;

	// Coin pickup chime (ef_coin1.wav)
	FString CoinPickupSoundPath;

	// Job level up chime (st_job_level_up.wav) — distinct from base level up
	FString JobLevelUpSoundPath;

	// Refining feedback (bs_refinesuccess/failed.wav)
	FString RefineSuccessSoundPath;
	FString RefineFailSoundPath;

	// Pharmacy/cooking feedback (p_success/failed.wav)
	FString PharmacySuccessSoundPath;
	FString PharmacyFailSoundPath;

	// MVP defeated fanfare (st_mvp.wav)
	FString MvpFanfareSoundPath;

	// Warp portal / teleport sounds (ef_portal/ef_teleportation.wav)
	FString WarpPortalSoundPath;
	FString TeleportSoundPath;

	// Player damage/death voice maps (key: "<jobclass>_<gender>" -> wav path).
	// Gender suffix is "male" or "female". Female has per-class variants;
	// male is generic (only damage_male.wav / die_male.wav exist in source set).
	TMap<FString, FString> PlayerDamageVoiceMap;
	TMap<FString, FString> PlayerDeathVoiceMap;

	// ---- Zone ambient state ----

	// Zone name -> list of looped wav paths to play simultaneously as ambient layers.
	// Multiple layers per zone allow stacking wind + birds + bells, etc. for a richer
	// soundscape. Built once in OnWorldBeginPlay.
	TMap<FString, TArray<FString>> ZoneAmbientMap;

	// Active ambient component(s) — one per layer of the current zone. Stopped and
	// recreated whenever PlayZoneAmbient() is called with a different zone. Held with
	// UPROPERTY so the GC keeps them alive while the zone is active.
	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> ActiveAmbientComponents;

	// Cached zone name to support idempotent calls (don't restart on identical zone).
	FString CurrentAmbientZone;

	// ---- 3D positional ambient sources ----

	// Per-zone fixed-position ambient emitters (water sources, fires, frogs, etc.).
	// Spawned as 3D-attenuated UAudioComponents at world coordinates when the zone
	// loads. Mirrors the RSW embedded ambient source pattern from RO Classic.
	struct FAmbientPoint
	{
		FString SoundPath;
		FVector Location;
		float   AttenuationRadius;
	};
	TMap<FString, TArray<FAmbientPoint>> Zone3DAmbientMap;

	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> Active3DAmbientComponents;

	// Spawned attenuation for 3D ambient (separate from monster attenuation —
	// uses larger radius for area ambient).
	UPROPERTY()
	TObjectPtr<USoundAttenuation> AmbientAttenuation;

	// ---- BGM state ----

	// Zone name -> BGM asset path (e.g., "/Game/SabriMMO/Audio/BGM/bgm_08").
	// Built once in OnWorldBeginPlay from the mp3nametable-style mapping.
	TMap<FString, FString> ZoneToBgmMap;

	// Currently playing BGM component. Held alive via UPROPERTY for GC.
	UPROPERTY()
	TObjectPtr<UAudioComponent> ActiveBgmComponent;

	// Cached BGM asset path so PlayZoneBgm/PlayBgm can be idempotent.
	FString CurrentBgmPath;

	// ---- Volume bus state ----

	// Sound class hierarchy: Master parent → BGM/SFX/Ambient children.
	// Created programmatically in OnWorldBeginPlay.
	UPROPERTY()
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> BgmSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> SfxSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> AmbientSoundClass;

	// Sound mix used to push volume modifiers to the sound classes at runtime.
	UPROPERTY()
	TObjectPtr<USoundMix> VolumeMix;

	// Cached current volumes (0.0–1.0).
	float CurrentMasterVolume  = 1.0f;
	float CurrentBgmVolume     = 0.7f;
	float CurrentSfxVolume     = 1.0f;
	float CurrentAmbientVolume = 0.5f;
	bool  bAudioMuted          = false;

	// Whether mute-on-focus-loss is currently active. Pushed by OptionsSubsystem.
	bool bMuteWhenMinimizedEnabled = true;

	// Focus delegate handles for cleanup
	FDelegateHandle FocusGainedHandle;
	FDelegateHandle FocusLostHandle;

	// Focus event handlers — toggle mute based on bMuteWhenMinimizedEnabled
	void OnApplicationActivated();
	void OnApplicationDeactivated();

	// Default cast windup sound — played by PlaySkillCastSound for any skill with
	// a cast time that doesn't have its own per-skill cast wav mapped.
	// RO Classic uses ef_beginspell.wav as the universal "spell beginning" sound.
	FString DefaultSkillCastSoundPath;

	// Skill SFX maps (skill ID -> wav path). Built once in OnWorldBeginPlay.
	// SkillCastSoundMap is OPTIONAL — entries here override the default cast sound
	// for skills with a unique windup (e.g. specialty skills). All other cast skills
	// fall back to DefaultSkillCastSoundPath automatically.
	TMap<int32, FString> SkillCastSoundMap;
	TMap<int32, FString> SkillImpactSoundMap;

	// Dedup: skill ID -> last play time (seconds). Skills that emit BOTH skill:effect_damage
	// and skill:buff_applied (Stone Curse, Frost Diver) would otherwise double-fire — this
	// 300ms window suppresses the second call.
	TMap<int32, double> RecentSkillImpactPlayTime;

	// Resolve job class -> base class string ("knight" -> "swordman").
	FString ResolveBaseClassFor(const FString& JobClass) const;

	// Resolve job class -> body material category (cloth/wood/metal).
	EPlayerBodyMaterial ResolveBodyMaterialFor(const FString& JobClass) const;

	// Asset path -> loaded USoundBase (lazy cache, GC-protected)
	UPROPERTY()
	TMap<FString, TObjectPtr<USoundBase>> SoundCache;

	// Programmatic attenuation so spatialization works regardless of per-asset import settings
	UPROPERTY()
	TObjectPtr<USoundAttenuation> MonsterAttenuation;

	// Per-type concurrency state
	TMap<EMonsterSoundType, FConcurrencyState> ConcurrencyTracking;
};
