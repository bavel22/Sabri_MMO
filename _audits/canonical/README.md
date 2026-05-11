# Canonical Pre-Renewal Reference Data

This directory holds canonical Ragnarok Online Pre-Renewal reference data used by audit scripts. The source of truth is **rAthena's `db/pre-re/` directory** (`mob_db.txt`, `item_db.yml`, `skill_db.txt`, `exp.txt`, etc.).

## Files

- `mob_db_pre_re_tier1.json` — Verified canonical stats for ~15 Tier-1 monsters (lvl 1-15) used by `audit_monster_stats.js`. Hand-curated from rAthena pre-re mob_db.txt. Each entry annotated with `verified: true|false` and `source` line.
- `class_hp_sp_canonical.json` — Per-class HP_JOB_A / HP_JOB_B / SP_JOB coefficients per iRO Wiki / rAthena `job_db1.txt`.
- `aspd_base_delays_canonical.json` — Per-class+weapon BTBA delays per rAthena `job_db2_pre.txt`.
- `exp_table_canonical.json` — Sample base/job EXP requirements per `exp.txt`.
- `refine_costs_canonical.json` — Per-weapon-level ore + zeny costs per iRO Wiki Refine guide.

## Extending the reference

For full project audit, populate these files from a rAthena clone:

```bash
# Once you have a local rAthena pre-re clone:
node scripts/extract_canonical_mob_db.js \
  --src ~/rathena/db/pre-re/mob_db.txt \
  --out _audits/canonical/mob_db_pre_re_full.json
```

The audit scripts gracefully fall back to whatever canonical entries exist — they only report monsters they have canonical data for.

## Why some monsters are skipped

Some templates are project-specific variants (e.g., `meta_fabre`, `provoke_*`, custom event mobs) that have no canonical equivalent. These are intentionally NOT in the canonical file.
