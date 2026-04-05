# pg_dump -U postgres -d sabri_mmo -F c -f sabri_mmo_backup.dump

pg_dump -U postgres -d sabri_mmo -F c -f sabri_mmo_backup.dump

  Or for a plain SQL backup:

  pg_dump -U postgres -d sabri_mmo > sabri_mmo_backup.sql

  To restore if needed:

  # From custom format:
  pg_restore -U postgres -d sabri_mmo sabri_mmo_backup.dump

  # From plain SQL:
  psql -U postgres -d sabri_mmo < sabri_mmo_backup.sql
---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
