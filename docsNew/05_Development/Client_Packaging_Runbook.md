# Client Packaging Runbook

> **Navigation**: [Documentation Index](../DocsNewINDEX.md) | [Hosting, Packaging, Distribution Plan](Hosting_Packaging_Distribution_Plan.md)
> **Companions**: [Server Deployment Runbook](Server_Deployment_Runbook.md) | [Client Endpoint Config Design](Client_Endpoint_Config_Design.md)
> **Target**: UE5.7 Shipping Win64 build ready to hand to 5–50 invitees.

---

## 0. Prerequisites

- UE5.7.4 or newer (earlier 5.7 hotfixes had packaging regressions).
- Repo at `C:\Sabri_MMO\`, `.uproject` at `C:\Sabri_MMO\client\SabriMMO\SabriMMO.uproject`.
- Visual Studio 2022 with **Game development with C++** workload + Windows 10/11 SDK 10.0.22621.0 or newer.
- **Endpoint-config changes applied** per [Client_Endpoint_Config_Design.md](Client_Endpoint_Config_Design.md) — your client must read the production URL, not default to `http://localhost:3001`.
- **Inno Setup 6.x** installed from https://jrsoftware.org/isdl.php.
- Optional: `sentry-unreal` plugin if you want crash reporting.

---

## 1. Pre-Build Cleanup

### 1.1 Update `DefaultGame.ini`

Edit `client/SabriMMO/Config/DefaultGame.ini` — currently still reads `ProjectName=Third Person Game Template`. Replace:

```ini
[/Script/EngineSettings.GeneralProjectSettings]
ProjectID=659C51EC416A312DBCBD83AD710448DE
ProjectName=Sabri MMO
ProjectVersion=0.4.0
CompanyName=Sabri Dev
CopyrightNotice=Copyright 2026 Sabri Dev
Description=Class-based action MMORPG
```

`ProjectVersion` is read at runtime via `UGeneralProjectSettings::Get()->ProjectVersion`. Bump it every build.

### 1.2 Update `SabriMMO.Target.cs`

`client/SabriMMO/Source/SabriMMO.Target.cs` — add Shipping flags:

```csharp
using UnrealBuildTool;
using System.Collections.Generic;

public class SabriMMOTarget : TargetRules
{
    public SabriMMOTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("SabriMMO");

        // Keep UE_LOG(Warning+) on in Shipping during early invitee phase.
        // Strip back to false once crashes are rare.
        if (Configuration == UnrealTargetConfiguration.Shipping)
        {
            bUseLoggingInShipping = true;
            bUseChecksInShipping  = false;
            bOverrideBuildEnvironment = true;
        }

        if (Configuration == UnrealTargetConfiguration.Shipping ||
            Configuration == UnrealTargetConfiguration.Test)
        {
            bBuildDeveloperTools      = false;
            bBuildWithEditorOnlyData  = false;
            bCompileWithPluginSupport = false;  // flip true only if you load plugins at runtime
        }
    }
}
```

### 1.3 Audit Plugins

Open `SabriMMO.uproject` in a text editor. Disable plugins you don't use — they add 10–100 MB each. Candidates commonly on-by-default:
- `ChaosClothAsset` — off unless you use cloth
- `WaterExtras` — off (you don't use UE5 Water system)
- `VirtualCamera` — off
- `NNE` (Neural Network Engine) — off
- `OpenXR` — off (not VR)
- `LevelSequenceEditor` / `MovieScene*` — off in runtime modules if not using cinematics

Example:
```json
{
    "Name": "ChaosClothAsset",
    "Enabled": false
}
```

For maximum aggression, add `"DisableEnginePluginsByDefault": true` at the root and then `"Enabled": true` for each plugin you need. Warning: this can break subtle dependencies — test the editor first.

### 1.4 Project Settings (via Editor)

Open UE5 Editor → Edit → Project Settings:

**Packaging** (most important):
- Build Configuration: **Shipping**
- Use Pak File: **ON**
- Use Io Store: **ON** (default)
- Use Zen Store: **OFF** unless you have Zen infra
- Share Material Shader Code: **ON**
- **For Distribution**: **ON** (critical for the final artifact)
- Include Crash Reporter: **ON**
- Include Debug Files: **OFF**
- Full Rebuild: **ON** (final build only)
- Exclude editor content when cooking: **ON**
- List of maps to include in a packaged build: add all `L_*` and zone levels you ship

**Project → Description**:
- Project Name: `Sabri MMO`
- Project Version: `0.4.0` (same as `DefaultGame.ini`)
- Company Name, Copyright Notice filled in
- Splash Images: set your splash (otherwise ships the UE logo)

**Project → Maps & Modes**:
- Game Default Map: `/Game/SabriMMO/Levels/L_Startup`
- Editor Startup Map: same

**Engine → General Settings**:
- Framerate → Smooth Frame Rate: OFF (let driver handle vsync) or cap at 120 if you prefer

### 1.5 Verify Content hygiene

Temporary / test / experimental assets that ship by accident are embarrassing. Before final build:

```bat
REM From your project root, find .umap and .uasset files modified in the last 24h
dir /s /o-d /b "C:\Sabri_MMO\client\SabriMMO\Content\*.umap"
dir /s /o-d /b "C:\Sabri_MMO\client\SabriMMO\Content\*.uasset" | find /i "test_"
dir /s /o-d /b "C:\Sabri_MMO\client\SabriMMO\Content\*.uasset" | find /i "_temp"
dir /s /o-d /b "C:\Sabri_MMO\client\SabriMMO\Content\*.uasset" | find /i "_old"
```

Delete or rename anything you don't want shipped.

**Folders that are NEVER cooked** (outside `Content/`):
- `server/`, `database/`, `docsNew/`, `_journal/`, `_prompts/`, `RagnaCloneDocs/`, `2D animations/`, `memory/`

You don't need to exclude these — the cooker physically can't touch them.

---

## 2. The Build Script

Save as `C:\Sabri_MMO\scripts\build_ship.bat`. This is your reproducible build command, check it into the repo:

```bat
@echo off
REM =====================================================================
REM SabriMMO Shipping Build Script (Win64)
REM =====================================================================
setlocal enabledelayedexpansion

set UE_ROOT=C:\Program Files\Epic Games\UE_5.7
set PROJECT=C:\Sabri_MMO\client\SabriMMO\SabriMMO.uproject
set OUT=C:\Sabri_MMO\Dist\SabriMMO-Win64-Shipping

REM Build ID: date + time, no spaces
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value ^| find "="') do set DT=%%I
set BUILD_ID=%DT:~0,8%-%DT:~8,4%

echo [SabriMMO] Build ID: %BUILD_ID%
echo [SabriMMO] Project:  %PROJECT%
echo [SabriMMO] Output:   %OUT%

REM Clean previous output
if exist "%OUT%" (
    echo [SabriMMO] Cleaning previous build output...
    rmdir /s /q "%OUT%"
)

REM Capture current git SHA for version traceability
for /f %%I in ('git -C C:\Sabri_MMO rev-parse --short HEAD') do set GIT_SHA=%%I

REM =====================================================================
REM RunUAT BuildCookRun
REM =====================================================================
"%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
    -project="%PROJECT%" ^
    -noP4 ^
    -platform=Win64 ^
    -clientconfig=Shipping ^
    -cook ^
    -allmaps ^
    -build ^
    -stage ^
    -pak ^
    -iostore ^
    -compressed ^
    -prereqs ^
    -package ^
    -archive ^
    -archivedirectory="%OUT%" ^
    -utf8output ^
    -nodebuginfo ^
    -CrashReporter

if errorlevel 1 (
    echo [SabriMMO] *** BUILD FAILED ***
    exit /b 1
)

REM =====================================================================
REM Write BUILD_ID.txt
REM =====================================================================
set STAGED=%OUT%\Windows\SabriMMO
if not exist "%STAGED%" (
    echo [SabriMMO] ERROR: staged output not found at %STAGED%
    exit /b 1
)

> "%STAGED%\BUILD_ID.txt" echo Build: %BUILD_ID%
>> "%STAGED%\BUILD_ID.txt" echo Git:   %GIT_SHA%
>> "%STAGED%\BUILD_ID.txt" echo Host:  %COMPUTERNAME%

echo [SabriMMO] Build complete.
echo [SabriMMO] Staged to: %STAGED%

REM =====================================================================
REM Show size
REM =====================================================================
for /f "tokens=3" %%A in ('dir /s /-c "%STAGED%" ^| find "File(s)"') do set SIZE=%%A
echo [SabriMMO] Total size: %SIZE% bytes

endlocal
```

Run:
```bat
cd C:\Sabri_MMO
scripts\build_ship.bat
```

First build: 20–60 min. Subsequent iterative builds: 5–15 min.

---

## 3. Verify the Pak

After the build succeeds, verify the pak does not leak anything it shouldn't:

```bat
set UNREALPAK="C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealPak.exe"
set PAK="C:\Sabri_MMO\Dist\SabriMMO-Win64-Shipping\Windows\SabriMMO\Content\Paks\SabriMMO-Windows.pak"

%UNREALPAK% %PAK% -List > C:\Temp\pak_list.txt

REM These should produce NO matches:
findstr /i /C:"_journal" /C:"RagnaCloneDocs" /C:"docsNew" /C:"_prompts" /C:".blend" /C:".psd" /C:"server/" C:\Temp\pak_list.txt

REM Sanity: expect hundreds of entries for Content/SabriMMO/*
findstr /i /c:"SabriMMO/" C:\Temp\pak_list.txt | find /c /v ""
```

Typical expected pak contents:
- `SabriMMO/Levels/*.umap` (your zone maps)
- `SabriMMO/Content/*` — sprites, atlases, UI textures, sounds
- `Engine/Content/*` — shared engine assets (particles, basic materials)

If anything unintended shows up, remove it from your Content folder or add it to `DefaultGame.ini` `[/Script/UnrealEd.ProjectPackagingSettings]` `DirectoriesToNeverCook=` and rebuild.

---

## 4. Sentry Crash Reporting (Recommended)

### 4.1 Install plugin

1. Download `sentry-unreal` from https://github.com/getsentry/sentry-unreal/releases or Fab marketplace.
2. Extract to `client/SabriMMO/Plugins/SentrySDK/`.
3. Open `SabriMMO.uproject`, verify Sentry shows in Plugins list (Edit → Plugins → search "Sentry").
4. Enable it, restart editor.

### 4.2 Configure

1. Sign up at https://sentry.io (free tier: 5K errors/mo).
2. Create a project → Platform: Unreal Engine.
3. Copy the DSN.
4. In UE5 Editor → Project Settings → Plugins → Sentry → paste DSN into "DSN". Click "Update global settings" — plugin writes:
   - `Config/DefaultEngine.ini` `[/Script/Sentry.SentrySettings]` DSN
   - `Engine/Programs/CrashReportClient/Config/DefaultEngine.ini` `[CrashReportClient]` `DataRouterUrl=`
5. To upload debug symbols after each build:
   ```bat
   npm install -g @sentry/cli
   sentry-cli login
   sentry-cli upload-dif --org=YOUR_ORG --project=sabrimmo "C:\Sabri_MMO\Dist\SabriMMO-Win64-Shipping\Windows\SabriMMO\Binaries\Win64"
   ```

---

## 5. Inno Setup Installer

### 5.1 Script

Save as `C:\Sabri_MMO\installer\SabriMMO.iss`:

```iss
; SabriMMO installer — Inno Setup 6.x
#define AppName        "SabriMMO"
#define AppVersion     "0.4.0"
#define AppPublisher   "Sabri Dev"
#define AppExeName     "SabriMMO.exe"
#define SrcDir         "C:\Sabri_MMO\Dist\SabriMMO-Win64-Shipping\Windows\SabriMMO"

[Setup]
AppId={{7B1F5E3C-8D42-4A94-B7F6-2F3E8D9C7A12}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL=https://sabrimmo.com
DefaultDirName={localappdata}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=C:\Sabri_MMO\Dist\Installer
OutputBaseFilename=SabriMMO-Setup-{#AppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayIcon={app}\{#AppExeName}
DisableWelcomePage=no
CloseApplications=force

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"
Name: "startmenu";   Description: "Create a &Start Menu shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Files]
; Main content
Source: "{#SrcDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; Prereq installer (copied temporarily for [Run])
Source: "{#SrcDir}\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe"; \
    DestDir: "{tmp}"; Flags: deleteafterinstall; \
    Check: ShouldRunPrereqs

[Icons]
Name: "{group}\{#AppName}";           Filename: "{app}\{#AppExeName}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#AppName}";   Filename: "{app}\{#AppExeName}"; Tasks: desktopicon
Name: "{userstartmenu}\{#AppName}";   Filename: "{app}\{#AppExeName}"; Tasks: startmenu

[Run]
; Silent prereq install (only runs if VC++ 2015-2022 runtime missing)
Filename: "{tmp}\UEPrereqSetup_x64.exe"; Parameters: "/quiet /norestart"; \
    StatusMsg: "Installing Visual C++ and DirectX runtimes..."; \
    Flags: waituntilterminated; \
    Check: ShouldRunPrereqs

; Optional launch after install
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; \
    Flags: nowait postinstall skipifsilent

[Code]
function IsVcRuntimeInstalled: Boolean;
var
    Version: String;
begin
    Result := RegQueryStringValue(
        HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64',
        'Version', Version);
end;

function ShouldRunPrereqs: Boolean;
begin
    Result := not IsVcRuntimeInstalled;
end;
```

### 5.2 Build the installer

```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /Qp "C:\Sabri_MMO\installer\SabriMMO.iss"
```

Output: `C:\Sabri_MMO\Dist\Installer\SabriMMO-Setup-0.4.0.exe` (typically 1.3–2.0 GB after LZMA2 ultra compression).

### 5.3 Alternative: just ship a ZIP

For 5-friend Scenario A, an installer is overkill. Zip the staged folder:

```powershell
Compress-Archive -Path "C:\Sabri_MMO\Dist\SabriMMO-Win64-Shipping\Windows\SabriMMO\*" `
    -DestinationPath "C:\Sabri_MMO\Dist\SabriMMO-0.4.0.zip" -CompressionLevel Optimal
```

Friends extract, run `SabriMMO.exe`. Disadvantage: they'll need the VC++ 2015–2022 x64 redistributable installed themselves (link them to https://aka.ms/vs/17/release/vc_redist.x64.exe).

---

## 6. TLS / cacert.pem Adjustment (If Needed)

UE5.7 FHttpModule uses libcurl + its own bundled CA store, NOT the Windows root store. Let's Encrypt's ISRG Root X1 is already bundled, so `https://sabrimmo.com` (Let's Encrypt cert via Caddy) just works.

**If you see `libcurl error: 60` in packaged builds**:

1. Identify the missing root. Check your cert chain:
   ```powershell
   openssl s_client -connect sabrimmo.com:443 -showcerts < NUL
   ```
2. Download the root CA PEM from the issuer (e.g., https://letsencrypt.org/certs/isrgrootx1.pem.txt).
3. Append to `C:\Sabri_MMO\client\SabriMMO\Content\Certificates\cacert.pem` (create folders if absent). You can concatenate multiple `-----BEGIN CERTIFICATE-----` blocks in one file.
4. In Project Settings → Packaging → **Additional Non-Asset Directories to Package**, add `Certificates`.
5. Rebuild.

**For the getnamo SocketIOClient plugin (WSS)**:
- In your `MMOGameInstance::ConnectSocket` (or wherever you construct the `FSocketIONative`), set:
  ```cpp
  NativeSocket->bShouldUseTlsLibraries = true;
  NativeSocket->bShouldSkipCertificateVerification = true; // plugin issue #303 — required until fix lands
  ```
- The plugin wraps sioclient + OpenSSL. For `wss://` URLs it uses TLS automatically.

See [Client_Endpoint_Config_Design.md](Client_Endpoint_Config_Design.md) for the C++ diff.

---

## 7. Code Signing (Optional — skip at <50 invitees)

### 7.1 Decision

| Invitees | Sign? | Why |
|---|---|---|
| 5–10 | No | SmartScreen friction acceptable for friends. |
| 11–50 | No, but submit to Defender FP (§8) | Signing doesn't help much until SmartScreen reputation builds. |
| 50–100 | Maybe | Depends on complaint volume. |
| 100+ | Yes | Defender/SmartScreen bounces are annoying at scale. |

### 7.2 Provider options (2026)

| Option | Cost | Requirement | Notes |
|---|---|---|---|
| **Azure Trusted Signing** (formerly Azure Code Signing) | $9.99/mo → $120/yr | **LLC with 3+ yr history** in US/CA/UK/EU (individual signups paused April 2025) | Cloud HSM, no USB token. Best if you qualify. |
| **Sectigo OV Code Signing** | ~$215/yr (3yr deal) | USB token shipped to you | Mainstream OV, good reputation curve. |
| **Sectigo EV Code Signing** | ~$280/yr (3yr deal) | USB token | EV gives slightly faster SmartScreen reputation build. |
| **DigiCert OV/EV** | $420–560/yr | USB or KeyLocker cloud | Premium brand, overpriced for solo dev. |
| **Self-signed** | Free | — | ❌ Does NOT bypass SmartScreen. Pointless. |

### 7.3 Signing commands

Assuming Azure Trusted Signing with `metadata.json` configured:

```bat
REM Download Windows SDK 10.0.22621.0+ if you don't have signtool
REM Install Azure.CodeSigning.Dlib via NuGet, extract the x64 DLL

set SIGNTOOL="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
set DLIB="C:\ArtifactSigning\Azure.CodeSigning.Dlib.dll"
set META="C:\ArtifactSigning\metadata.json"

%SIGNTOOL% sign /v /debug /fd SHA256 ^
    /tr "http://timestamp.acs.microsoft.com" /td SHA256 ^
    /dlib %DLIB% ^
    /dmdf %META% ^
    "C:\Sabri_MMO\Dist\SabriMMO-Win64-Shipping\Windows\SabriMMO\Binaries\Win64\SabriMMO-Win64-Shipping.exe"

%SIGNTOOL% sign /v /fd SHA256 ^
    /tr "http://timestamp.acs.microsoft.com" /td SHA256 ^
    /dlib %DLIB% /dmdf %META% ^
    "C:\Sabri_MMO\Dist\Installer\SabriMMO-Setup-0.4.0.exe"
```

---

## 8. Antivirus False-Positive Submission

UE5 Shipping builds are often flagged by AV heuristics (large EXE, compressed asset payloads). Submit both the game exe and the installer after each build:

| Vendor | URL | Typical turnaround |
|---|---|---|
| **Microsoft Defender** | https://www.microsoft.com/en-us/wdsi/filesubmission → "Software developer – false positive" | 24–72 hr |
| Avast / AVG | https://www.avast.com/false-positive-file-form.php | 1–3 days |
| Kaspersky | https://opentip.kaspersky.com/ | 1–3 days |
| Bitdefender | https://www.bitdefender.com/consumer/support/answer/29358/ | 1–3 days |
| Avira | https://www.avira.com/en/support-for-business-knowledgebase-detail?kbId=1823 | 1–3 days |

**Automation idea**: script to submit via curl to Microsoft Defender's API after each successful build (requires a Microsoft 365 Defender tenant). Overkill at <50 invitees — manual upload per release is fine.

---

## 9. Pre-Flight Checklist

Before sending a build to any invitee:

```
[ ] DefaultGame.ini ProjectName = "Sabri MMO" (not "Third Person Game Template")
[ ] DefaultGame.ini ProjectVersion bumped
[ ] SabriMMO.Target.cs bUseLoggingInShipping = true (in Shipping config)
[ ] BUILD_ID.txt exists alongside SabriMMO.exe
[ ] Endpoint-config points at production (https://sabrimmo.com) — see Client_Endpoint_Config_Design.md
[ ] SocketIOClient: bShouldUseTlsLibraries=true, bShouldSkipCertificateVerification=true
[ ] Project Settings > Packaging > For Distribution = ON
[ ] Project Settings > Packaging > Build Configuration = Shipping
[ ] UnrealPak.exe -List output has NO _journal, docsNew, .blend, server/ matches
[ ] Package size 1.2-4.0 GB (flag if outside range)
[ ] UEPrereqSetup_x64.exe bundled under Engine/Extras/Redist/en-us/
[ ] CrashReportClient.exe present alongside SabriMMO.exe
[ ] Sentry DSN configured (if using Sentry)
[ ] PDB symbols uploaded to Sentry via sentry-cli (if using Sentry)
[ ] /health handshake tested: server responds with JSON serverVersion, minClientVersion matches client
[ ] Test user accounts exist in prod DB for all invitees
[ ] Inno Setup compiled: SabriMMO-Setup-X.Y.Z.exe produced
[ ] If signing: both SabriMMO-Win64-Shipping.exe and SabriMMO-Setup-X.Y.Z.exe signed and timestamped
[ ] Install tested on a clean Windows 11 VM (no VC++ redist, no prior install)
[ ] Submitted to Microsoft Defender false-positive form
[ ] README / onboarding doc prepared (see Hosting_Packaging_Distribution_Plan.md §4.7)
[ ] Changelog posted to private Discord #announcements
```

---

## 10. Distribution (Quick Reference)

Once you have `SabriMMO-Setup-0.4.0.exe` (or the ZIP):

1. **itch.io** — push via butler to a Restricted project, issue per-invitee download keys. Full flow in [Hosting_Packaging_Distribution_Plan.md §4.2](Hosting_Packaging_Distribution_Plan.md).
2. **Cloudflare R2** (fallback) — upload the installer, serve via custom domain + JWT-gated redirect endpoint from your Node server.
3. **Discord** — drop the itch.io / R2 URL in a pinned `#downloads` message. Do not upload the binary itself (500 MB cap + 24h URL expiry).

---

## 11. Common Build Failures

| Error | Fix |
|---|---|
| `error MSB8020: The build tools for v143 cannot be found` | Install VS 2022 with "Game development with C++" workload. |
| `UnrealBuildTool failed: module X not found` | Check `SabriMMO.Build.cs` — a referenced module was removed or renamed. |
| Cook fails with "asset not found" | Check *Project Settings → Packaging → List of maps to include* — add missing zone. |
| Shipping build crashes on launch, log empty | `bUseLoggingInShipping = false` — rebuild with it true and re-test. |
| `LNK2019: unresolved external symbol` related to OpenSSL | SocketIOClient plugin's OpenSSL/zlib dep got pruned by IWYU. Add `"OpenSSL", "zlib"` to `SabriMMO.Build.cs` PublicDependencyModuleNames. |
| Inno Setup "SourceFile not found" | Run `scripts\build_ship.bat` first to produce the staged folder. |
| Shipping build size > 10 GB | Texture atlas cook blowup. Audit BC7 vs BC1 on non-UI textures. |
| Installer runs but game fails to launch on clean VM | Prereq didn't run — check `[Run]` section Check function; verify UEPrereqSetup_x64.exe was staged. |

---

**Next**: [Client Endpoint Config Design](Client_Endpoint_Config_Design.md) for the C++ diff that makes the client point at your production server instead of localhost.
