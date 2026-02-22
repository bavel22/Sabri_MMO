# ID Reference — Sabri_MMO

Comprehensive ID ranges for all game entities. Use these when writing server logic, Blueprint parsers, or database queries.

---

## Item IDs

Item IDs are stored in the `items` table (`item_id` column) and mapped client-side via `DT_ItemIcons` (row names are string versions of item_id).

### Range Overview

| Range | Category | Count |
|-------|----------|-------|
| 1001–1005 | Consumables | 5 |
| 2001–2008 | Loot / Misc drops | 8 |
| 3001–3006 | Weapons | 6 |
| 4001–4003 | Armor | 3 |
| **Total** | | **22** |

---

### Consumables (1001–1005)

| item_id | Name | Effect | HP Restore | SP Restore | Stack | Price |
|---------|------|--------|-----------|-----------|-------|-------|
| 1001 | Crimson Vial | Red HP potion | +50 HP | — | 99 | 25 |
| 1002 | Amber Elixir | Orange HP potion | +150 HP | — | 99 | 100 |
| 1003 | Golden Salve | Large HP potion | +350 HP | — | 99 | 275 |
| 1004 | Azure Philter | SP (mana) potion | — | +60 SP | 99 | 500 |
| 1005 | Roasted Haunch | Food item | +70 HP | — | 99 | 25 |

**Server restore logic** (`inventory:use` handler):
```javascript
const hpRestore = { 1001: 50, 1002: 150, 1003: 350, 1005: 70 };
// 1004 → spRestored = 60
```

---

### Loot / Misc Items (2001–2008)

Dropped by enemies. Sold to NPCs. All stackable.

| item_id | Name | Icon Asset | Drops From | Stack Max |
|---------|------|-----------|-----------|----------|
| 2001 | Gloopy Residue | Icon_GloopyResidue | Blobby (70%), Hoplet (50%), Crawlid (40%) | 999 |
| 2002 | Viscous Slime | Icon_ViscousSlime | Blobby (15%), Mosswort (15%) | 999 |
| 2003 | Chitin Shard | Icon_ChitinShard | Crawlid (50%) | 999 |
| 2004 | Downy Plume | Icon_DownyPlume | Hoplet (30%), Buzzer (30%) | 999 |
| 2005 | Spore Cluster | Icon_SporeCluster | Shroomkin (55%) | 999 |
| 2006 | Barbed Limb | Icon_BarbedLimb | Crawlid (25%), Buzzer (45%) | 999 |
| 2007 | Verdant Leaf | Icon_VerdantLeaf | Hoplet (10%), Shroomkin (20%), Mosswort (25%) | 99 |
| 2008 | Silken Tuft | Icon_SilkenTuft | Mosswort (60%) | 999 |

---

### Weapons (3001–3006)

All weapons use `equip_slot = 'weapon'`. Non-stackable.

| item_id | Name | Icon Asset | Type | ATK | Range | ASPD Mod | Req Lvl |
|---------|------|-----------|------|-----|-------|---------|---------|
| 3001 | Rustic Shiv | Icon_RusticShiv | dagger | 17 | 150 | +5 | 1 |
| 3002 | Keen Edge | Icon_KeenEdge | dagger | 30 | 150 | +5 | 1 |
| 3003 | Stiletto Fang | Icon_StilettoFang | dagger | 43 | 150 | +5 | 12 |
| 3004 | Iron Cleaver | Icon_IronCleaver | 1h_sword | 25 | 150 | 0 | 2 |
| 3005 | Crescent Saber | Icon_CrescentSaber | 1h_sword | 49 | 150 | 0 | 18 |
| 3006 | Hunting Longbow | Icon_HuntingLongbow | bow | 35 | 800 | -3 | 4 |

**Notes**:
- `range: 150` = melee range (`COMBAT.MELEE_RANGE`)
- `range: 800` = bow range (only Hunting Longbow)
- ASPD Mod: dagger +5 (faster), bow -3 (slower), sword 0 (neutral)

---

### Armor (4001–4003)

All armor uses `equip_slot = 'armor'`. Non-stackable.

| item_id | Name | Icon Asset | DEF | Req Lvl |
|---------|------|-----------|-----|---------|
| 4001 | Linen Tunic | Icon_LinenTunic | 1 | 1 |
| 4002 | Quilted Vest | Icon_QuiltedVest | 4 | 1 |
| 4003 | Ringweave Hauberk | Icon_RingweaveHauberk | 8 | 20 |

---

### Future ID Ranges (Reserved)

| Range | Planned Category |
|-------|-----------------|
| 1006–1099 | More consumables |
| 2009–2099 | More loot/misc |
| 3007–3099 | More weapons |
| 4004–4099 | More body armor |
| 5001–5099 | Head equipment (reserved) |
| 6001–6099 | Garment/cape (reserved) |
| 7001–7099 | Footgear (reserved) |
| 8001–8099 | Accessories/rings (reserved) |

---

## Enemy IDs

Enemy IDs are **runtime-generated** — they are NOT stored in the database. They exist only in the server's `enemies` Map for the lifetime of the server process.

### Runtime ID Allocation

```javascript
let nextEnemyId = 2000001;  // Starting value
// Each spawnEnemy() call: enemyId = nextEnemyId++
```

- First enemy spawned: `enemyId = 2000001`
- IDs are sequential and increment per spawn (including respawns after death)
- IDs reset to `2000001` on server restart
- **Do NOT store enemy IDs in the database** — they are ephemeral

### Enemy Templates

Enemy types are defined by string template keys (not numeric IDs).

| Template Key | Name | Level | HP | Damage | AI Type | Aggro Range | Exp | Respawn |
|-------------|------|-------|----|--------|---------|------------|-----|---------|
| `blobby` | Blobby | 1 | 50 | 1 | passive | 300 | 10 | 10s |
| `crawlid` | Crawlid | 2 | 75 | 2 | passive | 0 (no aggro) | 15 | 12s |
| `hoplet` | Hoplet | 3 | 100 | 3 | passive | 400 | 25 | 15s |
| `mosswort` | Mosswort | 3 | 5 | 2 | passive | 0 (no aggro) | 20 | 12s |
| `shroomkin` | Shroomkin | 4 | 120 | 4 | passive | 350 | 30 | 15s |
| `buzzer` | Buzzer | 5 | 150 | 5 | **aggressive** | 500 | 40 | 18s |

### Spawn Configuration (Current Map)

| Template | Count | General Location |
|---------|-------|-----------------|
| `blobby` | 3 | Center area (500,500), (-500,300), (200,-400) |
| `hoplet` | 2 | East/West mid (800,-200), (-700,-500) |
| `crawlid` | 2 | North area (-300,800), (400,700) |
| `shroomkin` | 2 | Far East/West (1000,300), (-900,100) |
| `buzzer` | 1 | Far SE (1200,-600) |
| `mosswort` | 2 | South area (0,-800), (600,-900) |
| **Total** | **12** | |

### Enemy Drop Tables

| Template | Item | Drop Chance | Qty |
|---------|------|------------|-----|
| blobby | Gloopy Residue (2001) | 70% | 1–2 |
| blobby | Viscous Slime (2002) | 15% | 1 |
| blobby | Crimson Vial (1001) | 5% | 1 |
| blobby | Rustic Shiv (3001) | 1% | 1 |
| hoplet | Gloopy Residue (2001) | 50% | 1 |
| hoplet | Downy Plume (2004) | 30% | 1 |
| hoplet | Verdant Leaf (2007) | 10% | 1 |
| hoplet | Crimson Vial (1001) | 8% | 1 |
| hoplet | Keen Edge (3002) | 1% | 1 |
| crawlid | Chitin Shard (2003) | 50% | 1 |
| crawlid | Gloopy Residue (2001) | 40% | 1 |
| crawlid | Barbed Limb (2006) | 25% | 1 |
| crawlid | Crimson Vial (1001) | 5% | 1 |
| shroomkin | Spore Cluster (2005) | 55% | 1 |
| shroomkin | Verdant Leaf (2007) | 20% | 1–2 |
| shroomkin | Crimson Vial (1001) | 10% | 1 |
| shroomkin | Linen Tunic (4001) | 2% | 1 |
| buzzer | Barbed Limb (2006) | 45% | 1 |
| buzzer | Downy Plume (2004) | 30% | 1 |
| buzzer | Amber Elixir (1002) | 5% | 1 |
| buzzer | Iron Cleaver (3004) | 2% | 1 |
| buzzer | Quilted Vest (4002) | 1% | 1 |
| buzzer | Hunting Longbow (3006) | 0.5% | 1 |
| mosswort | Silken Tuft (2008) | 60% | 1–3 |
| mosswort | Verdant Leaf (2007) | 25% | 1 |
| mosswort | Viscous Slime (2002) | 15% | 1 |
| mosswort | Roasted Haunch (1005) | 8% | 1 |

---

## Character IDs

Stored in the `characters` table (`character_id` column, auto-increment from PostgreSQL).

| Range | Notes |
|-------|-------|
| 1–∞ | Sequential from DB auto-increment. No reserved ranges. |

**Test character IDs** created by `create_test_users.js` start at whatever the DB sequence is at creation time (typically 1–10 during dev).

---

## Socket / Player Session IDs

Socket.io assigns string IDs (e.g., `"abc123XYZ"`). These are **not** stored in the database and reset on reconnect.

---

## Related Files

| File | Purpose |
|------|---------|
| `docsNew/03_Server_Side/Inventory_System.md` | Full item system docs (equip logic, loot system, socket events) |
| `docsNew/03_Server_Side/Enemy_System.md` | Enemy spawning, AI, aggro, combat |
| `docsNew/02_Client_Side/Blueprints/07_Widget_Blueprints.md` | DT_ItemIcons data table (client icon mapping) |
| `server/src/index.js` | ENEMY_TEMPLATES, item definitions, nextEnemyId |
| `database/init.sql` | Items table seed data |

---

**Last Updated**: 2026-02-19  
**Version**: 1.0  
**Status**: Complete
