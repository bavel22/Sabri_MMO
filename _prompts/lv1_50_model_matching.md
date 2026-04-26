# Level 1-50 Enemy → GLB Model Matching

Complete cross-reference of every `.glb` file in `C:\Sabri_MMO\2D animations\3d_models\enemies\` against the Level 1-50 enemy roster from `_prompts/enemy_sprite_session_resume.md`. Generated 2026-04-19.

Source folder has **~205 GLBs**. The Lv 1-50 roster covers ~190 enemy templates; most have a dedicated GLB, some share a base model via RO variant rules (meta_ / provoke_ / X_ suffix / color recolor).

---

## 1. Lv 1-50 Enemy → GLB Matching Table

Columns: **Lv** = monster level. **Template Key** = template key in `server/src/ro_monster_templates.js`. **Name** = display name. **GLB File** = filename in `2D animations/3d_models/enemies/`. **Matching Rule** = direct / alias / shared / tint / none.

### Level 1-10

| Lv  | Template Key     | Name                  | GLB File                | Matching Rule                              |
| --- | ---------------- | --------------------- | ----------------------- | ------------------------------------------ |
|  1  | `poring`         | Poring                | `poring.glb`            | direct                                     |
|  1  | `red_plant`      | Red Plant             | `red_plant.glb`         | direct                                     |
|  1  | `blue_plant`     | Blue Plant            | `blue_plant.glb`        | direct                                     |
|  1  | `green_plant`    | Green Plant           | `green_plant.glb`       | direct                                     |
|  1  | `yellow_plant`   | Yellow Plant          | `yellow_plant.glb`      | direct                                     |
|  1  | `white_plant`    | White Plant           | `white_plant.glb`       | direct                                     |
|  1  | `shining_plant`  | Shining Plant         | `shining_plant.glb`     | direct                                     |
|  1  | `black_mushroom` | Black Mushroom        | `black_mushroom.glb`    | direct                                     |
|  1  | `red_mushroom`   | Red Mushroom          | `red_mushroom.glb`      | direct                                     |
|  1  | `thief_mushroom` | Thief Mushroom        | `thief_mushroom.glb`    | direct                                     |
|  1  | `fabre_`         | Fabre (variant)       | `fabre.glb`             | shared — `fabre_` inherits `fabre` sprite  |
|  1  | `crystal_1`      | Wind Crystal **BOSS** | `wind_crystal.glb`      | direct                                     |
|  1  | `crystal_2`      | Earth Crystal **BOSS**| `earth_crystal.glb`     | direct                                     |
|  1  | `crystal_3`      | Fire Crystal **BOSS** | `fire_crystal.glb`      | direct                                     |
|  1  | `crystal_4`      | Water Crystal **BOSS**| `ice_crystal.glb`       | tint — no `water_crystal.glb`; use `ice_crystal.glb` with blue tint OR rename the file |
|  2  | `fabre`          | Fabre                 | `fabre.glb`             | direct                                     |
|  2  | `pupa`           | Pupa                  | `pupa.glb`              | direct                                     |
|  2  | `mastering`      | Mastering **BOSS**    | `mastering.glb`         | direct                                     |
|  2  | `meta_fabre`     | Fabre (meta)          | `fabre.glb`             | shared — meta variant inherits             |
|  2  | `meta_pupa`      | Pupa (meta)           | `pupa.glb`              | shared                                     |
|  3  | `pecopeco_egg`   | Peco Peco Egg         | `pecopeco_egg.glb`      | direct                                     |
|  3  | `picky`          | Picky                 | `picky.glb`             | direct                                     |
|  3  | `poring_`        | Santa Poring          | `santa_poring.glb`      | alias — `santa_poring.glb` filename ≠ `poring_` template key |
|  3  | `lunatic`        | Lunatic               | `lunatic.glb`           | direct                                     |
|  3  | `drops`          | Drops                 | `drops.glb`             | direct                                     |
|  3  | `meta_pecopeco_egg` | Peco Peco Egg (meta) | `pecopeco_egg.glb`    | shared                                     |
|  3  | `meta_picky`     | Picky (meta)          | `picky.glb`             | shared                                     |
|  4  | `wilow`          | Willow                | `willow.glb`            | alias — template spells `wilow`, file `willow` |
|  4  | `chonchon`       | Chonchon              | `chonchon.glb`          | direct                                     |
|  4  | `thief_bug_egg`  | Thief Bug Egg         | `thief_bug_egg.glb`     | direct                                     |
|  4  | `picky_`         | Picky (variant)       | `picky.glb`             | shared                                     |
|  4  | `ant_egg`        | Ant Egg               | `ant_egg.glb`           | direct                                     |
|  4  | `chonchon_`      | Chonchon (variant)    | `chonchon.glb`          | shared                                     |
|  4  | `meta_ant_egg`   | Ant Egg (meta)        | `ant_egg.glb`           | shared                                     |
|  4  | `meta_picky_`    | Picky (meta)          | `picky.glb`             | shared                                     |
|  5  | `condor`         | Condor                | `condor.glb`            | direct                                     |
|  5  | `roda_frog`      | Roda Frog             | `roda_frog.glb`         | direct                                     |
|  6  | `thief_bug`      | Thief Bug             | `thief_bug.glb`         | direct                                     |
|  6  | `eclipse`        | Eclipse **BOSS**      | `eclipse.glb`           | direct (or tint of `lunatic`)              |
|  7  | `savage_babe`    | Savage Babe           | `savage_babe.glb`       | direct                                     |
|  8  | `hornet`         | Hornet                | `hornet.glb`            | direct                                     |
|  8  | `farmiliar`      | Familiar              | `farmiliar.glb`         | direct                                     |
|  8  | `dragon_fly`     | Dragon Fly **BOSS**   | `dragon_fly.glb`        | direct                                     |
|  9  | `rocker`         | Rocker                | `rocker.glb`            | direct                                     |
|  9  | `desert_wolf_b`  | Baby Desert Wolf      | `desert_wolf_b.glb`     | direct                                     |
| 10  | `thief_bug_`     | Thief Bug Female      | `thief_bug_f.glb`       | alias — template key `thief_bug_`, file `thief_bug_f.glb` |
| 10  | `skeleton`       | Skeleton              | `skeleton_t_pose.glb`   | alias — template key `skeleton`, file `skeleton_t_pose.glb` |
| 10  | `toad`           | Toad **BOSS**         | `toad.glb`              | direct                                     |
| 10  | `plankton`       | Plankton              | `plankton.glb`          | direct                                     |
| 10  | `antonio`        | Antonio               | `antonio.glb`           | direct                                     |

### Level 11-20

| Lv  | Template Key     | Name                  | GLB File               | Matching Rule                              |
| --- | ---------------- | --------------------- | ---------------------- | ------------------------------------------ |
| 11  | `kukre`          | Kukre                 | `kukre.glb`            | direct                                     |
| 11  | `tarou`          | Tarou                 | `tarou.glb`            | direct                                     |
| 12  | `mandragora`     | Mandragora            | `mandragora.glb`       | direct                                     |
| 13  | `ambernite`      | Ambernite             | `ambernite.glb`        | direct                                     |
| 14  | `worm_tail`      | Wormtail              | `wormtail.glb`         | alias — template `worm_tail`, file `wormtail` |
| 14  | `poporing`       | Poporing              | `poporing.glb`         | direct                                     |
| 14  | `hydra`          | Hydra                 | `hydra.glb`            | direct                                     |
| 15  | `zombie`         | Zombie                | `zombie.glb`           | direct                                     |
| 15  | `snake`          | Boa                   | `boa.glb`              | alias — template `snake`, file `boa` (aliased in build_enemy_list.js) |
| 15  | `shellfish`      | Shellfish             | `shellfish.glb`        | direct                                     |
| 15  | `marin`          | Marin                 | `marin.glb`            | direct                                     |
| 15  | `boiled_rice`    | Boiled Rice           | —                      | **no GLB** — still missing                 |
| 16  | `spore`          | Spore                 | `spore.glb`            | direct                                     |
| 16  | `creamy`         | Creamy                | `creamy.glb`           | direct                                     |
| 16  | `stainer`        | Stainer               | `stainer.glb`          | direct                                     |
| 16  | `meta_creamy`    | Creamy (meta)         | `creamy.glb`           | shared                                     |
| 17  | `steel_chonchon` | Steel Chonchon        | `steel_chonchon.glb`   | direct (or tint of `chonchon`)             |
| 17  | `muka`           | Muka                  | —                      | **no GLB** — still missing                 |
| 17  | `andre`          | Andre                 | `andre.glb`            | direct                                     |
| 17  | `coco`           | Coco                  | `coco.glb`             | direct                                     |
| 17  | `rafflesia`      | Rafflesia             | `rafflesia.glb`        | direct                                     |
| 17  | `meta_andre`     | Andre (meta)          | `andre.glb`            | shared                                     |
| 18  | `smokie`         | Smokie                | `smokie.glb`           | direct                                     |
| 18  | `vocal`          | Vocal                 | `vocal.glb`            | direct                                     |
| 18  | `ghostring`      | Ghostring **BOSS**    | `ghostring.glb`        | direct                                     |
| 18  | `horn`           | Horn                  | `horn.glb`             | direct                                     |
| 18  | `martin`         | Martin                | `martin.glb`           | direct                                     |
| 18  | `piere`          | Piere                 | `piere.glb`            | direct                                     |
| 18  | `meta_piere`     | Piere (meta)          | `piere.glb`            | shared                                     |
| 18  | `aster`          | Aster                 | `aster.glb`            | direct                                     |
| 19  | `pecopeco`       | Peco Peco             | `pecopeco.glb`         | direct                                     |
| 19  | `thief_bug__`    | Thief Bug Male        | `thief_bug_male.glb`   | alias — template `thief_bug__`, file `thief_bug_male` |
| 19  | `vadon`          | Vadon                 | `vadon.glb`            | direct                                     |
| 19  | `poison_spore`   | Poison Spore          | `poison_spore.glb`     | direct (or tint of `spore`)                |
| 19  | `deniro`         | Deniro                | `deniro.glb`           | direct                                     |
| 19  | `provoke_yoyo`   | Yoyo (provoke)        | `yoyo.glb`             | shared — provoke variant                   |
| 19  | `meta_deniro`    | Deniro (meta)         | `deniro.glb`           | shared                                     |
| 20  | `elder_wilow`    | Elder Willow          | `elder_wilow.glb`      | direct (or tint of `willow`)               |
| 20  | `crab`           | Crab                  | `crab.glb`             | direct                                     |
| 20  | `angeling`       | Angeling **BOSS**     | `angeling.glb`         | direct                                     |
| 20  | `vitata`         | Vitata                | `vitata.glb`           | direct                                     |

### Level 21-30

| Lv  | Template Key       | Name                   | GLB File                | Matching Rule                              |
| --- | ------------------ | ---------------------- | ----------------------- | ------------------------------------------ |
| 21  | `yoyo`             | Yoyo                   | `yoyo.glb`              | direct                                     |
| 21  | `dustiness`        | Dustiness              | `dustiness.glb`         | direct                                     |
| 21  | `marina`           | Marina                 | `marina.glb`            | direct (or tint of `marin`)                |
| 21  | `raggler`          | Raggler                | `raggler.glb`           | direct                                     |
| 21  | `orc_baby`         | Orc Baby               | `orc baby.glb`          | alias — filename has a **space**, not underscore |
| 22  | `thara_frog`       | Thara Frog             | `thara_frog.glb`        | direct (or tint of `roda_frog`)            |
| 22  | `metaller`         | Metaller               | `metaller.glb`          | direct                                     |
| 22  | `goblin_5`         | Goblin                 | `goblin.glb`            | shared — base goblin model for variants 1-5 |
| 23  | `anacondaq`        | Anacondaq              | `Anacondaq.glb`         | alias — **capitalized filename**           |
| 23  | `cornutus`         | Cornutus               | `cornutus.glb`          | direct                                     |
| 23  | `caramel`          | Caramel                | `caramel.glb`           | direct                                     |
| 23  | `goblin_4`         | Goblin                 | `goblin.glb`            | shared                                     |
| 23  | `zerom`            | Zerom                  | `zerom.glb`             | direct                                     |
| 23  | `anopheles`        | Anopheles              | `anopheles.glb`         | direct                                     |
| 23  | `stapo`            | Stapo                  | `stapo.glb`             | direct                                     |
| 24  | `scorpion`         | Scorpion               | `scorpion.glb`          | direct                                     |
| 24  | `ork_warrior`      | Orc Warrior            | `orc_warrior.glb`       | alias — template `ork_warrior`, file `orc_warrior` |
| 24  | `megalodon`        | Megalodon              | `megalodon.glb`         | direct                                     |
| 24  | `vagabond_wolf`    | Vagabond Wolf **BOSS** | `vagabond_wolf.glb`     | direct (or tint of `wolf`)                 |
| 24  | `drainliar`        | Drainliar              | `drainiliar.glb`        | alias — template `drainliar`, file `drainiliar` (typo) |
| 24  | `eggyra`           | Eggyra                 | `eggrya.glb`            | alias — template `eggyra`, file `eggrya` (typo) |
| 24  | `goblin_2`         | Goblin                 | `goblin.glb`            | shared                                     |
| 24  | `goblin_3`         | Goblin                 | `goblin.glb`            | shared                                     |
| 24  | `orc_zombie`       | Orc Zombie             | `orc_zombie.glb`        | direct                                     |
| 24  | `smoking_orc`      | Smoking Orc            | `smoking_orc.glb`       | direct                                     |
| 24  | `orc_xmas`         | Christmas Orc          | `orc_xmas.glb`          | direct                                     |
| 25  | `wolf`             | Wolf                   | `wolf.glb`              | direct                                     |
| 25  | `golem`            | Golem                  | `golem.glb`             | direct                                     |
| 25  | `bigfoot`          | Bigfoot                | `bigfoot.glb`           | direct                                     |
| 25  | `pirate_skel`      | Pirate Skeleton        | `pirate_skel.glb`       | direct                                     |
| 25  | `argos`            | Argos                  | `argos.glb`             | direct                                     |
| 25  | `goblin_1`         | Goblin                 | `goblin.glb`            | shared                                     |
| 25  | `gobline_xmas`     | Christmas Goblin       | `goblin_xmas.glb`       | alias — template `gobline_xmas`, file `goblin_xmas` |
| 25  | `cookie`           | Cookie                 | `cookie.glb`            | direct                                     |
| 25  | `rotar_zairo`      | Rotar Zairo            | `rotar_zairo.glb`       | direct                                     |
| 26  | `flora`            | Flora                  | `flora.glb`             | direct                                     |
| 26  | `hode`             | Hode                   | `hode.glb`              | direct                                     |
| 26  | `magnolia`         | Magnolia               | `magnolia.glb`          | direct                                     |
| 26  | `mantis`           | Mantis                 | `mantis.glb`            | direct                                     |
| 26  | `phen`             | Phen                   | `phen.glb`              | direct                                     |
| 26  | `savage`           | Savage                 | `savage.glb`            | direct                                     |
| 26  | `m_savage`         | Savage (meta?)         | `savage.glb`            | shared                                     |
| 26  | `metaling`         | Metaling               | `metaling.glb`          | direct                                     |
| 27  | `desert_wolf`      | Desert Wolf            | `desert_wolf.glb`       | direct                                     |
| 27  | `m_desert_wolf`    | Desert Wolf (meta?)    | `desert_wolf.glb`       | shared                                     |
| 27  | `rice_cake_boy`    | Dumpling Child         | `dumpling_child.glb`    | alias — template `rice_cake_boy`, file `dumpling_child` |
| 28  | `marine_sphere`    | Marine Sphere          | `marine_sphere.glb`     | direct                                     |
| 28  | `orc_skeleton`     | Orc Skeleton           | `orc_skeleton.glb`      | direct                                     |
| 28  | `cookie_xmas`      | Christmas Cookie       | `cookie_xmas.glb`       | direct                                     |
| 28  | `goblin_archer`    | Goblin Archer          | `goblin_archer.glb`     | direct                                     |
| 28  | `porcellio`        | Porcellio              | `Porcellio.glb`         | alias — **capitalized filename**           |
| 29  | `soldier_skeleton` | Soldier Skeleton       | `soldier_skeleton.glb`  | direct                                     |
| 29  | `giearth`          | Giearth                | `Giearth.glb`           | alias — **capitalized filename**           |
| 30  | `munak`            | Munak                  | `munak.glb`             | direct                                     |
| 30  | `sword_fish`       | Swordfish              | `swordfish.glb`         | alias — template `sword_fish`, file `swordfish` |
| 30  | `frilldora`        | Frilldora              | `frilldora.glb`         | direct                                     |
| 30  | `skel_worker`      | Skeleton Worker        | `skel_worker.glb`       | direct                                     |
| 30  | `sasquatch`        | Sasquatch              | `sasquatch.glb`         | direct                                     |
| 30  | `karakasa`         | Karakasa               | `karakasa.glb`          | direct                                     |

### Level 31-40

| Lv  | Template Key       | Name               | GLB File                | Matching Rule                              |
| --- | ------------------ | ------------------ | ----------------------- | ------------------------------------------ |
| 31  | `archer_skeleton`  | Archer Skeleton    | `soldier_skeleton.glb`  | shared — same body, different bow pose (or tint `skeleton_t_pose`) |
| 31  | `obeaune`          | Obeaune            | `obeaune.glb`           | direct                                     |
| 31  | `kobold_2`         | Kobold             | `kobold.glb`            | shared                                     |
| 31  | `kobold_3`         | Kobold             | `kobold.glb`            | shared                                     |
| 31  | `marse`            | Marse              | `marse.glb`             | direct                                     |
| 31  | `matyr`            | Matyr              | `matyr.glb`             | direct                                     |
| 31  | `zenorc`           | Zenorc             | `zenorc.glb`            | direct                                     |
| 31  | `orc_lady`         | Orc Lady           | `orc_lady.glb`          | direct                                     |
| 31  | `deviling`         | Deviling **BOSS**  | `deviling.glb`          | direct                                     |
| 31  | `roween`           | Roween             | `roween.glb`            | direct                                     |
| 32  | `bon_gun`          | Bongun             | `bogun.glb`             | alias — template `bon_gun`, file `bogun`   |
| 32  | `tri_joint`        | Tri Joint          | `tri_joint.glb`         | direct                                     |
| 32  | `baby_leopard`     | Baby Leopard       | `baby_leopard.glb`      | direct                                     |
| 33  | `dokebi`           | Dokebi             | `dokebi.glb`            | direct                                     |
| 33  | `sohee`            | Sohee              | `sohee.glb`             | direct                                     |
| 33  | `kobold_archer`    | Kobold Archer      | `kobold_archer.glb`     | direct                                     |
| 33  | `miyabi_ningyo`    | Miyabi Doll        | `miyabi_doll.glb`       | alias — template `miyabi_ningyo`, file `miyabi_doll` |
| 34  | `horong`           | Horong             | `horong.glb`            | direct                                     |
| 34  | `sand_man`         | Sandman            | `sandman.glb`           | alias — template `sand_man`, file `sandman` |
| 34  | `whisper`          | Whisper            | `whisper.glb`           | direct                                     |
| 34  | `whisper_`         | Whisper (variant)  | `whisper.glb`           | shared                                     |
| 34  | `whisper_boss`     | Giant Whisper      | `whisper.glb`           | shared (or tint)                           |
| 34  | `kind_of_beetle`   | Beetle King        | `beetle_king.glb`       | alias — template `kind_of_beetle`, file `beetle_king` |
| 35  | `requiem`          | Requiem            | `requiem.glb`           | direct                                     |
| 35  | `cruiser`          | Cruiser            | `cruiser.glb`           | direct                                     |
| 35  | `steam_goblin`     | Goblin Steamrider  | `steam_goblin.glb`      | direct                                     |
| 35  | `zipper_bear`      | Zipper Bear        | `zipper_bear.glb`       | direct                                     |
| 35  | `noxious`          | Noxious            | `noxious.glb`           | direct                                     |
| 36  | `marc`             | Marc               | `marc.glb`              | direct                                     |
| 36  | `kobold_1`         | Kobold             | `kobold.glb`            | shared                                     |
| 36  | `lude`             | Lude               | `lude.glb`              | direct                                     |
| 36  | `mole`             | Holden             | `holden.glb`            | alias — template `mole`, file `holden`     |
| 37  | `mummy`            | Mummy              | `mummy.glb`             | direct                                     |
| 38  | `verit`            | Verit              | `verit.glb`             | direct                                     |
| 38  | `jakk`             | Jakk               | `jakk.glb`              | direct                                     |
| 38  | `myst`             | Myst               | `myst.glb`              | direct                                     |
| 38  | `jakk_xmas`        | Christmas Jakk     | `jakk_xmas.glb`         | direct (or tint of `jakk`)                 |
| 38  | `mystcase`         | Myst Case          | `myst_case.glb`         | alias — template `mystcase`, file `myst_case` |
| 38  | `wild_rose`        | Wild Rose          | `wild_rose.glb`         | direct                                     |
| 38  | `leaf_cat`         | Leaf Cat           | `leaf_cat.glb`          | direct                                     |
| 38  | `iceicle`          | Iceicle            | `iceicle.glb`           | direct                                     |
| 39  | `wootan_shooter`   | Wootan Shooter     | `wootan_shooter.glb`    | direct                                     |
| 40  | `ghoul`            | Ghoul              | `ghoul.glb`             | direct                                     |
| 40  | `marduk`           | Marduk             | `marduk.glb`            | direct                                     |
| 40  | `stem_worm`        | Stem Worm          | `stem_worm.glb`         | direct                                     |
| 40  | `neraid`           | Nereid             | `Nereid.glb`            | alias — template `neraid`, file `Nereid` (**capitalized**, different spelling) |
| 40  | `pest`             | Pest               | `pest.glb`              | direct                                     |
| 40  | `greatest_general` | Greatest General   | `greatest_general.glb`  | direct                                     |
| 40  | `quve`             | Quve               | `quve.glb`              | direct                                     |
| 40  | `mime_monkey`      | Mime Monkey        | `mime_monkey.glb`       | direct                                     |
| 40  | `magmaring`        | Magmaring          | `magmaring.glb`         | direct (or tint of `poring`)               |

### Level 41-50

| Lv  | Template Key       | Name             | GLB File                    | Matching Rule                              |
| --- | ------------------ | ---------------- | --------------------------- | ------------------------------------------ |
| 41  | `argiope`          | Argiope          | `argiope.glb`               | direct                                     |
| 41  | `marionette`       | Marionette       | `marionette.glb`            | direct                                     |
| 41  | `kapha`            | Kapha            | `kapha.glb`                 | direct                                     |
| 41  | `wootan_fighter`   | Wootan Fighter   | `wootan_fighter.glb`        | direct                                     |
| 42  | `hunter_fly`       | Hunter Fly       | `hunter_fly.glb`            | direct                                     |
| 42  | `chepet`           | Chepet           | `chepet.glb`                | direct                                     |
| 42  | `alligator`        | Alligator        | `alligator.glb`             | direct                                     |
| 42  | `stone_shooter`    | Stone Shooter    | `stone_shooter.glb`         | direct                                     |
| 42  | `venomous`         | Venomous         | `venomous.glb`              | direct                                     |
| 42  | `novus`            | Novus (base)     | `novus_blue.glb`            | alias — pick blue as canonical base        |
| 42  | `siroma`           | Siroma           | `siroma.glb`                | direct                                     |
| 43  | `side_winder`      | Side Winder      | `side_winder.glb`           | direct                                     |
| 43  | `punk`             | Punk             | `punk.glb`                  | direct                                     |
| 43  | `choco`            | Choco            | `choco.glb`                 | direct                                     |
| 43  | `sageworm`         | Sage Worm        | `sage_worm.glb`             | alias — template `sageworm`, file `sage_worm` |
| 43  | `blazzer`          | Blazer           | `blazer.glb`                | alias — template `blazzer`, file `blazer`  |
| 43  | `pitman`           | Pitman           | `pitman.glb`                | direct                                     |
| 43  | `hill_wind`        | Hill Wind        | `hill_wind.glb`             | direct                                     |
| 43  | `plasma_r`         | Plasma (red)     | `plasma.glb`                | shared + tint (red multiply on `plasma.glb`) |
| 43  | `novus_`           | Novus (variant)  | `novus_green.glb`           | alias — 2nd novus color variant            |
| 43  | `dragon_egg`       | Dragon Egg       | `dragon_egg.glb`            | direct                                     |
| 44  | `bathory`          | Bathory          | `bathory.glb`               | direct                                     |
| 44  | `petit`            | Petite (base)    | `petite_blue.glb`           | alias — pick blue as canonical base        |
| 44  | `plasma_b`         | Plasma (blue)    | `plasma.glb`                | shared + tint                              |
| 44  | `galion`           | Galion **BOSS**  | `galion.glb`                | direct                                     |
| 45  | `petit_`           | Petite (variant) | `petite_green.glb`          | alias — 2nd petite color variant           |
| 45  | `megalith`         | Megalith         | `megalith.glb`              | direct                                     |
| 45  | `hill_wind_1`      | Hill Wind        | `hill_wind.glb`             | shared                                     |
| 46  | `deviruchi`        | Deviruchi        | `deviruchi.glb`             | direct                                     |
| 46  | `brilight`         | Brilight         | `brilight.glb`              | direct                                     |
| 46  | `explosion`        | Explosion        | `explosion.glb`             | direct                                     |
| 46  | `poison_toad`      | Poison Toad      | `poison_toad.glb`           | direct (or tint of `toad`)                 |
| 46  | `wild_ginseng`     | Hermit Plant     | `hermit_plant.glb`          | alias — template `wild_ginseng`, file `hermit_plant` |
| 46  | `drosera`          | Drosera          | `Drosera.glb`               | alias — **capitalized filename**           |
| 47  | `isis`             | Isis             | `isis.glb`                  | direct                                     |
| 47  | `deviace`          | Deviace          | `deviace.glb`               | direct                                     |
| 47  | `iron_fist`        | Iron Fist        | `iron_fist.glb`             | direct                                     |
| 47  | `antique_firelock` | Firelock Soldier | `firelock_soldier.glb`      | alias — template `antique_firelock`, file `firelock_soldier` |
| 47  | `plasma_g`         | Plasma (green)   | `plasma.glb`                | shared + tint                              |
| 48  | `strouf`           | Strouf           | `strouf.glb`                | direct                                     |
| 48  | `gargoyle`         | Gargoyle         | `Gargoyle.glb`              | alias — **capitalized filename**           |
| 48  | `li_me_mang_ryang` | Jing Guai        | `jing_guai.glb`             | alias — template `li_me_mang_ryang`, file `jing_guai` |
| 49  | `nightmare`        | Nightmare        | `nightmare.glb`             | direct                                     |
| 49  | `orc_archer`       | Orc Archer       | `orc_archer.glb`            | direct                                     |
| 49  | `parasite`         | Parasite         | `parasite.glb`              | direct                                     |
| 49  | `chung_e`          | Green Maiden     | `green_maiden.glb`          | alias — template `chung_e`, file `green_maiden` |
| 49  | `plasma_p`         | Plasma (purple)  | `plasma.glb`                | shared + tint                              |
| 50  | `baphomet_`        | Baphomet Jr.     | `baphomet_jr.glb`           | alias — template `baphomet_`, file `baphomet_jr` |
| 50  | `dryad`            | Dryad            | `dryad.glb`                 | direct                                     |
| 50  | `kraben`           | Kraben           | `kraben.glb`                | direct                                     |
| 50  | `obsidian`         | Obsidian         | `obsidian.glb`              | direct                                     |
| 50  | `knocker`          | Knocker          | `knocker.glb`               | direct                                     |

---

## 2. Summary — Lv 1-50 Coverage

- **Total Lv 1-50 templates**: ~190 (counting meta_ / provoke_ / _ variants)
- **Has a matching GLB** (direct, alias, or shared): **~185**
- **Missing GLB**: **2** — `boiled_rice` (Lv 15), `muka` (Lv 17)
- **Shared / tint-derivable** (no unique model needed): **~25** meta / provoke / variant entries + the 4 plasma colors + 2 petite + 2 novus

### GLBs that cover multiple enemies via sharing or tint
- `fabre.glb` → `fabre`, `fabre_`, `meta_fabre`
- `pupa.glb` → `pupa`, `meta_pupa`
- `pecopeco_egg.glb` → `pecopeco_egg`, `meta_pecopeco_egg`
- `picky.glb` → `picky`, `picky_`, `meta_picky`, `meta_picky_`
- `ant_egg.glb` → `ant_egg`, `meta_ant_egg`
- `chonchon.glb` → `chonchon`, `chonchon_`
- `creamy.glb` → `creamy`, `meta_creamy`
- `andre.glb` → `andre`, `meta_andre`
- `piere.glb` → `piere`, `meta_piere`
- `deniro.glb` → `deniro`, `meta_deniro`
- `yoyo.glb` → `yoyo`, `provoke_yoyo`
- `savage.glb` → `savage`, `m_savage`
- `desert_wolf.glb` → `desert_wolf`, `m_desert_wolf`
- `whisper.glb` → `whisper`, `whisper_`, `whisper_boss` (Giant Whisper = scaled tint)
- `goblin.glb` → `goblin_1`, `goblin_2`, `goblin_3`, `goblin_4`, `goblin_5` (5 variants)
- `kobold.glb` → `kobold_1`, `kobold_2`, `kobold_3` (3 variants)
- `hill_wind.glb` → `hill_wind`, `hill_wind_1`
- `plasma.glb` → `plasma_r`, `plasma_b`, `plasma_g`, `plasma_p` (all 4 via tint)
- `soldier_skeleton.glb` → `soldier_skeleton`, `archer_skeleton` (variant-pose)
- `skeleton_t_pose.glb` → `skeleton` (direct)

### GLBs available but tint-candidates (optional atlas savings)
These have their own GLB but could instead be produced as a tint of a base (your 04-15 `spriteTint` system):
- `eclipse.glb` — could be tint of `lunatic` (darker Lunatic recolor)
- `thara_frog.glb` — tint of `roda_frog`
- `steel_chonchon.glb` — tint of `chonchon` (metallic)
- `poison_spore.glb` — tint of `spore` (already done as direct; keep or swap)
- `elder_wilow.glb` — darker tint of `willow`
- `vagabond_wolf.glb` — tint of `wolf`
- `marina.glb` — tint of `marin`
- `magmaring.glb` — tint of `poring` (red/magma)
- `poison_toad.glb` — tint of `toad`
- `jakk_xmas.glb` — tint of `jakk` (santa hat or green)
- `orc_xmas.glb` / `gobline_xmas` / `cookie_xmas` — tint of `orc_warrior` / `goblin` / `cookie`

---

## 3. GLBs in Folder That Do NOT Match Any Lv 1-50 Enemy

These are spillover into Lv 51+ territory (some already seen in the master list for those levels):
- `Anacondaq.glb` → ✅ Lv 23 (already matched)
- None in the folder go completely unused for Lv 1-50 + variants. Spot-check: every GLB in the folder maps to at least one template in Lv 1-50 or is a render-experiment variant (see next).

### Render-experiment variants / duplicates (ignore for new rendering work)
- `rocker-fixed.fbx`, `rocker-fixed.glb`, `rocker_rigged.fbx`, `rocker_rigged.glb` — variants of `rocker.glb`; canonical file is `rocker.glb`
- `skeleton_t_pose.fbx` — FBX twin of `skeleton_t_pose.glb`; use the GLB for rendering
- `zombie.fbx` — FBX twin of `zombie.glb`; use the GLB

### Animation-asset subfolders (not models, just per-enemy animation `.blend` outputs)
- `condor_animations/`, `desert_wolf_b_animations/`, `hornet_animations/`, `rocker_animations/`, `savage_babe_animations/` — existing animated-scene outputs for enemies already rendered. Reference if re-rendering.

### `originals/` subfolder
- Likely the pre-post-process GLBs. Check contents if working on a retexture pass.

---

## 4. T-Pose Biped List

**"T-pose biped"** = a standing humanoid (or humanoid-insectoid) model delivered by Tripo3D in A/T-pose, needing **rigging** before animation. Split into two rigging pipelines:

### 4A. Humanoid T-pose bipeds → use **Mixamo auto-rig** (or UniRig)
These are the classic "person-shaped" RO monsters — 2 arms, 2 legs, head, torso. They accept Mixamo's humanoid animation library directly (matches the `skeleton_t_pose` / `zombie` workflow already proven in-pipeline).

| GLB File                   | Template Key(s)                    | Notes                                              |
| -------------------------- | ---------------------------------- | -------------------------------------------------- |
| `skeleton_t_pose.glb`      | `skeleton`                         | ✅ already rigged + atlas done                     |
| `zombie.glb`               | `zombie`                           | ✅ already rigged + atlas done                     |
| `orc_warrior.glb`          | `ork_warrior`                      | Orc w/ axe, Mixamo humanoid                        |
| `orc_zombie.glb`           | `orc_zombie`                       | Shambling orc undead                               |
| `smoking_orc.glb`          | `smoking_orc`                      | Orc variant, smoking pose                          |
| `orc_xmas.glb`             | `orc_xmas`                         | Santa orc — or tint of `orc_warrior`               |
| `orc_skeleton.glb`         | `orc_skeleton`                     | Undead orc                                         |
| `orc_lady.glb`             | `orc_lady`                         | Female orc (witch pose?)                           |
| `orc baby.glb`             | `orc_baby`                         | **Filename has a space** — rename to `orc_baby.glb` for pipeline hygiene |
| `orc_archer.glb`           | `orc_archer`                       | Ranged orc                                         |
| `soldier_skeleton.glb`     | `soldier_skeleton`, `archer_skeleton` | Skeleton knight — same rig can cover archer variant via pose |
| `skel_worker.glb`          | `skel_worker`                      | Skeleton carrying pickaxe                          |
| `pirate_skel.glb`          | `pirate_skel`                      | Pirate skeleton                                    |
| `goblin.glb`               | `goblin_1`-`goblin_5`              | One rig → 5 templates via `spriteTint`             |
| `goblin_archer.glb`        | `goblin_archer`                    | Ranged goblin                                      |
| `goblin_xmas.glb`          | `gobline_xmas`                     | Santa goblin — or tint of `goblin`                 |
| `steam_goblin.glb`         | `steam_goblin`                     | Goblin on steam mech — biped humanoid hybrid       |
| `kobold.glb`               | `kobold_1`-`kobold_3`              | One rig → 3 templates via tint                     |
| `kobold_archer.glb`        | `kobold_archer`                    | Ranged kobold                                      |
| `zenorc.glb`               | `zenorc`                           | Zen orc (dark undead)                              |
| `munak.glb`                | `munak`                            | Chinese ghost, female hopping pose (but biped)     |
| `sohee.glb`                | `sohee`                            | Korean ghost, female biped                         |
| `miyabi_doll.glb`          | `miyabi_ningyo`                    | Japanese doll, biped                               |
| `karakasa.glb`             | `karakasa`                         | Umbrella yokai — **technically one-legged**; biped-ish rig works |
| `dokebi.glb`               | `dokebi`                           | Korean goblin humanoid                             |
| `bogun.glb`                | `bon_gun`                          | Korean hopping corpse (biped)                      |
| `requiem.glb`              | `requiem`                          | Skeletal mage                                      |
| `mummy.glb`                | `mummy`                            | Bandaged biped                                     |
| `verit.glb`                | `verit`                            | Undead biped (cat-headed skeleton)                 |
| `ghoul.glb`                | `ghoul`                            | Undead biped                                       |
| `lude.glb`                 | `lude`                             | Small undead biped                                 |
| `quve.glb`                 | `quve`                             | Undead biped (corpse)                              |
| `jakk.glb`                 | `jakk`                             | Pumpkin-head biped                                 |
| `jakk_xmas.glb`            | `jakk_xmas`                        | Xmas pumpkin biped — or tint of `jakk`             |
| `myst_case.glb`            | `mystcase`                         | Mimic-box biped (legs under box)                   |
| `marduk.glb`               | `marduk`                           | Bird-headed humanoid                               |
| `marionette.glb`           | `marionette`                       | Stringed puppet biped                              |
| `chepet.glb`               | `chepet`                           | Robotic biped                                      |
| `cookie.glb`               | `cookie`                           | Gingerbread biped                                  |
| `cookie_xmas.glb`          | `cookie_xmas`                      | Christmas gingerbread — or tint of `cookie`        |
| `dumpling_child.glb`       | `rice_cake_boy`                    | Child-shaped biped                                 |
| `whisper.glb`              | `whisper`, `whisper_`, `whisper_boss` | Floating ghost — biped shape, animate via float-bob + arm sway |
| `ghostring.glb`            | `ghostring`                        | Floating ghost biped                               |
| `deviling.glb`             | `deviling`                         | Flying devil with wings — biped torso              |
| `angeling.glb`             | `angeling`                         | Flying angel with wings — biped torso              |
| `deviruchi.glb`            | `deviruchi`                        | Small devil biped                                  |
| `isis.glb`                 | `isis`                             | Naga (snake body + humanoid torso) — upper-body only as biped, lower via curve |
| `nightmare.glb`            | `nightmare`                        | Nightmare horse-rider or pure humanoid — depends on model |
| `bathory.glb`              | `bathory`                          | Witch biped OR bat form (template says `bat` preset — verify which the GLB is) |
| `iron_fist.glb`            | `iron_fist`                        | Humanoid with fist weapon                          |
| `firelock_soldier.glb`     | `antique_firelock`                 | Skeleton soldier w/ musket                         |
| `pitman.glb`               | `pitman`                           | Mining biped                                       |
| `wootan_fighter.glb`       | `wootan_fighter`                   | Tribal humanoid                                    |
| `wootan_shooter.glb`       | `wootan_shooter`                   | Tribal humanoid w/ ranged                          |
| `green_maiden.glb`         | `chung_e`                          | Human female (qipao)                               |
| `baphomet_jr.glb`          | `baphomet_`                        | Small demon biped                                  |
| `greatest_general.glb`     | `greatest_general`                 | Chinese general biped                              |

**Count: ~52 humanoid T-pose bipeds in Lv 1-50 (of which 2 already rigged — skeleton, zombie).**

### 4B. Insectoid T-pose bipeds → use **shape-key `biped_insect` preset** (NOT Mixamo)
These stand upright on 2 legs but are insectoid / non-mammal. The `biped_insect` preset in `render_monster.py` animates them via shape keys (no rigging). Rocker is the proven-out example.

| GLB File                | Template Key(s)                         | Notes                            |
| ----------------------- | --------------------------------------- | -------------------------------- |
| `rocker.glb`            | `rocker`                                | ✅ already rendered + atlas done |
| `thief_bug.glb`         | `thief_bug`                             | ✅ already rendered + atlas done |
| `thief_bug_f.glb`       | `thief_bug_`                            | ✅ already rendered + atlas done |
| `thief_bug_male.glb`    | `thief_bug__`                           | Pending render                   |
| `metaller.glb`          | `metaller`                              | Biped insect                     |
| `andre.glb`             | `andre`, `meta_andre`                   | Red ant (biped)                  |
| `piere.glb`             | `piere`, `meta_piere`                   | Green ant (biped)                |
| `deniro.glb`            | `deniro`, `meta_deniro`                 | Black ant (biped)                |
| `vitata.glb`            | `vitata`                                | Worker ant (biped)               |
| `zerom.glb`             | `zerom`                                 | Sandworm-humanoid (biped)        |
| `horn.glb`              | `horn`                                  | Horned insect (biped)            |
| `vocal.glb`             | `vocal`                                 | Cricket-like biped (similar to rocker) |
| `scorpion.glb`          | `scorpion`                              | Upright scorpion                 |
| `argos.glb`             | `argos`                                 | Spider-like biped                |
| `mantis.glb`            | `mantis`                                | Praying mantis (biped)           |
| `megalodon.glb`         | `megalodon`                             | Undead (verify shape)            |
| `tri_joint.glb`         | `tri_joint`                             | Multi-jointed biped insect       |
| `beetle_king.glb`       | `kind_of_beetle`                        | Biped beetle                     |
| `noxious.glb`           | `noxious`                               | Insect biped                     |
| `brilight.glb`          | `brilight`                              | Firefly biped                    |
| `argiope.glb`           | `argiope`                               | Large spider biped               |
| `porcellio.glb`         | `porcellio`                             | Woodlouse biped                  |
| `tri_joint.glb`         | `tri_joint`                             | Biped insect (listed above)      |

**Count: ~22 insectoid T-pose bipeds in Lv 1-50 (3 already done).**

### 4C. NOT T-pose bipeds — other presets (for reference)
These are delivered in non-T-pose and use shape-key presets other than `biped_insect`:
- **Quadrupeds**: wolf, desert_wolf, desert_wolf_b, vagabond_wolf, savage, savage_babe, raggler, caramel, sasquatch, bigfoot, coco, tarou, smokie, martin, yoyo, matyr, frilldora, choco, explosion, baby_leopard, hill_wind, side_winder, galion, alligator, leaf_cat, zipper_bear, wild_rose, Nereid, pest
- **Blobs**: poring, drops, poporing, marin, marina, plankton, stapo, eggyra, magmaring
- **Caterpillars**: fabre, hode, anacondaq, boa (snake), worm_tail, sage_worm, stem_worm
- **Frogs / fish**: roda_frog, toad, poison_toad, thara_frog, kukre, vadon, crab, marse, obeaune, phen, marc, kapha, deviace, strouf, swordfish, aster
- **Trees / plants**: willow, elder_wilow, spore, poison_spore, hermit_plant, rafflesia, dryad, drosera, stone_shooter, parasite, punk, mime_monkey, mastering, golem, antonio, marine_sphere, flora, hydra, mandragora
- **Eggs**: ambernite, pupa, pecopeco_egg, picky, thief_bug_egg, ant_egg, dragon_egg, cornutus, shellfish
- **Birds**: condor, pecopeco
- **Flying insects**: chonchon, creamy, hornet, dragon_fly, stainer, steel_chonchon, dustiness, anopheles, hunter_fly
- **Bats**: farmiliar, drainliar, (bathory — verify)
- **Rabbits**: lunatic, eclipse

---

## 5. Filename Normalization Notes

When rendering, the pipeline reads the filename on disk. The `GLB_ALIAS` map in `_prompts/build_enemy_list.js` already handles some of these; add the rest if the matcher should become self-healing.

### Typos / spelling differences (template-key → on-disk filename)
| Template Key       | On-Disk Filename         |
| ------------------ | ------------------------ |
| `drainliar`        | `drainiliar.glb`         |
| `eggyra`           | `eggrya.glb`             |
| `thief_bug_`       | `thief_bug_f.glb`        |
| `thief_bug__`      | `thief_bug_male.glb`     |
| `worm_tail`        | `wormtail.glb`           |
| `wilow`            | `willow.glb`             |
| `snake`            | `boa.glb`                |
| `ork_warrior`      | `orc_warrior.glb`        |
| `poring_`          | `santa_poring.glb`       |
| `skeleton`         | `skeleton_t_pose.glb`    |
| `bon_gun`          | `bogun.glb`              |
| `blazzer`          | `blazer.glb`             |
| `mystcase`         | `myst_case.glb`          |
| `sword_fish`       | `swordfish.glb`          |
| `sand_man`         | `sandman.glb`            |
| `sageworm`         | `sage_worm.glb`          |
| `miyabi_ningyo`    | `miyabi_doll.glb`        |
| `gobline_xmas`     | `goblin_xmas.glb`        |
| `mole`             | `holden.glb`             |
| `kind_of_beetle`   | `beetle_king.glb`        |
| `rice_cake_boy`    | `dumpling_child.glb`     |
| `antique_firelock` | `firelock_soldier.glb`   |
| `li_me_mang_ryang` | `jing_guai.glb`          |
| `baphomet_`        | `baphomet_jr.glb`        |
| `wild_ginseng`     | `hermit_plant.glb`       |
| `chung_e`          | `green_maiden.glb`       |
| `neraid`           | `Nereid.glb`             |

### Capitalized filenames (need case handling on case-insensitive FS)
`Anacondaq.glb`, `Drosera.glb`, `Gargoyle.glb`, `Giearth.glb`, `Nereid.glb`, `Porcellio.glb`

### Space in filename (rename recommended)
- `orc baby.glb` → rename to `orc_baby.glb` to avoid shell-quoting friction

### Color-variant GLBs (for tint-driven template mapping)
- `novus_blue.glb`, `novus_green.glb` — map both to `novus` / `novus_` templates (tint differentiates)
- `petite_blue.glb`, `petite_green.glb` — map both to `petit` / `petit_` templates
- `plasma.glb` — one file, 4 tint variants (`plasma_r`/`_b`/`_g`/`_p`)

---

## 6. Quick-Pick Rendering Priority (Lv 1-50)

If picking ONE humanoid T-pose biped to validate the Mixamo pipeline beyond skeleton/zombie, best candidates (low Lv, common in Prontera-area spawns, simple weapon loadout):
1. **`goblin.glb`** (Lv 22-25, covers 5 templates via tint) — highest ROI, small humanoid
2. **`orc_warrior.glb`** (Lv 24, bread-and-butter enemy) — armor + 2H axe, good test for Mixamo animation retargeting with weapon
3. **`soldier_skeleton.glb`** (Lv 29, covers `archer_skeleton` via pose) — similar to existing skeleton so pipeline proof is fast
4. **`kobold.glb`** (Lv 31-36, covers 3 templates via tint) — small humanoid w/ weapon

If picking ONE insectoid biped to extend the rocker workflow to new enemies:
1. **`andre.glb`** (Lv 17, covers `meta_andre` too) — ant base; renders could later tint-cover `deniro` / `piere` / `vitata` if models match closely enough
2. **`vocal.glb`** (Lv 18) — cricket, closest to the proven-out `rocker.glb`

---

## 7. How To Regenerate This List

```bash
# From repo root
node _prompts/build_enemy_list.js     # Dumps .enemy_dump.json
node _prompts/build_enemy_markdown.js # Generates the Lv 1-99+ table sections
```

The scripts read `server/src/ro_monster_templates.js`, `2D animations/3d_models/enemies/*.glb`, and `2D animations/atlas_configs/*_v2.json`. Any changes to templates, GLB set, or atlas configs can regenerate the mapping without hand-editing.

If a filename alias above is still missing from `GLB_ALIAS` in `build_enemy_list.js`, add it there so the auto-status column in `enemy_sprite_session_resume.md` stays accurate on the next run.
