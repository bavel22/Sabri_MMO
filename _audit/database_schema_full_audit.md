# Database Schema Full Audit Report

**Date**: 2026-03-23
**Scope**: `database/init.sql`, 25 migration files, server auto-creation block (`server/src/index.js` lines 31982-32547)

---

## Table of Contents

1. [Complete Schema Documentation](#1-complete-schema-documentation)
2. [Migration Summary](#2-migration-summary)
3. [Server Auto-Creation Columns](#3-server-auto-creation-columns)
4. [Foreign Key Analysis](#4-foreign-key-analysis)
5. [Column Type & Constraint Issues](#5-column-type--constraint-issues)
6. [Index Coverage Analysis](#6-index-coverage-analysis)
7. [Soft-Delete Data Integrity](#7-soft-delete-data-integrity)
8. [Missing Tables / Columns](#8-missing-tables--columns)
9. [Critical Issues Summary](#9-critical-issues-summary)
10. [Recommendations](#10-recommendations)

---

## 1. Complete Schema Documentation

### 1.1 `users`

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `user_id` | SERIAL | NO | auto | **PK** |
| `username` | VARCHAR(50) | NO | - | UNIQUE |
| `email` | VARCHAR(100) | NO | - | UNIQUE |
| `password_hash` | VARCHAR(255) | NO | - | bcrypt hash |
| `created_at` | TIMESTAMP | YES | CURRENT_TIMESTAMP | |
| `last_login` | TIMESTAMP | YES | NULL | |

**Indexes**: `idx_users_username(username)`, `idx_users_email(email)`
**Source**: `init.sql`

### 1.2 `characters`

| Column | Type | Nullable | Default | Source |
|--------|------|----------|---------|--------|
| `character_id` | SERIAL | NO | auto | **PK**, init.sql |
| `user_id` | INTEGER | YES | - | FK -> `users(user_id)` ON DELETE CASCADE, init.sql |
| `name` | VARCHAR(50) | NO | - | UNIQUE, init.sql |
| `class` | VARCHAR(20) | YES | 'warrior' | init.sql |
| `level` | INTEGER | YES | 1 | init.sql |
| `experience` | INTEGER | YES | 0 | Legacy, init.sql |
| `x` | FLOAT | YES | 0 | init.sql |
| `y` | FLOAT | YES | 0 | init.sql |
| `z` | FLOAT | YES | 0 | init.sql |
| `health` | INTEGER | YES | 100 | init.sql |
| `max_health` | INTEGER | YES | 100 | init.sql + auto |
| `mana` | INTEGER | YES | 100 | init.sql |
| `max_mana` | INTEGER | YES | 100 | init.sql + auto |
| `hair_style` | INTEGER | YES | 1 | init.sql + auto + migration |
| `hair_color` | INTEGER | YES | 0 | init.sql + auto + migration |
| `gender` | VARCHAR(10) | YES | 'male' | init.sql + auto + migration |
| `delete_date` | TIMESTAMP | YES | NULL | init.sql + auto + migration |
| `deleted` | BOOLEAN | NO | FALSE | init.sql + migration |
| `created_at` | TIMESTAMP | YES | CURRENT_TIMESTAMP | init.sql |
| `last_played` | TIMESTAMP | YES | NULL | init.sql |
| `str` | INTEGER | YES | 1 | auto |
| `agi` | INTEGER | YES | 1 | auto |
| `vit` | INTEGER | YES | 1 | auto |
| `int_stat` | INTEGER | YES | 1 | auto |
| `dex` | INTEGER | YES | 1 | auto |
| `luk` | INTEGER | YES | 1 | auto |
| `stat_points` | INTEGER | YES | 48 | auto |
| `zuzucoin` | INTEGER | YES | 0 | auto + migration |
| `job_level` | INTEGER | YES | 1 | auto + migration |
| `base_exp` | BIGINT | YES | 0 | auto + migration |
| `job_exp` | BIGINT | YES | 0 | auto + migration |
| `job_class` | VARCHAR(30) | YES | 'novice' | auto + migration |
| `skill_points` | INTEGER | YES | 0 | auto + migration |
| `zone_name` | VARCHAR(50) | YES | 'prontera_south' | auto + migration |
| `save_map` | VARCHAR(50) | YES | 'prontera' | auto + migration |
| `save_x` | FLOAT | YES | 0 | auto + migration |
| `save_y` | FLOAT | YES | 0 | auto + migration |
| `save_z` | FLOAT | YES | 580 | auto + migration |
| `plagiarized_skill_id` | INTEGER | YES | NULL | auto + migration |
| `plagiarized_skill_level` | INTEGER | YES | 0 | auto + migration |
| `has_cart` | BOOLEAN | YES | false | auto + migration |
| `cart_type` | INTEGER | YES | 0 | auto + migration |

**Indexes**: `idx_characters_user_id(user_id)`, `idx_characters_deleted(deleted) WHERE deleted=FALSE` (partial), `idx_characters_job_class(job_class)`, `idx_characters_level(level)`, `idx_characters_delete_date(delete_date) WHERE delete_date IS NOT NULL`, `idx_characters_zone(zone_name)`

### 1.3 `items` (Static item definitions)

| Column | Type | Nullable | Default | Source |
|--------|------|----------|---------|--------|
| `item_id` | SERIAL | NO | auto | **PK**, init.sql |
| `name` | VARCHAR(100) | NO | - | init.sql |
| `aegis_name` | VARCHAR(100) | YES | NULL | init.sql |
| `description` | TEXT | YES | '' | init.sql |
| `full_description` | TEXT | YES | NULL | init.sql |
| `item_type` | VARCHAR(20) | NO | 'etc' | init.sql |
| `equip_slot` | VARCHAR(20) | YES | NULL | init.sql |
| `weight` | INTEGER | YES | 0 | init.sql |
| `price` | INTEGER | YES | 0 | Legacy, init.sql |
| `buy_price` | INTEGER | YES | 0 | init.sql |
| `sell_price` | INTEGER | YES | 0 | init.sql |
| `atk` | INTEGER | YES | 0 | init.sql |
| `def` | INTEGER | YES | 0 | init.sql |
| `matk` | INTEGER | YES | 0 | init.sql |
| `mdef` | INTEGER | YES | 0 | init.sql |
| `str_bonus` | INTEGER | YES | 0 | init.sql |
| `agi_bonus` | INTEGER | YES | 0 | init.sql |
| `vit_bonus` | INTEGER | YES | 0 | init.sql |
| `int_bonus` | INTEGER | YES | 0 | init.sql |
| `dex_bonus` | INTEGER | YES | 0 | init.sql |
| `luk_bonus` | INTEGER | YES | 0 | init.sql |
| `max_hp_bonus` | INTEGER | YES | 0 | init.sql |
| `max_sp_bonus` | INTEGER | YES | 0 | init.sql |
| `hit_bonus` | INTEGER | YES | 0 | init.sql + auto |
| `flee_bonus` | INTEGER | YES | 0 | init.sql + auto |
| `critical_bonus` | INTEGER | YES | 0 | init.sql + auto |
| `required_level` | INTEGER | YES | 1 | init.sql |
| `stackable` | BOOLEAN | YES | false | init.sql |
| `max_stack` | INTEGER | YES | 1 | init.sql |
| `icon` | VARCHAR(100) | YES | 'default_item' | init.sql |
| `weapon_type` | VARCHAR(30) | YES | NULL | init.sql + auto (VARCHAR(20) mismatch) |
| `aspd_modifier` | FLOAT | YES | 0 | init.sql (FLOAT) vs auto (INTEGER) -- **TYPE MISMATCH** |
| `weapon_range` | INTEGER | YES | 150 | init.sql (150) vs auto (0) -- **DEFAULT MISMATCH** |
| `weapon_level` | INTEGER | YES | 0 | init.sql |
| `armor_level` | INTEGER | YES | 0 | init.sql |
| `slots` | INTEGER | YES | 0 | init.sql |
| `equip_level_min` | INTEGER | YES | 0 | init.sql |
| `equip_level_max` | INTEGER | YES | 0 | init.sql |
| `refineable` | BOOLEAN | YES | false | init.sql |
| `jobs_allowed` | TEXT | YES | 'All' | init.sql |
| `classes_allowed` | TEXT | YES | 'All' | init.sql |
| `gender_allowed` | VARCHAR(10) | YES | 'Both' | init.sql |
| `equip_locations` | TEXT | YES | NULL | init.sql |
| `script` | TEXT | YES | NULL | init.sql |
| `equip_script` | TEXT | YES | NULL | init.sql |
| `unequip_script` | TEXT | YES | NULL | init.sql |
| `sub_type` | VARCHAR(30) | YES | NULL | init.sql |
| `view_sprite` | INTEGER | YES | 0 | init.sql |
| `two_handed` | BOOLEAN | YES | false | init.sql |
| `element` | VARCHAR(10) | YES | 'neutral' | init.sql |
| `card_type` | VARCHAR(20) | YES | NULL | init.sql |
| `card_prefix` | VARCHAR(50) | YES | NULL | init.sql |
| `card_suffix` | VARCHAR(50) | YES | NULL | init.sql |
| `class_restrictions` | TEXT | YES | NULL | init.sql |
| `perfect_dodge_bonus` | INTEGER | YES | 0 | init.sql |
| `ammo_type` | VARCHAR(20) | YES | NULL | init.sql |
| `max_hp_rate` | INTEGER | YES | 0 | auto + migration |
| `max_sp_rate` | INTEGER | YES | 0 | auto + migration |
| `aspd_rate` | INTEGER | YES | 0 | auto + migration |
| `hp_regen_rate` | INTEGER | YES | 0 | auto + migration |
| `sp_regen_rate` | INTEGER | YES | 0 | auto + migration |
| `crit_atk_rate` | INTEGER | YES | 0 | auto + migration |
| `cast_rate` | INTEGER | YES | 0 | auto + migration |
| `use_sp_rate` | INTEGER | YES | 0 | auto + migration |
| `heal_power` | INTEGER | YES | 0 | auto + migration |

**Indexes**: None defined beyond PK. Item lookups use `loadItemDefinitions()` which loads all items into a JavaScript `Map` at startup.

### 1.4 `character_inventory`

| Column | Type | Nullable | Default | Source |
|--------|------|----------|---------|--------|
| `inventory_id` | SERIAL | NO | auto | **PK**, init.sql |
| `character_id` | INTEGER | YES | - | FK -> `characters(character_id)` ON DELETE CASCADE, init.sql |
| `item_id` | INTEGER | YES | - | FK -> `items(item_id)`, init.sql |
| `quantity` | INTEGER | YES | 1 | init.sql |
| `is_equipped` | BOOLEAN | YES | false | init.sql |
| `slot_index` | INTEGER | YES | -1 | init.sql |
| `created_at` | TIMESTAMP | YES | CURRENT_TIMESTAMP | init.sql |
| `equipped_position` | VARCHAR(20) | YES | NULL | auto + migration |
| `refine_level` | INTEGER | YES | 0 | migration only |
| `compounded_cards` | JSONB | YES | '[]' | migration only |
| `forged_by` | VARCHAR(100) | YES | NULL | auto + migration |
| `forged_element` | VARCHAR(10) | YES | NULL | auto + migration |
| `forged_star_crumbs` | INTEGER | YES | 0 | auto + migration |
| `identified` | BOOLEAN | YES | true | auto + migration |
| `is_broken` | BOOLEAN | YES | FALSE | migration only |

**Indexes**: `idx_inventory_character(character_id)`, `idx_inventory_item(item_id)`, `idx_ci_equipped_cards(character_id) WHERE is_equipped=true AND compounded_cards IS NOT NULL AND compounded_cards != '[]'`, `idx_inventory_broken(character_id, is_broken) WHERE is_broken=TRUE`

### 1.5 `character_hotbar`

| Column | Type | Nullable | Default | Source |
|--------|------|----------|---------|--------|
| `character_id` | INTEGER | NO | - | FK -> `characters(character_id)` ON DELETE CASCADE |
| `row_index` | INTEGER | NO | 0 | auto + migration |
| `slot_index` | INTEGER | NO | - | CHECK >= 0 AND <= 8 |
| `inventory_id` | INTEGER | YES* | - | FK -> `character_inventory(inventory_id)` ON DELETE CASCADE; *made nullable by auto |
| `item_id` | INTEGER | YES* | - | *made nullable by auto |
| `item_name` | VARCHAR(100) | NO | '' | |
| `slot_type` | VARCHAR(10) | YES | 'item' | auto |
| `skill_id` | INTEGER | YES | 0 | auto |
| `skill_name` | VARCHAR(100) | YES | '' | auto |
| `skill_level` | INTEGER | YES | 0 | **NOT IN auto-creation or any migration -- only works if column was added manually** |

**PK**: `(character_id, row_index, slot_index)` -- updated from original `(character_id, slot_index)` by auto/migration
**Indexes**: `idx_hotbar_character(character_id)`, `idx_hotbar_character_row(character_id, row_index)`
**Constraints**: `check_row_index CHECK (row_index >= 0 AND row_index <= 3)` (from migration)

### 1.6 `skills` (Static skill definitions, populated from JS at startup)

| Column | Type | Nullable | Default | Source |
|--------|------|----------|---------|--------|
| `skill_id` | INTEGER | NO | - | **PK**, migration + auto |
| `internal_name` | VARCHAR(100) | NO | - | UNIQUE |
| `display_name` | VARCHAR(100) | NO | - | |
| `class_id` | VARCHAR(30) | NO | - | |
| `max_level` | INTEGER | NO | 1 | |
| `skill_type` | VARCHAR(20) | NO | 'active' | |
| `target_type` | VARCHAR(20) | NO | 'none' | |
| `element` | VARCHAR(20) | NO | 'neutral' | |
| `skill_range` | INTEGER | NO | 0 | |
| `description` | TEXT | YES | '' | |
| `icon` | VARCHAR(100) | YES | 'default_skill' | |
| `tree_row` | INTEGER | YES | 0 | |
| `tree_col` | INTEGER | YES | 0 | |

**Indexes**: `idx_skills_class(class_id)`
**Note**: FK on `character_skills.skill_id -> skills.skill_id` is explicitly dropped at startup (line 32011) because skill definitions live in `ro_skill_data.js`, not the DB.

### 1.7 `skill_prerequisites`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `skill_id` | INTEGER | NO | - | FK -> `skills(skill_id)` ON DELETE CASCADE |
| `required_skill_id` | INTEGER | NO | - | FK -> `skills(skill_id)` ON DELETE CASCADE |
| `required_level` | INTEGER | NO | 1 | |

**PK**: `(skill_id, required_skill_id)`
**Indexes**: `idx_skill_prereqs_skill(skill_id)`, `idx_skill_prereqs_required(required_skill_id)`

### 1.8 `skill_levels`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `skill_id` | INTEGER | NO | - | FK -> `skills(skill_id)` ON DELETE CASCADE |
| `level` | INTEGER | NO | - | |
| `sp_cost` | INTEGER | NO | 0 | |
| `cast_time_ms` | INTEGER | NO | 0 | |
| `cooldown_ms` | INTEGER | NO | 0 | |
| `effect_value` | INTEGER | YES | 0 | |
| `duration_ms` | INTEGER | YES | 0 | |
| `description` | TEXT | YES | '' | |

**PK**: `(skill_id, level)`
**Indexes**: `idx_skill_levels_skill(skill_id)`

### 1.9 `character_skills`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `character_id` | INTEGER | NO | - | FK -> `characters(character_id)` ON DELETE CASCADE |
| `skill_id` | INTEGER | NO | - | FK -> `skills(skill_id)` ON DELETE CASCADE (DROPPED at startup) |
| `level` | INTEGER | NO | 1 | |

**PK**: `(character_id, skill_id)`
**Indexes**: `idx_char_skills_character(character_id)`, `idx_char_skills_skill(skill_id)`

### 1.10 `character_cart`

| Column | Type | Nullable | Default | Source |
|--------|------|----------|---------|--------|
| `cart_id` | SERIAL | NO | auto | **PK** |
| `character_id` | INTEGER | YES | - | FK -> `characters(character_id)` ON DELETE CASCADE |
| `item_id` | INTEGER | YES | - | FK -> `items(item_id)` |
| `quantity` | INTEGER | YES | 1 | |
| `slot_index` | INTEGER | YES | -1 | |
| `identified` | BOOLEAN | YES | true | |
| `refine_level` | INTEGER | YES | 0 | |
| `compounded_cards` | JSONB | YES | '[]' | |
| `forged_by` | VARCHAR(50) | YES | NULL | |
| `forged_element` | VARCHAR(10) | YES | NULL | |
| `forged_star_crumbs` | INTEGER | YES | 0 | |
| `created_at` | TIMESTAMP | YES | CURRENT_TIMESTAMP | |

**Indexes**: `idx_cart_character(character_id)`
**Source**: auto + migration

### 1.11 `vending_shops`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `shop_id` | SERIAL | NO | auto | **PK** |
| `character_id` | INTEGER | YES | - | FK -> `characters(character_id)` ON DELETE CASCADE |
| `title` | VARCHAR(80) | NO | 'Shop' | |
| `zone` | VARCHAR(50) | NO | - | |
| `x` | REAL | NO | - | |
| `y` | REAL | NO | - | |
| `z` | REAL | YES | 0 | |
| `created_at` | TIMESTAMP | YES | CURRENT_TIMESTAMP | |

**Indexes**: `idx_vending_zone(zone)`

### 1.12 `vending_items`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `vend_item_id` | SERIAL | NO | auto | **PK** |
| `shop_id` | INTEGER | YES | - | FK -> `vending_shops(shop_id)` ON DELETE CASCADE |
| `cart_id` | INTEGER | NO | - | No FK (references `character_cart.cart_id` logically) |
| `item_id` | INTEGER | NO | - | No FK (references `items.item_id` logically) |
| `amount` | INTEGER | NO | - | |
| `price` | INTEGER | NO | - | CHECK > 0 AND <= 1,000,000,000 |

### 1.13 `character_homunculus`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `homunculus_id` | SERIAL | NO | auto | **PK** |
| `character_id` | INTEGER | NO | - | FK -> `characters(id)` ON DELETE CASCADE -- **BUG: should be characters(character_id)** |
| `type` | VARCHAR(20) | NO | - | CHECK IN ('lif','amistr','filir','vanilmirth') |
| `sprite_variant` | INTEGER | NO | 1 | |
| `name` | VARCHAR(24) | NO | 'Homunculus' | |
| `level` | INTEGER | NO | 1 | |
| `experience` | BIGINT | NO | 0 | |
| `intimacy` | INTEGER | NO | 250 | CHECK >= 0 AND <= 1000 (migration only) |
| `hunger` | INTEGER | NO | 50 | CHECK >= 0 AND <= 100 (migration only) |
| `hp_current` | INTEGER | NO | 150 | |
| `hp_max` | INTEGER | NO | 150 | |
| `sp_current` | INTEGER | NO | 40 | |
| `sp_max` | INTEGER | NO | 40 | |
| `str` | INTEGER | NO | 10 | |
| `agi` | INTEGER | NO | 10 | |
| `vit` | INTEGER | NO | 10 | |
| `int_stat` | INTEGER | NO | 10 | |
| `dex` | INTEGER | NO | 10 | |
| `luk` | INTEGER | NO | 10 | |
| `skill_1_level` | INTEGER | NO | 1 | |
| `skill_2_level` | INTEGER | NO | 0 | |
| `skill_3_level` | INTEGER | NO | 0 | |
| `skill_points` | INTEGER | NO | 0 | |
| `is_evolved` | BOOLEAN | NO | FALSE | |
| `is_alive` | BOOLEAN | NO | TRUE | |
| `is_summoned` | BOOLEAN | NO | FALSE | |
| `created_at` | TIMESTAMP | YES | NOW() | |
| `updated_at` | TIMESTAMP | YES | NOW() | |

**Indexes**: `idx_homunculus_character(character_id)` UNIQUE, `idx_homunculus_summoned(character_id, is_summoned)`

### 1.14 `character_pets`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `id` | SERIAL | NO | auto | **PK** |
| `character_id` | INTEGER | NO | - | **No FK** -- missing reference to `characters(character_id)` |
| `mob_id` | INTEGER | NO | - | |
| `egg_item_id` | INTEGER | NO | - | |
| `pet_name` | VARCHAR(24) | YES | '' | |
| `intimacy` | INTEGER | YES | 250 | |
| `hunger` | INTEGER | YES | 100 | |
| `equip_item_id` | INTEGER | YES | 0 | |
| `is_hatched` | BOOLEAN | YES | FALSE | |
| `is_active` | BOOLEAN | YES | FALSE | |
| `created_at` | TIMESTAMP | YES | NOW() | |

**Indexes**: `idx_character_pets_char(character_id)`
**Source**: auto-creation only (no migration file)

### 1.15 `parties`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `party_id` | SERIAL | NO | auto | **PK** |
| `name` | VARCHAR(24) | NO | - | UNIQUE |
| `leader_id` | INTEGER | NO | - | FK -> `characters(id)` in migration / **No FK** in auto -- **BUG in migration** |
| `exp_share` | SMALLINT | YES | 0 | |
| `item_share` | SMALLINT | YES | 0 | |
| `item_distribute` | SMALLINT | YES | 0 | |
| `created_at` | TIMESTAMP | YES | NOW() | |

**Indexes**: `idx_parties_leader(leader_id)`
**Note**: The auto-creation (line 32491) omits the FK on `leader_id`. The migration references `characters(id)` which does not exist.

### 1.16 `party_members`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `party_id` | INTEGER | NO | - | FK -> `parties(party_id)` ON DELETE CASCADE |
| `character_id` | INTEGER | NO | - | FK -> `characters(id)` in migration / **No FK** in auto |
| `joined_at` | TIMESTAMP | YES | NOW() | |

**PK**: `(party_id, character_id)`
**Indexes**: `idx_party_members_char(character_id)`

### 1.17 `character_memo`

| Column | Type | Nullable | Default |
|--------|------|----------|---------|
| `character_id` | INTEGER | NO | - | **No FK** |
| `slot_index` | INTEGER | NO | - | CHECK >= 0 AND <= 2 |
| `zone_name` | VARCHAR(50) | NO | - | |
| `x` | FLOAT | NO | 0 | |
| `y` | FLOAT | NO | 0 | |
| `z` | FLOAT | NO | 580 | |

**PK**: `(character_id, slot_index)`
**Source**: auto-creation only (no migration file)

---

## 2. Migration Summary

| # | Migration File | Target Table | Changes |
|---|---------------|-------------|---------|
| 1 | `add_character_hotbar.sql` | character_hotbar | CREATE TABLE (composite PK), idx_hotbar_character |
| 2 | `add_zeny_column.sql` | characters | ADD `zuzucoin` INTEGER DEFAULT 0 |
| 3 | `add_ro_drop_items.sql` | items | INSERT 126 items (consumables, loot, weapons, armor, cards) |
| 4 | `add_exp_leveling_system.sql` | characters | ADD `job_level`, `base_exp` BIGINT, `job_exp` BIGINT, `job_class`, `skill_points`; indexes |
| 5 | `add_class_skill_system.sql` | skills, skill_prerequisites, skill_levels, character_skills | CREATE 4 tables with FKs and indexes |
| 6 | `add_equipped_position.sql` | character_inventory | ADD `equipped_position` VARCHAR(20); backfill |
| 7 | `add_hotbar_multirow.sql` | character_hotbar | ADD `row_index`; PK update to 3-col; CHECK constraint; slot normalization |
| 8 | `add_character_customization.sql` | characters | ADD `hair_style`, `hair_color`, `gender`, `delete_date`; partial index |
| 9 | `add_zone_system.sql` | characters | ADD `zone_name`, `save_map`, `save_x`, `save_y`, `save_z`; zone index |
| 10 | `add_soft_delete.sql` | characters | ADD `deleted` BOOLEAN NOT NULL DEFAULT FALSE; partial index |
| 11 | `migrate_to_canonical_ids.sql` | items, character_inventory, character_hotbar | Remap custom IDs -> rAthena canonical IDs; add many columns to items; DROP/RESTORE FKs |
| 12 | `add_item_inspect_fields.sql` | character_inventory | ADD `refine_level`, `compounded_cards` JSONB |
| 13 | `fix_item_element_matk_twohanded.sql` | items | DATA FIX: element from scripts, clear renewal MATK, fix two_handed, fix descriptions |
| 14 | `fix_item_script_column_audit.sql` | items | DATA FIX: 91 column corrections (value mismatches, conditional bonuses, perfect_dodge) |
| 15 | `add_card_compound_support.sql` | character_inventory, items | Ensure `compounded_cards`; populate `card_type` from `equip_locations`; partial index |
| 16 | `fix_card_perfect_dodge_bonus.sql` | items | DATA FIX: 6 cards missing `perfect_dodge_bonus` |
| 17 | `populate_card_naming.sql` | items | DATA FIX: Populate `card_prefix`/`card_suffix` for 441 cards |
| 18 | `add_equipment_rate_columns.sql` | items | ADD 9 rate columns (`max_hp_rate`, `max_sp_rate`, `aspd_rate`, etc.) |
| 19 | `add_equipment_break.sql` | character_inventory | ADD `is_broken` BOOLEAN; partial index |
| 20 | `add_stone_item.sql` | items | INSERT Stone item (ID 7049) |
| 21 | `add_forge_columns.sql` | character_inventory | ADD `forged_by`, `forged_element`, `forged_star_crumbs` |
| 22 | `add_homunculus_table.sql` | character_homunculus | CREATE TABLE with CHECK constraints |
| 23 | `add_cart_vending_identify.sql` | characters, character_inventory, character_cart, vending_shops, vending_items | ADD cart/identify columns; CREATE 3 tables; INSERT Magnifier |
| 24 | `add_party_system.sql` | parties, party_members | CREATE 2 tables with FK/indexes |
| 25 | `add_plagiarism_columns.sql` | characters | ADD `plagiarized_skill_id`, `plagiarized_skill_level` |

---

## 3. Server Auto-Creation Columns

The server startup block (lines 31982-32547) auto-creates the following. Entries marked with an issue are highlighted.

### 3.1 Characters table auto-created columns
- `str`, `agi`, `vit`, `int_stat`, `dex`, `luk`, `stat_points`, `max_health`, `max_mana` -- OK
- `zuzucoin` -- OK
- `hair_style`, `hair_color`, `gender`, `delete_date` -- OK (also in init.sql)
- `job_level`, `base_exp`, `job_exp`, `job_class`, `skill_points` -- OK
- `zone_name`, `save_map`, `save_x`, `save_y`, `save_z` -- OK
- `plagiarized_skill_id`, `plagiarized_skill_level` -- OK
- `has_cart`, `cart_type` -- OK

**MISSING from auto-creation**: `deleted` BOOLEAN NOT NULL DEFAULT FALSE -- The `deleted` column is in init.sql and migration, but NOT in the auto-creation block. If a fresh DB is created via auto-creation only, the `deleted` column will not exist, and soft-delete queries will fail.

### 3.2 Character_inventory auto-created columns
- `equipped_position` -- OK
- `forged_by`, `forged_element`, `forged_star_crumbs` -- OK
- `identified` -- OK

**MISSING from auto-creation**:
- **`refine_level`** INTEGER DEFAULT 0 -- queried by `getPlayerInventory()` at line 4983
- **`compounded_cards`** JSONB DEFAULT '[]' -- queried by `getPlayerInventory()` at line 4983
- **`is_broken`** BOOLEAN DEFAULT FALSE -- queried at line 23296

### 3.3 Character_hotbar auto-created columns
- `slot_type`, `skill_id`, `skill_name` -- OK
- `row_index` -- OK

**MISSING from auto-creation**:
- **`skill_level`** INTEGER DEFAULT 0 -- queried at line 5048, written at line 9076. Column will not exist on a fresh auto-created DB.

### 3.4 Items table auto-created columns
- `weapon_type`, `aspd_modifier`, `weapon_range` -- OK (but type/default mismatches, see Section 5)
- `hit_bonus`, `flee_bonus`, `critical_bonus` -- OK
- `max_hp_rate`, `max_sp_rate`, `aspd_rate`, `hp_regen_rate`, `sp_regen_rate`, `crit_atk_rate`, `cast_rate`, `use_sp_rate`, `heal_power` -- OK

**MISSING from auto-creation**: Many columns that exist in init.sql are not auto-created because the auto-creation uses a minimal `CREATE TABLE IF NOT EXISTS items` (lines 32186-32213) with only basic columns. If the items table was created by this auto-creation instead of init.sql, these columns would be missing:
- `aegis_name`, `full_description`, `buy_price`, `sell_price`, `matk`, `mdef`, `str_bonus` through `luk_bonus`, `max_hp_bonus`, `max_sp_bonus`, `required_level`, `stackable`, `max_stack`, `weapon_level`, `armor_level`, `slots`, `equip_level_min/max`, `refineable`, `jobs_allowed`, `classes_allowed`, `gender_allowed`, `equip_locations`, `script/equip_script/unequip_script`, `sub_type`, `view_sprite`, `two_handed`, `element`, `card_type/prefix/suffix`, `class_restrictions`, `perfect_dodge_bonus`, `ammo_type`

However, the items table in auto-creation only uses `CREATE TABLE IF NOT EXISTS`, so if init.sql was already run, these columns exist. The risk is a fresh DB bootstrapped only from auto-creation.

### 3.5 Tables auto-created (no migration file)
- `character_pets` -- auto-created only, no migration file
- `character_memo` -- auto-created only, no migration file

---

## 4. Foreign Key Analysis

### 4.1 Correct FKs
| FK | From -> To | ON DELETE | Status |
|----|-----------|-----------|--------|
| `characters.user_id` | -> `users(user_id)` | CASCADE | OK |
| `character_inventory.character_id` | -> `characters(character_id)` | CASCADE | OK |
| `character_inventory.item_id` | -> `items(item_id)` | (none) | OK (items are static) |
| `character_hotbar.character_id` | -> `characters(character_id)` | CASCADE | OK |
| `character_hotbar.inventory_id` | -> `character_inventory(inventory_id)` | CASCADE | OK |
| `character_cart.character_id` | -> `characters(character_id)` | CASCADE | OK |
| `character_cart.item_id` | -> `items(item_id)` | (none) | OK |
| `character_skills.character_id` | -> `characters(character_id)` | CASCADE | OK |
| `vending_shops.character_id` | -> `characters(character_id)` | CASCADE | OK |
| `vending_items.shop_id` | -> `vending_shops(shop_id)` | CASCADE | OK |
| `skill_prerequisites.*` | -> `skills(skill_id)` | CASCADE | OK |
| `skill_levels.skill_id` | -> `skills(skill_id)` | CASCADE | OK |
| `party_members.party_id` | -> `parties(party_id)` | CASCADE | OK |

### 4.2 BROKEN FKs (Critical)

| Issue | Table | Column | References | Problem |
|-------|-------|--------|------------|---------|
| **BUG-FK-1** | `character_homunculus` | `character_id` | `characters(id)` | `characters` PK is `character_id`, not `id`. This FK will FAIL on CREATE TABLE if the column doesn't exist. |
| **BUG-FK-2** | `parties` (migration) | `leader_id` | `characters(id)` | Same issue -- `characters` has no `id` column. |
| **BUG-FK-3** | `party_members` (migration) | `character_id` | `characters(id)` | Same issue. |

**Impact**: The migration files for homunculus and party will fail with `ERROR: column "id" referenced in foreign key constraint does not exist` if run on a DB where `characters` PK is `character_id`. The auto-creation at startup works around this:
- `character_homunculus` auto-creation at line 32118 has the same bug (`characters(id)`) -- this CREATE TABLE will also fail unless PostgreSQL somehow resolves it.
- `parties` auto-creation at line 32491 avoids the issue by omitting the FK entirely (`leader_id INTEGER NOT NULL` with no REFERENCES).
- `party_members` auto-creation at line 32499 also omits the FK (`character_id INTEGER NOT NULL` with no REFERENCES).

**Likely scenario**: The auto-creation at startup silently fails for `character_homunculus` (caught by try/catch, logged as warning), and the table is never created until someone manually fixes the FK. Or PostgreSQL interprets `characters(id)` as referencing `character_id` via some alias -- this needs testing.

### 4.3 MISSING FKs

| Table | Column | Should Reference | Risk |
|-------|--------|-----------------|------|
| `character_pets.character_id` | - | `characters(character_id)` ON DELETE CASCADE | Orphaned pets if character is hard-deleted (currently soft-delete only, so low risk) |
| `character_memo.character_id` | - | `characters(character_id)` ON DELETE CASCADE | Orphaned memo entries |
| `vending_items.cart_id` | - | `character_cart(cart_id)` | Orphaned vending item references |
| `vending_items.item_id` | - | `items(item_id)` | Referential integrity gap |
| `parties.leader_id` | - | `characters(character_id)` | Orphaned party with invalid leader |
| `party_members.character_id` (auto) | - | `characters(character_id)` | Orphaned membership records |

### 4.4 Orphan Risk from Missing CASCADE

| Table | FK Missing CASCADE | Risk |
|-------|-------------------|------|
| `character_inventory.item_id` -> `items(item_id)` | No CASCADE | If an item definition is deleted from `items`, inventory rows become orphaned. Low risk since item definitions are static. |

---

## 5. Column Type & Constraint Issues

### 5.1 Type Mismatches Between init.sql and Auto-Creation

| Column | init.sql Type | Auto-Creation Type | Impact |
|--------|--------------|-------------------|--------|
| `items.weapon_type` | VARCHAR(30) | VARCHAR(20) | If auto-creation runs first and init.sql runs later, the column already exists at VARCHAR(20). Some weapon types like `two_hand_sword` (15 chars) fit, but longer custom types would be truncated. |
| `items.aspd_modifier` | FLOAT | INTEGER | If auto-creation runs first, values like `5.0` or `-2.5` would be truncated to integers. All current values are whole numbers, so no data loss currently. |
| `items.weapon_range` | DEFAULT 150 | DEFAULT 0 | Not a type mismatch, but default differs. Items created before init.sql would get range 0 instead of 150. |

### 5.2 Zeny Column Type

| Issue | Details |
|-------|---------|
| `characters.zuzucoin` | INTEGER (max 2,147,483,647 = ~2.1 billion zeny). RO Classic max zeny is 999,999,999 (~1 billion). INTEGER is sufficient. |
| `vending_items.price` | INTEGER with CHECK <= 1,000,000,000. Appropriate. |

### 5.3 EXP Column Types

| Column | Type | Max Value | Adequate? |
|--------|------|-----------|-----------|
| `characters.base_exp` | BIGINT | 9.2 * 10^18 | Yes |
| `characters.job_exp` | BIGINT | 9.2 * 10^18 | Yes |
| `characters.experience` | INTEGER | ~2.1 billion | Legacy column, not used for RO-style leveling. OK as-is. |

### 5.4 Missing NOT NULL Constraints

The following columns should logically be NOT NULL but are nullable:

| Table | Column | Should Be | Reason |
|-------|--------|-----------|--------|
| `characters.user_id` | NOT NULL | Every character must belong to a user |
| `character_inventory.character_id` | NOT NULL | Every inventory row must belong to a character |
| `character_inventory.item_id` | NOT NULL | Every inventory row must reference an item |
| `character_cart.character_id` | NOT NULL | Every cart item must belong to a character |
| `vending_shops.character_id` | NOT NULL | Every shop must belong to a character |

### 5.5 Default Value Concerns

| Table | Column | Default | Issue |
|-------|--------|---------|-------|
| `characters.class` | 'warrior' | Should be 'novice' to match RO Classic. The actual creation code uses 'novice', but the DB default is 'warrior'. |
| `character_inventory.slot_index` | -1 | Negative slot indexes are fixed at startup by the auto-slot-fix code (line 32463). This works but is fragile. |
| `character_pets.hunger` | 100 | Auto-creation uses 100, but migration uses CHECK 0-100. Consistent but no CHECK in auto-creation. |

---

## 6. Index Coverage Analysis

### 6.1 Current Indexes

| Table | Index | Columns | Type |
|-------|-------|---------|------|
| users | idx_users_username | username | B-tree |
| users | idx_users_email | email | B-tree |
| characters | idx_characters_user_id | user_id | B-tree |
| characters | idx_characters_deleted | deleted | Partial (WHERE deleted=FALSE) |
| characters | idx_characters_job_class | job_class | B-tree |
| characters | idx_characters_level | level | B-tree |
| characters | idx_characters_delete_date | delete_date | Partial (WHERE NOT NULL) |
| characters | idx_characters_zone | zone_name | B-tree |
| character_inventory | idx_inventory_character | character_id | B-tree |
| character_inventory | idx_inventory_item | item_id | B-tree |
| character_inventory | idx_ci_equipped_cards | character_id | Partial (equipped + has cards) |
| character_inventory | idx_inventory_broken | (character_id, is_broken) | Partial (is_broken=TRUE) |
| character_hotbar | idx_hotbar_character | character_id | B-tree |
| character_hotbar | idx_hotbar_character_row | (character_id, row_index) | B-tree |
| character_cart | idx_cart_character | character_id | B-tree |
| vending_shops | idx_vending_zone | zone | B-tree |
| character_pets | idx_character_pets_char | character_id | B-tree |
| skills | idx_skills_class | class_id | B-tree |
| skill_prerequisites | idx_skill_prereqs_skill | skill_id | B-tree |
| skill_prerequisites | idx_skill_prereqs_required | required_skill_id | B-tree |
| skill_levels | idx_skill_levels_skill | skill_id | B-tree |
| character_skills | idx_char_skills_character | character_id | B-tree |
| character_skills | idx_char_skills_skill | skill_id | B-tree |
| character_homunculus | idx_homunculus_character | character_id | UNIQUE |
| character_homunculus | idx_homunculus_summoned | (character_id, is_summoned) | B-tree |
| party_members | idx_party_members_char | character_id | B-tree |
| parties | idx_parties_leader | leader_id | B-tree |

### 6.2 Missing Indexes (Recommended)

| Table | Suggested Index | Justification |
|-------|----------------|---------------|
| `character_inventory` | `(character_id, is_equipped)` | Frequent query pattern: `WHERE character_id = $1 AND is_equipped = true/false` (equipment loading, equip/unequip). Current index on `character_id` alone doesn't filter by `is_equipped`. |
| `character_inventory` | `(character_id, item_id)` composite | Frequent pattern: `WHERE character_id = $1 AND item_id = $2 AND is_equipped = false` (item consumption, stacking). |
| `characters` | `(user_id, deleted)` composite | The character list query filters by both `user_id` and `deleted = FALSE`. A composite index would be more efficient than scanning `idx_characters_user_id` and then filtering. |
| `character_inventory` | `(character_id, identified) WHERE identified = false` | Used by identify item queries (`WHERE ci.character_id = $1 AND ci.identified = false`). |
| `items` | None needed | All items are loaded into an in-memory Map at startup. DB queries against `items` are rare (only `loadItemDefinitions()`). |
| `character_hotbar` | None needed beyond current | Current composite PK + row_index index covers all query patterns. |
| `vending_shops` | `(character_id)` | Queried when a character opens/closes their shop. |

### 6.3 Redundant Indexes

| Index | Issue |
|-------|-------|
| `idx_users_username` on `users(username)` | `username` already has a UNIQUE constraint, which implicitly creates an index. This index is redundant. |
| `idx_users_email` on `users(email)` | Same -- `email` UNIQUE constraint creates an implicit index. Redundant. |

---

## 7. Soft-Delete Data Integrity

### 7.1 Correctly Filtered Queries (include `deleted = FALSE`)

| Line | Query | Status |
|------|-------|--------|
| 5312 | `SELECT 1 FROM characters WHERE character_id=$1 AND user_id=$2 AND deleted=FALSE` | OK |
| 31700 | Character list: `WHERE user_id=$1 AND delete_date IS NULL AND deleted=FALSE` | OK |
| 31745 | Character count: `WHERE user_id=$1 AND delete_date IS NULL AND deleted=FALSE` | OK |
| 31856 | Delete check: `WHERE character_id=$1 AND user_id=$2 AND deleted=FALSE` | OK |

### 7.2 Queries MISSING `deleted = FALSE` Filter

| Line | Query | Risk |
|------|-------|------|
| **31754** | `SELECT 1 FROM characters WHERE LOWER(name) = LOWER($1)` | **Name uniqueness check during character creation**. A deleted character's name cannot be reused because this query finds deleted characters too. **MEDIUM severity** -- users cannot reclaim names from deleted characters. |
| **31809** | `SELECT ... FROM characters WHERE character_id=$1 AND user_id=$2` | **GET /api/characters/:id** endpoint. Returns deleted characters if their ID is known. **LOW severity** -- requires knowing the character ID, but could leak data about deleted characters. |
| **31898** | `SELECT character_id FROM characters WHERE character_id=$1 AND user_id=$2` | **PUT /api/characters/:id/position** endpoint. Allows saving position for a deleted character. **LOW severity** -- harmless data write. |
| **5336** | `SELECT ... FROM characters WHERE character_id=$1` | **player:join socket event** (data fetch). No `deleted` filter, but this is called after the ownership check at line 5312 which does filter. Acceptable because the prior check gates it. |
| **5385** | `SELECT ... FROM characters WHERE character_id=$1` | **player:join socket event** (stat fetch). Same as above -- gated by prior ownership + deleted check. OK. |
| **6537** | `SELECT save_map FROM characters WHERE character_id=$1` | Save point lookup during teleport. Only called for connected players, so implicitly filtered. OK. |
| **7742** | `SELECT save_map ... WHERE character_id=$1` | Respawn location lookup. Same -- only for connected players. OK. |
| Multiple | Various `UPDATE characters SET ... WHERE character_id=$1` | Updates to connected players. Only connected players are in `connectedPlayers` Map, which requires passing the deleted check. OK. |

### 7.3 Hard-Delete Usage

Characters are **never** hard-deleted -- `DELETE /api/characters/:id` sets `deleted = TRUE` (line 31871). This is correct per the design.

However, the following tables DO use hard-delete (appropriate for transactional data):
- `character_inventory` -- rows deleted when items are consumed/dropped (correct)
- `character_hotbar` -- rows deleted when slots are cleared (correct)
- `character_cart` -- rows deleted when items are removed from cart (correct)
- `vending_shops` / `vending_items` -- rows deleted when shop closes (correct)
- `character_pets` -- rows deleted when pet runs away (correct)
- `party_members` -- rows deleted when leaving party (correct)
- `character_skills` -- rows deleted on class change reset (correct)

---

## 8. Missing Tables / Columns

### 8.1 Missing Auto-Creation Entries (HIGH Priority)

These columns are queried by the server but are NOT in the auto-creation block. A fresh DB bootstrapped from auto-creation will crash on these queries.

| Table | Column | Queried At | Fix |
|-------|--------|-----------|-----|
| `character_inventory.refine_level` | INTEGER DEFAULT 0 | Line 4983 (`getPlayerInventory`) | Add to auto-creation |
| `character_inventory.compounded_cards` | JSONB DEFAULT '[]' | Line 4983 (`getPlayerInventory`) | Add to auto-creation |
| `character_inventory.is_broken` | BOOLEAN DEFAULT FALSE | Line 23296 | Add to auto-creation |
| `character_hotbar.skill_level` | INTEGER DEFAULT 0 | Line 5048 (`getPlayerHotbar`), Line 9076 (INSERT) | Add to auto-creation |
| `characters.deleted` | BOOLEAN NOT NULL DEFAULT FALSE | Lines 5312, 31700, 31745, 31856 | Add to auto-creation |

### 8.2 Tables Only in Auto-Creation (No Migration File)

| Table | Status | Risk |
|-------|--------|------|
| `character_pets` | Auto-creation only | If auto-creation fails (e.g., due to FK issue), table won't exist. Should have a migration file. |
| `character_memo` | Auto-creation only | Same risk. Should have a migration file. |

### 8.3 Items Table Auto-Creation vs init.sql Gap

The auto-creation `CREATE TABLE IF NOT EXISTS items` at line 32186 defines only 22 columns. The init.sql defines 42+ columns. If the items table is created by auto-creation first, 20+ columns will be missing. Many are then added by later `ADD COLUMN IF NOT EXISTS` statements, but these are still missing from both auto-creation and ADD COLUMN blocks:

- `aegis_name`, `full_description`, `buy_price`, `sell_price`
- `matk`, `mdef`
- `str_bonus`, `agi_bonus`, `vit_bonus`, `int_bonus`, `dex_bonus`, `luk_bonus`
- `max_hp_bonus`, `max_sp_bonus`
- `weapon_level`, `armor_level`, `slots`
- `equip_level_min`, `equip_level_max`
- `refineable`, `jobs_allowed`, `classes_allowed`, `gender_allowed`
- `equip_locations`, `script`, `equip_script`, `unequip_script`
- `sub_type`, `view_sprite`, `two_handed`, `element`
- `card_type`, `card_prefix`, `card_suffix`
- `class_restrictions`, `perfect_dodge_bonus`, `ammo_type`

**Mitigation**: The `loadItemDefinitions()` function does `SELECT * FROM items`, so it reads whatever columns exist. The `getPlayerInventory()` explicitly references many of these columns and would fail. In practice, init.sql should always be run before the server starts, so this is a documentation/bootstrapping issue rather than a runtime bug.

---

## 9. Critical Issues Summary

### CRITICAL (will cause runtime errors or data corruption)

| ID | Issue | Severity | Location |
|----|-------|----------|----------|
| **C1** | `character_homunculus.character_id` references `characters(id)` -- column `id` does not exist (PK is `character_id`) | CRITICAL | `add_homunculus_table.sql` line 6, `index.js` line 32118 |
| **C2** | `parties.leader_id` references `characters(id)` in migration -- same broken FK | CRITICAL | `add_party_system.sql` line 7 |
| **C3** | `party_members.character_id` references `characters(id)` in migration -- same broken FK | CRITICAL | `add_party_system.sql` line 16 |
| **C4** | `character_hotbar.skill_level` column not auto-created or in any migration -- INSERT/SELECT at lines 5048, 9076 will fail | CRITICAL | `index.js` missing ADD COLUMN |
| **C5** | `character_inventory.refine_level` not auto-created -- `getPlayerInventory()` SELECT will fail | CRITICAL | `index.js` missing ADD COLUMN |
| **C6** | `character_inventory.compounded_cards` not auto-created -- `getPlayerInventory()` SELECT will fail | CRITICAL | `index.js` missing ADD COLUMN |

### HIGH (data integrity or correctness issues)

| ID | Issue | Severity | Location |
|----|-------|----------|----------|
| **H1** | `characters.deleted` column not in auto-creation block -- soft-delete queries will fail on fresh auto-created DB | HIGH | `index.js` line 31982 block |
| **H2** | `character_inventory.is_broken` not auto-created -- weapon repair query at line 23296 will fail | HIGH | `index.js` missing ADD COLUMN |
| **H3** | Name uniqueness check (line 31754) does not filter `deleted = FALSE` -- deleted character names cannot be reused | HIGH | `index.js` line 31754 |
| **H4** | `character_pets` has no FK on `character_id` -- orphaned pets possible | HIGH | `index.js` line 32163 |
| **H5** | `character_memo` has no FK on `character_id` -- orphaned memo entries possible | HIGH | `index.js` line 32512 |
| **H6** | `items.aspd_modifier` type mismatch: init.sql=FLOAT, auto-creation=INTEGER | HIGH | init.sql vs index.js line 32211 |

### MEDIUM (non-critical but should be fixed)

| ID | Issue | Severity |
|----|-------|----------|
| **M1** | `characters.class` default is 'warrior', should be 'novice' (init.sql) |
| **M2** | `GET /api/characters/:id` (line 31809) returns deleted characters |
| **M3** | `PUT /api/characters/:id/position` (line 31898) updates deleted characters |
| **M4** | `idx_users_username` and `idx_users_email` are redundant (UNIQUE constraint already creates index) |
| **M5** | `items.weapon_type` size mismatch: init.sql=VARCHAR(30), auto=VARCHAR(20) |
| **M6** | `items.weapon_range` default mismatch: init.sql=150, auto=0 |
| **M7** | Multiple columns on `characters` and `character_inventory` are nullable when they should logically be NOT NULL |
| **M8** | `parties` auto-creation (line 32491) omits FK on `leader_id` -- no referential integrity |

---

## 10. Recommendations

### Immediate Fixes (Blocking Issues)

1. **Fix FK references from `characters(id)` to `characters(character_id)`** in:
   - `add_homunculus_table.sql` (line 6)
   - `add_party_system.sql` (lines 7, 16)
   - Server auto-creation at `index.js` line 32118

2. **Add missing auto-creation columns** to `index.js` startup block:
   ```sql
   -- character_inventory
   ALTER TABLE character_inventory
     ADD COLUMN IF NOT EXISTS refine_level INTEGER DEFAULT 0,
     ADD COLUMN IF NOT EXISTS compounded_cards JSONB DEFAULT '[]'::jsonb,
     ADD COLUMN IF NOT EXISTS is_broken BOOLEAN DEFAULT FALSE;

   -- character_hotbar
   ALTER TABLE character_hotbar
     ADD COLUMN IF NOT EXISTS skill_level INTEGER DEFAULT 0;

   -- characters
   ALTER TABLE characters
     ADD COLUMN IF NOT EXISTS deleted BOOLEAN NOT NULL DEFAULT FALSE;
   ```

3. **Fix name uniqueness query** at line 31754:
   ```sql
   SELECT 1 FROM characters WHERE LOWER(name) = LOWER($1) AND deleted = FALSE
   ```

### Short-Term Improvements

4. **Add missing FKs** to `character_pets` and `character_memo`:
   ```sql
   ALTER TABLE character_pets
     ADD CONSTRAINT fk_pets_character
     FOREIGN KEY (character_id) REFERENCES characters(character_id) ON DELETE CASCADE;

   ALTER TABLE character_memo
     ADD CONSTRAINT fk_memo_character
     FOREIGN KEY (character_id) REFERENCES characters(character_id) ON DELETE CASCADE;
   ```

5. **Add composite indexes** for frequent query patterns:
   ```sql
   CREATE INDEX IF NOT EXISTS idx_inventory_char_equipped
     ON character_inventory(character_id, is_equipped);

   CREATE INDEX IF NOT EXISTS idx_inventory_char_item
     ON character_inventory(character_id, item_id);

   CREATE INDEX IF NOT EXISTS idx_characters_user_deleted
     ON characters(user_id, deleted);

   CREATE INDEX IF NOT EXISTS idx_vending_shops_character
     ON vending_shops(character_id);
   ```

6. **Fix items table type mismatches**: Update auto-creation to match init.sql types (VARCHAR(30) for weapon_type, FLOAT for aspd_modifier, DEFAULT 150 for weapon_range).

7. **Add `deleted = FALSE` filter** to `GET /api/characters/:id` and `PUT /api/characters/:id/position`.

8. **Change `characters.class` default** from 'warrior' to 'novice' in init.sql.

### Long-Term Structural Improvements

9. **Create migration files** for `character_pets` and `character_memo` tables (currently auto-creation only).

10. **Remove redundant indexes** `idx_users_username` and `idx_users_email` (covered by UNIQUE constraints).

11. **Add NOT NULL constraints** where appropriate (see Section 5.4).

12. **Consolidate init.sql and auto-creation**: The auto-creation block tries to recreate what init.sql does but with different column lists, types, and defaults. Consider making auto-creation only add columns that are missing from init.sql, and document that init.sql must always be run first.

---

## Appendix: Column Provenance Map

This shows where each column's authoritative definition lives:

| Table.Column | init.sql | Migration | Auto-Create | Server Queries |
|-------------|----------|-----------|-------------|----------------|
| characters.str | comment only | - | YES | YES |
| characters.deleted | YES | YES | **NO** | YES |
| characters.zone_name | - | YES | YES | YES |
| ci.refine_level | - | YES | **NO** | YES |
| ci.compounded_cards | - | YES | **NO** | YES |
| ci.is_broken | - | YES | **NO** | YES |
| ch.skill_level | - | **NO** | **NO** | YES |
| items.ammo_type | YES | - | **NO** | YES |
| items.perfect_dodge_bonus | YES | FIX | **NO** | YES |

**Legend**: ci = character_inventory, ch = character_hotbar
