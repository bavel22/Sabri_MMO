# Standardize and Re-Render

Use when the render pipeline has changed and existing class sprites need to be re-rendered with the new standard.

## Prompt Template

```
can you update the documentation and skills for the new {CHANGE_DESCRIPTION} rules. make sure the {TECHNIQUE} is always used, using the .blend with the fbx t poses. obsolete the old {OLD_TECHNIQUE}. make sure the character renders are always done in the same style and way that you just did the {REFERENCE_CLASS}. also make sure the {LAYER_SYSTEMS} are all fully documented with the occlusion and other aspects fully documented so it can be repeatable. all renders should be done like that from now on. can you redo the {CLASSES_TO_RERENDER} character renders with these new rules.
```

## Example (from 2026-03-27)

```
can you update the documentation and skills for the new lighting rules. make sure the shared armature technique is always used, using the .blend with the fbx t poses. obsolete the old render techniques. make sure the character renders are always done in the same style and way that you just did the merchant_f. also make sure the weapon, headgear, and hair layering systems are all fully documented with the occlusion and other aspects fully documented so it can be repeatable. all renders should be done like that from now on. can you redo the swordsman_f and knight_f character renders with these new rules.
```

## Skills to Load
`/sabrimmo-sprites`, `/sabrimmo-3d-to-2d`, `/sabrimmo-art`, `/docs`

## What It Does
1. Updates documentation and skills with new pipeline rules
2. Obsoletes old render techniques
3. Re-renders specified classes with the new standard
4. Documents all layer systems for repeatability
