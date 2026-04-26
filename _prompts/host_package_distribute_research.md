# Deep Research: Host, Package, and Distribute Sabri_MMO

## Goal

Produce a comprehensive, implementation-ready research report covering **three connected pillars**:

1. **Hosting** the Node.js game server so other people can connect from across the internet (my PC, another PC, a VPS / cloud box, or anything else you can justify)
2. **Packaging** the UE5.7 client into a Shipping build that points at the remote server, not `localhost`
3. **Distributing** the packaged client to specific people — direct file share, Steam "private" (Playtest / restricted beta branches / friend-only access), itch.io, Epic, Discord, or whatever actually works for invite-only distribution in 2026

Nothing is out of scope. Research first, recommend second. I want trade-offs, costs, latency, legal concerns, operational pitfalls — not just "use AWS."

---

## Load Context First

Per the project's standing rule (`CLAUDE.md` → "prefer loading MORE skills than strictly necessary"), load all of these before starting. A false-positive skill load costs only context; a missing skill costs correctness.

**Hosting / server / ops (Pillar 1):**
- `/full-stack` — Node.js server, PostgreSQL, Redis, Socket.io architecture
- `/realtime` — Socket.io event architecture, tick rates, bandwidth
- `/sabrimmo-persistent-socket` — GameInstance socket, reconnect behavior, zone transitions (reconnect behavior over a remote/TLS link is a live question)
- `/security` — JWT handling, secrets, SQL injection, server-authoritative patterns, anti-cheat surface
- `/performance` — Server tick rate tuning, bandwidth, scalability ceilings for the single-process monolith

**Packaging / client config (Pillar 2):**
- `/sabrimmo-build-compile` — UE5.7 build target, editor vs game, Live Coding pitfalls, Shipping target gotchas
- `/sabrimmo-options` — **directly overlaps**: the endpoint-config design is a SaveGame persistence decision, and CLAUDE.md's "Use SaveGame for custom persistence, NOT GConfig" rule lives here
- `/sabrimmo-login-screen` — login is the entry point for server connection; any endpoint config change + version handshake surface ends up here
- `/sabrimmo-resolution` — Shipping build runs on user machines with varied resolutions / DPI; CLAUDE.md flags standalone-mode squishing as a known issue
- `/ui-architect` + `/sabrimmo-ui` — if the version mismatch / server selection UX needs a widget
- `/code-quality` — the C++ diff for endpoint config must follow project conventions

**Planning / output (orchestration):**
- `/planner` — this task IS a multi-phase plan; load the planning skill's methodology
- `/docs` — the deliverable is documentation; follow the project's doc style
- `/project-docs` — pull full project context up front so the plan reflects actual architecture, not assumed
- `/opus-45-thinking` — multi-system architecture decision spanning server + client + distribution
- `/deep-research` — use this skill's methodology (WebSearch + WebFetch + parallel agents) for the external research
- `/debugger` — for "what happens when X fails" failure-mode sections of each runbook

**Documentation to read:**
- `CLAUDE.md` (root) — already auto-loaded; note the design patterns list (SaveGame over GConfig, FSlateBrush over deprecated, deferred widget creation in standalone)
- `docsNew/00_Global_Rules/Global_Rules.md` — project-wide rules
- `docsNew/00_Project_Overview.md`
- `docsNew/04_Integration/Networking_Protocol.md`

**Source to read:**
- Skim `server/src/index.js` header (first ~200 lines) for `PORT`, CORS config, env vars
- Read `server/package.json`, `server/.env.example` (if present), `database/init.sql` header
- Read the endpoint configuration surface in the client:
  - `client/SabriMMO/Source/SabriMMO/MMOHttpManager.cpp` (`GetServerBaseUrl` — currently falls back to `http://localhost:3001`)
  - `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h` / `.cpp` (`ServerBaseUrl`, `GetServerSocketUrl`, persistent socket connect)
  - How `/api/servers` populates `FServerInfo { Host, Port }` and what picks which server — the server list is already a first-class feature, this is the natural seam for remote hosting
  - `client/SabriMMO/Source/SabriMMO/UI/LoginFlowSubsystem.*` and related login widgets (where the version handshake likely lives)
  - Any existing `USaveGame` subclass used by `/sabrimmo-options` — the endpoint config should either join that SaveGame or follow its pattern

---

## Known Baseline (Do Not Re-Derive)

- **Stack**: UE5.7 (C++ + BP) client ↔ Node.js + Express + Socket.io ↔ PostgreSQL + Redis
- **Server**: single monolithic process, `server/src/index.js` (~30,000 lines), listens on `process.env.PORT || 3000`. CORS enabled (`app.use(cors())` — open).
- **Client endpoint resolution**: `MMOHttpManager::GetServerBaseUrl` returns `GI->ServerBaseUrl` or falls back to `http://localhost:3001`. `MMOGameInstance::GetServerSocketUrl` returns the same. Server list is fetched from `/api/servers` and each entry has `{ Host, Port }`.
- **Auth**: JWT. Secret in `server/.env` (`JWT_SECRET`). DB creds in `.env` too.
- **Persistent socket**: `TSharedPtr<FSocketIONative>` on the GameInstance, survives `OpenLevel()`. Reconnect behavior on loss matters.
- **No official Steam / Epic / launcher integration exists yet.** Clean slate.
- **IP concern**: the game is "inspired by Ragnarok Online Classic," uses RO skill/monster/item naming, and bundles RO-adjacent assets. Gravity won a $4M lawsuit against NovaRO in 2022 (already in memory). Public distribution has real legal exposure; private invite-only to specific known individuals is much safer but not zero-risk. Factor this into every distribution option.

---

## Research Pillar 1 — Hosting

Investigate and compare every realistic option for running the Node.js server so people outside my LAN can connect. For **each** option produce:

- **What it is** in one sentence
- **Setup steps** (concrete commands, not "install Node")
- **Monthly cost** at small scale (≤20 concurrent users) and at "small community" scale (~100 concurrent)
- **Latency profile** (does my player base's geography matter? where should the box live?)
- **Uptime / reliability** (consumer ISP TOS, residential IP blocklisting, power/network reliability)
- **Security surface** (exposed ports, DDoS exposure, what's encrypted)
- **Operational burden** (patching, monitoring, logs, restarts, backups)
- **Dealbreakers** (ISP blocks inbound? CGNAT? Windows-only? IPv6-only? etc.)

### Options to cover in depth

1. **Hosting on my own PC (Windows 11)**
   - Port forwarding through a consumer router (UPnP vs manual), finding WAN IP, dealing with CGNAT (how to detect if my ISP does it)
   - Dynamic DNS (DuckDNS, No-IP, Cloudflare DNS + ddclient) — which is most reliable in 2026
   - Running Node as a Windows service (NSSM, node-windows, PM2 + pm2-windows-service, WinSW) — compare, recommend
   - PostgreSQL on Windows (official installer vs Docker Desktop vs WSL2) and Redis on Windows (Memurai vs WSL2 vs Docker Desktop) — which actually works in production, not just "runs"
   - Windows Firewall rules, Defender exclusions for the game server
   - What happens when I sleep/restart/update my PC? Wake-on-LAN setups? Can I actually use my PC while it's the server?
   - Power/thermal concerns running 24/7 on a gaming rig
   - ISP TOS — which major US ISPs explicitly forbid server hosting; what "residential server" enforcement looks like in practice
   - Exposing this safely: should all traffic go through Cloudflare Tunnel / Tailscale Funnel / ngrok / Cloudflare Zero Trust even when self-hosting? Pros/cons for a realtime WebSocket game.

2. **Hosting on a dedicated secondary PC on my LAN**
   - Same as above but the tradeoffs shift (old gaming rig as always-on server, headless Mini PC like Beelink / Minisforum, Intel NUC, Raspberry Pi 5 8GB, N100 mini PC)
   - Do any of these have enough CPU for a Node monolith driving ~100 players? Profile what the server actually needs (single-threaded Node, PostgreSQL working set, Redis RAM)
   - Linux vs Windows on the secondary PC — recommend one and explain
   - Is a $300 N100 mini PC running Debian + pm2 + Postgres + Redis a legit path? Back this with actual benchmarks if findable.

3. **VPS / Cloud IaaS**
   - Shortlist and actually price out: Hetzner Cloud (EU + US), OVH, DigitalOcean, Linode/Akamai, Vultr, Contabo, Oracle Cloud Free Tier (real limits 2026), AWS Lightsail, AWS EC2 t4g, Azure B-series, GCP e2-small
   - For each: cheapest tier that can realistically run Node + Postgres + Redis together (RAM floor, egress pricing, IPv4 surcharge — Hetzner now charges for IPv4)
   - Where to deploy for a mostly-US player base vs mixed US/EU
   - Egress bandwidth cost traps (AWS egress is notorious — calculate for 100 concurrent at current tick/bandwidth)
   - Dedicated boxes (Hetzner AX / OVH SoYouStart) for when the community grows — at what player count does this pay off?
   - Clear top-pick + runner-up for **small** (invite-only, ≤20 CCU) and **growth** (100+ CCU) scales with concrete configurations

4. **Managed / split-component**
   - **Database**: Supabase, Neon, AWS RDS, Railway Postgres, DigitalOcean Managed Postgres, ElephantSQL, Aiven — which handles ~50GB game DB with reasonable pricing and low-latency connection to my app tier
   - **Redis**: Upstash (per-request pricing), Redis Cloud, Railway Redis, DigitalOcean Managed Redis. What's the latency cost of NOT colocating Redis with the app? Is Upstash's per-command model even compatible with a hot-path game loop that hits Redis every tick?
   - **App tier only**: Fly.io, Railway, Render, Northflank, Heroku — which support long-lived WebSocket connections properly, which have issues with Socket.io sticky sessions, which kill idle containers
   - Recommend: is split-component worth it, or is a single box simpler and cheaper at this scale?

5. **Specialized game hosting**
   - GPORTAL, Nitrado, Shockbyte, Pingperfect, Streamline Servers, PebbleHost — do any support custom Node.js + Postgres + Redis MMOs or are they all locked to Minecraft/Ark/Valheim templates? If none fit, say so clearly.
   - Are there any "managed indie MMO" platforms worth knowing about? Heroic Labs Nakama as hosted service? PlayFab? Colyseus Cloud? Evaluate each against the existing authoritative-monolith architecture — rewriting server is out of scope, but IF one of these lets me run my existing Node code as-is, that's interesting.

6. **Peer-to-peer / relay alternatives**
   - Tailscale / ZeroTier / Hamachi-style mesh VPN: can I give 5 friends a Tailscale invite and they connect to my PC over the mesh? No port forwarding, no public IP, encrypted. Does Socket.io work over Tailscale? Latency overhead? Free tier limits in 2026 (Tailscale free is 3 users last I checked — verify)
   - Cloudflare Tunnel for WebSocket + HTTP: does it properly proxy Socket.io with sticky sessions? Free tier bandwidth cap? Is this my #1 pick for "host on my PC, give URL to friends"?
   - Playit.gg, ngrok Reserved Domains — any of these actually suited for persistent game servers vs. quick demos?

### Cross-cutting operational research (applies to whichever host)

- **Reverse proxy**: Nginx vs Caddy vs Traefik for a Socket.io + REST API backend. Caddy does automatic Let's Encrypt — is that the easy win? Any known Socket.io v4 + Nginx gotchas with `proxy_read_timeout`, Upgrade headers, sticky sessions?
- **TLS / WSS**: the client currently uses `http://` and `ws://`. Research what changes are needed on both sides to move to `https://` + `wss://`. Does Socket.io auto-upgrade? Does the UE5 SocketIOClient plugin support WSS out of the box — version / config / cert pinning?
- **Domain + DNS**: cheapest reputable registrars (Cloudflare, Porkbun, Namecheap), TTL for a game server, wildcard for subdomains (`auth.`, `ws.`, `cdn.`)
- **Process supervision**: PM2 vs systemd vs Docker Compose with `restart: always` — recommend one for each hosting option
- **Log management**: local rotating files vs shipping to a log aggregator (Better Stack / Axiom / self-hosted Loki). What's the minimum viable setup?
- **Monitoring / uptime**: UptimeRobot / Better Stack / healthchecks.io hitting `/health`. Alerting: Discord webhook vs email vs push
- **Backups**: PostgreSQL nightly `pg_dump` to S3/B2/rsync.net/Backblaze, point-in-time recovery options, how to test restores. What's the cheapest "I won't lose character data" tier?
- **Secrets**: `.env` hygiene, rotating `JWT_SECRET`, what happens to existing sessions when it rotates, whether DB creds should live somewhere more robust than a file
- **Firewall / hardening**: ufw / Windows Firewall / iptables rules. Only expose 443 (or 80+443). SSH: key-only, non-standard port, fail2ban — is that still recommended in 2026 or overkill for a tiny server?
- **DDoS**: realistic for a 5-friend private game? If it matters, Cloudflare in front (proxied WebSocket requires Pro plan for non-standard ports? Verify)
- **Scaling ceilings**: at what concurrent user count does the single-process Node monolith break (CPU-bound tick loop)? When do I need to shard by zone or move to cluster mode? Not asking for implementation — just the signal to watch for.

### Deliverable for Pillar 1

A **decision matrix** with concrete recommendations for three scenarios:

- **Scenario A — "5 close friends, free or near-free, minimum fuss":** my pick + full setup runbook
- **Scenario B — "20–50 invitees, reliable, ~$20–40/mo":** my pick + full setup runbook
- **Scenario C — "growing community, 100+ CCU, DDoS-resilient, stable":** my pick + full setup runbook

Every "full setup runbook" must be copy-pasteable: DNS records to create, commands to run, config files to write, firewall rules to open, how to point the UE5 client at it, how to verify end-to-end.

---

## Research Pillar 2 — Packaging the UE5 Client

Investigate everything needed to turn the current UE5.7 project into a Shipping build that strangers can run.

1. **UE5.7 Shipping build fundamentals**
   - Differences between DebugGame / Development / Shipping / Test targets for a game like this
   - `SabriMMO.Build.cs` review — any modules that must change between dev and ship? `NetworkPrediction` is already intentionally excluded (CLAUDE.md) — are there other gotchas?
   - Cooked content vs pak files, compression (Oodle vs Zlib), pak encryption (protects assets from casual extraction, defeated by RAM dump but raises the bar)
   - Shipping build log/trace reduction, `UE_LOG` fate in Shipping
   - Dependencies: what VC redist / DirectX / .NET components does a packaged UE5.7 game need? Are they bundled in the `Prereq` installer the packager generates?
   - Final package size estimate and what's inflating it (which folders in `Content/SabriMMO` dominate)
   - Reproducible packaging: Automation Tool (`RunUAT BuildCookRun`) command line that I can script/check into the repo

2. **Making server endpoint configurable for users**
   - The project already supports multiple servers via `/api/servers` + `FServerInfo { Host, Port }`, but the initial request goes to the client's current `ServerBaseUrl` which defaults to `http://localhost:3001`. Research and recommend the cleanest way to make the "master server list" endpoint configurable in a Shipping build:
     - INI file (`Config/DefaultGame.ini`) loaded via `GConfig` vs SaveGame vs hardcoded constant for the build vs command-line arg (`-server=host:port`) vs user-facing in-game setting
     - CLAUDE.md explicitly notes "Use SaveGame for custom persistence, NOT GConfig custom ini (doesn't load in standalone)" — factor this in
   - Recommend where the default production master URL lives in the binary and how a tech-savvy user could override it if we ever want to (bug reports, QA testers)
   - Whether to have the client hit a tiny always-on "server list" endpoint first (so I can add/move game servers without re-shipping the client) — pros/cons
   - How to handle TLS: client must speak `https://` + `wss://` to production. Research UE5.7 HTTP module TLS (trusts system CA store? any config needed?) and SocketIOClient plugin WSS support. Is there a known plugin version / source patch required?

3. **Versioning + protocol handshake**
   - The client and server share an implicit event protocol. What's the minimum viable version check so an old client can't connect to a newer server (and vice versa) and silently break? Add a version string in REST `/health` or on socket connect, client displays "Please update" and blocks login.
   - Where should the build version number live (single source of truth: a generated header from git describe? a constant? `Project Settings > Project > Version`?)

4. **Installer / packaging for distribution**
   - Compare: raw .zip with `SabriMMO.exe`, Inno Setup installer, NSIS, MSIX, Advanced Installer. Recommend one for invite-only distribution. (MSIX requires code signing to install cleanly on modern Windows — factor in.)
   - Code signing: SmartScreen warnings when running unsigned EXEs from the internet, SignPath.io free for OSS, DigiCert/Sectigo EV/OV certs and real 2026 pricing, self-signed as a non-solution. Is code signing worth it for private distribution to 5–50 known people? Where's the cutoff?
   - Auto-updater options: Epic's launcher-style patch system (overkill), custom in-app "you need to update" + download link (simplest), third-party updaters (Squirrel.Windows, WinSparkle, velopack) — recommend one
   - Pre-launch: Anti-virus false positives on unsigned UE5 builds are common. How to get ahead of this (submit to MS Defender / Kaspersky / Avast for whitelisting, use a code signing cert)

5. **Asset and IP hygiene for the shipped build**
   - Verify what's actually cooked into the pak: do `.psd` / `.blend` / `.wav` source files get excluded? Which asset folders must be excluded from Shipping?
   - `RagnaCloneDocs/`, `_journal/`, `docsNew/`, `_prompts/`, `server/` must obviously not ship — confirm the default cook doesn't include them since they're outside `client/SabriMMO/Content/`
   - The sprites and textures are derived from RO assets in some cases — if distributing (even privately) to people outside the immediate team, what's the minimum I should know about RO IP / Gravity Co./ NovaRO-style enforcement risk? Not a legal opinion — just factual context so I can make an informed call.
   - What telemetry / analytics (if any) should a small private build have? PostHog / Sentry / self-hosted GlitchTip for crash reports?

### Deliverable for Pillar 2

- A **packaging runbook** I can execute end-to-end to produce `SabriMMO-v0.x.y-Win64.exe` (or `.zip` + installer) that points at a production server URL
- An **endpoint-config design** (decision on SaveGame vs INI vs hardcoded vs CLI vs master-list endpoint) with the actual C++ diff to `MMOGameInstance` + `MMOHttpManager` to implement it
- A **client/server version handshake spec** (what's sent, where it's checked, what UX happens on mismatch)
- A **pre-flight checklist** before sending a build to anyone: signed? antivirus-submitted? crash reporting on? endpoint verified? DB has test account? version handshake tested?

---

## Research Pillar 3 — Distribution to Specific People

I want to give the packaged build to **specific invited people** — not the general public. Investigate every realistic channel and rank them.

1. **Direct file transfer**
   - Google Drive, Dropbox, MEGA, OneDrive, WeTransfer, Smash, self-hosted (Backblaze B2 + public link, Cloudflare R2 + signed URLs) — bandwidth limits, max file size, download speeds, link expiration, whether they scan EXEs and flag them
   - Torrent (private tracker, Resilio Sync) for large patches
   - How big is the packaged build likely to be? (Reference UE5 sample Shipping builds — ballpark)
   - Best path for "send a 3GB installer to 5 people" vs "host a ~3GB installer for 50 people"

2. **Steam private / invite-only distribution**
   - **This is the headline question.** Investigate thoroughly and lay out every option Steam offers for private distribution in 2026:
     - **Steam Playtest** — free, invite-only via Steam key distribution, separate app ID or same as main game? Limits on testers? Does the game need to be an accepted Steam app first? Can it be hidden from the store?
     - **Release with restricted / password-protected beta branches** — publish a hidden app, gate access via `setlive` branches with passwords, only share password with invitees. How permanent does the app have to be? Can a fully unreleased app use this?
     - **Steam Keys** — can I generate keys for a product and only share them? Do keys work for a never-released store page?
     - **Steam Family Sharing** — only useful for same-household, probably not applicable
     - **Curator lists / Wishlist targeting** — not relevant for access, only discoverability
     - Steam's $100 app submission fee, content review for hidden/private apps, whether Valve allows a clone-adjacent game on the store even privately (this is where the RO-inspired IP concern re-enters — Gravity has taken down RO clones from public Steam before)
   - **Steamworks integration effort** — if I go Steam, I need SDK integration for DRM/auth/friends. Research Steamworks SDK UE5 plugin (official Steam Subsystem), integration steps, whether it's compatible with my existing JWT auth or if I need to layer Steam auth on top
   - Final call: is Steam private realistic or are the overhead + legal risk too high for something that just needs to go to <50 people?

3. **Alternative stores with private/key distribution**
   - **itch.io** — password-protected games, key-only access, restricted pages, "creator-only" vs public. Detailed mechanics + what they forbid in their TOS re: IP-adjacent content. This is probably the real answer for invite-only; confirm.
   - **Epic Games Store** — private distribution? Probably none for indies; verify
   - **GOG** — probably not
   - **Game Jolt** — any private/beta features?

4. **Discord / community-native distribution**
   - Discord server with a #downloads channel, file uploads up to Nitro limits (verify 2026 limit), direct links to CDN
   - Discord's attachment URL caching behavior (links expire now — this matters)
   - Combining Discord community (invite gate) + external storage (direct link in pinned message)

5. **GitHub Releases on a private repo**
   - Private repo, add invitees as collaborators, they download from Releases tab
   - File size limit (2GB per asset in 2026 — verify), Git LFS for larger
   - Does the build fit? If not, split into launcher + data, or use a separate CDN

6. **Self-hosted launcher / updater**
   - Minimum viable "Sabri launcher" that checks a version endpoint, downloads the patch, runs the game. Worth building, or premature? If worth, what framework (Electron, Tauri, WinForms, raw C++)? There are open-source launchers (Heroic, Playnite) but those are for users managing libraries, not for devs shipping games — confirm there isn't an off-the-shelf "indie launcher" I should be using.

7. **Cross-cutting distribution concerns**
   - **Access control**: how do I stop a friend from forwarding the installer to strangers? Practically — nothing if the build isn't hardware/account-locked. Options: require Steam login, require my server account (JWT login gate before launch), bind install to an invite code that's single-use, just trust my friends. Rank by effort vs payoff.
   - **Updates**: when the server protocol changes, every client must update. How does each distribution channel handle this? (Steam: auto. itch.io app: auto via itch launcher. Direct download: manual — but the version handshake in Pillar 2 forces the issue.)
   - **Rollbacks**: when a patch is broken, how do I get everyone back on v1.4 quickly?

### Deliverable for Pillar 3

A **distribution decision tree**:
- If audience = 1–5 known friends → [specific channel] + [exact steps]
- If audience = 10–30 invited testers → [channel] + [steps] + [access control]
- If audience = "small private community 50+" → [channel] + [steps] + [access control] + [update pipeline]

For the top-recommended channel in each tier, include a **full onboarding doc I could send to an invitee**: "Here's how to install, here's how to log in, here's what to do if it doesn't launch, here's how you get updates."

---

## Final Deliverable

**One master plan document** at `docsNew/05_Development/Hosting_Packaging_Distribution_Plan.md` that stitches all three pillars together with:

1. **Executive summary** — what I should actually do, in order, for my current situation (invite-only, small group, start cheap)
2. **Pillar 1 full report** — all hosting options researched + decision matrix + three runbooks (A/B/C scenarios)
3. **Pillar 2 full report** — packaging + endpoint-config design + version handshake + pre-flight checklist + C++ diffs
4. **Pillar 3 full report** — distribution options + decision tree + invitee onboarding doc draft
5. **End-to-end "ship it" runbook** — concrete numbered steps from "zero" to "a friend is playing on my server" using the recommended stack for the small scenario
6. **Future migration path** — when I need to go from Scenario A → B → C, what changes? Is there lock-in to avoid now?
7. **Appendix: costs** — consolidated monthly cost table for every recommended option
8. **Appendix: risks** — IP/legal, ISP TOS, Steam acceptance, code signing friction, anti-virus false positives, etc. — each with a mitigation

Supplementary documents where they make sense:
- `docsNew/05_Development/Server_Deployment_Runbook.md` — the copy-paste runbook for the recommended host
- `docsNew/05_Development/Client_Packaging_Runbook.md` — the copy-paste runbook for producing a Shipping build
- `docsNew/05_Development/Client_Endpoint_Config_Design.md` — the C++ design for the configurable server endpoint + version handshake

---

## Ground Rules for the Research

- **Search broadly, cite sources.** Use WebSearch + WebFetch for every claim about pricing, limits, feature availability. Prices in 2026 are what matters — anything pre-2025 must be verified still current. Link the source.
- **Iterate.** First pass is a map of options, second pass is comparison, third pass is concrete recommendations. Do not stop after one pass.
- **Parallelize independent research.** Use the Agent tool with `subagent_type=Explore` (or `general-purpose`) to research hosting providers, Steam policy, and itch.io policy in parallel since they're independent.
- **Be concrete, not diplomatic.** If a provider is bad for this use case, say so and why. If Steam private is a dead end for RO-adjacent content, say so.
- **Factor in my actual constraints**:
  - RTX 5090 gaming PC on Windows 11 (current dev box)
  - Solo developer, not a company
  - Invite-only small scale initially, might grow
  - Server is authoritative already (good news — less anti-cheat work needed)
  - Current code uses `http://` and `ws://` — moving to TLS is part of this work
  - Player base geography unknown; assume mostly US for now, confirm with me if it changes the recommendation meaningfully
- **No vaporware.** If a service's free tier changed in 2024/2025 and the research isn't verifiable, flag it instead of assuming.
- **Don't design new game features** — the scope is hosting, packaging, distribution. If you find a game code issue along the way, note it in the appendix, don't fix it.

---

> **Navigation**: [Documentation Index](../docsNew/DocsNewINDEX.md)
