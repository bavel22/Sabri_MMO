# Obsidian + Claude Code Workflow

## How the Two Systems Work Together

```
┌─────────────────────┐     ┌─────────────────────┐
│     OBSIDIAN         │     │    CLAUDE CODE       │
│  (Your workspace)    │     │  (AI workspace)      │
│                      │     │                      │
│  _journal/           │     │  ~/.claude/memory/   │
│   ├── Dashboard.md   │     │   ├── 57 memory files│
│   ├── Session Tracker│     │   └── MEMORY.md      │
│   └── 2026-03-21.md  │     │                      │
│                      │     │  Reads your vault:   │
│  _prompts/           │◄────┤   ├── CLAUDE.md      │
│   └── good prompts   │     │   ├── docsNew/       │
│                      │     │   ├── Dev Notes/     │
│  docsNew/            │     │   └── server/src/    │
│   └── INDEX.md       │     │                      │
└─────────────────────┘     └─────────────────────┘
```

**Obsidian** = your view (planning, tracking, reviewing)
**Claude Code memory** = Claude's view (what it remembers across sessions)
**Your vault files** = shared space both can read/write

---

## Before a Session

1. **Open Dashboard** (`_journal/Dashboard.md`) — check "What's Next"
2. **Open Daily Note** (`Ctrl+P` > "Open daily note") — write your goal
3. **Check Session Tracker** if you want to resume a previous session:
   ```bash
   claude --resume <id-from-tracker>
   ```

## During a Session

### Saving the resume ID
When you start Claude Code, note the session. You can find the current session ID later with:
```bash
# The session ID shows in the Claude Code header, or check:
ls -lt ~/.claude/sessions/ | head -1
cat ~/.claude/sessions/<latest>.json
```
Paste the resume command in your daily note.

### When Claude produces good output
1. If it's a **reusable prompt** → copy to `_prompts/` with a descriptive filename
2. If it's a **key finding/decision** → note in your daily note under "Key Decisions"
3. If it's a **session summary** → Claude's memory will auto-save it, but note highlights in your daily note for your own reference

### When something doesn't work
Note it under "Issues / Blockers" — helps you remember what to try differently next session.

## After a Session

1. **Fill in daily note**: What Got Done, Next Session
2. **Add to Session Tracker**: Resume ID + one-line summary
3. **Update Dashboard**: Move completed items, add new next steps
4. If a prompt worked exceptionally well → move to `_prompts/`

## Tips

### Don't duplicate Claude's memory
Claude Code automatically remembers:
- Bug fixes and what caused them
- Architecture decisions and why
- Your preferences and corrections
- System implementation details

You do NOT need to write these down — Claude will recall them. Instead, focus your notes on:
- Your priorities and planning (Claude doesn't know these)
- Which sessions to resume for what topic
- Prompts that worked (Claude forgets the exact prompt phrasing)
- High-level progress tracking

### Use Obsidian's strengths
- **Backlinks** (`[[Dashboard]]` in any note links back)
- **Graph View** (`Ctrl+G`) to see how systems connect
- **Tags** — add `#bug`, `#feature`, `#blocked` to daily notes for filtering
- **Search** (`Ctrl+Shift+F`) — find anything across all docs
- **Bookmarks** — pin Dashboard, Session Tracker, INDEX.md

### Use Claude Code's strengths
- **Memory** — it remembers what you did across sessions automatically
- **Skills** — `/sabrimmo-*` skills load detailed context per system
- **CLAUDE.md** — Claude reads this every session, so put permanent rules there
- **Agents** — use background agents for parallel research/audits

### Keyboard shortcuts
| Action | Shortcut |
|--------|----------|
| Open daily note | `Ctrl+P` > "daily" |
| Quick switcher | `Ctrl+O` |
| Search all files | `Ctrl+Shift+F` |
| Graph view | `Ctrl+G` |
| Bookmarks | `Ctrl+P` > "bookmarks" |
| Split pane | Drag tab to edge |
| Toggle edit/preview | `Ctrl+E` |
