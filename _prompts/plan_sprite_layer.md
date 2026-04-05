# Plan New Sprite Layer

Use when adding a new layer to the sprite layering system (headgear, hair, shield, garment, etc.).

## Prompt Template

```
i want to start planning the implementation of another layer of my sprite layering system. specifically the {LAYER_NAME} layer. can you plan this entire implementation and how it will interact with all other layers. this should behave exactly like ragnarok online classic. make sure to plan the full pipeline and implementation for this. will it behave like the weapon layering system where we will build a 3d model, pair it to the character's {BONE_NAME} somehow and build an atlas to layer just like the dagger or bow? this is only for the player characters, not for the enemies
```

## Skills to Load
`/sabrimmo-sprites`, `/sabrimmo-3d-to-2d`, `/sabrimmo-art`

## What It Produces
- Full implementation plan doc in `docsNew/05_Development/`
- Pipeline: 3D model -> bone parent -> .blend template -> dual-pass holdout render -> v2 atlas pack -> UE5 import
- Interaction plan with existing layers (visibility, occlusion, hiding rules)
- Server broadcast requirements (which emits need the new field)
- C++ changes needed (if any — headgear needed zero, hair needed tint support)

## Examples
- HeadgearTop: parented to `mixamorig:Head`, holdout occlusion, always_front depth
- Hair: same as headgear + TintColor material multiply for runtime recoloring
