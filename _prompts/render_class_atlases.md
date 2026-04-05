# Render New Class Atlases

Use when adding a new character class body sprite (male or female variant).

## Prompt Template

```
can you render all the animation atlases for my new {GENDER} class: {CLASS_NAME}. i created the .blend file with the tpose fbx parented to the {BASE_ARMATURE} armature. can you use our documented process to render all the animation atlases for this new class. the .blend file is @"{PATH_TO_BLEND_FILE}". make sure you do this exactly like you did for the female merchant.
```

## Prerequisites
- `.blend` file with the class model's T-pose FBX parented to the base armature (`base_f` or `base_m`)
- The base armature FBX must be imported into the .blend scene
- The class model must have Automatic Weights applied to the shared armature

## Skills to Load
`/sabrimmo-sprites`, `/sabrimmo-3d-to-2d`, `/sabrimmo-art`

## Pipeline
1. `render_blend_all_visible.py` with `--texture-from {EXACT_GLB}` and `--cel-shadow 0.92 --cel-mid 0.98`
2. 17 animations from `standard_template_v2.json`
3. 8 directions per animation
4. `pack_atlas.py` with v2 config
5. Import PNGs into UE5, set sRGB, verify in SpriteCharacterActor

## Batch Version (multiple classes)

```
can you render all the animation atlases for my 2 new female classes: {CLASS1} and {CLASS2}. i created the .blend files with the tpose fbx parented to the base_f armature. can you use our documented process to render all the animations atlases for both new female classes. the .blend files are @"{PATH1}" and @"{PATH2}". make sure you do this exactly like you did for the female merchant.
```

## Key Rules
- `--texture-from` MUST use the exact GLB uploaded to Mixamo
- Lighting: `--cel-shadow 0.92 --cel-mid 0.98` (mandatory)
- All renders use `render_blend_all_visible.py` with `.blend` scenes
- Animation dir: `animations/characters/base_m/` or `base_f/` (shared armature)
