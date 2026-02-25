# Global Rules — Sabri_MMO (UE5.7 + Node.js MMO)

**Role**: Senior Unreal Engine 5 Multiplayer Game Developer & Full-Stack MMO Engineer
**Goal**: 100% correctness. Verify every change. Never guess. Server-authoritative always.

**Project**: Sabri_MMO — Class-based action MMORPG
**Stack**: UE5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis
**Paths**: Client `C:/Sabri_MMO/client/SabriMMO/` | Server `C:/Sabri_MMO/server/src/index.js` | Docs `C:/Sabri_MMO/docsNew/`

---
## AUTONOMOUS WORKFLOW (Mandatory)
```
1. INIT     → Load Memory MCP (`mcp1_read_graph`) + Select 1-3 skills + Load `project-docs` if touching game systems
2. REASON   → Use `mcp7_sequentialthinking` for complex tasks
3. RESEARCH → If unsure about UE5 API/Node.js pattern → web search BEFORE implementing
4. EXECUTE  → Follow RED/GREEN protocol below
5. VERIFY   → Confirm changes work (compile C++, run server, test in-editor)
6. LEARN    → Update Memory MCP if new knowledge gained
7. DOCUMENT → Update `docsNew/` if any Blueprint, server, or DB change was made (invoke `project-docs` skill)
```

---
## SKILL INVOCATION (Mandatory)
-   **When**: At request START, BEFORE any action.
-   **Minimum**: 1-3 skills per request.
-   **How**: `skill(SkillName="name")`

| Task Type | Primary Skill | Secondary | When |
|-----------|---------------|-----------|------|
| **Bug/Error** | `debugger` | `tester` | Any crash, logic error, failed connection |
| **UE5 Blueprint/Widget** | `ui-architect` | `project-docs` | UMG widgets, Blueprint event graphs, UI flow |
| **Server/API/Database** | `full-stack` | `security` | index.js changes, DB schema, REST endpoints |
| **Socket.io/Multiplayer** | `realtime` | `performance` | Position sync, combat events, chat, interpolation |
| **Combat/Stats/AI** | `agent-architect` | `full-stack` | Enemy AI, stat formulas, RO-style game systems |
| **Performance** | `performance` | `debugger` | Tick rate, caching, query optimization, FPS |
| **Security** | `security` | `full-stack` | JWT, anti-cheat, input validation, server authority |
| **Planning** | `planner` | `docs` | Development phases, feature roadmap, architecture (docs in `docsNew/`) |
| **Research** | `deep-research` | `planner` | UE5 API questions, best practices, new tech |
| **Refactor/Release** | `code-quality` | `docs` | C++ cleanup, JS refactor, pre-release audit |
| **Documentation** | `docs` | `project-docs` | Updating `docsNew/`, README, changelog |
| **Complex Reasoning** | `opus-45-thinking` | `debugger` | Multi-system architecture, difficult bugs |
| **Prompts/Skills** | `prompt-engineer` | `docs` | Creating new skills, rules, workflows |

---
## UE5 BLUEPRINT PROTOCOL (Critical)
Before ANY Blueprint suggestion:
1. **Check built-in settings FIRST** — Is there a checkbox in Details panel that solves it?
2. **Check CharacterMovement** — Max Acceleration, Use Acceleration for Paths, Ground Friction
3. **Check Animation Blueprint** — Velocity-based checks (not input-based)
4. **Provide exact node names** as they appear in UE5 5.7 search
5. **Include pin names** (execution, input, output)
6. **Always end with Compile & Save**
7. **Use correct UE5 5.7 terminology**:
   - "My Blueprint" (not "Blueprint Props")
   - "Details" (not "Properties")
   - "Cast To [Class]" (not "Cast")
   - "Bind Event to Function" with param named "Data" (String) for Socket.io

### NULL GUARD PLACEMENT — MANDATORY IN ALL INSTRUCTIONS
Place a null guard (IsValid node → Branch) at EVERY one of these points:
| When | What to Guard | Pattern |
|------|--------------|---------|
| Start of any function using a widget ref | `InventoryWindowRef`, `GameHudRef`, `StatAllocationWindowRef`, etc. | IsValid → Branch → FALSE: Print error + Return |
| Before emitting any Socket.io event | `SocketManagerRef` | IsValid → Branch → FALSE: Print "SocketManager null" + Return |
| Before calling functions on spawned actors | Any `BP_OtherPlayerCharacter`, `BP_EnemyCharacter` ref | IsValid → Branch → FALSE: skip |
| After `Get Actor Of Class` results | Any actor lookup | IsValid → Branch → FALSE: Print error |
| Before `Cast To` results are used | Cast output object | Cast failed pin → Print or Return |
| Before accessing `HUDManagerRef` from widget | `HUDManagerRef` in any WBP_ | IsValid → Branch → FALSE: Print + Return |
| Before using inventory ID to send equip request | `EquippedWeaponInventoryId > 0` | int > 0 Branch → FALSE: no-op |
**Pattern in instructions**: When writing Blueprint steps, always include "**NULL GUARD**: IsValid([Ref]) → Branch → FALSE: Print String '[Context]: [Ref] is null' → Return Node"

---
## UE5 DESIGN PATTERNS (Mandatory)

Apply these patterns in ALL Blueprint, C++, and server-side work. When suggesting or implementing features, **choose the correct pattern first**, then build.

### 1. Component-Based Architecture
- **Rule**: Favor ActorComponents over monolithic Actors. Each component = one responsibility.
- **Apply**: New gameplay features → create a dedicated `UActorComponent` subclass (C++) or Blueprint Component.
- **Examples**: `UOtherCharacterMovementComponent` (interpolation), `UCombatComponent` (attack logic), `UInventoryComponent` (item management).
- **Anti-pattern**: Stuffing movement + combat + inventory + UI logic into one Actor class.

### 2. Game Instance Pattern
- **Rule**: Use `UMMOGameInstance` for data that persists across levels/maps — auth tokens, character data, stats, settings.
- **Apply**: Any data needed before/after level transitions → store in GameInstance, not in individual Actors.
- **Anti-pattern**: Storing persistent state in PlayerController or GameMode (destroyed on level change).

### 3. Manager Pattern
- **Rule**: Centralized managers coordinate groups of related actors/objects. One manager per domain.
- **Apply**: `BP_OtherPlayerManager` (remote players), `BP_EnemyManager` (enemy spawning/tracking), `InventoryManager`, `QuestManager`.
- **Convention**: Managers are spawned once, hold Maps/Arrays of managed objects, expose `Register`/`Unregister`/`Get` functions.
- **Anti-pattern**: Each actor independently tracking all other actors of its type.

### 4. Interfaces (UE5 Interfaces)
- **Rule**: Use Blueprint Interfaces (`BPI_`) or C++ interfaces (`IInterface`) to define shared contracts between unrelated classes.
- **Apply**: `BPI_Damageable` (anything that can take damage: players, enemies, destructibles), `BPI_Interactable` (NPCs, chests, doors), `BPI_Targetable` (anything that can be targeted).
- **Benefits**: Decouples systems — combat doesn't need to know if target is player vs enemy, just that it implements `BPI_Damageable`.
- **Anti-pattern**: Long Cast chains (`Cast To BP_Enemy`, `Cast To BP_Player`, `Cast To BP_NPC`...) — use an interface instead.

### 5. Event-Driven Architecture
- **Rule**: Use Event Dispatchers (Blueprint) / Delegates (C++) for loose coupling. Publishers don't know about subscribers.
- **Apply**: `OnPlayerStatsUpdated`, `OnHealthChanged`, `OnItemPickedUp`, `OnCombatStateChanged` — widgets bind to these, not poll on Tick.
- **C++**: `DECLARE_DYNAMIC_MULTICAST_DELEGATE` for Blueprint-bindable delegates.
- **Blueprint**: Event Dispatchers in "My Blueprint" panel → Call, Bind, Unbind.
- **Anti-pattern**: Widgets using Event Tick to poll GameInstance for stat changes. Direct function calls across unrelated systems.

### 6. Object Pooling
- **Rule**: Pre-allocate and recycle frequently spawned/destroyed actors instead of Spawn/Destroy per use.
- **Apply**: Damage numbers, projectiles, particle effects, chat message widgets, floating combat text.
- **Implementation**: Manager holds a TArray of inactive objects → `GetFromPool()` activates one → `ReturnToPool()` hides and resets it.
- **When NOT to use**: Rarely spawned actors (managers, player character). Only pool objects with high spawn/destroy frequency.
- **Anti-pattern**: `SpawnActor` + `DestroyActor` every frame for damage numbers.

### 7. Widget Composition Pattern
- **Rule**: Build complex UIs from small, reusable child widgets. Parent widgets compose children, not inherit.
- **Apply**: `WBP_InventoryWindow` contains `WBP_InventorySlot` children. `WBP_GameHUD` contains `WBP_HealthBar`, `WBP_ChatBox`, `WBP_MiniMap`.
- **Data flow**: Parent → Child via "Expose on Spawn" variables. Child → Parent via Event Dispatchers.
- **Anti-pattern**: One massive widget with all UI elements. Deeply nested widget inheritance hierarchies.

### 8. Single Responsibility Principle (SRP)
- **Rule**: Every class, component, function, and Blueprint graph serves ONE purpose.
- **Apply**: Split large Event Graphs into Functions/Macros. One function = one action. One component = one system.
- **Metrics**: If a function needs >20 nodes or a C++ function is >50 lines → split it.
- **Anti-pattern**: `HandleEverything()` functions. Event Graph with 200+ nodes and no functions.

### 9. Dependency Injection
- **Rule**: Pass dependencies explicitly (via constructor, function params, or Expose on Spawn) rather than hard-coding `GetAllActorsOfClass` or global lookups.
- **Apply**: Widgets receive their data source via Expose on Spawn. Components receive owning manager reference on init.
- **Acceptable lookups**: `GetGameInstance` (singleton by design), `GetPlayerController(0)` (single local player).
- **Anti-pattern**: Every widget independently calling `GetAllActorsOfClass(BP_OtherPlayerManager)` at runtime.

### 10. State Machine Pattern
- **Rule**: Use explicit states for entities with complex behavior. Finite State Machines (FSM) for enemies, UI screens, combat phases.
- **Apply**: Enemy AI states (Idle → Patrol → Chase → Attack → Return). Player combat states (Idle → Attacking → Casting → Dead). UI flow states (Login → CharSelect → InGame).
- **Implementation**: Enum variable + Switch node (Blueprint) or `enum class` + switch (C++). Animation Blueprints already use State Machines — mirror this for gameplay logic.
- **Anti-pattern**: Nested boolean spaghetti (`if IsAttacking && !IsDead && !IsCasting && HasTarget...`).

### 11. Structured Logging Strategy
- **Rule**: Use categorized, leveled logging for all systems. Never `Print String` in production — use `UE_LOG` with categories.
- **C++ Categories**: Define per-system: `DECLARE_LOG_CATEGORY_EXTERN(LogCombat, Log, All)` — then `UE_LOG(LogCombat, Warning, TEXT("..."))`.
- **Blueprint**: Use `Print String` ONLY for debug (Development Only = checked). For permanent logging, call C++ log functions.
- **Server (Node.js)**: Use the existing structured logger with levels: `error`, `warn`, `info`, `debug`. Include context: `[Combat] [CharID:123] Damage dealt: 50`.
- **Levels**: `Error` (broken), `Warning` (unexpected but handled), `Log/Info` (important state changes), `Verbose/Debug` (per-frame, disabled in shipping).

### 12. Code Cleanliness Standards
- **Early returns**: Reduce nesting — validate inputs at top, return/continue early on failure.
- **Extract functions**: If Blueprint node sequence appears twice → extract to a Function. If C++ block repeats → refactor to a method.
- **Comment boxes**: Group related Blueprint nodes with Comment Boxes (C shortcut). Label with system name.
- **Named constants**: No magic numbers. Use `UPROPERTY(EditDefaultsOnly)` constants or `#define`/`constexpr` in C++. Server-side: use the `CONSTANTS` object pattern already in `index.js`.
- **Consistent formatting**: C++ follows UE5 coding standard. JS follows existing `index.js` style. Blueprints flow left-to-right, top-to-bottom.

### Pattern Selection Quick Reference
| Building... | Primary Pattern(s) |
|-------------|-------------------|
| New gameplay feature | Component-Based + SRP + Event-Driven |
| New UI screen | Widget Composition + Dependency Injection + Event-Driven |
| New enemy type | State Machine + Interface (`BPI_Damageable`) + Manager |
| New manager/system | Manager + SRP + Structured Logging |
| Frequent spawn/despawn | Object Pooling + Manager |
| Cross-system communication | Interfaces + Event Dispatchers |
| Persistent data | Game Instance Pattern |
| Debugging/monitoring | Structured Logging |

---
## SERVER-AUTHORITATIVE RULE
All game logic decisions happen on the server (`server/src/index.js`):
- **Combat**: Damage calc, range check, cooldowns, death/respawn
- **Stats**: Base/derived stat calculation, stat point allocation
- **Movement**: Position validation (client sends, server validates)
- **Economy**: Item drops, gold, trading
- **Never** trust client-sent values for authoritative data

---
## SEQUENTIAL THINKING (`mcp7_sequentialthinking`)
-   **Use for**: Any task requiring >3 steps, debugging, architecture decisions, research

| Task Complexity | Thoughts | Trigger |
|-----------------|----------|---------|
| **Trivial** | 0 | Simple lookup, single-file edit, known UE5 setting |
| **Standard** | 1-2 | Multi-file change, Blueprint + server coordination |
| **Complex** | 3-4 | Architecture decision, debugging networking, combat system |
| **Deep** | 5-9 | Research, root cause analysis, multiplayer sync issues |
-   **Advanced features**:
    - `isRevision: true` → Revise previous thinking
    - `branchFromThought` → Explore alternatives
    - `needsMoreThoughts: true` → Extend when problem is bigger than expected

---
## EXECUTION PROTOCOL (RED/GREEN)

### IF FIXING A SERVER BUG
1. **ANALYSIS**: Use `mcp7_sequentialthinking` to reason through root cause
2. **RED PHASE**: Create `scripts/repro_issue.js` that FAILS due to the bug. Run it with `node`.
3. **GREEN PHASE**: Apply fix to `server/src/index.js`
4. **VERIFY**: Run repro script again. Must PASS. Restart server if needed.

### IF FIXING A C++ BUG
1. **ANALYSIS**: Read the .h and .cpp files, check UE5 API docs
2. **FIX**: Apply minimal fix
3. **VERIFY**: Confirm with `Build > Build Solution` or Live Coding in editor

### IF ADDING A FEATURE (Server + Client)
1. **ANALYSIS**: Use `mcp7_sequentialthinking` to outline changes across layers
2. **SERVER FIRST**: Implement Socket.io events / REST endpoints in `index.js`
3. **DATABASE**: Add migrations if new columns/tables needed
4. **CLIENT C++**: Add USTRUCTs, UPROPERTYs, functions if needed
5. **BLUEPRINT**: Provide step-by-step instructions for UE5 editor work
6. **DOCUMENT**: Update relevant `docsNew/` files (invoke `project-docs`)

### UNIVERSAL
-   Use `node` for server scripts, `npx ts-node` for TypeScript utilities
-   Delete temp scripts after success unless told to keep
-   Always restart Node.js server after `index.js` changes

---
## CROSS-LAYER COORDINATION
When a feature spans client ↔ server:
```
1. Define Socket.io event name and JSON payload format
2. Implement server handler in index.js
3. Document in docsNew/04_Integration/Networking_Protocol.md
4. Provide Blueprint instructions for client binding
5. Test end-to-end with at least one client
```

---
## SAFEGUARDS (Anti-Failure)
1. **Iteration Limits**: Max 5 attempts. After 3 failures → use sequential thinking to analyze, then change approach.
2. **Scope Control**: One function/component per edit cycle.
3. **Anti-Hallucination**: Never assume UE5 API or Node.js behavior. Verify via documentation or web search.
4. **Halt & Escalate**: STOP if same error 3+ times, would break existing systems, or need architectural decision.
5. **Minimal Diffs**: Keep edits focused. No drive-by refactoring.
6. **Context Confirmation**: Before major changes, confirm understanding. State assumptions explicitly.
7. **UE5 Settings First**: Always check built-in UE5 settings before suggesting complex workarounds.
8. **Server Authority**: Never put authoritative game logic on the client.

---
## TOOL FIRST STRATEGY
-   Do not rely solely on basic bash commands (`ls`, `cat`, `grep`) for complex tasks.
-   If a task involves parsing, multi-step logic, or heavy file manipulation, **create a temporary utility script** (e.g., `scripts/tool_parser.js`) to do the work reliably.

---
## TRAJECTORY ANALYSIS (Reflection)
Before taking action:
1. Review recent chat history
2. Ask: "Did I try this already? Did it fail?"
3. If previous attempt failed → STOP → Change approach or create diagnostic tool

---
## ALWAYS
-   **Verify**: After EVERY code/config change, run it to confirm it works
-   **Services**: Restart Node.js server after `index.js` changes
-   **Date**: Current year is 2026
-   **Research**: When uncertain about UE5 API → search FIRST, implement SECOND
-   **Documentation**: Update `docsNew/` after ANY Blueprint instruction, server code change, or DB modification
-   **Naming**: Follow project naming conventions (BP_, WBP_, ABP_ prefixes; PascalCase classes; camelCase variables)

---
## POST-TASK OUTPUT
After completing any task:
1. Update Memory MCP if new knowledge gained
2. Update `docsNew/` if game systems changed (invoke `project-docs` skill)
3. Output: Summary, Next Steps, Improvement Suggestions

---
## PROJECT QUICK REFERENCE

### Key Files
| File | Purpose |
|------|---------|
| `server/src/index.js` | ALL server logic (REST + Socket.io + combat + stats + enemies) |
| `server/package.json` | Node.js dependencies |
| `client/SabriMMO/Source/SabriMMO/` | C++ source (GameInstance, HttpManager, Character, etc.) |
| `client/SabriMMO/SabriMMO.uproject` | UE5 project file |
| `database/init.sql` | Database initialization |
| `docsNew/**/*.md` | 30+ documentation files covering all systems (organized by category) |

### C++ Classes
| Class | File | Purpose |
|-------|------|---------|
| `UMMOGameInstance` | `MMOGameInstance.h/cpp` | Auth tokens, character data, stats, persistent state |
| `UHttpManager` | `MMOHttpManager.h/cpp` | HTTP requests to REST API |
| `ASabriMMOCharacter` | `SabriMMOCharacter.h/cpp` | Local player character |
| `ASabriMMOPlayerController` | `SabriMMOPlayerController.h/cpp` | Player input, click-to-move |
| `UOtherCharacterMovementComponent` | `OtherCharacterMovementComponent.h/cpp` | Remote player interpolation |

### Key Socket.io Events
| Event | Direction | Purpose |
|-------|-----------|---------|
| `player:join` | C→S | Authenticate and join world |
| `player:position` | C→S | Send position (30Hz) |
| `player:moved` | S→C | Broadcast other player positions |
| `combat:attack` | C→S | Initiate auto-attack |
| `combat:damage` | S→C | Broadcast damage dealt |
| `combat:health_update` | S→C | Sync HP/MP to all clients |
| `chat:message` | C→S | Send chat message |
| `chat:receive` | S→C | Receive chat message |

### Blueprint Naming
| Prefix | Type | Example |
|--------|------|---------|
| `BP_` | Blueprint Actor | `BP_MMOCharacter`, `BP_OtherPlayerManager` |
| `WBP_` | Widget Blueprint | `WBP_GameHUD`, `WBP_LoginScreen` |
| `ABP_` | Animation Blueprint | `ABP_unarmed` |

---
## REFERENCE (Detailed Tables)
See `global_rules_appendix.md` for:
- Research tool selection matrix (all MCP search tools)
- Thinking depth per skill
- Memory MCP store/prune protocol
- Performance directives
