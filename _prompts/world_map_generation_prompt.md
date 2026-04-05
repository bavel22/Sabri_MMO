# World Map Generation Prompt — Sabri_MMO

## Style Reference
Ragnarok Online Classic world map: hand-painted fantasy cartographic illustration, soft watercolor/gouache style, warm palette, anime-influenced but not overly stylized. Bird's-eye overhead view like a fantasy atlas page.

---

## Prompt (Midjourney v6 / DALL-E 3)

```
Fantasy world map illustration, overhead bird's-eye cartographic view, hand-painted watercolor style inspired by Ragnarok Online Classic. A single large continent surrounded by ocean with varied biome regions clearly visible:

CENTER: Lush green temperate meadows and rolling hills with a grand walled capital city (Prontera). Roads radiate outward from the capital. Gentle farmland, stone bridges, windmills.

WEST: Mystical purple-tinted forests with a magic tower city (Geffen). Glowing ley lines, ancient ruins, mushroom groves. Transitions to dark enchanted woods.

SOUTHEAST: Dense bamboo forests and misty mountain valleys with an Asian-style pagoda village (Payon). Waterfalls, wooden bridges, autumn-colored canopy.

SOUTHWEST: Vast golden desert with sand dunes, pyramids, and a sandstone bazaar city (Morroc). Oasis pools, sphinx monument, heat shimmer.

SOUTH: Rocky coastline with a harbor port city (Alberta). Trading ships, lighthouse, wooden docks. Small islands visible offshore.

NORTH-CENTER: Dramatic mountain range (Mt. Mjolnir) running east-west, snow-capped peaks, pine forests at the base. Mountain passes with bridges.

NORTHEAST: Industrial steampunk region with smokestacks and gear-shaped buildings (Einbroch/Juno). Floating islands, airship docks, metal scaffolding.

NORTHWEST: Frozen tundra and glacial landscape. Ice caves, aurora borealis in the sky, a snow-covered village. Crystalline ice formations.

FAR NORTH: Active volcanic region with lava rivers, obsidian cliffs, and sulfurous vents. Dark red and black rocky terrain. A fortress carved into the mountainside.

OCEAN: Deep blue sea with scattered tropical islands to the east (Japanese shrine island, Chinese palace island, Thai temple island). Sea monsters tentacles barely visible. Sailing ships on trade routes.

SOUTHEAST COAST: Swampy bayou region with mangrove trees, murky water, will-o-wisps. A hidden underground cave entrance.

Style: Warm color palette, soft lighting, no harsh outlines. Terrain transitions are natural gradients (green meadow fading to brown desert, forest fading to snow). Cities are small but recognizable clusters of buildings. Roads shown as thin brown lines. Rivers shown as blue flowing lines. Mountain ranges have subtle shadow. Ocean has gentle wave texture. The overall feel is cozy, inviting, and magical — like opening a storybook atlas.

Aspect ratio 1:1, high detail, 4096x4096 resolution. No text, no labels, no UI elements, no borders. Clean illustration only.
```

---

## Prompt (Stable Diffusion XL / ComfyUI)

```
(fantasy world map:1.4), (bird's eye cartographic illustration:1.3), (hand-painted watercolor:1.2), ragnarok online style, overhead view, single continent surrounded by ocean, varied biomes, (green meadows:1.1) center with walled city, (golden desert:1.1) southwest with pyramids, (snowy mountains:1.1) north, (volcanic region:1.1) far north with lava, (bamboo forest:1.1) southeast with pagodas, (purple magic forest:1.1) west with tower, (industrial steampunk:1.0) northeast, (frozen tundra:1.1) northwest with ice, (tropical islands:1.0) in ocean, harbor port south, mountain range dividing regions, rivers and roads, warm palette, soft lighting, cozy fantasy atlas, storybook illustration, no text, no labels, no UI, clean illustration

Negative: text, words, labels, letters, UI elements, borders, frames, dark, grim, realistic photo, 3D render, low quality, blurry, watermark, signature
```

---

## Region Layout Guide (for positioning zones on the final image)

```
                    FROZEN TUNDRA     VOLCANIC REGION
                    (ice/snow)        (lava/obsidian)
                         \               /
            MAGIC FOREST -- MT. MJOLNIR RANGE -- STEAMPUNK CITY
            (Geffen)     (mountain pass)         (Einbroch/Juno)
                  \           |            /
                   \    PRONTERA (capital)  /
                    \    (green meadows)   /
            BAMBOO    \       |        /    ENCHANTED
            FOREST     \      |       /     WOODS
            (Payon)     SOUTH FIELD
                         (starter area)
                              |
            DESERT ---- DUNGEON ---- PORT CITY
            (Morroc)   (underground)  (Alberta)
                                         |
                              OCEAN (trade routes)
                            /     |      \
                     AMATSU  LOUYANG  AYOTHAYA
                     (JP)    (CN)     (TH)
```

---

## After Generation

1. Import the image into UE5: `Content/SabriMMO/Textures/UI/T_WorldMap_Midgard.png`
2. Settings: Compression=BC7, MipGen=NoMipmaps, TextureGroup=UI, NeverStream=true
3. Update `ro_world_map_data.js` zone bounds to match the actual pixel positions of each region on the generated image
4. The `SWorldMapWidget` will use this as the background texture (replace the current procedural green/blue rendering)

---

## Tiling Considerations

- The image should be a **single complete illustration** (not tileable)
- Resolution: **4096x4096** recommended (scales well at 1080p-4K viewports)
- If using **2048x2048**, ensure regions are still distinguishable when scaled to fit a 1920x1080 screen
- Leave ~5% margin around edges for ocean (no regions touching the image border)
