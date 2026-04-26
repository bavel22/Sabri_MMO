# Hosting, Packaging, and Distribution Plan

> **Navigation**: [Documentation Index](../DocsNewINDEX.md) | [Global Rules](../00_Global_Rules/Global_Rules.md) | [Project Overview](../00_Project_Overview.md)
> **Companion docs**: [Server Deployment Runbook](Server_Deployment_Runbook.md) | [Client Packaging Runbook](Client_Packaging_Runbook.md) | [Client Endpoint Config Design](Client_Endpoint_Config_Design.md)
> **Status**: 2026-04-16 — research phase complete, implementation pending
> **Audience**: Solo dev, invite-only release, RO-adjacent IP sensitivity, US-based, RTX 5090 Windows 11 dev box

---

## 0. Executive Summary (Do This)

**Your own hardware can carry you through both Scenario A (5 friends) AND Scenario B (20–50 invitees)** — you only *need* a VPS once you outgrow your home connection's upload bandwidth or hit ~100 CCU. Spare Windows PC or a spare PC converted to Linux both work as 24/7 servers; a $250 N100 mini PC is the cleanest dedicated self-host option.

**Start with Scenario A (self-host). Migrate to B when you pass ~10 testers (same hardware, add a public URL). Migrate to C only if you outgrow a single box.**

| Question | Scenario A: 5 friends | Scenario B: 20–50 invitees | Scenario C: 100+ CCU |
|---|---|---|---|
| Where does the server run? | **Your own hardware** — gaming PC, spare Windows PC, Linux on spare PC, or N100 mini PC | **Same own hardware** OR VPS if you prefer | Hetzner CCX23 US (~$36/mo) or AX41 dedicated (~$55/mo) — home bandwidth usually caps here |
| How do clients reach it? | **Tailscale mesh** (private WireGuard, 6 users free) | **Cloudflare Tunnel** → Caddy → Node (TLS auto, no port forwarding, CGNAT-friendly) | **Caddy + Let's Encrypt** on the VPS public IP, Cloudflare DNS |
| Client → server URL | `ws://100.x.y.z:3001` (inside tailnet, plain WS OK) | `wss://sabrimmo.com` | `wss://sabrimmo.com` (same) |
| Domain | None needed | Cloudflare Registrar `.com` ($10.46/yr) | Same domain, keep Cloudflare DNS |
| How do invitees install? | ZIP over Discord DM | **itch.io Restricted + bulk download keys + butler** | itch.io Restricted, same |
| Access control | Server JWT login (already built) | Server JWT + per-invitee register code | JWT + register code + version handshake |
| Code signing | No | No (submit to Defender on each build) | Azure Trusted Signing if you have an LLC ($120/yr), otherwise no |
| Crash reporting | `sentry-unreal` + Sentry free tier | Same | Same (upgrade to paid if >5K errors/mo) |
| Auto-updater | Custom version handshake | Same + itch.io app auto-delta | Same, consider Velopack at 100+ |
| Monthly cost | **~$1–12** (electricity only) | **~$1–12** (same hardware) + $0.87/mo domain | **$35–55** (VPS/dedicated) + domain |
| Ops burden / week | ~0 hrs | ~1 hr | ~2 hrs |

**Self-host hardware options**, cheapest first:
- **Your gaming PC** — $0 if you share it, but pin Node to one core and accept occasional 10–30 ms tick dips during heavy gaming.
- **Spare Windows PC** you already own — $0 hardware, $3–12/mo electricity. Same Windows stack (PM2 + EDB Postgres + Memurai).
- **Spare Windows PC, reinstalled as Linux (Debian 12 or Ubuntu Server 24.04 LTS)** — $0 hardware, same electricity, cleaner ops (systemd + native Postgres + native Redis, no Memurai licensing grey area), better long-term reliability.
- **$250 N100 mini PC (Beelink EQ12 or similar) running Linux** — $250 one-time, ~$1.15/mo electricity, payback in ~2 years on electricity alone vs gaming PC. Recommended if your "spare PC" is a power-hungry tower or if you want a dedicated box.
- **Raspberry Pi 5 8GB + NVMe hat** — ~$150 one-time, works because `recast-navigation` is a WebAssembly package (no native ARM issues). Slightly tight for 50+ CCU; fine for ≤20.

Full side-by-side tracks in [Server Deployment Runbook](Server_Deployment_Runbook.md).

**The three unshakable findings**:

1. **Don't use Steam for this.** Even with Nov-2024 private branches, Steam Direct requires a publicly visible "Coming Soon" page for at least two weeks during review — which exposes the app's name and capsule art to Gravity Co.'s monitoring. The 2026 Dreadmyst delisting (solo-dev MMO removed from Steam via an NCsoft DMCA) is the direct precedent. $100 fee + 6–8 weeks onboarding + 15–50 dev-hours of UE5.7 Steamworks integration is all wasted if they file. Use itch.io Restricted.
2. **Don't use AWS / Azure / GCP for the server.** Their egress pricing has a cliff at a few TB/month ($441/mo at 5 TB on AWS vs $0 on Hetzner/OVH). Their burstable instance tiers (t4g, B2s, e2-medium) will throttle your 50 ms combat tick when CPU credits run out. This is the wrong tool for an MMO backend.
3. **Never enable Upstash Redis PAYG.** A 20 Hz tick loop at 100 CCU is roughly 6.5 billion Redis commands/month. Upstash PAYG at $0.20 per 100K = **~$13,000/month**. The fixed Upstash plan at $10/month has unmetered commands and works fine. This is the single biggest accidental-bankruptcy risk in any managed-hosting option.

---

## 1. Navigation

- [§2 Pillar 1 — Hosting](#2-pillar-1--hosting)
- [§3 Pillar 2 — Packaging the UE5 Client](#3-pillar-2--packaging-the-ue57-client)
- [§4 Pillar 3 — Distribution](#4-pillar-3--distribution)
- [§5 End-to-End "Ship It" Runbook (Scenario A)](#5-end-to-end-ship-it-runbook-scenario-a)
- [§6 Future Migration Path](#6-future-migration-path)
- [§7 Appendix A — Cost Summary](#7-appendix-a--cost-summary)
- [§8 Appendix B — Risks and Mitigations](#8-appendix-b--risks-and-mitigations)
- [§9 Appendix C — IP / Legal Hygiene](#9-appendix-c--ip--legal-hygiene)
- [§10 Appendix D — Code-Level Gotchas Discovered](#10-appendix-d--code-level-gotchas-discovered)

---

## 2. Pillar 1 — Hosting

### 2.1 Decision matrix

**Own-hardware options** (recommended for A and B):

| Hardware option | A: 5 friends | B: 20–50 | C: 100+ CCU | Verdict |
|---|---|---|---|---|
| **Gaming PC (shared) + Tailscale mesh** | ✅ | ✗ (6-user cap) | ✗ | Free, zero public exposure. Pin Node to core 0 so games don't starve it. |
| **Gaming PC + Cloudflare Tunnel** | ✅ | ✅ | ⚠ (home upload) | No port forwarding, WSS auto, CGNAT-friendly, 20–60 ms latency vs direct. |
| **Spare Windows PC + Tailscale** | ✅ **Top pick for 5 friends** | ✗ | ✗ | $0 hardware, $3–12/mo electricity. Runs Windows stack (PM2, EDB Postgres, Memurai). |
| **Spare Windows PC + Cloudflare Tunnel** | ✅ | ✅ **Top pick B1** | ⚠ | Same hardware, scales to 20–50 via public `sabrimmo.com`. |
| **Spare PC reinstalled as Linux + Tunnel** | ✅ | ✅ **Top pick B2 (recommended long-term)** | ⚠ | Cleaner ops (systemd + apt Postgres + apt Redis), better uptime, no Memurai licensing grey area. |
| **$250 N100 mini PC (Linux) + Tunnel** | ✅ | ✅ | ⚠ | 10 W idle = ~$1.15/mo electricity. Pays for itself in ~2 yrs vs gaming PC server. |
| **Raspberry Pi 5 8GB + NVMe + Tunnel** | ✅ | ⚠ (tight at 50 CCU) | ✗ | ~$150 one-time. `recast-navigation` is WASM so ARM64 is fine. Pi 5 NVMe hat mandatory (don't run Postgres on SD). |

**VPS options** (only if own-hardware isn't viable, or you've outgrown it):

| VPS option | A: 5 friends | B: 20–50 | C: 100+ CCU | Verdict |
|---|---|---|---|---|
| **Oracle Free A1** (ARM, $0) | ✅ | ✅ | ⚠ | 4 OCPU / 24 GB / 10 TB — genuinely free forever. Capacity gamble — may take days to provision. |
| **OVH VPS-1 US** ($7.60) | ✅ | ✅ | ⚠ | 15 US DCs, 8 GB RAM, 400 Mbps unmetered. |
| **OVH VPS-2 US** ($9.99) | ✅ | ✅ **Top VPS pick for B** | ✅ | 6 vCore / 12 GB / NVMe / 1 Gbps unmetered. Strong headroom. |
| **Hetzner CPX22 EU** (€7.99) | ✅ | ✅ | ⚠ | ~120 ms Atlantic RTT but cheapest. US CPX has 1 TB cap. |
| **Hetzner CCX13 US** (~$18) | — | ✅ | ✅ | 2 truly-dedicated AMD vCPU — no noisy-neighbor tick dips. |
| **Hetzner CCX23 US** (~$36) | — | — | ✅ **Top C pick (US)** | 4 dedicated / 16 GB / 3 TB / Ashburn. Solid 100–200 CCU. |
| **Hetzner AX41-NVMe EU** (~$55) | — | — | ✅ **Top C pick (EU)** | 6c/12t Ryzen, 64 GB, 2×512 GB NVMe, unmetered 1 Gbps. 120ms to US. |
| **DigitalOcean / Linode / Vultr** ($24) | — | ✅ | ⚠ | Polished ops, 2× OVH's price for similar specs. Pick if you prefer their UX. |
| **Fly.io + Upstash Fixed + Supabase Pro** (~$55/mo split) | — | ✅ | ✅ | Best managed-stack option if you refuse to run a VM. Much pricier than raw VPS. |

**Never use**:

| Option | Why |
|---|---|
| **Contabo** | Documented 8–30% CPU steal, spikes to 66% — kills tick loop. |
| **AWS Lightsail / EC2 t4g / Azure B / GCP e2** | Burstable throttle under sustained tick load + egress cliff (~$441/mo at 5 TB on AWS vs $0 on Hetzner/OVH). |
| **Scaleway** | No US presence. |
| **Render / Heroku Eco / Replit autoscale** | Idle-kill or per-request billing hostile to persistent WebSockets. |
| **GPORTAL / Nitrado / Shockbyte / PebbleHost** | All template-locked (Minecraft, Ark, Valheim). No custom Node.js mode. |
| **Nakama Cloud / PlayFab / Colyseus Cloud** | Require rewriting ~30 KLoC of your `index.js`. Out of scope. |
| **Upstash PAYG Redis** | 20 Hz tick × 100 CCU = ~$13,000/month. Use Upstash fixed tier ($10) or Redis Cloud Essentials ($5). |
| **Neon Postgres (scale-to-zero)** | 300–1000 ms cold start freezes combat. If you must use Neon, disable scale-to-zero (then it's $77/mo, no longer the deal). |

**Read the matrix**: for your situation (own hardware available, 5 friends first, US), you can stay entirely on **your own hardware through Scenarios A and B** using Tailscale → Cloudflare Tunnel. Only jump to VPS if your home upload saturates, your ISP complains, or you want to stay online while the PC is off. When you finally need a VPS, **OVH VPS-2 US ($10)** for B or **Hetzner CCX23 US ($36)** for C.

### 2.2 Self-hosting on your own hardware — the four tracks

You have four hardware choices for own-hardware hosting. Pick one; all four support both Scenario A (Tailscale, 5 friends) and Scenario B (Cloudflare Tunnel, 20–50 invitees) without changing hardware — just change the remote-access layer.

| Track | When to pick | Stack |
|---|---|---|
| **1A — Gaming PC (shared)** | No spare hardware, want to start today | Windows 11 + PM2 (via `pm2-installer`) + EDB Postgres + Memurai Redis. Pin Node to one core. |
| **1B — Spare Windows PC (dedicated)** | You have an old PC collecting dust | Same Windows stack as 1A. Reboots don't affect your gaming rig. |
| **2 — Spare PC reinstalled as Linux** | Long-term reliability, prefer industry-standard ops | Debian 12 or Ubuntu Server 24.04 LTS + systemd + apt Postgres + apt Redis. Cleaner, no Memurai licensing grey area. |
| **3 — $250 N100 mini PC (Linux)** | Your spare PC is a power-hungry tower, want lowest electricity cost | Same as Track 2. ~$1.15/mo electricity vs $11.50 for a gaming-PC tower. Pays back in ~2 years. |

**Full step-by-step setup for all tracks is in [Server_Deployment_Runbook.md](Server_Deployment_Runbook.md).**

**Shared prereqs** (all tracks):
- **Detect CGNAT first**. Compare your router's WAN IP to `https://ifconfig.me`. If different, or if the router WAN IP falls inside `100.64.0.0/10`, you're behind CGNAT and inbound port forwarding is dead — that's fine, **Tailscale mesh and Cloudflare Tunnel both sidestep CGNAT** by dialing outbound from your box to the edge provider. You only care about CGNAT if you're trying to expose a direct public port.
- **Check ISP TOS**. Comcast, Spectrum, Cox, Verizon FiOS, T-Mobile Home all prohibit or CGNAT-block residential server hosting. **Google Fiber is the only major US ISP that explicitly allows non-commercial servers** — its Residential AUP permits "multi-player gaming, video-conferencing, and home security." AT&T Fiber is AUP-based with no blanket ban. Cloudflare Tunnel's outbound-only model makes AUP enforcement effectively a non-issue (no inbound traffic signatures that trigger automated abuse systems). Enforcement at <50 players is documented as effectively never across ISPs, but account-termination is a real tail risk if you ever get caught.

**Track-specific quick notes** (full detail in the runbook):

Tracks 1A & 1B (Windows):
- **Supervisor**: **PM2 via `pm2-installer`** (https://github.com/jessety/pm2-installer) — proper Windows service under LocalService, auto-start, keeps `pm2 logs`/`monit`/`restart` UX. NSSM works but lacks PM2 ergonomics. `pm2-windows-service` is unmaintained.
- **PostgreSQL**: install via the **EDB installer** (native Windows service). Skip Docker Desktop (licensing concerns and I/O overhead) and WSL2 (doesn't auto-start on boot).
- **Redis**: **Memurai Developer Edition** is Redis Labs' official Windows partner — native, free for dev/test, auto-starts as a Windows service. WSL2 and Docker are both janky. Licensing caveat: Memurai Developer is licensed for dev/test only, which is a grey area for a 50-friend invite-only alpha. If it ever matters, switch to Track 2 (Linux) which has no such restriction.

Tracks 2 & 3 (Linux):
- **Distro**: **Ubuntu Server 24.04 LTS** (longest LTS, newer kernel drivers for modern mini PCs) or **Debian 12** (conservative stability). Both work.
- **Supervisor**: **systemd unit** (native, already installed, integrates with `journalctl`). PM2 also works on Linux if you want that workflow.
- **PostgreSQL**: `apt install postgresql-15` (via PGDG repo) or `apt install postgresql` for Postgres 16. Your auto-column-create startup logic handles version differences.
- **Redis**: `apt install redis-server` — native, no licensing grey area.
- **Node.js**: install 20 LTS via the NodeSource `apt` repo (Ubuntu-packaged Node is often old).
- **ARM64 compatibility**: `recast-navigation` is a WebAssembly package per its npm listing, so Oracle Free A1 ARM, Raspberry Pi 5, and Apple Silicon Mac servers all work.

**Remote access layer** (all tracks — pick one):
- **Scenario A (5 friends)**: Tailscale private mesh. 6 users + 100 devices free. Plain WS inside WireGuard. Setup: 5 minutes.
- **Scenario B (20–50 invitees)**: Cloudflare Tunnel. No port forwarding. CGNAT-friendly. Free. WSS at `https://sabrimmo.com` auto-TLS. Setup: 15 minutes once you own the domain.
- **Scenario B alt (direct public IP)**: Caddy + Let's Encrypt + DDNS. Only if you don't want Cloudflare in the path. Requires inbound 443 from your ISP.

**Windows-specific hardening** (Tracks 1A/1B):
```powershell
# Firewall: only allow Node inbound when NOT using Tunnel
New-NetFirewallRule -DisplayName "Sabri MMO Node" `
    -Direction Inbound -Protocol TCP -LocalPort 3001 -Action Allow -Profile Any
# Postgres + Redis: LAN only
New-NetFirewallRule -DisplayName "Postgres LAN" `
    -Direction Inbound -Protocol TCP -LocalPort 5432 -Action Allow `
    -RemoteAddress 192.168.0.0/16
New-NetFirewallRule -DisplayName "Redis LAN" `
    -Direction Inbound -Protocol TCP -LocalPort 6379 -Action Allow `
    -RemoteAddress 192.168.0.0/16
```

**Power / thermal / "can I play games on it?"** (Track 1A only):
- Node is single-threaded — pin it to one core (`start /affinity` or PM2 ecosystem affinity) and pin games to others.
- RTX 5090 rig at 24/7 idle ≈ 100 W = **~$11.50/mo electricity** at US average $0.16/kWh.
- A **$250 N100 mini PC** (Track 3) at ~10 W avg = **$1.15/mo**, saves ~$120/yr, and frees the gaming PC for actually playing games. Worth doing if you run the server past a month or two.
- Raspberry Pi 5 8GB works and is cheaper ($150 total with NVMe hat) — pick it if you want the smallest possible footprint and ≤20 CCU.

**Tailscale mesh setup** (5 friends, zero public exposure):
1. Install Tailscale on server + each friend's PC.
2. Your server's `tailscale ip` returns a 100.x.y.z address.
3. Friends log into your tailnet via invite email.
4. UE5 client connects to `ws://100.x.y.z:3000` — **plain WS is fine** inside the WireGuard mesh.
5. Free plan: 6 users, up to 100 devices, fast direct P2P (1–5 ms overhead); DERP relay fallback 25–150 ms if NAT traversal fails.

**Cloudflare Tunnel setup** (20–50 testers, public URL, free):
1. Buy `sabrimmo.com` at Cloudflare Registrar ($10.46/yr).
2. `cloudflared tunnel login && cloudflared tunnel create sabrimmo && cloudflared tunnel route dns sabrimmo sabrimmo.com`.
3. Write `~/.cloudflared/config.yml` (see [Server Deployment Runbook](Server_Deployment_Runbook.md)).
4. `cloudflared service install` — auto-starts on boot.
5. UE5 client connects to `wss://sabrimmo.com`. Cloudflare terminates TLS at the edge, tunnel carries plain HTTP to localhost:3000.

**Windows-specific hardening**:
```powershell
# Firewall: only allow Node inbound when NOT using Tunnel
New-NetFirewallRule -DisplayName "Sabri MMO Node" `
    -Direction Inbound -Protocol TCP -LocalPort 3000 -Action Allow -Profile Any
# Postgres + Redis: LAN only
New-NetFirewallRule -DisplayName "Postgres LAN" `
    -Direction Inbound -Protocol TCP -LocalPort 5432 -Action Allow `
    -RemoteAddress 192.168.0.0/16
New-NetFirewallRule -DisplayName "Redis LAN" `
    -Direction Inbound -Protocol TCP -LocalPort 6379 -Action Allow `
    -RemoteAddress 192.168.0.0/16
```

**Power / thermal / "can I play games on it?"**:
- Node is single-threaded — pin it to one core (`start /affinity`) and pin games to others.
- RTX 5090 rig at 24/7 idle ≈ 100 W = **~$11.50/mo electricity** at US average $0.16/kWh.
- A $200–300 **N100 mini PC** (Beelink EQ12) at ~10 W avg = **$1.15/mo**, saves ~$120/year, and frees the gaming PC. Worth doing if you keep the server running past a month.
- Raspberry Pi 5 8GB works — `recast-navigation` is a WebAssembly package per its npm listing, so ARM64 is fine.

### 2.3 VPS hosting — the $10/mo path

**For US players**: **OVH VPS-2 US** ($9.99/mo) wins. 6 vCore, 12 GB RAM, 100 GB NVMe, 1 Gbps unmetered, 15 US datacenters (Vint Hill = Ashburn area, Hillsboro, LA, Atlanta, Chicago, Dallas, Miami, NY, Seattle). Per-request OVH's 2026 rollout — no setup fee.

**For budget / EU-tolerant**: **Hetzner CPX22 EU** (€7.99) at Falkenstein or Helsinki. 2 AMD vCPU, 4 GB, 80 GB, 20 TB bandwidth. Adds ~120 ms transatlantic latency but half the price. Hetzner US exists but bandwidth was slashed to 1 TB in December 2024 — not usable for a real player base.

**For free**: **Oracle Cloud Always Free Ampere A1** in us-ashburn-1 or us-phoenix-1. 4 OCPU ARM, 24 GB RAM, 200 GB block, 10 TB/month egress — genuinely free forever. Two caveats: (1) capacity is scarce, you may wait days to provision; (2) idle-reclamation requires <20% CPU, network, AND memory over 7 days — any active game server clears this bar. **recast-navigation's WASM build runs fine on ARM64**, Postgres 15 and Redis 7 are native ARM64 packages.

**Do NOT use**:
- **Contabo** — documented 8–30% CPU steal, spikes to 66%. Tick loop rubberbands under any load.
- **AWS / Azure / GCP** — both burstable throttle AND egress cliff. At 5 TB/mo AWS egress costs ~$441/mo.

**Reverse proxy: Caddy**. Five-line `Caddyfile` for auto-TLS and WebSocket upgrade:
```caddyfile
sabrimmo.com {
    reverse_proxy localhost:3000 {
        stream_timeout 24h
        stream_close_delay 5m
    }
}
```
Caddy's `reverse_proxy` handles WebSocket upgrade automatically, unlike Nginx which needs 40 lines of `map $http_upgrade $connection_upgrade` plumbing. Nginx is fine if you already run it; otherwise pick Caddy.

**TLS**: Let's Encrypt works out of the box with UE5.7's FHttpModule (the engine bundles ISRG Root X1 in its cacert.pem). No client-side cert changes needed.

### 2.4 Managed-stack alternative (if you refuse to run a VM)

If you want zero server admin and are willing to pay ~7× more than a raw VPS:

- **App**: Fly.io (~$15–25/mo for 2 GB shared-CPU)
- **Redis**: **Upstash fixed-tier $10/mo** (250 MB). ⚠ **NEVER enable PAYG** — a 20 Hz tick at 100 CCU is ~$13K/mo on PAYG.
- **Postgres**: Supabase Pro $25/mo (or Railway ~$15, DigitalOcean Managed $15)
- **Total**: ~$50–65/mo for a co-located single-region stack

**Critical mitigations for managed hosting**:
- Force `transports: ['websocket']` in Socket.io client to skip polling — eliminates the sticky-session problem across Render/Fly/Railway/Heroku load balancers.
- Put Upstash Redis *inside* the Fly.io org so it's on the same bare metal (sub-ms latency). Cross-region Redis at 80–150 ms is a combat-tick killer.
- If using Neon Postgres, **disable scale-to-zero** (adds 300–1000 ms cold-start to the first hit after idle — freezes combat). With it disabled, Neon costs ~$77/mo minimum. Just use Supabase.

**Never use**:
- Upstash PAYG (see above)
- Render free tier (spins down after 15 min idle, including WebSocket messages)
- Heroku Eco (sleeps after 30 min)
- Replit autoscale deployments (documented $300–500/mo bills for persistent-WS apps)
- Cloudflare Workers / Durable Objects (can't run a Node.js monolith — would require full rewrite)

### 2.5 Cross-cutting ops

| Topic | Recommendation | Why |
|---|---|---|
| **Domain registrar** | Cloudflare Registrar ($10.46/yr .com) | At-cost, no markup, no WHOIS-privacy fee, integrates with DNS/Tunnel/SSL. |
| **DNS provider** | Cloudflare (free tier, unlimited records) | Anycast resolution, orange-cloud proxy option, Tunnel integration. |
| **Reverse proxy** | Caddy v2.8+ | Auto-TLS, auto-WebSocket, 5-line config. Nginx only if you already run it. |
| **TLS** | Let's Encrypt via Caddy ACME | Free, auto-renewed. Works with UE5 HTTP module out of the box. |
| **Process supervisor** | PM2 (Windows via `pm2-installer`; Linux native) | `pm2 logs`, `pm2 monit`, `pm2 reload` workflow. systemd fine on Linux, more verbose. |
| **Log rotation** | PM2's built-in + `pm2-logrotate` module | Sufficient for solo dev. Axiom/Better Stack if you want remote aggregation. |
| **Uptime monitoring** | UptimeRobot free (5-min interval, 50 monitors) hitting `/health` | Alerts via Discord webhook. |
| **Tick-loop heartbeat** | Healthchecks.io (20 checks free) | Cron-style; server pings it every 5 min from the tick loop. Catches silent tick-loop death. |
| **Backups** | Nightly `pg_dump` → Backblaze B2 ($0.005/GB storage) or Cloudflare R2 (free egress) | `pg_dump` is ~1 MB/month at your scale. Keep 7 daily + 4 weekly. Test restore monthly. |
| **Secret hygiene** | Secrets in `server/.env`, `.env` in `.gitignore` | **ROTATE `JWT_SECRET` before production** — the committed default is public. Rotate invalidates all issued JWTs, forcing re-login. |
| **DB password** | Rotate from current `goku22` | Anything > 16 random chars via `openssl rand -base64 24`. |
| **SSH hardening (VPS)** | Key-only auth, non-standard port, ufw allow 22/443/80 | fail2ban is overkill at this scale. |
| **Cloudflare-origin whitelist** | Optional: `ufw` allows 443 only from Cloudflare IP ranges | Closes the "someone finds your origin IP and DDoSes it bypassing CF" hole. Worth doing at Scenario C. |
| **DDoS protection** | Cloudflare Free tier (L3/L4 included) | Overkill for 50 players, but free. |

### 2.6 Scaling ceilings

- Single Node process tick loop on a dedicated 3+ GHz core: good for **~200–300 CCU** before the tick starts missing its 50 ms budget during combat-heavy zones.
- PostgreSQL on a 4 GB VPS: good for **thousands of inventory/stats writes/sec** if queries are indexed. Not your bottleneck.
- Redis on localhost: **not your bottleneck** at any realistic indie scale.
- **Bottleneck at 300+ CCU** will be Node's single-threaded event loop. Mitigation: shard by zone (one Node process per 100-player zone, separate ports, shared Postgres+Redis). This is a code-architecture change, not an ops change — your `ZONE_REGISTRY` already has the boundary.
- **Scenario C box sizing**: CCX23 ($36) for 100 CCU, AX41 ($55) for 200+, multi-box for 500+.

### 2.7 Runbook links

- **Scenario A — 5 friends, free, your Windows PC**: see [§5 End-to-End Runbook](#5-end-to-end-ship-it-runbook-scenario-a) below (complete from zero to friend-online).
- **Scenario B — VPS + Caddy + TLS**: see [Server Deployment Runbook](Server_Deployment_Runbook.md).
- **Scenario C — dedicated box**: same runbook with a larger instance. Migration: `pg_dump` + Redis `SAVE` + DNS cutover.

---

## 3. Pillar 2 — Packaging the UE5.7 Client

### 3.1 Overview

Three deliverables:
1. **Endpoint-config design** → [Client_Endpoint_Config_Design.md](Client_Endpoint_Config_Design.md) for the C++ diffs.
2. **Version handshake spec** → §3.4 below.
3. **Packaging runbook** → [Client_Packaging_Runbook.md](Client_Packaging_Runbook.md) for full RunUAT commands, Inno Setup script, and signing steps.

### 3.2 UE5.7 Shipping build essentials

Use **UE5.7.4 minimum** — the 5.7.0/.1/.2 series had packaging regressions (shader-compile, Nanite, Shipping-link failures) addressed by the 5.7.4 hotfix.

**Key `SabriMMO.Target.cs` additions** for early invitee builds:
```csharp
if (Configuration == UnrealTargetConfiguration.Shipping)
{
    bUseLoggingInShipping = true;     // keep UE_LOG(Warning+) in Shipping
    bUseChecksInShipping  = false;    // strip asserts (perf)
}
if (Configuration == UnrealTargetConfiguration.Shipping ||
    Configuration == UnrealTargetConfiguration.Test)
{
    bBuildDeveloperTools     = false;
    bBuildWithEditorOnlyData = false;
}
```

**Don't ship the template name**. `DefaultGame.ini` currently still reads `ProjectName=Third Person Game Template`. Replace:
```ini
[/Script/EngineSettings.GeneralProjectSettings]
ProjectID=659C51EC416A312DBCBD83AD710448DE
ProjectName=Sabri MMO
ProjectVersion=0.4.0
CompanyName=Sabri Dev
CopyrightNotice=Copyright 2026 Sabri Dev
```

**`.pak` vs IoStore**: keep both on by default (`-pak -iostore -compressed`). IoStore (`.ucas`/`.utoc`) is UE5's faster loader and is the default in 5.7.

**Oodle compression is free** in UE5.7 under Epic's EULA since 2023. Leave `Kraken`/`Mermaid` enabled — cuts pak size 30–50%.

**Pak encryption**: skip for 5–50 invitees. The AES key has to live in `Config/DefaultCrypto.ini` (plaintext in repo, compiled into exe), so it only blocks casual FModel extraction. Adds cook time and load-time CPU cost. Revisit only if you sign NDAs.

**Expected package size**: 1.2–2.0 GB for SabriMMO (textures dominate; 500+ atlases + BGM mp3s). Reduce by:
- Auditing `Plugins` in `.uproject` — disable `ChaosClothAsset`, `WaterExtras`, `NNE`, `OpenXR`, `VirtualCamera` if unused (50–150 MB savings)
- Setting `DisableEnginePluginsByDefault=True` in `.uproject` and whitelisting only what's needed
- Using OPUS for BGM instead of WAV (10 MB → ~1 MB each)
- Running `UnrealPak.exe -List` on the output pak and grepping for unintended content (see pre-flight checklist)

**What's NOT cooked by default** (safe to leave in repo):
- `server/`, `database/`, `docsNew/`, `_journal/`, `_prompts/`, `RagnaCloneDocs/`, `2D animations/` — all outside `Content/`, never touched by the cooker.
- `.blend`, `.psd`, `.xcf` — only cooked if a .uasset references them, which generally it doesn't.

### 3.3 Endpoint-config design (production deployment seam)

**The problem**: `UMMOGameInstance::ServerBaseUrl` defaults to `http://localhost:3001` (line 118 of `MMOGameInstance.h`). Both `MMOHttpManager::GetServerBaseUrl` and `MMOGameInstance::GetServerSocketUrl` read from it. A Shipping build that hits `http://localhost:3001` is useless to the invitee.

**Design decision** (see [Client_Endpoint_Config_Design.md](Client_Endpoint_Config_Design.md) for full C++ diffs):

1. **Default URL** lives in a new `MasterServerUrl` constant in `SabriMMO.Build.cs` → compile-time `#define SABRI_DEFAULT_MASTER_URL "https://sabrimmo.com"`. Baked into the binary so Shipping builds don't need any config file.
2. **Startup resolves order**:
   1. Command-line `-server=host:port` (overrides for QA / bug repros — read via `FParse::Value(FCommandLine::Get(), TEXT("server="), ServerArg)`)
   2. `UOptionsSaveGame::CustomServerUrl` (user-settable in an advanced Options panel; null/empty falls through)
   3. Compile-time `SABRI_DEFAULT_MASTER_URL` (production default)
3. **`/api/servers`** stays the secondary layer — the master URL above is hit for the server list, then the player picks a server and `SelectServer()` updates `ServerBaseUrl` as today. This means you can add/move game servers without re-shipping the client.
4. **TLS scheme**: production URL is `https://` / `wss://`. Derive WSS URL from `ServerBaseUrl` by replacing `http` → `ws` (already in `GetServerSocketUrl`). No plugin changes needed; the getnamo SocketIOClient plugin handles WSS when the URL starts with `wss://`.
5. **SocketIOClient plugin caveat** (⚠ plugin bug): **set `bShouldSkipCertificateVerification = true`** on the SocketIOClientComponent. Plugin issue #303 — `bShouldVerifyTLSCertificate` always fails in v2.11.0. This means a MITM with any cert can decrypt Socket.io traffic. It's still better than plain WS, but don't lean on TLS for cheat protection — your server-authoritative validation is the real defense.
6. **UE5 HTTP module** (used by `FHttpModule` for REST) uses its own bundled `cacert.pem` (NOT Windows root store). Let's Encrypt's ISRG Root X1 is already in the engine bundle, so `https://sabrimmo.com` Just Works. If you ever hit `libcurl error: 60`, append the missing root to `{Project}/Content/Certificates/cacert.pem` and add `Certificates` to *Project Settings → Packaging → Additional Non-Asset Directories to Package*.

See `Client_Endpoint_Config_Design.md` for the exact code.

### 3.4 Version handshake spec

**Goal**: an old client can't silently connect to a newer server (socket handshake would succeed but event payloads would mismatch → bug reports).

**Spec**:
- **Source of truth**: `ProjectVersion` in `Config/DefaultGame.ini` (e.g., `0.4.2`). Read at runtime via `UGeneralProjectSettings::Get()->ProjectVersion`.
- **Server publishes** its expected `MIN_CLIENT_VERSION` and its own `SERVER_VERSION` on `/health`:
  ```json
  GET /health → {
    "status": "OK",
    "serverVersion": "0.4.2",
    "minClientVersion": "0.4.0",
    "timestamp": "2026-04-16T18:00:00.000Z"
  }
  ```
  Server code lives alongside the existing `/health` handler in `server/src/index.js` line 34142. Bump `MIN_CLIENT_VERSION` whenever a Socket.io event shape changes.
- **Client checks in `HealthCheck()`** (`MMOHttpManager.cpp:58`) before any login. If `client < minClient`, display a modal "Please update" with the download URL (production: `https://sabrimmo.com/download` redirects to your itch.io page) and disable the login button.
- **Semver comparison** via a small util `FVersion::Compare(A, B)`. Patch-version mismatches are informational only; minor/major are blocking.
- **Rollback UX**: if you ship a broken server, reduce `MIN_CLIENT_VERSION` in `server/.env` to let older clients back in while you investigate.

**Alternative**: include version in the socket handshake payload (`player:join` already exists as a seam — extend with `clientVersion`). Server disconnects with a reason code. Slightly noisier because by then the client is already past login. The `/health`-up-front check is cleaner.

**Build-ID bake-in**: `scripts/build_ship.bat` writes `BUILD_ID.txt` alongside `SabriMMO.exe` with git SHA + timestamp. Display in the login screen's debug footer so invitees can tell you exactly which build they're on.

### 3.5 Installer

**Use Inno Setup 6.x**. Free, mature, Pascal scripting, first-class prereq handling. Output is ~1.5 GB `.exe` installer with embedded UE prereqs (VC++ redist 2015–2022, DirectX).

Full `.iss` script template in [Client_Packaging_Runbook.md](Client_Packaging_Runbook.md). Highlights:
- Installs per-user to `%LOCALAPPDATA%\SabriMMO` (no admin required, except for prereq run)
- Runs `UEPrereqSetup_x64.exe /quiet /norestart` silently via `[Run]` section
- Skips the prereq run if VC++ 2015–2022 x64 runtime is already present (via `[Code]` registry check)
- Creates Start Menu + optional Desktop shortcuts

**Skip**: MSIX (requires code signing to sideload), Advanced Installer ($499/yr), WiX (enterprise-heavy), NSIS (syntax inferior to Inno).

### 3.6 Code signing

**Decision by invitee count**:

| Scale | Sign? | Product | Cost |
|---|---|---|---|
| **5–10** | ❌ No | — | $0 |
| **11–50** | ❌ No; submit to Microsoft Defender on every build | — | $0 |
| **50–100** | ⚠ Borderline, depends on complaints | Azure Trusted Signing ($120/yr) or Sectigo OV (~$215/yr w/USB token) | $120–215/yr |
| **100–500** | ✅ Yes | Azure Trusted Signing | $120/yr |
| **500+** | ✅ Yes, EV | Sectigo EV (~$280/yr 3-yr deal) | $280/yr |

**2026 reality check**:
- **Self-signed certs do NOT bypass SmartScreen**. Don't bother.
- **OV and EV certs both require hardware** since June 2023 (CA/Browser Forum rule): USB token or cloud HSM. No downloadable `.pfx`.
- **Azure Trusted Signing** was the sweet spot at $9.99/mo, but **Microsoft paused new individual-developer signups in April 2025**. You now need an incorporated entity (LLC) with 3+ years of tax history, in US/CA/UK/EU. If you already provisioned an individual identity before April 2025 it still works. Otherwise: incorporate an LLC (~$100 state filing, 1–3 hours), or use Sectigo OV with a USB token.
- **SmartScreen reputation**: even EV certs no longer grant instant reputation as of March 2024. OV certs build reputation over 3–6 months of signed download telemetry. Signing your first 50-invitee build gives you ~0 reputation benefit on day 1 but accumulates over time.

**Pragmatic**: don't bother signing at <50 invitees. Instead, on every build:
1. Submit `SabriMMO-Win64-Shipping.exe` and `SabriMMO-Setup-0.4.2.exe` to Microsoft Defender at https://www.microsoft.com/en-us/wdsi/filesubmission → "Software developer – false positive". 24–72 hr turnaround.
2. Include a `README.md` with your installer telling invitees "If SmartScreen warns, click More info → Run anyway."

### 3.7 Crash reporting

**Use `sentry-unreal` plugin + Sentry free tier**.
- Free tier: 5,000 errors/month, 10,000 performance events, 50 session replays. More than enough for 50 invitees.
- Plugin: https://github.com/getsentry/sentry-unreal (Fab marketplace or GitHub release).
- Setup: 5–10 minutes. Paste DSN into *Project Settings → Plugins → Sentry*, the plugin writes CRC's `DataRouterUrl` automatically.
- Upload debug symbols via `sentry-cli upload-dif` so you get symbolicated stack traces.

**Alternative**: GlitchTip (self-hosted Sentry-compatible) if you don't want third-party data capture. Same plugin, different DSN.

### 3.8 Auto-updater

**For 5–50 scale**: **custom version-handshake is enough**. No launcher needed.

Flow:
1. Client hits `GET /health` before login.
2. If `client.version < server.minClientVersion`, modal shows "Please update" with download URL → itch.io page.
3. User downloads + runs new installer → replaces install → relaunches.
4. itch.io app users get auto-delta-patched by butler (see §4).

**Above 50**: consider **Velopack** (https://velopack.io/) — modern Rust-based delta updater, handles code signing, background download, ~2 s relaunch. Successor to Squirrel.Windows. Supports non-.NET UE games via launcher exe wrapper. Free.

**Never build a bespoke C++ launcher for this** — Velopack + itch.io app handle everything the launcher would.

### 3.9 Pre-flight checklist (every build before handing out)

```
[ ] SabriMMO.Target.cs has bUseLoggingInShipping = true
[ ] SabriMMOEditor.Target.cs only referenced from Editor target
[ ] Config/DefaultGame.ini ProjectName = "Sabri MMO" (not "Third Person Game Template")
[ ] Config/DefaultGame.ini ProjectVersion bumped
[ ] BUILD_ID.txt exists alongside SabriMMO.exe
[ ] ServerBaseUrl default points at production (https://sabrimmo.com), not localhost
[ ] SocketIOClientComponent: bShouldUseTlsLibraries=true, bShouldSkipCertificateVerification=true
[ ] UnrealPak.exe -List → no "_journal" / "docsNew" / "RagnaCloneDocs" / "server" / ".blend" matches
[ ] Package size is 1.2-4.0 GB (flag if outside)
[ ] UEPrereqSetup_x64.exe bundled in Engine/Extras/Redist/en-us/
[ ] Crash Reporter included (CrashReportClient.exe present)
[ ] Sentry DSN configured in Project Settings → Plugins → Sentry
[ ] PDB symbols uploaded to Sentry via sentry-cli
[ ] Tested /health handshake against production server (minClientVersion correct)
[ ] Test user accounts exist in prod DB for all invitees
[ ] Inno Setup compiled installer (SabriMMO-Setup-X.Y.Z.exe)
[ ] If signing: both SabriMMO.exe and SabriMMO-Setup.exe signed and timestamped
[ ] Installer tested on a clean Windows 11 VM (no VC++ redist, no previous install)
[ ] SabriMMO-Win64-Shipping.exe submitted to Microsoft Defender FP form
[ ] README.md with "SmartScreen → More info → Run anyway" instructions
[ ] Changelog posted to Discord #announcements
```

---

## 4. Pillar 3 — Distribution

### 4.1 Channel decision

| Channel | 1–5 friends | 10–30 testers | 50+ | Verdict |
|---|---|---|---|---|
| **Discord DM + ZIP** | ✅ | ✗ (500 MB max + 24h URL expiry) | ✗ | Only for 5 friends with a one-shot link. |
| **itch.io Restricted + download keys + butler** | ✅ | ✅ **Primary winner** | ✅ | Free, unlisted, delta updates, per-invitee keys revocable. |
| **Cloudflare R2 + custom domain (signed URLs)** | ✅ | ✅ | ✅ **Winner if itch DMCAs** | $0 egress, $0.015/GB storage, no storefront exposure. ~$0.05/mo. |
| **Backblaze B2 + Cloudflare CDN** | ✅ | ✅ | ✅ | Alternative to R2. $0.005/GB storage. |
| **GitHub Releases (private org repo)** | ✅ (free tier = 3 collaborators) | ⚠ (need org) | ⚠ | 2 GB/asset cap forces split. |
| **Discord server #downloads (link only)** | ✅ | ✅ | ✅ | Community host, NOT binary host. Pair with R2/itch URL. |
| **Steam Playtest / private beta branches** | — | — | — | ❌ **Skip**. 2-week Coming-Soon window + IP exposure + $100 + 6–8 wk. See §9. |
| **Epic Games Store / GOG** | — | — | — | ❌ No invite-only SKU. IP-reviewed at submission. |
| **itch.io Public (unlisted)** | — | — | — | ❌ Still indexable; weaker than Restricted. |
| **Google Drive / Dropbox / OneDrive** | ⚠ | ✗ | ✗ | Malware scans + rate limits + 2 GB caps. Not reliable. |
| **MEGA** | ✅ | ⚠ | ✗ | 5 GB/6h transfer quota per IP frustrates users mid-download. |
| **WeTransfer / Smash** | — | — | — | ❌ Links expire 7/30 days. |
| **Torrent (private) / Resilio Sync** | ⚠ | ⚠ | ⚠ | High friction for non-technical testers. |
| **Self-hosted launcher** | — | — | ⚠ | Don't build one at <50. Velopack at 50+ if needed. |

### 4.2 itch.io Restricted — the primary channel

**Why it wins**:
- **Restricted mode**: project not listed in itch browse/search, de-indexed from external search engines.
- **Download keys**: bulk-generate N keys, each labeled (e.g., "Alice alpha tester"). Invitee clicks URL, itch.io attaches the key to their account — per-invitee revocation.
- **butler** (itch CLI): delta patches. First push uploads 2–4 GB; subsequent pushes upload only changed bytes (typically 50–200 MB). The itch.io desktop app applies deltas automatically.
- **Free** unless you charge for the game.
- **File size**: butler accepts up to 30 GB uncompressed per upload. Your 2–4 GB build fits trivially.

**Operational hygiene** (critical given IP sensitivity):
- **Never use "Ragnarok" / "RO" / "Poring" / "Acolyte" in the itch.io page title, tags, or description.** Name the game something original. Describe as "classic Korean-MMO-inspired action RPG."
- **Restricted mode is not a force field** — a disgruntled invitee could leak the URL. Your server JWT still gates actual play; losing the URL is not catastrophic. But itch.io will DMCA-takedown on notice (they already have for RO-derived content historically), and that loses you the channel.

**Workflow**:
1. Create account + game at itch.io. Set **Visibility = Restricted** on Distribute tab.
2. Generate bulk download keys: Distribute → Download keys → Bulk Generate N → label each.
3. Install butler: `go install github.com/itchio/butler@latest` (or download from itch).
4. Configure: `butler login`.
5. Push build: `butler push ./Dist/SabriMMO-Win64-Shipping/Windows/SabriMMO yourname/sabrimmo:windows --userversion 0.4.2`
6. Email each invitee their key URL individually.
7. On new build: bump version in `DefaultGame.ini`, rebuild, `butler push ...`. Users who have the itch.io app auto-delta-update on next launch.

### 4.3 Cloudflare R2 fallback (if itch DMCAs you)

Set up now so it's ready as a fallback:
1. Sign up for Cloudflare R2 (free tier 10 GB storage + unlimited egress).
2. `rclone` or AWS CLI — upload `SabriMMO-Setup-0.4.2.exe` to a bucket.
3. Point a subdomain (e.g., `builds.sabrimmo.com`) at the bucket.
4. Issue signed URLs from your existing Node.js server's authenticated `/api/download` endpoint (JWT-gated): `GET /api/download?version=0.4.2` → 302 redirect to 1-hour-signed R2 URL.
5. Total cost at 3 GB × 50 downloads = $0.05/mo storage + $0 egress = **under $1/year**.

### 4.4 Discord (community, not binary)

- **File upload caps 2026**: Free 10 MB, Nitro Basic 50 MB, Nitro 500 MB, Server Boost L3 150 MB. None fit a 2–4 GB build.
- **Attachment URL expiry (Dec 2023)**: CDN links expire every ~24 h outside the Discord client. You can't pin a download link and walk away.
- **Use Discord for**: private invite-only server, #announcements channel, #bugs channel, patch notes, voice chat. **Distribution link always points to itch.io or R2.**
- **Bot for invite-code gating**: MEE6, Carl-bot, or custom — auto-assign a role when user posts their register code (matching the one embedded in their installer).

### 4.5 Access control

| Layer | Status | Effort | Strength |
|---|---|---|---|
| **Server JWT login** | ✅ Already in place | 0 | Strong — no playing without a registered account. |
| **Per-invitee register code** | ⏳ New (~2 hrs code) | Low | Medium — invitee shares installer but not the code. Code burns on first register. |
| **HWID soft lock** | ⏳ Possible | Medium | Weak (HWIDs change on reinstall). More support burden than value. |
| **Online-only + ban button** | ✅ Already in place (server-authoritative) | 0 | Strong — you can ban instantly. |

**Recommendation**: JWT + online-only + register code (for 10+ invitees). Skip HWID lock. Steam's hardware-lock UX is invisible because Valve owns the OS-level integration; rolling your own is a support nightmare.

### 4.6 Updates (per channel)

| Channel | User update experience |
|---|---|
| itch.io app | Auto-delta on next launch (best case). |
| itch.io web download | Manual re-download. |
| R2 / Backblaze direct | Manual re-download (or via launcher). |
| Discord link | Same as R2 (link points to R2). |
| GitHub Releases | Manual re-download. |
| Velopack launcher | Auto-delta on launch. |

**Your version handshake (§3.4) makes the update pipeline self-enforcing**: old clients can't connect to the new server, so they're forced to update. Without the handshake, a user with a stale client produces silent bugs.

### 4.7 Invitee onboarding doc template

Save as `docsNew/Invitee_Onboarding.md` and ship with each build. Copy-paste into a Discord welcome channel or email:

> ### Welcome to Sabri MMO Closed Alpha
>
> Thanks for joining. Here's how to get in.
>
> **Step 1 — Your itch.io key**
> Check your email for a link like `https://yourname.itch.io/sabrimmo/download/xxxxx`. Click it, log in or create an itch.io account — the game gets added to your library permanently.
>
> **Step 2 — Install (two options)**
> - **Recommended**: install the itch.io desktop app (https://itch.io/app). Log in with the same account, find Sabri MMO in your library, click Install. The app auto-patches future builds.
> - **Alternative**: direct-download the ZIP from the game page, extract anywhere, run `SabriMMO.exe`. You'll manually re-download for updates.
>
> **Step 3 — Log in**
> Launch `SabriMMO.exe`. Enter the username and password I sent you in a separate email. **The game requires an internet connection — there's no offline mode.**
>
> **Step 4 — If it doesn't launch**
> - If Windows SmartScreen warns: **More info → Run anyway**.
> - Install the Visual C++ Redistributable x64 (https://aka.ms/vs/17/release/vc_redist.x64.exe) — usually handled by the installer, occasionally missed.
> - Your game needs outbound TCP 443 open (it connects to `sabrimmo.com:443` over HTTPS/WSS).
> - If login says "version mismatch": update via the itch.io app, or re-download from the itch.io page.
>
> **Step 5 — Updates**
> I post in Discord #announcements when a new build is live. The itch.io app picks it up automatically. Direct-download users: grab the new ZIP.
>
> **Step 6 — Feedback + crashes**
> Bugs → `#bugs`. Crashes: the game reports them automatically via Sentry. If you can, include a brief description of what you were doing in `#bugs`.
>
> **Step 7 — Please don't share**
> The alpha is invite-only. The less it gets shared, the longer it lives. If a friend asks, tell them to DM me.

---

## 5. End-to-End "Ship It" Runbook (Scenario A)

Zero → a friend is online, running on your own hardware, with 5 friends invited. Budget: $0 (plus electricity). Time: ~2 hours.

**Pick your server hardware first** (see §2.2 for full track comparison):

| If you're using… | Follow these steps |
|---|---|
| **Gaming PC or spare Windows PC** (Tracks 1A/1B) | Steps 1–3 below using Windows stack (PM2 via `pm2-installer`, EDB Postgres, Memurai Redis) |
| **Spare PC converted to Linux, or N100 mini PC, or Pi 5** (Tracks 2/3) | Same steps but swap commands from [Server_Deployment_Runbook.md §4](Server_Deployment_Runbook.md) (systemd, apt Postgres, apt Redis) |

The Windows track is shown below. Linux equivalents are in the runbook — same ~2-hour time budget.

### Step 1 — Server side (Windows track)

1. **Install PostgreSQL 15** via EDB installer (https://www.postgresql.org/download/windows/). Set a password (generate via PowerShell: `[Convert]::ToBase64String((1..24 | % { [byte](Get-Random -Max 256) }))`), leave port 5432.
2. **Install Memurai Developer** (https://www.memurai.com/). Free Windows Redis replacement, auto-starts as a service.
3. **Install Node.js 20 LTS** (https://nodejs.org/).
4. **Clone + configure**:
   ```powershell
   cd C:\Sabri_MMO\server
   npm install
   ```
5. **Edit `server/.env`** — rotate `DB_PASSWORD` and `JWT_SECRET` (see Step 1 password generator). Also set `MIN_CLIENT_VERSION=0.4.0` and `SERVER_VERSION=0.4.0` for the version handshake.
6. **Create the database** via pgAdmin: `CREATE DATABASE sabri_mmo;` then `\i database/init.sql` in psql.
7. **Install PM2 + pm2-installer**:
   ```powershell
   # Elevated PowerShell
   git clone https://github.com/jessety/pm2-installer C:\Tools\pm2-installer
   cd C:\Tools\pm2-installer
   npm run configure
   npm run configure-policy
   npm run setup
   # New elevated PowerShell:
   pm2 start C:\Sabri_MMO\server\src\index.js --name sabri-mmo
   pm2 save
   ```
8. **Verify locally**: `curl http://localhost:3001/health` returns JSON with `status: "OK"`, `serverVersion`, `minClientVersion`.

### Step 1 (Linux alternative — spare PC converted, or mini PC)

Follow [Server_Deployment_Runbook.md §4](Server_Deployment_Runbook.md) which gives the apt commands for Debian/Ubuntu: `apt install postgresql redis-server`, NodeSource Node 20, systemd unit, `.env`, `journalctl -u sabri-mmo`. Same ~30 minutes once the OS is installed.

### Step 2 — Tailscale mesh

1. Install Tailscale on the server from https://tailscale.com/download (Linux: `curl -fsSL https://tailscale.com/install.sh | sh`). `tailscale up`. Note the 100.x.y.z IP.
2. From Tailscale admin console (https://login.tailscale.com/admin), invite 5 friends by email (Personal plan: 6 users, 100 devices, free).
3. Each friend installs Tailscale, accepts invite, logs in.
4. From a friend's machine: `ping 100.x.y.z` should succeed.

### Step 3 — Client side (UE5.7)

1. Implement the endpoint-config design from [Client_Endpoint_Config_Design.md](Client_Endpoint_Config_Design.md) — add `UOptionsSaveGame::CustomServerUrl` and the resolution order. Compile.
2. In the editor, run with `-server=100.x.y.z:3001` on the command line, verify login + move around.
3. Follow the [Client Packaging Runbook](Client_Packaging_Runbook.md) to produce a Shipping build:
   ```bat
   scripts\build_ship.bat
   ```
4. `UnrealPak.exe -List` → verify no `_journal` / `docsNew` / `RagnaCloneDocs` / `.blend` leaks.
5. Build Inno Setup installer: `ISCC.exe installer\SabriMMO.iss`.
6. Submit both exes to Microsoft Defender FP form.

### Step 4 — Distribute

1. Create an itch.io account, create `sabrimmo` project, set **Restricted** visibility.
2. Install butler: `butler login`.
3. `butler push Dist\SabriMMO-Win64-Shipping\Windows\SabriMMO yourname/sabrimmo:windows --userversion 0.4.0`
4. Generate 5 download keys labeled per friend.
5. Email each friend:
   - Their itch key URL
   - Their in-game username + password (you create accounts directly in the DB for scenario A, or build a register flow)
   - Installer instructions (from §4.7)
6. Tell them to use `-server=100.x.y.z:3001` (pass via a desktop shortcut's Target field) since Scenario A has no domain.

**Alternative to step 6**: bake the Tailscale IP into the build via a named "Dev" entry in `UOptionsSaveGame::CustomServerUrl` and ship a tiny script that writes it to the SaveGame slot. Simpler for non-technical friends.

### Step 5 — Verify end to end

- Friend launches game → login → character select → spawn → sees you move.
- Check `server.log` shows their `player:join`.
- Check Sentry dashboard for any errors.

**Total**: $0/mo, zero public IP exposure, 5 friends can play.

---

## 6. Future Migration Path

```
Scenario A (5 friends, Tailscale + your own hardware)
    │
    │ (triggered by: outgrow 6-user Tailscale cap,
    │  or want a public URL for a fresh test cohort)
    ▼
Scenario B (20–50 invitees, SAME own hardware + Cloudflare Tunnel + domain)
    │
    │ (triggered by: home upload saturates, ISP complaint,
    │  unreliable uptime, 80+ concurrent on a mini PC's CPU,
    │  or want the server online when your PC is off)
    ▼
Scenario B-VPS (same player count, moved to OVH VPS-2 / Oracle Free)
    │
    │ (triggered by: 100+ concurrent, bandwidth or CPU
    │  on a single box is saturated, want colocation
    │  closer to a distant player base)
    ▼
Scenario C (Hetzner CCX23 US or AX41 dedicated)
    │
    │ (triggered by: 300+ CCU, tick loop missing deadlines)
    ▼
Zone-sharded multi-process (code change — out of scope)
```

**The key insight**: Scenario B can stay on your own hardware. The transition from A to B is "add a domain and Cloudflare Tunnel" — no hardware change. You only *need* a VPS if one of these becomes true:
- Your home upload consistently saturates during peak play
- Your ISP sends an AUP warning
- You want the server online while the host PC is off/updating
- You want colocation near a player base that's far from your home
- You're approaching ~100 CCU and the tick loop is starting to miss deadlines on your mini PC

**What changes A → B (same hardware, just add public URL)**:
- Buy a domain at Cloudflare Registrar ($10.46/yr).
- Install `cloudflared` on the server, create tunnel, route `sabrimmo.com` to `localhost:3001`.
- Update `UMMOGameInstance::SABRI_DEFAULT_MASTER_URL` in `SabriMMO.Build.cs` to `https://sabrimmo.com`, rebuild client.
- Push new itch.io build.
- Issue itch download keys to new invitees.
- Old Tailscale friends still work via Tailscale IP OR the new `sabrimmo.com` URL — both point to the same server process.
- **No `pg_dump`, no data migration, no downtime.**

**What changes B (own hardware) → B-VPS (remote hardware)**:
- Provision VPS (Oracle Free A1, OVH VPS-2, or similar). 1 hour.
- On server: `pg_dump -Fc sabri_mmo > sabri_mmo.dump`. Copy to VPS via rsync/scp.
- On VPS: `pg_restore -d sabri_mmo sabri_mmo.dump`.
- Copy Redis RDB snapshot (`redis-cli SAVE` then `scp dump.rdb`).
- `git clone` server code, install Node/PM2/systemd, configure `.env`, start.
- Install Caddy + Let's Encrypt (direct public IP now, no tunnel).
- DNS: swap the `sabrimmo.com` A record from tunnel target to VPS IP. TTL=60 during swap.
- **Client code is unchanged** — it's still pointed at `sabrimmo.com`.
- Retire the home Tunnel once the VPS is stable.

**What changes B-VPS → C**:
- Upsize VPS via provider console (Hetzner/OVH/DO/Linode resize keeps IP within same region in most cases). OR
- Provision a new Hetzner AX41/CCX23 and migrate (DNS TTL to 60 s, `pg_dump`+restore, swap DNS, retire old box).
- No client code changes needed.
- Consider Cloudflare Zone whitelist on the origin (only allow :443 from Cloudflare IPs) to harden against direct-IP DDoS.

**Lock-in to avoid now**:
- **Don't** bake the server IP into the client. The design uses a domain (`sabrimmo.com`) that you control DNS for, so server IPs can change at will.
- **Don't** pick Upstash PAYG even for dev — the free tier is fine, PAYG will bite you on scale. Set a hard spending cap of $15 on Upstash as a safety net if you ever use it.
- **Don't** use Neon Postgres — the branching is cool but scale-to-zero cold start is incompatible with a game server. Supabase / DO Managed / raw Postgres migrate cleanly between scenarios.
- **Don't** pick a hosting provider without a data-export path. All the ones recommended (Hetzner, OVH, DO, Linode, Vultr, Oracle, self-host) support `pg_dump` egress at standard rates.
- **Avoid Steam integration until you're sure you want to go public.** It's 20+ hours of engine work that becomes sunk cost if you stay private.

---

## 7. Appendix A — Cost Summary

### A.1 Scenarios using your own hardware (recommended A and B)

| Scenario | Hardware | Monthly | Annual | Notes |
|---|---|---|---|---|
| **A1** Gaming PC (shared) + Tailscale | — | $11.52 | $138 | 100 W × 24/7 × $0.16/kWh. No domain needed inside Tailscale. |
| **A2** Spare Windows PC (100 W) + Tailscale | — | $11.52 | $138 | Same electricity profile if it's a full desktop. |
| **A3** Spare Windows PC (40 W laptop/SFF) + Tailscale | — | $4.61 | $55 | Smaller form-factor PCs idle lower. |
| **A4** N100 mini PC (Linux) + Tailscale | $250 one-time | $1.15 | $14 | 10 W idle. Pays back vs gaming PC in ~2 yr on electricity alone. |
| **A5** Raspberry Pi 5 8 GB + NVMe hat + Tailscale | $150 one-time | $0.92 | $11 | 8 W idle. Cheapest viable dedicated server. |
| **B1** Same own hardware + Cloudflare Tunnel + .com domain | — | (A# cost) | (A# cost) + $10.46 | No VPS cost. Just add domain. |

### A.2 Scenarios using a VPS (only when own hardware isn't viable)

| Scenario | VPS | Monthly | Annual | Notes |
|---|---|---|---|---|
| **B-VPS-Oracle** Oracle Free A1 ARM | Free | $0 | $0 + $10.46 domain | Genuinely free forever. Capacity gamble. |
| **B-VPS-OVH** OVH VPS-2 US | — | $9.99 | $120 + $10.46 domain | 6 vCore, 12 GB, NVMe, unmetered. |
| **B-VPS-Hetzner** Hetzner CPX22 EU | — | €7.99 (~$8.60) | $103 + $10.46 domain | 120 ms to US. Cheapest. |
| **C1** Hetzner CCX23 US | — | $35.69 | $428 + $10.46 domain | 4 dedicated AMD, 16 GB. Best for 100–200 CCU US. |
| **C2** Hetzner AX41-NVMe EU (dedicated) | — | ~$55 | $660 + $10.46 domain | 6c/12t Ryzen, 64 GB. Huge headroom. 120 ms to US. |

### A.3 Other costs (apply to both own-hardware and VPS)

| Item | Monthly | Annual | Notes |
|---|---|---|---|
| Sentry free tier | $0 | $0 | 5K errors/mo. Paid +$26/mo if you exceed. |
| Cloudflare Tunnel | $0 | $0 | Up to ~100K concurrent WS per domain. |
| Cloudflare DNS | $0 | $0 | Free plan covers what you need. |
| UptimeRobot | $0 | $0 | 50 monitors, 5-min interval. |
| Healthchecks.io | $0 | $0 | 20 checks. |
| Backblaze B2 (pg_dump backups) | ~$0.05 | ~$1 | 1 MB/day dumps × 30 days retention. |
| itch.io hosting | $0 | $0 | Restricted mode + butler, free uploads. |
| Cloudflare R2 builds bucket | ~$0.05 | ~$1 | ~3 GB stored + zero egress. |
| Azure Trusted Signing (if LLC) | $9.99 | $120 | Only if signing. Otherwise $0. |

### A.4 Putting it together

| Path | Monthly | Annual |
|---|---|---|
| **Cheapest self-host (A5 — Pi 5, Tailscale, no domain)** | ~$1 | ~$11 |
| **Typical self-host (A4 — N100 Linux, Tunnel, domain)** | ~$2 | ~$25 |
| **Spare-Windows-PC self-host (B1 via A2, Tunnel, domain)** | ~$12 | ~$148 |
| **Gaming-PC self-host + Tunnel + domain** | ~$12 | ~$148 |
| **VPS (Oracle Free + domain)** | <$1 | $11 |
| **VPS (OVH VPS-2 + domain)** | ~$11 | ~$131 |
| **Growth (Hetzner CCX23 + domain + Sentry)** | ~$72 | ~$870 |

### A.5 Not in table (one-time or ad-hoc)

- LLC incorporation (state filing): ~$100–300 one-time + annual state fees. Only needed for Azure Trusted Signing access.
- Spare hardware purchase if you don't already have it:
  - N100 mini PC: ~$250 (Beelink EQ12 or similar)
  - Raspberry Pi 5 8GB + NVMe hat + 512GB SSD: ~$150
  - Both pay back vs a gaming-PC-server's electricity in 2 years.

---

## 8. Appendix B — Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| **Gravity Co. IP takedown (itch.io)** | Low if names scrubbed on page; High if explicit | Loss of distribution channel | See §9. Rename all public-facing RO terms. Fallback: R2 with signed URLs. |
| **Gravity Co. lawsuit (commercialization)** | Low for non-commercial private; High if monetized | Up to $4M (NovaRO precedent) | **Never monetize** (no Patreon, no donations, no in-game purchases). Keep private. Asset-scrub if going public. |
| **ISP TOS notice (residential hosting)** | Very low at <50 players | Account termination | Scenario A fallback: move to Scenario B (VPS) within days. Google Fiber if available is explicitly OK. |
| **CGNAT blocks inbound (Scenario A)** | Depends on ISP | Inbound unreachable | Tailscale mesh sidesteps it. Cloudflare Tunnel also sidesteps. |
| **SocketIOClient plugin TLS verification disabled (issue #303)** | Certain | MITM can read/modify game packets | Server-authoritative validation is the real defense. Keep JWT signing. Don't store anything in socket packets you'd fear in plaintext. |
| **SmartScreen / antivirus false positives on unsigned build** | ~30–40% bounce rate from first-time installers | Support tickets | Submit to Defender on every build. README instructions "More info → Run anyway". Sign at 50+ invitees. |
| **Upstash PAYG accidental enable** | Low if aware | Catastrophic ($5K–13K/mo bill) | Set $15 hard spending cap. Use fixed-tier only. **Don't use Upstash unless you've read this risk.** |
| **Neon scale-to-zero cold start** | Certain if enabled | Player experiences frozen hits | Never use Neon for a game server. Use Supabase, DO Managed, or raw Postgres. |
| **Steam public Coming-Soon page exposes IP-sensitive app** | Certain | Channel termination (app loss) | **Don't use Steam.** If you ever want to: fully scrub all RO references first. |
| **Invitee leaks installer / itch URL** | Medium over time | Unknown third parties trying to play | Server JWT account gate. Per-invitee register codes. Ban-button. |
| **Gaming PC crashes during server uptime** | Occasional | 5-min outage for friends | Auto-restart via PM2 at process level + Windows scheduled task at OS level. UptimeRobot alert. |
| **Combat tick loop stalls under Shipping build heavy load** | Possible | Rubberbanding | Log tick budget warnings via `bUseLoggingInShipping`. Profile with `stat unit` in a Development build before shipping. |
| **Discord CDN URL expiry (24h outside client)** | Certain | Pinned link stops working | Never pin binary links directly — always link to itch.io / R2. |
| **Cloudflare 100s WebSocket idle timeout (Free/Pro)** | Mitigated | WS drop after 100s silence | Socket.io default `pingInterval: 25000` keeps the connection alive. Don't raise above 60s. |
| **UE5.7 Shipping-build OpenSSL link failure (SocketIOClient)** | Possible | Build fails or crashes on launch | Ensure `OpenSSL` and `zlib` remain in plugin's `Build.cs`; avoid aggressive IWYU pruning. Test Shipping-packaged build on clean VM. |
| **PostgreSQL DB corrupts on power loss (gaming PC)** | Low | Data loss | Nightly `pg_dump` to Backblaze B2 ($0.60/yr). Monthly restore test. |

---

## 9. Appendix C — IP / Legal Hygiene

**Context you already have** (from CLAUDE.md + memory): the game uses RO skill names, monster names, item names, and RO-adjacent sprites. Gravity Co. Ltd. won a $4M default judgment against NovaRO in 2022 (Case 2:22-cv-02763, CDCA) for running a monetized RO private server.

**What triggers enforcement** (pattern across cases):
1. **Monetization** (NovaRO case: they sold in-game items). Non-monetized private projects are lower-priority targets but still exposed on copyright.
2. **Public storefront presence** (Dreadmyst 2026: NCsoft DMCA on Steam → delisted). The Steam Coming-Soon window is the acute risk.
3. **Public community** (Discord, subreddit, Twitter) with RO branding. DMCAs flow from public awareness.
4. **Asset reuse**: sprites, audio, exact text from iRO/kRO clients, trademark strings.

**Pattern across cases**: copyright and trademark are both in play. Copyright protects specific expression (sprites, code, audio). Trademark protects brand identity (names: "Ragnarok", "Poring", "Kafra", "Acolyte").

**Practical hygiene for SabriMMO (private, invite-only)**:

| Layer | Risk | Action |
|---|---|---|
| Server code internals (`ro_monster_templates.js`, `ro_skill_data.js`) | Low — not distributed | Leave as-is. Private Node.js source. |
| Client sprites / assets | Medium — cooked into pak, shipped to invitees | If sprites are AI-generated or original, fine. If they're pixel-derived from iRO client, higher risk. Audit by checking against iRO client pixel data. |
| Distribution page (itch.io, Discord server name, announcements) | **High — public surface** | **Scrub**: no "Ragnarok", "RO", "Poring", "Acolyte", "Prontera", "Kafra" on any public-facing surface. Name the game something original. Describe as "classic Korean-MMO-inspired action RPG." |
| Client UI strings visible to invitee (item names, skill names) | Medium | Less critical than the distribution page. Invitee-only. But if you ever go public, all of this has to be renamed anyway. Plan the string table abstraction now. |
| Monetization | Extreme if done with RO assets | **Never monetize while using RO-derived anything.** No Patreon, no donations, no cosmetic sales, no "supporter tiers." This is the brightest line in the Gravity precedent. |

**If you ever want to go public** (Steam, Epic, general internet):
1. Full clean-room rebrand: rename all RO-trademarked strings (Poring → Jellybit, Acolyte → Healer, Prontera → StarterTown, Kafra → Nimbus Courier).
2. Original sprite audit: any sprite referenced from RO originals must be regenerated from scratch (AI + Mixamo pipeline you already have).
3. Original audio: regenerate BGM or license replacement tracks. iRO BGM is explicit copyright.
4. Original item descriptions: don't copy-paste from iRO client tooltips.
5. Only *then* consider Steam Direct + $100 + public storefront.

**Detection surface** (what a Gravity paralegal might scrape):
- Steam Coming-Soon feed (easy, automated)
- itch.io public browse (easy — Restricted mode defeats this)
- GitHub DMCA history (they've filed against `forestbelton/ragnarok`, `ROClientSide/Translation`, `roBrowser`)
- Discord server name + public invite links (easy if you publicize)
- Twitter mentions

**Detection surface you control**:
- Private Discord, Restricted itch page, private distribution URLs — all effectively invisible to automated scrapers.

---

## 10. Appendix D — Code-Level Gotchas Discovered

During the internal recon, these came up and should be tracked (don't fix in this planning pass — per the prompt's instructions — but note them):

1. **`DefaultGame.ini` still says `ProjectName=Third Person Game Template`.** Fix when implementing endpoint-config. Proper: `ProjectName=Sabri MMO`, add `ProjectVersion=0.4.0`.

2. **`server/.env` ships weak defaults**:
   - `DB_PASSWORD=goku22`
   - `JWT_SECRET=your-super-secret-jwt-key-change-this-in-production-2026`

   Both must be rotated before any production deployment. JWT rotation invalidates all issued tokens (users must re-login).

3. **`SERVER_HOST` env var is unset** in `.env`, so `/api/servers` returns `host: "localhost"`. Production `.env` must set `SERVER_HOST=sabrimmo.com` (or the public host).

4. **CORS is wide open** (`app.use(cors())` at `index.js:34049`). Scenario C should tighten to `cors({ origin: ['https://sabrimmo.com'] })` if a web client is ever added. Not urgent since only the UE5 client talks to the server; CORS is moot for non-browser clients.

5. **Rate limit is 100 req/15min on `/api/*`** (`index.js:34053–34058`). Fine for current scale. Revisit if you add burst-heavy endpoints.

6. **`/health` does a DB round-trip**. Good — external monitors will detect DB outage. Consider adding a cheap `/health/light` that skips DB for Cloudflare origin-health checks (reduces DB load from 1/5s).

7. **No `REDIS_URL` in `.env`** — server presumably defaults or fails gracefully. Verify before Scenario B rollout.

8. **SocketIOClient plugin TLS verification broken** (plugin issue #303): documented above. Must set `bShouldSkipCertificateVerification = true` and accept the MITM-feasibility caveat.

9. **UE5.7 known issues**: use 5.7.4+ for Shipping builds. Verify no regressions in the Online Subsystem Steam path if Steam is ever integrated (but we're not integrating).

10. **Inno Setup default install dir** should be `%LOCALAPPDATA%\SabriMMO` not `Program Files` — avoids admin UAC on every update. The runbook's .iss template uses `{localappdata}\{#AppName}` accordingly.

---

## 11. References

External sources are cited inline throughout the research artifacts. The seven research agents that produced this plan cited ~150 URLs across:

- UE5.7 engine docs (dev.epicgames.com)
- Valve partner docs (partner.steamgames.com)
- itch.io creator docs
- Cloudflare, Caddy, Nginx, Let's Encrypt docs
- VPS provider pricing pages (Hetzner, OVH, DigitalOcean, Linode, Vultr, Contabo, Oracle, AWS, Azure, GCP)
- US ISP residential AUPs (Comcast, Spectrum, Cox, Verizon, AT&T, T-Mobile, Google Fiber)
- Case law summaries (Gravity v. NovaRO, Dreadmyst delisting, Myth of Empires)
- SocketIOClient-Unreal plugin (getnamo/GitHub)
- Sentry UE plugin docs
- sentry-unreal, Velopack, Inno Setup, cloudflared, Tailscale, Cloudflare Registrar documentation

Full agent output logs are archived at `C:\Users\pladr\AppData\Local\Temp\claude\C--Sabri-MMO\…\tasks\*.output` for reference.

---

**End of plan.** Supplementary runbooks:
- [Server Deployment Runbook](Server_Deployment_Runbook.md) — copy-paste VPS + Caddy setup
- [Client Packaging Runbook](Client_Packaging_Runbook.md) — copy-paste Shipping build + Inno Setup
- [Client Endpoint Config Design](Client_Endpoint_Config_Design.md) — C++ diffs for `UMMOGameInstance` + `UOptionsSaveGame`
