---
name: Claude Code Opus 4.6 1M Setup
description: How to configure Claude Code CLI for Opus 4.6 with 1M context, max thinking, and 128K output tokens
type: reference
---

# Claude Code CLI — Opus 4.6 1M Context Setup

As of **March 13, 2026**, the 1M context window is GA for Opus 4.6 and Sonnet 4.6. No beta header, no premium pricing.

## Settings to change in `~/.claude/settings.json`

### 1. Model — 1M context window

```json
"model": "opus[1m]"
```

- Native 1M context, no beta header needed
- Standard pricing: $5/$25 per MTok (flat, even at 900K tokens)
- The old `context-1m-2025-08-07` beta header is deprecated for Opus 4.6 / Sonnet 4.6

### 2. Effort level — max thinking

```json
"effortLevel": "max"
```

Also set the env var for consistency:

```json
"env": {
  "CLAUDE_CODE_EFFORT_LEVEL": "max"
}
```

This maps to the API as:
- `thinking: {"type": "adaptive"}` — Claude decides when/how much to think (the **mode**)
- `effort: "max"` — maximum reasoning intensity (the **dial**)

**Note:** `{"type": "enabled", "budget_tokens": N}` is deprecated on Opus 4.6. Adaptive + max is the correct way to get maximum thinking.

### 3. Max output tokens — 128K

```json
"env": {
  "CLAUDE_CODE_MAX_OUTPUT_TOKENS": "128000"
}
```

Opus 4.6 supports up to 128K output tokens. The previous default was 64K.

## Complete minimal settings diff

```json
{
  "env": {
    "CLAUDE_CODE_EFFORT_LEVEL": "max",
    "CLAUDE_CODE_MAX_OUTPUT_TOKENS": "128000"
  },
  "model": "opus[1m]",
  "effortLevel": "max"
}
```

## API equivalent (for reference)

```python
response = client.messages.create(
    model="claude-opus-4-6",
    max_tokens=128000,
    thinking={"type": "adaptive"},
    effort="max",
    messages=[{"role": "user", "content": "..."}],
)
```

No beta header. No `betas` array. No `extra_headers`. Just the model ID.

## Pricing (GA as of March 13, 2026)

| Model | Input | Output | Context |
|-------|-------|--------|---------|
| Opus 4.6 | $5/MTok | $25/MTok | 1M (flat, no surcharge) |
| Sonnet 4.6 | $3/MTok | $15/MTok | 1M (flat, no surcharge) |
| Sonnet 4.5 / 4 | $3/MTok (std) / $6 (>200K) | $15 (std) / $22.50 (>200K) | 1M (beta header required) |

Old premium rates ($10/$37.50 for Opus, $6/$22.50 for Sonnet) only apply to Sonnet 4.5 and Sonnet 4 with the legacy beta header.

## Source

- Official blog: https://claude.com/blog/1m-context-ga
- Pricing docs: https://platform.claude.com/docs/en/about-claude/pricing
- Context windows docs: https://platform.claude.com/docs/en/build-with-claude/context-windows
