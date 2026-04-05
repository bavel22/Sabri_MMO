# Session Organization Prompt

Use at the start of a new day to organize yesterday's work.

## Prompt

```
can you look at the work done yesterday and organize it. record the sessions, what was the goal and accomplished/done in each sessions, and add the blockers to the proper locations so we can come back to them later, file any prompts recorded away, fill in the session details sections, and add the next sessions to the list of things to do. anything that wasnt completed should be added to the next day's daily log. make sure you create the next day log
```

## What It Does
- Reads the previous day's daily note from `_journal/`
- Cross-references git history for commits
- Fills in session details (goal, what was done per session)
- Adds sessions to `_journal/Session Tracker.md`
- Updates `_journal/Dashboard.md` recent sessions
- Files any prompts from "Prompts That Worked Well" to `_prompts/`
- Identifies blockers and adds them to Dashboard or daily note
- Adds next session items to the daily note and Dashboard
