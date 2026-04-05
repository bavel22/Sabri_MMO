"""
Sabri_MMO Sprite Prompt Generator
Generates prompt CSV files for ComfyUI batch processing.

Usage:
  python generate_prompts.py --class knight --gender male
  python generate_prompts.py --all
  python generate_prompts.py --monsters
"""

import csv
import os
import argparse

# RO Classes with their visual descriptions
CLASSES = {
    "novice":     {"weapon": "knife", "armor": "simple tunic", "features": "beginner adventurer"},
    "swordsman":  {"weapon": "sword and buckler", "armor": "light plate armor", "features": "warrior"},
    "mage":       {"weapon": "wooden rod", "armor": "robe and mage hat", "features": "spellcaster"},
    "archer":     {"weapon": "short bow and quiver", "armor": "leather vest", "features": "ranger"},
    "acolyte":    {"weapon": "mace", "armor": "white robes and biretta", "features": "healer cleric"},
    "thief":      {"weapon": "twin daggers", "armor": "dark leather outfit", "features": "rogue assassin"},
    "merchant":   {"weapon": "axe", "armor": "apron and merchant clothes", "features": "trader"},
    "knight":     {"weapon": "longsword", "armor": "full plate armor and helm", "features": "heavy warrior"},
    "wizard":     {"weapon": "staff", "armor": "wizard robe and pointed hat", "features": "archmage"},
    "hunter":     {"weapon": "composite bow", "armor": "green ranger outfit", "features": "marksman with falcon"},
    "priest":     {"weapon": "rod", "armor": "white vestments and mitre", "features": "holy cleric"},
    "assassin":   {"weapon": "katar", "armor": "dark tight-fitting outfit", "features": "shadow killer"},
    "blacksmith": {"weapon": "hammer", "armor": "heavy apron and gloves", "features": "craftsman"},
    "crusader":   {"weapon": "sword and large shield", "armor": "holy knight armor with cross", "features": "paladin"},
    "sage":       {"weapon": "book", "armor": "scholar robes and cape", "features": "magic researcher"},
    "bard":       {"weapon": "guitar instrument", "armor": "performer outfit with hat", "features": "musician"},
    "dancer":     {"weapon": "whip", "armor": "dancing outfit", "features": "performer"},
    "monk":       {"weapon": "knuckle weapon", "armor": "martial arts robes", "features": "martial artist"},
    "rogue":      {"weapon": "dagger and bow", "armor": "hooded cloak outfit", "features": "trickster"},
    "alchemist":  {"weapon": "axe", "armor": "lab coat and bottles", "features": "potion maker"},
}

# Only generate 5 unique directions (mirror the other 3)
DIRECTIONS = [
    ("south",     "facing camera directly, front view"),
    ("southwest", "three-quarter view facing bottom-left"),
    ("west",      "profile view facing left, side view"),
    ("northwest", "three-quarter back view facing upper-left"),
    ("north",     "facing away from camera, back view, rear"),
]

GENDERS = ["male", "female"]

MONSTERS = {
    "poring":       {"desc": "pink slime blob with cute face", "size": "tiny"},
    "drops":        {"desc": "orange slime blob with cute face", "size": "tiny"},
    "lunatic":      {"desc": "white fluffy rabbit with red eyes", "size": "small"},
    "fabre":        {"desc": "green caterpillar insect", "size": "small"},
    "rocker":       {"desc": "brown grasshopper playing guitar", "size": "small"},
    "pupa":         {"desc": "green chrysalis cocoon", "size": "small"},
    "condor":       {"desc": "brown vulture bird", "size": "normal"},
    "thief_bug":    {"desc": "purple beetle insect", "size": "small"},
    "goblin":       {"desc": "small green humanoid with club", "size": "normal"},
    "skeleton":     {"desc": "undead skeleton warrior with sword", "size": "normal"},
    "orc_warrior":  {"desc": "large green orc with axe", "size": "large"},
    "wolf":         {"desc": "grey wolf quadruped", "size": "normal"},
    "zombie":       {"desc": "shambling undead humanoid", "size": "normal"},
    "poison_spore": {"desc": "purple poisonous mushroom", "size": "small"},
    "horn":         {"desc": "brown rhinoceros beetle", "size": "normal"},
}

PROMPT_TEMPLATE = (
    "ROSPRITE, pixel art sprite, chibi character, ragnarok online style, "
    "{class_name} class, {gender}, full body, {direction_desc}, "
    "{equipment}, {features}, "
    "white background, detailed pixel shading, high contrast, "
    "crisp edges, no anti-aliasing, 32-bit color palette, isometric perspective"
)

NEGATIVE_PROMPT = (
    "blurry, smooth, realistic proportions, 3D render, photograph, "
    "watermark, text, bad anatomy, extra limbs, deformed, low quality, "
    "jpeg artifacts, anti-aliasing, gradient, multiple characters, "
    "cropped, out of frame, worst quality, signature"
)

MONSTER_TEMPLATE = (
    "ROSPRITE, pixel art sprite, ragnarok online monster style, "
    "{name}, {desc}, full body, {direction_desc}, "
    "cute but dangerous, fantasy creature, "
    "white background, detailed pixel shading, crisp edges"
)


def generate_character_prompts(class_name, gender):
    """Generate prompts for one character class + gender."""
    info = CLASSES[class_name]
    prompts = []
    for dir_name, dir_desc in DIRECTIONS:
        prompt = PROMPT_TEMPLATE.format(
            class_name=class_name,
            gender=gender,
            direction_desc=dir_desc,
            equipment=f"{info['weapon']}, {info['armor']}",
            features=info["features"],
        )
        prompts.append({
            "filename": f"{class_name}_{gender}_{dir_name}",
            "positive": prompt,
            "negative": NEGATIVE_PROMPT,
            "class": class_name,
            "gender": gender,
            "direction": dir_name,
        })
    return prompts


def generate_monster_prompts(monster_name):
    """Generate prompts for one monster (4 directions only)."""
    info = MONSTERS[monster_name]
    monster_dirs = DIRECTIONS[:4]  # Monsters only need 4 directions
    prompts = []
    for dir_name, dir_desc in monster_dirs:
        prompt = MONSTER_TEMPLATE.format(
            name=monster_name.replace("_", " "),
            desc=info["desc"],
            direction_desc=dir_desc,
        )
        prompts.append({
            "filename": f"monster_{monster_name}_{dir_name}",
            "positive": prompt,
            "negative": NEGATIVE_PROMPT,
            "class": "monster",
            "gender": "",
            "direction": dir_name,
        })
    return prompts


def write_csv(prompts, output_path):
    """Write prompts to CSV for ComfyUI batch loading."""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["filename", "positive", "negative", "class", "gender", "direction"])
        writer.writeheader()
        writer.writerows(prompts)
    print(f"  Written {len(prompts)} prompts to {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Sabri_MMO Sprite Prompt Generator")
    parser.add_argument("--class", dest="char_class", help="Generate for one class (e.g., knight)")
    parser.add_argument("--gender", default="both", help="male, female, or both")
    parser.add_argument("--all", action="store_true", help="Generate for ALL classes")
    parser.add_argument("--monsters", action="store_true", help="Generate monster prompts")
    parser.add_argument("--output", default="prompts", help="Output directory")
    args = parser.parse_args()

    output_dir = os.path.join(os.path.dirname(__file__), args.output)
    os.makedirs(output_dir, exist_ok=True)

    if args.monsters:
        all_prompts = []
        for monster in MONSTERS:
            all_prompts.extend(generate_monster_prompts(monster))
        write_csv(all_prompts, os.path.join(output_dir, "monsters_all.csv"))
        print(f"\nTotal monster prompts: {len(all_prompts)}")
        return

    genders = GENDERS if args.gender == "both" else [args.gender]
    classes = CLASSES.keys() if args.all else [args.char_class] if args.char_class else []

    if not classes:
        parser.print_help()
        return

    total = 0
    for cls in classes:
        if cls not in CLASSES:
            print(f"Unknown class: {cls}")
            continue
        for gender in genders:
            prompts = generate_character_prompts(cls, gender)
            write_csv(prompts, os.path.join(output_dir, f"{cls}_{gender}.csv"))
            total += len(prompts)

    print(f"\nTotal character prompts: {total}")
    if args.all:
        print(f"Classes: {len(list(classes))} x {len(genders)} genders x {len(DIRECTIONS)} directions")


if __name__ == "__main__":
    main()
