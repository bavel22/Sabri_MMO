# MCP Servers for UE5 Game Development with Claude Code

## Comprehensive Research Report — March 2026

---

## TIER 1: CRITICAL — Direct Impact on Your Stack

These MCPs directly integrate with your existing Sabri_MMO stack (UE5 C++ / Node.js / PostgreSQL / Redis / Socket.io).

| # | MCP Server | Description | How It Helps Your MMO | GitHub / Source |
|---|-----------|-------------|----------------------|----------------|
| 1 | **Context7** (Upstash) | Fetches current, version-specific docs for 9,000+ libraries. 44k stars. Free, no API key. | Ensures Claude uses correct Socket.io v4, Express v5, UE5.7 C++ APIs instead of hallucinating deprecated methods. Prevents subtle API version bugs in your 2269-line index.js. | [github.com/upstash/context7](https://github.com/upstash/context7) |
| 2 | **PostgreSQL MCP** (Official Anthropic) | Read-only PostgreSQL access with schema introspection. Part of official MCP servers repo (18k stars). | Directly query your `sabri_mmo` database — inspect `users`, `characters`, `items`, `character_inventory` tables, debug stat columns, validate migration results, all from Claude Code. | [github.com/modelcontextprotocol/servers/src/postgres](https://github.com/modelcontextprotocol/servers/tree/main/src/postgres) |
| 3 | **Redis MCP** (Official Redis) | Full Redis data structure support — hashes, lists, sets, sorted sets, streams, sessions. | Inspect your player sessions, cached combat tick data, rate limiting state, enemy spawn cache, and real-time game state. Debug your existing Redis usage without switching tools. | [github.com/redis/mcp-redis](https://github.com/redis/mcp-redis) |
| 4 | **Unreal Analyzer MCP** | UE5 C++ source code analysis using Tree-sitter. Class hierarchy, reference finding, subsystem analysis, pattern detection. 137 stars. | Analyze your SabriMMO C++ classes (SabriMMOCharacter, MMOHttpManager, all Slate widgets). Map inheritance chains, find all UPROPERTY/UFUNCTION usages, detect anti-patterns. | [github.com/ayeletstudioindia/unreal-analyzer-mcp](https://github.com/ayeletstudioindia/unreal-analyzer-mcp) |
| 5 | **mcp-cpp** (clangd) | Semantic C++ analysis via clangd LSP. Symbol search, inheritance trees, call hierarchies. 60 stars, 218 commits. | Point at your UE5 C++ code for deep semantic understanding — navigate complex UE macros, template instantiations, find all callers of `HandleSomeEvent()`, trace subsystem dependencies. | [github.com/mpsm/mcp-cpp](https://github.com/mpsm/mcp-cpp) |
| 6 | **UnrealClaude** (Natfii) | Claude Code CLI integration for UE5.7 specifically. 20+ tools + dynamic UE5.7 API documentation context. | Built for your exact UE version (5.7). Provides accurate Slate, Actor, Blueprint, Animation, Replication API docs on demand — eliminates guessing at UE5.7-specific APIs. | [github.com/Natfii/UnrealClaude](https://github.com/Natfii/UnrealClaude) |

---

## TIER 2: HIGH VALUE — Significant Productivity Gains

| # | MCP Server | Description | How It Helps Your MMO | GitHub / Source |
|---|-----------|-------------|----------------------|----------------|
| 7 | **Blender MCP** (ahujasid) | Natural language 3D modeling in Blender. Poly Haven + Sketchfab + Hyper3D asset gen. **17,500 stars** — the most popular 3D MCP. | Create 3D models for your MMO — weapons, armor, environment props, RO-style monsters — via text prompts. Export FBX/OBJ directly to UE5. Generate prototype assets instantly. | [github.com/ahujasid/blender-mcp](https://github.com/ahujasid/blender-mcp) |
| 8 | **ComfyUI MCP** (joenorton) | Bridge to local ComfyUI for Stable Diffusion. 70+ workflows, supports Flux/SDXL/SD3.5. **Free (local GPU)**. 210 stars. | Generate game textures (stone, wood, metal), item icons, concept art, UI mockups, character portraits — all locally, no API costs. Batch generate tileables for UE5 materials. | [github.com/joenorton/comfyui-mcp-server](https://github.com/joenorton/comfyui-mcp-server) |
| 9 | **Game Asset MCP** | Purpose-built for game assets. 2D sprites (pixel art via Flux LoRA) + 3D models (OBJ/GLB via Hunyuan3D-2). 115 stars. | Generate RO-style sprites for UI, 3D weapon/armor models ready for UE5 import. Text → game asset pipeline designed specifically for game devs. | [github.com/MubarakHAlketbi/game-asset-mcp](https://github.com/MubarakHAlketbi/game-asset-mcp) |
| 10 | **ElevenLabs MCP** (Official) | TTS, voice cloning, transcription, soundscapes. 1,200 stars. **10k free credits/month**. | Generate NPC dialogue voices, narrator lines, unique character voices for your MMO. Create ambient soundscapes for zones (Prontera market, dungeon depths). | [github.com/elevenlabs/elevenlabs-mcp](https://github.com/elevenlabs/elevenlabs-mcp) |
| 11 | **Figma MCP** (Official) | Exposes live Figma design data — layout, styles, components, hierarchy — to AI. | Design your MMO UI panels (inventory, skill tree, party window) in Figma, then Claude translates the exact specs into Slate/UMG code. Eliminates guesswork translating mockups to code. | [developers.figma.com/docs/figma-mcp-server](https://developers.figma.com/docs/figma-mcp-server/) |
| 12 | **Sentry MCP** (Official) | Error tracking with AI root cause analysis. 16 tools for issue investigation. | Track Node.js server crashes, debug multi-player race conditions (like your previous EXCEPTION_ACCESS_VIOLATION fix), monitor error trends across releases. | [docs.sentry.io/product/sentry-mcp](https://docs.sentry.io/product/sentry-mcp/) |
| 13 | **GitHub MCP** (Official) | GitHub's official MCP — 80+ tools for repos, PRs, issues, code search. 13k stars. | Manage your Sabri_MMO repo, create issues from bugs found during dev, review PRs, search code patterns across the codebase, check CI status. | [github.com/github/github-mcp-server](https://github.com/github/github-mcp-server) |
| 14 | **Sequential Thinking MCP** (Official Anthropic) | Structured step-by-step reasoning with branching, revision, and hypothesis testing. | Complex MMO architecture decisions — combat formula balancing, database schema migrations, multi-system features (class/skill/equipment interactions), debugging race conditions. | [github.com/modelcontextprotocol/servers/src/sequentialthinking](https://github.com/modelcontextprotocol/servers/tree/main/src/sequentialthinking) |
| 15 | **ESLint MCP** (Official) | Built-in ESLint CLI MCP server. Lint + auto-fix JavaScript/TypeScript. | Lint your 2269-line `server/src/index.js` and all server modules (`ro_skill_data.js`, `ro_monster_templates.js`, etc.) directly from Claude Code. | [eslint.org/docs/latest/use/mcp](https://eslint.org/docs/latest/use/mcp) |

---

## TIER 3: VALUABLE — Strong Use Cases for Game Dev

| # | MCP Server | Description | How It Helps Your MMO | GitHub / Source |
|---|-----------|-------------|----------------------|----------------|
| 16 | **UnrealProjectAnalyzer** (syan2018) | Cross-domain analysis: Blueprint <-> C++ <-> Asset reference tracing. | Traces reference chains across your mixed BP/C++ architecture. Find which Blueprint references which C++ class, which asset is used where. Essential for refactoring. | [lobehub.com/mcp/syan2018-unrealprojectanalyzer](https://lobehub.com/mcp/syan2018-unrealprojectanalyzer) |
| 17 | **Firecrawl MCP** | Web scraping returning clean LLM-ready markdown. Site crawling, structured data extraction. Free tier: 10 scrapes/min. | Scrape RO wikis (iRO Wiki, Divine-Pride.net) for monster stats, item data, skill formulas. Feed data directly into your `ro_monster_templates.js` or `ro_skill_data.js`. | [github.com/firecrawl/firecrawl-mcp-server](https://github.com/firecrawl/firecrawl-mcp-server) |
| 18 | **Stability AI MCP** | Image gen/edit/upscale via Stability AI API. Outpainting, sketch-to-image, 4K upscaling. | Generate high-quality textures with upscaling to 4K for UE5 materials. Outpainting for seamless tileables. Sketch-to-image for rapid concept iteration. | [github.com/tadasant/mcp-server-stability-ai](https://github.com/tadasant/mcp-server-stability-ai) |
| 19 | **PiAPI MCP** (Multi-Model) | Unified API for Midjourney + Flux + Kling + LumaLabs + Suno + Udio + Trellis. One server, all media. | One MCP for everything — images (textures), video (cutscenes), music (zone BGM), 3D models (game assets). Single API key replaces 7 separate services. | [github.com/apinetwork/piapi-mcp-server](https://github.com/apinetwork/piapi-mcp-server) |
| 20 | **Playwright MCP** (Official Microsoft) | Browser automation via structured accessibility snapshots. Cross-browser E2E testing. | Test your REST API endpoints in a browser, automate testing of web-based admin/GM tools, scrape game reference sites with full JS rendering. | [github.com/microsoft/playwright-mcp](https://github.com/microsoft/playwright-mcp) |
| 21 | **Docker MCP** | Container lifecycle management, compose stacks, image management, log streaming. | Manage your PostgreSQL + Redis + Node.js containers. Restart crashed servers, check container health, tail logs — all from Claude Code. | [github.com/QuantGeekDev/docker-mcp](https://github.com/QuantGeekDev/docker-mcp) |
| 22 | **Grafana MCP** (Official) | AI-powered observability. Dashboard search, Prometheus queries, incident management. | Monitor MMO server performance — player counts, response times, combat tick latency, memory usage. Query metrics like "average tick time in the last hour." | [github.com/grafana/mcp-grafana](https://github.com/grafana/mcp-grafana) |
| 23 | **Suno MCP** | Music generation via Suno AI. Custom lyrics, style, continuation. v3.5-v5 models. | Generate background music for MMO zones — peaceful town themes, tense dungeon tracks, epic boss fight music. Iterate on game soundtrack with text prompts. | [github.com/CodeKeanu/suno-mcp](https://github.com/CodeKeanu/suno-mcp) |
| 24 | **Screenshot MCP** | Capture screen regions for AI analysis. Full-screen and area capture with compression. | Capture your UE5 editor viewport and send to Claude for visual analysis. Debug Slate UI layout issues by showing Claude exactly what you see on screen. | [github.com/codingthefuturewithai/screenshot_mcp_server](https://github.com/codingthefuturewithai/screenshot_mcp_server) |

---

## TIER 4: SITUATIONAL — Useful When You Need Them

| # | MCP Server | Description | When You'd Use It | GitHub / Source |
|---|-----------|-------------|-------------------|----------------|
| 25 | **Houdini MCP** | 43 tools for Houdini. Full hou module access, node ops, Karma rendering. | Procedural terrain, vegetation, particle effects, destructible environments for UE5 open-world zones. | [github.com/oculairmedia/houdini-mcp](https://github.com/oculairmedia/houdini-mcp) |
| 26 | **DCC-MCP** (Universal) | One MCP for Maya + Houdini + 3ds Max + Nuke. Standardized API across all DCC software. | If you use multiple DCC tools in your asset pipeline — one AI interface for all of them. | [github.com/loonghao/dcc-mcp](https://github.com/loonghao/dcc-mcp) |
| 27 | **AWS MCP** (Official) | S3, DynamoDB, RDS, CloudWatch, ECS, Bedrock. OAuth 2.0 + WAF + multi-AZ. | When deploying to AWS — manage EC2 instances, RDS PostgreSQL, ElastiCache Redis, S3 game assets, CloudWatch metrics. | [github.com/awslabs/mcp](https://github.com/awslabs/mcp) |
| 28 | **Notion MCP** (Official) | Read/write Notion pages, databases, comments. Hosted at mcp.notion.com. | If you store game design docs, lore, quest designs, or balance spreadsheets in Notion — AI reads your docs and translates to code. | [developers.notion.com/docs/mcp](https://developers.notion.com/docs/mcp) |
| 29 | **Tripo3D MCP** | Text-to-3D, image-to-3D, multiview-to-3D. Direct Blender import. | Generate high-quality 3D models from text/images for UE5 after retopology. | [github.com/VAST-AI-Research/tripo-mcp](https://github.com/VAST-AI-Research/tripo-mcp) |
| 30 | **Composio MCP** | Single endpoint for 250+ services (GitHub, Slack, Gmail, Notion, Jira, etc.). Auto OAuth. | One MCP to replace many — instead of installing 10 individual MCPs, get broad coverage from one server. | [composio.dev](https://composio.dev) |

---

## ALTERNATIVE UE5 MCPs (Compared to Your Current flopperam/unreal-engine-mcp)

| MCP | Stars | Unique Strengths | Consider If... |
|-----|-------|-----------------|-----------------|
| **chongdashu/unreal-mcp** | ~1,500 | Largest community, most battle-tested | You want broader community support |
| **mirno-ehf/ue5-mcp** | — | Built specifically for Claude Code | You want tighter Claude Code integration |
| **VedantRGosavi/UE5-MCP** | — | Animation Blueprints, Blend Spaces, Behavior Trees, Landscape sculpting | You need animation system or AI behavior tree control |
| **prajwalshettydev/UnrealGenAISupport** | 415 | Multi-LLM support (GPT, Claude, Gemini), Blueprint auto-generation | You want in-editor AI chat with multiple model providers |
| **CLAUDIUS CODE** (Commercial) | — | 130+ commands, PIE control (manipulate running game), ~10-50ms latency | You need real-time play-in-editor control |
| **ChiR24/Unreal_mcp** | — | 36 tools including Niagara, Behavior Trees, Skeletal Meshes, Rust WASM performance | You need broader asset type coverage (particles, animations) |

---

## MCP DISCOVERY DIRECTORIES

Keep checking these for new MCPs as the ecosystem grows rapidly:

| Directory | URL | Size |
|-----------|-----|------|
| **PulseMCP** | [pulsemcp.com/servers](https://www.pulsemcp.com/servers) | 8,600+ servers |
| **MCP.so** | [mcp.so](https://mcp.so/) | 3,000+ tools |
| **MCPMarket** | [mcpmarket.com](https://mcpmarket.com/) | 373 game dev servers |
| **Smithery.ai** | [smithery.ai](https://smithery.ai/) | Hosted + local, discovery |
| **Glama.ai** | [glama.ai/mcp/servers](https://glama.ai/mcp/servers) | Categorized browsing |
| **awesome-mcp-servers** | [github.com/appcypher/awesome-mcp-servers](https://github.com/appcypher/awesome-mcp-servers) | Curated list |
| **TensorBlock catalog** | [github.com/TensorBlock/awesome-mcp-servers](https://github.com/TensorBlock/awesome-mcp-servers) | 7,260 servers |
| **VideoGameMCP** | [videogamemcp.com](https://www.videogamemcp.com/) | Game-specific |
| **Docker MCP Catalog** | [docs.docker.com/ai/mcp-catalog-and-toolkit](https://docs.docker.com/ai/mcp-catalog-and-toolkit/) | 100+ verified |

---

## MY TOP 10 RECOMMENDATIONS FOR SABRI_MMO

If I had to pick the 10 MCPs to install right now for maximum impact on your project:

1. **Context7** — Correct API docs for every library you use. Free. Zero config.
2. **PostgreSQL MCP** — Query your game database directly. Debug instantly.
3. **Redis MCP** — Inspect your sessions/cache layer. Debug combat state.
4. **Unreal Analyzer MCP** — Understand your C++ codebase semantically.
5. **Blender MCP** — Generate 3D game assets from text descriptions.
6. **ComfyUI MCP** — Free local texture/icon/concept art generation.
7. **ElevenLabs MCP** — NPC voices and ambient sounds. 10k free credits/month.
8. **Figma MCP** — Design UI in Figma, Claude generates the Slate code.
9. **Sentry MCP** — Track and debug server crashes with AI root cause analysis.
10. **GitHub MCP** — Enhanced repo management, issue tracking, code search.
