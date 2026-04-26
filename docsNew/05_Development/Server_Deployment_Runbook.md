# Server Deployment Runbook

> **Navigation**: [Documentation Index](../DocsNewINDEX.md) | [Hosting, Packaging, Distribution Plan](Hosting_Packaging_Distribution_Plan.md)
> **Companions**: [Client Packaging Runbook](Client_Packaging_Runbook.md) | [Client Endpoint Config Design](Client_Endpoint_Config_Design.md)
> **Audience**: solo dev, own hardware or VPS, bringing `sabri-mmo` Node.js server online so friends outside your LAN can connect.

This document is a copy-paste runbook. Pick **one track** in each step.

---

## 0. Pick Your Hardware Track

| Track | When to use | Hardware | OS | Cost |
|---|---|---|---|---|
| **1A — Windows spare PC** | You have an old PC collecting dust, want zero learning curve | Any x64 PC with 8+ GB RAM, SSD, wired Ethernet | Windows 10/11 (Pro preferred for RDP) | $0 + ~$1–12/mo electricity |
| **1B — Windows gaming PC (shared)** | No spare PC, want to start right now | Your primary PC | Windows 11 | $0 + already paying for electricity |
| **2 — Linux spare PC** | You want 24/7 reliability, less memory pressure, better uptime, or cheaper electricity | Spare x64 PC or mini PC (N100 / Beelink / NUC) or Raspberry Pi 5 8GB | Debian 12 or Ubuntu Server 24.04 LTS | $0 + ~$1–3/mo electricity (mini PC) |
| **3 — VPS** | You don't want to run own hardware, or home ISP blocks inbound | Hetzner CCX / OVH VPS / DigitalOcean / Oracle Free | Debian 12 or Ubuntu 24.04 | $0 (Oracle Free) to ~$36/mo |

**Self-hosting is the recommendation for scenarios A and B** (5 friends → 20–50 testers). You only need a VPS if:
- Your ISP puts you behind CGNAT (inbound port forwarding impossible) AND you can't use Cloudflare Tunnel for some reason,
- You consistently hit your residential upload cap, or
- You want to stay online while your PC is off for extended periods.

**Cloudflare Tunnel sidesteps CGNAT entirely** (see §5), so CGNAT is not a dealbreaker for own-hardware hosting.

### Decision flow

```
Do you have a spare PC that can be left on 24/7?
├── Yes ─ Keep Windows for familiarity or switch to Linux?
│         ├── Windows → Track 1A
│         └── Linux   → Track 2
└── No  ─ Can you share your gaming PC?
          ├── Yes → Track 1B (with affinity pinning to avoid interfering with games)
          └── No  → Track 3 (VPS — start with Oracle Free or OVH VPS-2)
```

### Minimum hardware (all tracks)

| Component | Minimum for 5 friends | Recommended for 20–50 |
|---|---|---|
| CPU | 2 cores, 2.0 GHz+ | 4 cores, 2.4 GHz+ |
| RAM | 4 GB total | 8 GB |
| Storage | 20 GB free | 50 GB SSD |
| Network | Wired Ethernet | Wired, ≥10 Mbps upload |
| Power | Can leave on 24/7 | Same + UPS preferable |

Per the research, a **$250 N100 mini PC** (Beelink EQ12 or similar) at ~10 W idle runs the full stack with headroom for 50+ CCU and costs ~$1.15/mo electricity in the US. Worth considering if your "spare PC" is a power-hungry tower.

**Raspberry Pi 5 8GB** works (`recast-navigation` is a WASM package, so ARM64 is fine). Use an NVMe hat — never run Postgres on an SD card. Slightly tight for 50+ CCU; fine for ≤20.

---

## 1. Prerequisites (all tracks)

### 1.1 Check for CGNAT (if you plan to use direct port forwarding)

If you're using Tailscale or Cloudflare Tunnel (recommended), **skip this** — both sidestep CGNAT entirely.

If you want to open a port directly to the internet:

1. From any LAN device, visit `https://ifconfig.me` → note your WAN IP (call it `PUBLIC_IP`).
2. Log into your router admin (usually `192.168.1.1`). Find the WAN/Internet IP on the status page (call it `ROUTER_IP`).
3. If `PUBLIC_IP == ROUTER_IP` → **no CGNAT**, direct port forwarding works.
4. If `PUBLIC_IP != ROUTER_IP`, or `ROUTER_IP` is inside `100.64.0.0/10` (i.e. `100.64.x.x` through `100.127.x.x`) → **CGNAT**, direct port forwarding is impossible. Use Tailscale or Cloudflare Tunnel.

### 1.2 Check ISP Terms of Service

| ISP | Residential server hosting? | Source |
|---|---|---|
| Comcast / Xfinity | Prohibited | [Comcast AUP](https://www.xfinity.com/corporate/customers/policies/highspeedinternetaup) |
| Spectrum / Charter | Prohibited | [Spectrum AUP](https://www.spectrum.com/policies/residential-aup.html) |
| Cox | Prohibited | [Cox AUP](https://www.cox.com/aboutus/policies/acceptable-use-policy.html) |
| Verizon FiOS | Prohibited | [Verizon FiOS Agreement](https://www.verizon.com/support/fios-customer-agreement/) |
| AT&T Fiber | Permitted via AUP | [AT&T AUP](https://www.att.com/legal/terms.aup.html) |
| T-Mobile Home Internet | CGNAT blocks inbound anyway | — |
| **Google Fiber** | **Explicitly allowed** for non-commercial use | [Google Fiber AUP](https://gfiber.com/legal/accepteduse/residential/) |

**Enforcement at <50 players is documented as effectively never** across these major ISPs, but the tail risk is account termination. Mitigations:
- Run through Cloudflare Tunnel instead of opening ports (looks like normal outbound HTTPS to your ISP).
- Never monetize (Patreon/donations flip the risk profile).
- Have a VPS ready to switch to if you get an AUP email.

### 1.3 Install PostgreSQL 15, Redis, Node.js 20 LTS

Track-specific instructions below.

---

## 2. Track 1A — Windows Spare PC (Dedicated Server)

This is for an old PC you can leave on 24/7. You'll sign in once, then never touch the desktop again — RDP in only when needed.

### 2.1 Windows setup

1. **Windows 10/11 Pro** is recommended (Home works but lacks RDP; you'd SSH via OpenSSH Server as an alternative).
2. **Windows Update** to latest, then disable **automatic restart for updates**: Settings → Windows Update → Advanced options → Active hours = 24h. You don't want Windows rebooting your server at 3 AM during a play session.
3. **Power settings**: Control Panel → Power Options → High performance → **Never sleep**, **Turn off hard disk: Never**, **Turn off display: 10 min**.
4. **Wake-on-LAN** (optional): BIOS → enable WoL. Router → static DHCP lease for the server's MAC. Lets you power on remotely.
5. **Fixed LAN IP**: router admin → DHCP reservation → assign this PC a stable IP (e.g. `192.168.1.50`).
6. **Enable Remote Desktop** (Pro only): Settings → System → Remote Desktop → On. Or use **Tailscale SSH** for remote access (see §5.1).
7. **Create a non-admin local account** `sabrisrv` for running the server. Don't run the Node process as the admin account.

### 2.2 Install PostgreSQL 15 (Windows native)

1. Download EDB installer from https://www.postgresql.org/download/windows/ (Postgres 15.x, Windows x86-64).
2. Run installer as admin. During install:
   - Install directory: `C:\Program Files\PostgreSQL\15` (default)
   - Data directory: `C:\Program Files\PostgreSQL\15\data` (default)
   - Password: **use `openssl rand -base64 24`** to generate — save in a password manager, this is your `DB_PASSWORD`.
   - Port: 5432 (default)
   - Locale: Default
3. Deselect Stack Builder at the end (not needed).
4. **Verify service**: Services app (`services.msc`) → `postgresql-x64-15` → should be **Running**, Startup type **Automatic**.
5. **Create database** via pgAdmin 4 (installed alongside):
   - Open pgAdmin, connect with password set above.
   - Right-click Databases → Create → Database → name: `sabri_mmo`, owner: `postgres`.
6. **Run the schema**: open a Query Tool on `sabri_mmo`, paste contents of `C:\Sabri_MMO\database\init.sql`, execute.

### 2.3 Install Memurai (Redis for Windows)

Redis has no official Windows build since Redis Labs dropped it years ago. **Memurai Developer Edition** is Redis-API-compatible, maintained by Redis Labs' official Windows partner, and free for dev/test.

1. Download from https://www.memurai.com/get-memurai → pick Developer (free).
2. Run installer. It installs as a Windows service that auto-starts on boot. Default port 6379.
3. **Verify**: `services.msc` → `Memurai` should be Running.
4. **Test**: open PowerShell → `memurai-cli ping` → expect `PONG`.

**Licensing note**: Developer Edition is licensed for dev/test only. For truly "production" use (commercial), buy Enterprise. For a 50-player invite-only alpha, this is a grey area; if it ever matters, switch to Linux (Track 2) which has no such restriction.

### 2.4 Install Node.js 20 LTS

1. Download from https://nodejs.org/en/download → Windows Installer (x64), LTS version.
2. Run installer. Defaults are fine. Installs `node` and `npm` to `C:\Program Files\nodejs`.
3. Verify: `node --version` → `v20.x.x`.

### 2.5 Configure server

1. Clone or copy the repo to the spare PC: `C:\Sabri_MMO\`.
2. Install dependencies:
   ```powershell
   cd C:\Sabri_MMO\server
   npm install
   ```
3. Create `server\.env` with **rotated production values**:
   ```dotenv
   # Server
   PORT=3001
   SERVER_HOST=sabrimmo.com
   NODE_ENV=production

   # PostgreSQL
   DB_HOST=localhost
   DB_PORT=5432
   DB_NAME=sabri_mmo
   DB_USER=postgres
   DB_PASSWORD=<paste-the-rotated-password-from-section-2.2>

   # Redis
   REDIS_URL=redis://localhost:6379

   # JWT — MUST be rotated before production
   JWT_SECRET=<run-this-in-powershell-and-paste-output>
   # PowerShell: [Convert]::ToBase64String((1..48 | % { [byte](Get-Random -Max 256) }))

   # Logging
   LOG_LEVEL=INFO

   # Client version handshake (bumped when socket event shapes change)
   MIN_CLIENT_VERSION=0.4.0
   SERVER_VERSION=0.4.0
   ```
4. **First-run sanity check**:
   ```powershell
   cd C:\Sabri_MMO\server
   node src\index.js
   ```
   Expect log lines about DB connection, Redis connection, NavMesh load, `Server running on port 3001`.
   `curl http://localhost:3001/health` from another PowerShell window should return JSON with `status: "OK"`.

### 2.6 Install PM2 as a Windows service (via pm2-installer)

Running `node src/index.js` in a console dies when you log out. Use PM2 + `pm2-installer` to run it as a proper Windows service that survives reboots.

1. **Elevated PowerShell** (Run as Administrator):
   ```powershell
   cd C:\
   git clone https://github.com/jessety/pm2-installer C:\Tools\pm2-installer
   cd C:\Tools\pm2-installer
   npm run configure
   npm run configure-policy
   npm run setup
   ```
   This installs PM2 as a Windows service running under `LocalService` account.
2. **Close that PowerShell** and open a **new elevated PowerShell** so PATH picks up the changes.
3. **Start the server under PM2**:
   ```powershell
   pm2 start C:\Sabri_MMO\server\src\index.js --name sabri-mmo --output C:\Sabri_MMO\logs\pm2-out.log --error C:\Sabri_MMO\logs\pm2-err.log
   pm2 save
   ```
4. **Reboot the PC** to verify auto-start: `shutdown /r /t 0`. After reboot, log back in, open PowerShell, `pm2 list` should show `sabri-mmo` online.
5. **Useful PM2 commands** going forward:
   ```powershell
   pm2 list                              # status
   pm2 logs sabri-mmo                    # tail logs
   pm2 restart sabri-mmo                 # restart
   pm2 reload sabri-mmo                  # zero-downtime reload
   pm2 monit                             # live dashboard
   ```

### 2.7 Windows Firewall

```powershell
# Elevated PowerShell
# Node inbound on 3001 (only needed if using direct port forward — skip for Tunnel/Tailscale)
New-NetFirewallRule -DisplayName "Sabri MMO Node" -Direction Inbound -Protocol TCP -LocalPort 3001 -Action Allow -Profile Any

# Postgres and Redis: LAN only
New-NetFirewallRule -DisplayName "Postgres LAN" -Direction Inbound -Protocol TCP -LocalPort 5432 -Action Allow -RemoteAddress 192.168.0.0/16 -Profile Any
New-NetFirewallRule -DisplayName "Redis LAN"    -Direction Inbound -Protocol TCP -LocalPort 6379 -Action Allow -RemoteAddress 192.168.0.0/16 -Profile Any
```

### 2.8 Defender exclusions (only if AV is slowing things down)

```powershell
# Elevated PowerShell — skip unless you see perf issues
Add-MpPreference -ExclusionPath    "C:\Sabri_MMO\server"
Add-MpPreference -ExclusionProcess "node.exe"
Add-MpPreference -ExclusionPath    "C:\Program Files\PostgreSQL\15\data"
Add-MpPreference -ExclusionProcess "postgres.exe"
Add-MpPreference -ExclusionPath    "C:\Program Files\Memurai"
Add-MpPreference -ExclusionProcess "memurai.exe"
```

---

## 3. Track 1B — Windows Gaming PC (Shared)

Same as Track 1A except:
- You'll also be playing games on this box. Pin Node to one core to prevent scheduler starvation:
  ```powershell
  # Use taskkill/taskmgr affinity, or start Node via:
  cmd /c start /affinity 0x1 node C:\Sabri_MMO\server\src\index.js
  ```
  Affinity mask `0x1` = core 0 only. In PM2 ecosystem file:
  ```javascript
  // C:\Sabri_MMO\server\ecosystem.config.js
  module.exports = {
    apps: [{
      name: 'sabri-mmo',
      script: 'src/index.js',
      node_args: [],
      instances: 1,
      autorestart: true,
      exec_mode: 'fork',
      env: { NODE_ENV: 'production' }
    }]
  };
  ```
- When gaming, launch games in **borderless windowed** (not exclusive fullscreen) to avoid kernel reordering scheduler priorities.
- Skip CPU-intensive AAA titles during play sessions with friends, or expect 10–30 ms tick-loop hiccups during heavy GPU/CPU use.
- Consider a **N100 mini PC** (~$250 one-time) to migrate to Track 1A/2 once you're tired of this — payback on electricity alone in ~2 years.

Everything else in Track 1A applies verbatim.

---

## 4. Track 2 — Linux Spare PC (Debian 12 or Ubuntu Server 24.04 LTS)

This is the recommended long-term track for own-hardware hosting. Better uptime, no licensing grey areas with Memurai, cheaper electricity if you use a mini PC, and the ops tooling (systemd, apt, pg_dump) is industry-standard.

### 4.1 Pick a distro

| Distro | Support length | Freshness | Pick if |
|---|---|---|---|
| **Debian 12 (Bookworm)** | ~3 yr full + ~2 yr LTS | Conservative | You want rock-solid stability; prefer Debian philosophy |
| **Ubuntu Server 24.04 LTS** | 5 yr free, 10 yr with Ubuntu Pro (free for up to 5 personal machines) | Newer kernel | You have new hardware (N100 Alder Lake-N, recent mini PCs) needing recent kernel drivers; want longer LTS |

Both work. Ubuntu 24.04 has slightly newer kernel drivers; Debian 12 has fewer moving parts. **Pick Ubuntu 24.04 LTS** unless you have a reason to prefer Debian.

### 4.2 Install the OS (one-time)

1. Download Ubuntu Server 24.04 LTS from https://ubuntu.com/download/server.
2. Flash to a USB stick with Rufus (Windows) or `dd` (Mac/Linux).
3. Boot the spare PC from USB. Follow the installer:
   - Language: English
   - Keyboard: your layout
   - Network: let DHCP assign IP, note it (you'll reserve in step 4).
   - Storage: Use an entire disk, LVM optional.
   - Profile: hostname `sabrisrv`, your name, username (e.g. `sabri`), strong password.
   - **Install OpenSSH server: YES** (so you can SSH in from your main PC).
   - **Skip snap selections** (no Docker, no Heimdall) — we'll install what we need manually.
4. Reboot, remove USB. Log in at the console once to verify.
5. From your main PC, SSH in: `ssh sabri@192.168.1.50` (use the IP the installer showed).
6. **Reserve a static DHCP lease** in your router for this MAC.

### 4.3 Base hardening (one-time)

```bash
# As user sabri
sudo apt update && sudo apt full-upgrade -y
sudo apt install -y ufw fail2ban unattended-upgrades rsync curl wget gnupg2 build-essential

# Enable automatic security updates
sudo dpkg-reconfigure --priority=low unattended-upgrades

# Firewall — allow SSH, HTTP, HTTPS only
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow 22/tcp
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw enable
sudo ufw status verbose

# Generate SSH key on your main PC (if you don't have one) and copy it:
#   ssh-keygen -t ed25519
#   ssh-copy-id sabri@192.168.1.50

# Disable password SSH after confirming key-based works
sudo sed -i 's/^#*PasswordAuthentication.*/PasswordAuthentication no/' /etc/ssh/sshd_config
sudo sed -i 's/^#*PermitRootLogin.*/PermitRootLogin no/' /etc/ssh/sshd_config
sudo systemctl restart ssh
```

### 4.4 Install PostgreSQL 15

Ubuntu 24.04 ships Postgres 16 in `apt`; if you want 15 specifically (matches your dev environment), use the PGDG repo:

```bash
# PGDG repo for consistent Postgres 15
sudo sh -c 'echo "deb https://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list'
wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
sudo apt update
sudo apt install -y postgresql-15 postgresql-client-15
sudo systemctl status postgresql

# Set the postgres user password
sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'PASTE-ROTATED-PASSWORD';"
# Create DB
sudo -u postgres createdb sabri_mmo
# Import schema
sudo -u postgres psql -d sabri_mmo -f /home/sabri/Sabri_MMO/database/init.sql
```

If you're OK with Postgres 16, simpler:
```bash
sudo apt install -y postgresql postgresql-client
sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'PASTE-ROTATED-PASSWORD';"
sudo -u postgres createdb sabri_mmo
sudo -u postgres psql -d sabri_mmo -f /home/sabri/Sabri_MMO/database/init.sql
```

The project's auto-create-column startup logic (per CLAUDE.md) handles version differences.

### 4.5 Install Redis 7

```bash
sudo apt install -y redis-server
sudo systemctl enable --now redis-server
redis-cli ping   # expect PONG

# Optional: set maxmemory + eviction policy in /etc/redis/redis.conf
sudo sed -i 's/^# maxmemory <bytes>/maxmemory 1gb/' /etc/redis/redis.conf
sudo sed -i 's/^# maxmemory-policy noeviction/maxmemory-policy allkeys-lru/' /etc/redis/redis.conf
sudo systemctl restart redis-server
```

### 4.6 Install Node.js 20 LTS via NodeSource

The Ubuntu-packaged Node.js is often old. Use NodeSource's official repo:

```bash
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs
node --version  # v20.x.x
npm --version
```

### 4.7 Get the server code on the box

**Option A — direct git clone** (if you use GitHub private):
```bash
cd /home/sabri
# You'll need a deploy key or personal access token
git clone git@github.com:yourusername/Sabri_MMO.git Sabri_MMO
cd Sabri_MMO/server
npm install
```

**Option B — rsync from your dev PC** (no git needed on the server):
```bash
# From your Windows dev PC (WSL or Git Bash):
rsync -avz --delete \
    --exclude node_modules --exclude .env --exclude logs --exclude '*.log' \
    /c/Sabri_MMO/server/ sabri@192.168.1.50:/home/sabri/Sabri_MMO/server/

# Then on the server:
cd /home/sabri/Sabri_MMO/server
npm install
```

This rsync is also your deploy command later — alias it in your dev shell.

### 4.8 Configure `server/.env`

```bash
cat > /home/sabri/Sabri_MMO/server/.env <<'EOF'
PORT=3001
SERVER_HOST=sabrimmo.com
NODE_ENV=production

DB_HOST=localhost
DB_PORT=5432
DB_NAME=sabri_mmo
DB_USER=postgres
DB_PASSWORD=PASTE-ROTATED-PASSWORD

REDIS_URL=redis://localhost:6379

JWT_SECRET=PASTE-JWT-SECRET-ROTATED

LOG_LEVEL=INFO
MIN_CLIENT_VERSION=0.4.0
SERVER_VERSION=0.4.0
EOF

# Generate the JWT secret
openssl rand -base64 48
# Paste output into JWT_SECRET above

# Lock permissions
chmod 600 /home/sabri/Sabri_MMO/server/.env
```

### 4.9 Run under systemd

PM2 works on Linux too, but systemd is native, already on the box, and integrates with journalctl. Pick one:

**Track 2A — systemd (recommended for Linux)**:

```bash
sudo tee /etc/systemd/system/sabri-mmo.service > /dev/null <<'EOF'
[Unit]
Description=Sabri MMO Node.js server
After=network.target postgresql.service redis-server.service
Requires=postgresql.service redis-server.service

[Service]
Type=simple
User=sabri
Group=sabri
WorkingDirectory=/home/sabri/Sabri_MMO/server
ExecStart=/usr/bin/node /home/sabri/Sabri_MMO/server/src/index.js
Restart=always
RestartSec=3
StandardOutput=append:/var/log/sabri-mmo/server.log
StandardError=append:/var/log/sabri-mmo/server-err.log
Environment=NODE_ENV=production
# Resource limits (optional, nice-to-have)
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
EOF

sudo mkdir -p /var/log/sabri-mmo
sudo chown sabri:sabri /var/log/sabri-mmo

sudo systemctl daemon-reload
sudo systemctl enable --now sabri-mmo
sudo systemctl status sabri-mmo

# Useful commands:
#   sudo systemctl restart sabri-mmo
#   journalctl -u sabri-mmo -f           # tail logs
#   sudo systemctl stop sabri-mmo
```

Log rotation via logrotate:
```bash
sudo tee /etc/logrotate.d/sabri-mmo > /dev/null <<'EOF'
/var/log/sabri-mmo/*.log {
    daily
    rotate 14
    compress
    delaycompress
    missingok
    notifempty
    create 0644 sabri sabri
    postrotate
        systemctl reload sabri-mmo >/dev/null 2>&1 || true
    endscript
}
EOF
```

**Track 2B — PM2 on Linux** (if you like the PM2 workflow):
```bash
sudo npm install -g pm2
cd /home/sabri/Sabri_MMO/server
pm2 start src/index.js --name sabri-mmo
pm2 save
# Generate a systemd unit that auto-starts PM2:
pm2 startup systemd -u sabri --hp /home/sabri
# It prints a command like: sudo env PATH=$PATH:/usr/bin pm2 startup systemd -u sabri --hp /home/sabri
# Run that command verbatim.
pm2 save
```

---

## 5. Remote Access (All Tracks)

You have three options for letting friends outside your LAN reach the server. Pick one.

### 5.1 Option: Tailscale mesh (Scenario A — 5 friends, free, no public exposure)

Tailscale creates a private WireGuard mesh. Your server is only reachable to devices you invite. Zero public IP, zero TLS needed inside the mesh.

**Setup on the server** (works on Windows and Linux):

```bash
# Linux
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up
# Follow the login URL it prints. Authenticate in browser.
tailscale ip       # note the 100.x.y.z address
```

```powershell
# Windows: download from https://tailscale.com/download
# Install, log in via tray icon.
# Get IP from tray menu or: tailscale ip
```

**Invite friends**:
1. Go to https://login.tailscale.com/admin/users → Invite users (up to 6 free).
2. Each friend installs Tailscale, accepts the invite.
3. From a friend's machine: `ping 100.x.y.z` to verify connectivity.

**Client side**: UE5 client connects to `ws://100.x.y.z:3001` — plain WS is fine inside the WireGuard tunnel because all traffic is already encrypted end-to-end.

**Costs**: $0. Limits: 6 users, 100 devices. Graceful upgrade path: when you exceed 6 users, flip to Cloudflare Tunnel (§5.2), same server, no hardware changes.

### 5.2 Option: Cloudflare Tunnel (Scenario B — 20–50 invitees, free, public URL)

Cloudflare Tunnel exposes your server over the public internet without opening any inbound ports. `cloudflared` dials outbound from your server to Cloudflare's edge; visitors hit `https://sabrimmo.com` and Cloudflare routes through the tunnel.

**Prereqs**:
1. **Domain**: buy `sabrimmo.com` (or your chosen name) at **Cloudflare Registrar** (~$10.46/yr at-cost, no markup).
   - Go to https://dash.cloudflare.com → Domain Registration → register.
2. Your zone is automatically added to Cloudflare DNS.
3. Cloudflare dashboard → SSL/TLS → Overview → select **Full (strict)**.
4. Cloudflare dashboard → Network → enable **WebSockets** (on by default).

**Install `cloudflared`**:

```bash
# Linux
curl --location --output cloudflared.deb \
    "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-$(dpkg --print-architecture).deb"
sudo dpkg -i cloudflared.deb
rm cloudflared.deb
```

```powershell
# Windows: download MSI from
#   https://github.com/cloudflare/cloudflared/releases/latest
# Install it. cloudflared.exe will be in PATH.
```

**Create tunnel**:

```bash
# Login (opens browser for Cloudflare auth)
cloudflared tunnel login

# Create tunnel
cloudflared tunnel create sabrimmo
# → outputs a tunnel UUID and writes ~/.cloudflared/<UUID>.json credentials

# Route DNS
cloudflared tunnel route dns sabrimmo sabrimmo.com
```

**Config file** at `~/.cloudflared/config.yml` (Linux) or `%USERPROFILE%\.cloudflared\config.yml` (Windows):

```yaml
tunnel: <PASTE-UUID-FROM-CREATE-STEP>
credentials-file: /home/sabri/.cloudflared/<UUID>.json
# On Windows: C:\Users\sabri\.cloudflared\<UUID>.json

originRequest:
  connectTimeout: 30s
  noHappyEyeballs: true

ingress:
  # Socket.io on /socket.io/*
  - hostname: sabrimmo.com
    path: /socket.io/*
    service: ws://localhost:3001
  # Everything else (REST API + /health) to same Node process
  - hostname: sabrimmo.com
    service: http://localhost:3001
  # Required catch-all
  - service: http_status:404
```

**Run as a service**:

```bash
# Linux
sudo cloudflared service install
sudo systemctl enable --now cloudflared
sudo systemctl status cloudflared
```

```powershell
# Windows (elevated PowerShell)
cloudflared service install
# Service name: cloudflared. Starts automatically on boot.
```

**Verify end-to-end**:
```bash
curl -v https://sabrimmo.com/health
# Expect JSON response with server version info, TLS 1.3, Cloudflare cert
```

**Costs**: $0. Limits: the Cloudflare Free plan has no documented bandwidth cap for Tunnel traffic, and community reports put practical capacity around 100K concurrent WebSockets per domain. The **100-second idle timeout** on Free/Pro is handled automatically by Socket.io's default 25-second `pingInterval` — no config needed.

**Note on direct port forwarding (Scenario B alt)**: if you want to run without Cloudflare (pure Caddy + Let's Encrypt on your residential IP + DDNS), see §5.3. Cloudflare Tunnel is simpler and more ISP-friendly.

### 5.3 Option: Direct public IP + Caddy + Let's Encrypt + DDNS

Only pick this if you specifically don't want Cloudflare in the path. Requires your ISP to allow inbound 443 and no CGNAT.

1. **Router port forward**: external TCP 443 → internal 192.168.1.50:443. Disable UPnP; configure manually.
2. **DDNS**: sign up for DuckDNS (https://www.duckdns.org/) — free, 5 subdomains.
   - Install the DDNS updater on the server (Linux: `apt install ddclient` configured for DuckDNS; Windows: https://www.duckdns.org/install.jsp — they provide a simple script).
   - Alternative: use Cloudflare Registrar domain + a Cloudflare DDNS script like https://github.com/favonia/cloudflare-ddns (Go binary) for a stable `sabrimmo.com` A record.
3. **Install Caddy**:

   ```bash
   # Linux
   sudo apt install -y debian-keyring debian-archive-keyring apt-transport-https
   curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
   curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | sudo tee /etc/apt/sources.list.d/caddy-stable.list
   sudo apt update
   sudo apt install -y caddy
   ```

   ```powershell
   # Windows: download from https://caddyserver.com/download → choose windows/amd64
   # Place caddy.exe in C:\Tools\Caddy\
   # Install as a Windows service via NSSM or just run it under PM2.
   ```

4. **Caddyfile** at `/etc/caddy/Caddyfile` (Linux) or `C:\Tools\Caddy\Caddyfile` (Windows):

   ```caddyfile
   sabrimmo.com {
       encode gzip zstd
       reverse_proxy localhost:3001 {
           # WebSocket upgrade is automatic in Caddy
           stream_timeout 24h
           stream_close_delay 5m
           header_up X-Real-IP {remote_host}
           header_up X-Forwarded-For {remote_host}
           header_up X-Forwarded-Proto {scheme}
       }
       log {
           output file /var/log/caddy/sabrimmo.log {
               roll_size 100mb
               roll_keep 10
           }
           format json
       }
   }
   ```

5. **Reload / start**:
   ```bash
   # Linux
   sudo systemctl reload caddy    # or: sudo caddy fmt --overwrite /etc/caddy/Caddyfile && sudo systemctl restart caddy
   ```

6. **Verify** that Caddy got a cert:
   ```bash
   curl -v https://sabrimmo.com/health
   ```

7. **Firewall**: Linux has ufw already allowing 443; Windows needs:
   ```powershell
   New-NetFirewallRule -DisplayName "Caddy HTTPS" -Direction Inbound -Protocol TCP -LocalPort 443 -Action Allow -Profile Any
   New-NetFirewallRule -DisplayName "Caddy HTTP"  -Direction Inbound -Protocol TCP -LocalPort 80  -Action Allow -Profile Any
   ```

This mode exposes your residential IP. Flag for risks:
- Direct DDoS target.
- Exposed to internet port-scan noise (add fail2ban if it bothers you).
- ISP AUP enforcement risk (~never for small traffic, but real).

Cloudflare Tunnel (§5.2) sidesteps all three issues. Prefer it.

---

## 6. Track 3 — VPS (only if own-hardware isn't viable)

If you can't self-host (CGNAT + Tunnel unreliable, unstable power, etc.), a VPS is the backup. Everything in §4 applies verbatim — just run on the VPS instead of a LAN box.

### 6.1 Provider pick

| Pick | When |
|---|---|
| **Oracle Cloud Free Tier — Ampere A1 ARM** | You want free forever. 4 OCPU ARM + 24 GB + 10 TB/mo in us-ashburn-1. Capacity is scarce; retry provisioning. |
| **OVH VPS-2 US** ($9.99/mo) | Paid, US player base, unmetered 1 Gbps, 15 US datacenters. **Best $/perf for US.** |
| **Hetzner CPX22 EU** (€7.99/mo) | Paid, EU player base or cost priority. Falkenstein/Helsinki, 20 TB. Add IPv4 (€0.50). |
| **Hetzner CCX23 US** ($35.69/mo) | Growth phase (100+ CCU). Dedicated vCPU. |
| **Hetzner AX41-NVMe** (€51.18/mo) | Full dedicated box (6c/12t/64 GB). EU only. 120 ms to US but 1/10 the price of equivalent US dedicated. |

### 6.2 Setup flow (Linux VPS)

Identical to Track 2 §4.2 onward — the VPS is just a remote Debian/Ubuntu box. Order the instance, SSH in, follow sections 4.3–4.9.

Caveats:
- Use **§5.3 direct Caddy + Let's Encrypt** (no Tunnel needed; you have a public IP).
- Set up **nightly pg_dump → Backblaze B2** (§7.4 below) since the VPS can be reprovisioned or lost.
- **Scale-up**: most VPS providers let you resize the instance in-place. Hetzner + OVH keep the IP on resize within the same region.

---

## 7. Cross-cutting Ops (All Tracks)

### 7.1 Domain + DNS (needed for Scenario B onwards)

1. Register at Cloudflare Registrar: `sabrimmo.com` (~$10.46/yr .com).
2. Cloudflare DNS: Already active if you registered there.
3. Add an `A` record:
   - `sabrimmo.com` → `<your-VPS-IP>` (orange-cloud proxied, recommended) — Scenario B VPS
   - OR `sabrimmo.com` → `<tunnel-UUID>.cfargotunnel.com` (CNAME, auto-created by `cloudflared tunnel route dns`) — Scenario A+B own-hardware
4. TTL: **300 s while iterating**, **3600 s once stable**.
5. Cloudflare SSL/TLS mode: **Full (strict)**.

### 7.2 Backups

**Nightly pg_dump → Backblaze B2** (cheapest at $0.005/GB storage, free 10 GB).

**Linux**:
```bash
# Install rclone for B2
curl https://rclone.org/install.sh | sudo bash
rclone config   # interactive: new remote → Backblaze B2 → paste keyID + applicationKey

# Backup script
sudo tee /usr/local/bin/backup-sabri-mmo > /dev/null <<'EOF'
#!/bin/bash
set -e
TS=$(date +%Y%m%d-%H%M)
DUMP=/tmp/sabri_mmo-$TS.sql.gz
sudo -u postgres pg_dump sabri_mmo | gzip > "$DUMP"
rclone copy "$DUMP" b2:your-bucket-name/backups/
rm "$DUMP"
# Retain last 30 days of backups (B2 lifecycle rule handles this too)
rclone delete --min-age 30d b2:your-bucket-name/backups/
EOF
sudo chmod +x /usr/local/bin/backup-sabri-mmo

# Cron: 3am nightly
echo "0 3 * * * /usr/local/bin/backup-sabri-mmo >> /var/log/sabri-mmo/backup.log 2>&1" | sudo tee /etc/cron.d/sabri-mmo-backup
```

**Windows** (PowerShell + Task Scheduler):
```powershell
# Install pg_dump path first: C:\Program Files\PostgreSQL\15\bin is already in PATH if PostgreSQL installer added it

# backup-sabri-mmo.ps1
$ts = Get-Date -Format yyyyMMdd-HHmm
$dump = "C:\Temp\sabri_mmo-$ts.sql"
& "C:\Program Files\PostgreSQL\15\bin\pg_dump.exe" -U postgres -F c -b -v -f $dump sabri_mmo
# Upload via rclone (install from https://rclone.org/downloads/)
& "C:\Tools\rclone\rclone.exe" copy $dump b2:your-bucket-name/backups/
Remove-Item $dump

# Schedule via Task Scheduler (elevated):
schtasks /Create /SC DAILY /TN "Sabri MMO Backup" /TR "powershell.exe -File C:\Sabri_MMO\scripts\backup-sabri-mmo.ps1" /ST 03:00 /RU SYSTEM
```

**Monthly restore test**: pick a random nightly dump, restore to a scratch DB, verify row counts. `gunzip -c /path/to/dump.sql.gz | psql scratch_db`.

### 7.3 Monitoring

**UptimeRobot** (free, 50 monitors, 5-min interval):
- Monitor type: HTTPS keyword
- URL: `https://sabrimmo.com/health`
- Keyword: `"status":"OK"`
- Alert: Discord webhook (UptimeRobot supports webhooks natively)

**Healthchecks.io** (free, 20 checks) — for tick-loop heartbeat:
- Register a check, get a URL like `https://hc-ping.com/<uuid>`.
- Add to `server/src/index.js` tick loop or a 5-minute `setInterval`:
  ```javascript
  if (process.env.HEALTHCHECK_URL) {
      setInterval(() => {
          fetch(process.env.HEALTHCHECK_URL).catch(() => {});
      }, 5 * 60 * 1000);
  }
  ```
- Alert if the server stops pinging (tick loop dead but process alive).

### 7.4 Secrets hygiene

- `.env` **not in git** (already gitignored).
- Rotate `JWT_SECRET` and `DB_PASSWORD` before production. JWT rotation invalidates all issued tokens — users must re-login once after rotation.
- Never commit production `.env` to any repo (public or private).
- For team-sharing secrets: use Bitwarden, 1Password, or a gpg-encrypted `.env.prod.gpg` checked into git.

### 7.5 Logs

| Track | Where logs land |
|---|---|
| Windows + PM2 | `C:\Sabri_MMO\logs\pm2-out.log`, `pm2-err.log` (per ecosystem config). Also `pm2 logs sabri-mmo` streams live. |
| Linux + systemd | `/var/log/sabri-mmo/server.log` + `journalctl -u sabri-mmo`. Rotated daily via logrotate (§4.9). |
| Linux + PM2 | `~/.pm2/logs/sabri-mmo-out.log`, same rotation via `pm2 install pm2-logrotate`. |

For remote aggregation later (when you have multiple servers), use **Axiom** or **Better Stack** — both have generous free tiers and accept stdout via systemd-journal-remote or PM2 modules. Out of scope for single-server scenarios.

---

## 8. Verification Matrix

After each track's setup, run through:

| Check | Command (any track) |
|---|---|
| Server listens on 3001 | `curl http://localhost:3001/health` returns 200 + JSON |
| Postgres accessible | `psql -U postgres -d sabri_mmo -c "SELECT COUNT(*) FROM users;"` |
| Redis accessible | `redis-cli ping` → PONG (or `memurai-cli ping` on Windows) |
| Process supervisor auto-restarts | `sudo systemctl stop sabri-mmo; sleep 10; sudo systemctl status sabri-mmo` — should be running again (or kill node via taskkill on Windows and verify PM2 restarts) |
| Survives reboot | Reboot the box, log back in, verify `curl http://localhost:3001/health` works without you starting anything |
| External reach (Scenario A) | From a friend's machine inside the tailnet: `curl http://100.x.y.z:3001/health` |
| External reach (Scenario B Tunnel) | From your phone on cellular: `https://sabrimmo.com/health` |
| External reach (Scenario B direct) | Same — `https://sabrimmo.com/health` with Caddy's Let's Encrypt cert |
| WebSocket upgrade works | `websocat wss://sabrimmo.com/socket.io/?EIO=4&transport=websocket` (install websocat from https://github.com/vi/websocat) — expect Socket.io handshake frames |
| UE5 client connects | Run the client with `-server=...` matching scenario, reach login screen, log in, spawn character |
| Version handshake rejects stale client | Set `MIN_CLIENT_VERSION=99.99.0` in `.env`, restart server, verify client shows "Please update" modal, then revert |
| Crash reporting | Force a crash (e.g., navigate to a broken skill), verify the crash appears in Sentry dashboard |
| Backups running | `rclone ls b2:your-bucket-name/backups/` shows at least one nightly dump |

---

## 9. Common Problems

| Symptom | Probable cause | Fix |
|---|---|---|
| `ECONNREFUSED 127.0.0.1:5432` on server start | Postgres not running | `sudo systemctl status postgresql` / `services.msc` → Postgres service running? |
| `ECONNREFUSED 127.0.0.1:6379` | Redis not running | Same, for `redis-server` / Memurai |
| Clients can't connect inside Tailscale | Firewall or Tailscale down | `sudo ufw allow from 100.64.0.0/10 to any port 3001` on Linux; verify `tailscale status` is "authenticated" |
| Cloudflare Tunnel 502 on all requests | Node server not listening on port | `curl http://localhost:3001/health` first. If that fails, fix the Node process before debugging Tunnel. |
| WebSocket closes every 100s through Cloudflare | Socket.io `pingInterval` set too high | Default is 25s; verify `new Server(server, { pingInterval: 25000 })` or similar |
| `crypto.createHmac` mysteries | Node version too old | Require Node 20+; `node --version` |
| UE5 client shows "Cannot connect to server" | Client-side `ServerBaseUrl` still `http://localhost:3001` | Rebuild client with production default (see [Client_Endpoint_Config_Design.md](Client_Endpoint_Config_Design.md)) |
| Windows: `pm2 list` shows nothing after reboot | PM2 service didn't save state | Re-run `pm2 save` while sabri-mmo is running, then reboot to verify |
| Linux: `sabri-mmo.service` start fails with "permission denied" on `.env` | `.env` owned by root | `sudo chown sabri:sabri /home/sabri/Sabri_MMO/server/.env` |
| Memurai demands license after 30 days | Developer Edition is dev/test only | Switch to Linux (Redis native) or buy Enterprise |
| `libcurl error 60` in UE5 Shipping build | cacert.pem missing a root | See [Client_Packaging_Runbook.md §6](Client_Packaging_Runbook.md) for the fix |

---

## 10. Scenario Quick Reference

| Scenario | Hardware | OS | Remote access | Monthly cost |
|---|---|---|---|---|
| **A1** 5 friends, your Windows gaming PC | Gaming PC | Windows 11 | Tailscale (free) | ~$12 (electricity) |
| **A2** 5 friends, spare Windows PC | Spare PC | Windows 10/11 | Tailscale (free) | ~$3–12 (electricity) |
| **A3** 5 friends, Linux mini PC | N100 mini PC ($250 one-time) | Ubuntu 24.04 | Tailscale (free) | ~$1.15 (electricity) |
| **B1** 20–50 testers, same Windows spare PC | Spare PC | Windows | Cloudflare Tunnel | ~$0.87/yr domain only |
| **B2** 20–50 testers, Linux spare PC | Spare PC or mini PC | Ubuntu 24.04 | Cloudflare Tunnel | ~$0.87/yr domain only |
| **B3** 20–50 testers, VPS | — | Ubuntu 24.04 | Direct Caddy + Let's Encrypt | $10 OVH VPS-2 + $0.87 domain |
| **C1** 100+ CCU, Hetzner US | — | Ubuntu 24.04 | Direct Caddy + Let's Encrypt | $36 Hetzner CCX23 + $0.87 domain |
| **C2** 100+ CCU, Hetzner EU dedicated | AX41-NVMe | Ubuntu 24.04 | Direct Caddy + Let's Encrypt | $55 Hetzner AX41 + $0.87 domain |

All scenarios share the same Node.js + Postgres + Redis + `.env` setup — only hardware and remote-access options differ.

---

**Next**: [Client Packaging Runbook](Client_Packaging_Runbook.md) to produce a Shipping build pointing at your new server.
