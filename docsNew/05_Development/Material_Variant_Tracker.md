# M_Landscape_RO — Material Variant Tracker

| Version | Name                         | Visual Description                                                                                                                                  | Technical Changes                                                                                           | Slope | Dirt | Normals | Roughness | Date       |
| ------- | ---------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------- | ----- | ---- | ------- | --------- | ---------- |
| 00      | M_Landscape_RO               | Obvious tile grid everywhere, ghostly overlapping textures, very small repeating squares visible across entire ground                               | Original — small tile size (500), no UV distortion, no normals                                              | 2.5   | 0.25 | 0.0     | 0.95      | 2026-03-30 |
| 01      | M_Landscape_RO (rebuilt)     | Larger tiles visible, less grid but still clear rectangular seam lines, both grass textures blending uniformly instead of in patches                | Tile size 4000, noise scales 10x larger, still no UV distortion                                             | 2.5   | 0.25 | 0.0     | 0.95      | 2026-03-30 |
| 02      | M_Landscape_RO (rebuilt)     | Seam lines now wavy instead of straight grid, much less visible tiling, splotchy green patches blending nicely, first version that looked good      | Added UV noise distortion, irrational tile ratios per layer, seamless-fixed textures                        | 2.5   | 0.25 | 0.0     | 0.95      | 2026-03-30 |
| 03      | M_Landscape_RO (rebuilt)     | Same as v02 but with subtle surface detail from normals and darker crevices from AO — hard to notice the depth on flat ground                       | Added normal maps (4 layers), AO, normal strength 0.3, roughness 0.95                                       | 2.5   | 0.25 | 0.3     | 0.95      | 2026-03-30 |
| 04      | M_Landscape_RO_04            | Greener flat areas (less brown dirt patches), rock shows on even gentle slopes, stronger normal detail but still hard to see without actual terrain | Less dirt (0.08), aggressive slope rock (power 0.5), normals 2.0, roughness 0.75                            | 0.5   | 0.08 | 2.0     | 0.75      | 2026-03-30 |
| 05      | M_Landscape_RO_05            | Same as v04 with comment node fix                                                                                                                   | Same as v04 with comment fix                                                                                | 0.5   | 0.08 | 2.0     | 0.75      | 2026-03-30 |
| 06      | M_Landscape_RO_06            | Almost no visible dirt, very green flat areas, hard to tell difference from v05                                                                     | Dirt patches less frequent (noise 0.003) and very faint (amount 0.03)                                       | 0.5   | 0.03 | 2.0     | 0.75      | 2026-03-30 |
| 07      | M_Landscape_RO_07            | Zero dirt on flat ground, purely grass everywhere on flat areas                                                                                     | No noise dirt on flat areas (amount 0.0), dirt only on slopes                                               | 0.5   | 0.00 | 2.0     | 0.75      | 2026-03-30 |
| 08      | M_Landscape_RO_08            | Original balance restored, rock uses XZ projection for vertical cliffs                                                                              | Original balance (dirt 0.25, roughness 0.95). Rock XZ projection for cliff faces                            | 2.0   | 0.25 | 0.5     | 0.95      | 2026-03-30 |
| 09      | **M_Landscape_RO_09 (BEST)** | **Hard 45-deg rock cutoff — very distinct cliff line. Grass below 40deg, sharp rock above 53deg. Cliff = unclimbable terrain visual cue.**          | Hard SmoothStep [0.60-0.75] cutoff, ~13deg transition band, XZ rock projection, original grass/dirt balance | 45deg | 0.25 | 0.5     | 0.95      | 2026-03-30 |

## Slope Angle Reference (v09 — Current Best)

The rock/cliff texture is a **gameplay indicator** — if the ground looks like rock, the player cannot walk there.

| Angle         | DotProduct | Material Result            | Gameplay Meaning                    |
| ------------- | ---------- | -------------------------- | ----------------------------------- |
| 0-40 degrees  | 0.77-1.0   | 100% grass/dirt            | **Walkable** terrain                |
| 40-53 degrees | 0.60-0.77  | Grass-to-rock transition   | Edge of walkable area               |
| 53-90 degrees | 0.0-0.60   | 100% rock/cliff            | **Unclimbable** — visual barrier    |

## Design Rule

Rock/cliff texture = impassable terrain boundary. This aligns with RO Classic where cliff faces are visual walls. The hard 45-degree cutoff makes walkability immediately obvious to players — no ambiguity. When building zones, sculpt slopes above 45 degrees wherever the player should not be able to go.

## 30 Variants (Material Instances of v09) — 2026-03-30

| Name | Description | Ground Textures | Dirt | Cliff | DirtAmt |
|------|-------------|-----------------|------|-------|---------|
| V01_Green_Classic | Classic green — LushGreen 03+02, earth dirt, gray stone cliff | lush3+lush2 | earth | cliff2 | 0.25 |
| V02_Green_MossForest | Dark forest floor — LushGreen 03 + dark moss, earth, gray cliff | lush3+moss | earth | cliff2 | 0.2 |
| V03_Green_BrightField | Bright meadow — LushGreen 02+01, earth, lighter cliff | lush2+lush1 | earth | cliff1 | 0.25 |
| V04_Green_DenseRich | Dense rich green — DenseGrass + LushGreen 03, rocky dirt, gray cliff | dense+lush3 | rocky | cliff2 | 0.15 |
| V05_Green_WildMeadow | Wild meadow — DirtGrass + LushGreen 02, earth dirt, original rock | dirtgrass+lush2 | earth | rock1 | 0.3 |
| V06_Green_SoftBlend | Soft subtle — Mixed_B + LushGreen 03, basic dirt, gray cliff | mixed_b+lush3 | dirt1 | cliff2 | 0.2 |
| V07_Green_WarmGrass | Warm grass — GrassWarmV1 + LushGreen 03, earth, gray cliff | grassv1+lush3 | earth | cliff2 | 0.25 |
| V08_Green_NoDirt | Pure green, no dirt — LushGreen 03+02, zero dirt, gray cliff | lush3+lush2 | earth | cliff2 | 0.0 |
| V09_Green_HeavyDirt | Heavy worn paths — LushGreen 03+02, lots of earth showing, gray cliff | lush3+lush2 | earth | cliff2 | 0.5 |
| V10_Green_DarkForest | Dark moody forest — MossFloor + DirtGrass, earth, gray cliff | moss+dirtgrass | earth | cliff2 | 0.3 |
| V11_Sand_Classic | Classic desert — warm sand + dune waves, dry earth, layered sandstone cliff | sw2+sw1 | de1 | ss1 | 0.25 |
| V12_Sand_DuneWaves | Flowing dunes — warm sand variants, dry earth, canyon wall cliff | sw1+sw3 | de1 | ss2 | 0.2 |
| V13_Sand_CoolDesert | Cooler desert — warm + cool sand mix, dry earth, sandstone layers | sw2+sc2 | de1 | ss1 | 0.25 |
| V14_Sand_RoughDesert | Rough terrain — warm sand, crater earth, cobble-stone cliff | sw1+sw2 | de2 | ss3 | 0.3 |
| V15_Sand_CleanDunes | Clean sand dunes — no dirt patches, pure sand, layered cliff | sw2+sw1 | de1 | ss1 | 0.0 |
| V16_Sand_PaleDust | Pale dusty — cool sand + warm sand, dry earth, canyon walls | sc2+sw2 | de1 | ss2 | 0.2 |
| V17_Sand_GoldenHot | Hot golden desert — all warm sand tones, dry earth, sandstone | sw3+sw1 | de1 | ss1 | 0.15 |
| V18_Sand_PurpleRock | Sand with purple-gray cliff — sand flats, gray stone slopes | sw2+sw1 | de1 | cliff2 | 0.25 |
| V19_Sand_HeavyErosion | Eroded desert — warm sand, heavy crater earth, canyon cliff | sw1+sw2 | de2 | ss2 | 0.45 |
| V20_Sand_WarmCanyon | Warm canyon floor — dry earth blending into sand, layered cliff | sw2+de1 | sw1 | ss1 | 0.35 |
| V21_Mixed_GrassToSand | Grass with sandy patches — green base, sand variant, earth, gray cliff | lush3+sw2 | earth | cliff2 | 0.25 |
| V22_Mixed_SandToGrass | Sand with grass patches — sand base, green variant, dry earth, sandstone | sw2+lush3 | de1 | ss1 | 0.25 |
| V23_Mixed_GreenSandstone | Green fields with sandstone cliffs — classic green, sandstone slopes | lush3+lush2 | earth | ss1 | 0.25 |
| V24_Mixed_SandGrayRock | Sand flats with gray rock slopes — desert meets mountain | sw1+sw2 | earth | cliff2 | 0.2 |
| V25_Mixed_DarkMoss | Dark mossy forest — moss + lush green, dry earth, dark cliff | moss+lush2 | de1 | cliff1 | 0.35 |
| V26_Mixed_GreenCanyon | Green plateau with canyon walls — lush green, canyon cliff slopes | lush3+lush2 | de1 | ss2 | 0.2 |
| V27_Mixed_AutumnField | Autumn field — warm grass + sand tones, rocky dirt, sandstone cliff | grassv3+sw2 | rocky | ss1 | 0.3 |
| V28_Mixed_LushForest | Lush dense forest — DenseGrass + Moss, earth, gray cliff | dense+moss | earth | cliff2 | 0.15 |
| V29_Mixed_AridHighDesert | High desert — sand + dry earth dominant, lots of dirt showing, sandstone | sw2+sw1 | de1 | ss1 | 0.5 |
| V30_Mixed_Coastal | Coastal zone — sand + green grass, earth transition, gray cliff | sw2+lush3 | earth | cliff2 | 0.3 |
| 11 | M_Landscape_RO_11 | All-parameter parent — every setting exposed as MI parameter for 500+ variants. Hard 45-deg cutoff, XZ rock, UV distortion. | 2.0 | 0.25 | 0.5 | | 12 | M_Landscape_RO_12 | v12 MASTER — 23 parameters. Color tinting (warmth/saturation/brightness/contrast), blending control (grass balance/noise scale), slope noise, all MI-overridable. | 2.0 | 0.25 | 0.5 | | 13 | M_Landscape_RO_13 | v13 MASTER — 25 parameters. Cell bombing anti-tiling, color tinting, blending control, slope noise, all MI-overridable. | 2.0 | 0.25 | 0.5 | | 13 | M_Landscape_RO_13 | v13 MASTER — 25 parameters. Cell bombing anti-tiling, color tinting, blending control, slope noise, all MI-overridable. | 2.0 | 0.25 | 0.5 | | 13 | M_Landscape_RO_13 | v13 MASTER — 25 parameters. Cell bombing anti-tiling, color tinting, blending control, slope noise, all MI-overridable. | 2.0 | 0.25 | 0.5 | | 13 | M_Landscape_RO_13 | v13 MASTER — 25 parameters. Cell bombing anti-tiling, color tinting, blending control, slope noise, all MI-overridable. | 2.0 | 0.25 | 0.5 | | 14 | M_Landscape_RO_14 | v14 MASTER — 23 parameters. Reverted cell bombing (caused visible cell grid). Back to v12 approach: UV distortion + irrational ratios + noise blending. | 2.0 | 0.25 | 0.5 | | 14 | M_Landscape_RO_14 | v14 MASTER — 23 parameters. Reverted cell bombing (caused visible cell grid). Back to v12 approach: UV distortion + irrational ratios + noise blending. | 2.0 | 0.25 | 0.5 | | 17 | M_Landscape_RO_17 | v17 — Paintable grass layers. LandscapeLayerWeight for manual brush control of grass density zones. All v14 features plus paint layers. | 2.0 | 0.25 | 0.5 | 