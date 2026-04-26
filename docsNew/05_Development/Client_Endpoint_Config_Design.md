# Client Endpoint Config Design

> **Navigation**: [Documentation Index](../DocsNewINDEX.md) | [Hosting, Packaging, Distribution Plan](Hosting_Packaging_Distribution_Plan.md)
> **Companions**: [Server Deployment Runbook](Server_Deployment_Runbook.md) | [Client Packaging Runbook](Client_Packaging_Runbook.md)
> **Audience**: implementing the C++ changes so Shipping builds connect to production instead of `http://localhost:3001`.

---

## 1. Problem

Today (verified by reading the source):

- `UMMOGameInstance::ServerBaseUrl` is a `UPROPERTY` with default `http://localhost:3001` (`MMOGameInstance.h:118`).
- `UHttpManager::GetServerBaseUrl` returns `GI->ServerBaseUrl` or falls back to `http://localhost:3001` (`MMOHttpManager.cpp:25-32`).
- `UMMOGameInstance::GetServerSocketUrl` returns `http://{SelectedServer.Host}:{SelectedServer.Port}` if a server is selected, else `ServerBaseUrl` (`MMOGameInstance.cpp:104-111`).
- `UMMOGameInstance::ConnectSocket` feeds that URL into `FSocketIONative->Connect()`.
- There is **no version handshake**; the server's `/health` (at `server/src/index.js:34142`) returns only `status`, `timestamp`, `message`.

A Shipping build handed to an invitee currently defaults to `http://localhost:3001` — useless. We need:

1. A **production default URL** baked into Shipping builds.
2. An **override path** for QA / bug repros (command-line flag and/or advanced SaveGame option).
3. **TLS support** so production uses `https://` + `wss://`.
4. A **version handshake** so old clients can't silently connect to a newer server.

---

## 2. Design

### 2.1 Resolution order

`UMMOGameInstance::Init()` populates `ServerBaseUrl` in this order, first non-empty wins:

1. **Command-line** — `-server=host:port` via `FParse::Value(FCommandLine::Get(), TEXT("server="), ServerArg)`. Useful for QA: launch `SabriMMO.exe -server=staging.sabrimmo.com:443`.
2. **SaveGame override** — `UOptionsSaveGame::CustomServerUrl` (added as a new UPROPERTY). Null/empty falls through. Settable from a hidden Options panel or a config tool.
3. **Compile-time default** — `SABRI_DEFAULT_MASTER_URL` preprocessor define. Baked into the binary so Shipping builds don't depend on any config file.

```
Final URL = CLI ?? SaveGame ?? Compile-time default
```

### 2.2 Scheme handling

- Production URL is `https://sabrimmo.com` (TLS).
- `GetServerBaseUrl()` returns that URL as-is for REST calls (`FHttpModule` speaks HTTPS natively).
- `GetServerSocketUrl()` must return `wss://sabrimmo.com` for the Socket.io client — simple string replace `http://` → `ws://`, `https://` → `wss://`.

### 2.3 Server list is still secondary

`/api/servers` continues to populate a list of `FServerInfo { Host, Port }`. When a user selects a server, `SelectServer()` overwrites `ServerBaseUrl` as today. This means:
- Master URL (e.g. `https://sabrimmo.com`) is a **bootstrap** — used only to fetch the server list.
- After selection, actual play-server URL is whatever `/api/servers` returned.
- You can spin up a new game server at `https://us-west.sabrimmo.com` and add it to `/api/servers` without re-shipping the client.

### 2.4 Version handshake

- **Client version**: read from `UGeneralProjectSettings::Get()->ProjectVersion` (set in `DefaultGame.ini`).
- **Server publishes on `/health`**:
  ```json
  {
    "status": "OK",
    "serverVersion": "0.4.2",
    "minClientVersion": "0.4.0",
    "timestamp": "2026-04-16T18:00:00.000Z"
  }
  ```
- **Client checks during `HealthCheck()`** (called by `LoginFlowSubsystem` before login UI activates).
- If `ClientVersion < MinClientVersion` → show "Please update" modal with download URL, disable login.
- Same check can happen on socket connect as belt-and-suspenders, but the `/health` pre-check is cleaner UX.

### 2.5 TLS caveats (SocketIOClient plugin issue #303)

The getnamo `SocketIOClient` plugin v2.11.0 has a known defect: `bShouldVerifyTLSCertificate = true` always fails. Workaround: **leave `bShouldSkipCertificateVerification = true`** (the plugin default). This means:
- TLS encryption works (traffic is encrypted in transit).
- **Cert chain is NOT validated** — a MITM with any cert can decrypt and modify Socket.io traffic.
- Mitigation: server-authoritative validation remains the real defense. JWT signatures are still checked. Don't put anything in socket packets you'd fear in plaintext (e.g. don't trust client-claimed stats).
- Track the issue for a future plugin update: https://github.com/getnamo/SocketIOClient-Unreal/issues/303

---

## 3. Code Changes

### 3.1 `SabriMMO.Build.cs` — bake in production URL

File: `client/SabriMMO/Source/SabriMMO/SabriMMO.Build.cs`

Add a `PublicDefinitions` entry. The exact URL should be overridable by environment variable at build time so you can ship different builds for dev/staging/prod:

```csharp
// SabriMMO.Build.cs (after the existing PublicDependencyModuleNames section)

// Production server URL — bake into Shipping builds.
// Override per build via env var SABRI_DEFAULT_MASTER_URL.
string DefaultMasterUrl = System.Environment.GetEnvironmentVariable("SABRI_DEFAULT_MASTER_URL")
                          ?? "https://sabrimmo.com";

// Dev builds default to localhost for iteration convenience.
if (Target.Configuration == UnrealTargetConfiguration.Development ||
    Target.Configuration == UnrealTargetConfiguration.DebugGame)
{
    DefaultMasterUrl = System.Environment.GetEnvironmentVariable("SABRI_DEFAULT_MASTER_URL")
                       ?? "http://localhost:3001";
}

PublicDefinitions.Add($"SABRI_DEFAULT_MASTER_URL=TEXT(\"{DefaultMasterUrl}\")");
```

Now in C++ you can reference `SABRI_DEFAULT_MASTER_URL` as a `const TCHAR*`.

### 3.2 `UOptionsSaveGame` — add `CustomServerUrl`

File: `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h`

In the existing `UOptionsSaveGame` class (starts at line 20):

```cpp
UCLASS()
class SABRIMMO_API UOptionsSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    // ... existing UPROPERTYs ...

    // --- Endpoint override (advanced — rarely set) ---
    // If non-empty, overrides the compile-time default master URL.
    // Set via command-line override or a hidden Options panel.
    // Format: "https://host:port" or "http://host:port"
    UPROPERTY()
    FString CustomServerUrl;
};
```

### 3.3 `UMMOGameInstance` — resolve URL on Init, add version handshake

File: `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h`

Add members (near the existing `ServerBaseUrl` at line 118):

```cpp
// ---- Configurable Server URL ----
UPROPERTY(BlueprintReadWrite, Category = "MMO Server")
FString ServerBaseUrl;  // Changed: removed default = TEXT("http://localhost:3001"); now set in Init()

// ---- Version Handshake ----
UPROPERTY(BlueprintReadOnly, Category = "MMO Version")
FString ClientVersion;  // populated from UGeneralProjectSettings::ProjectVersion

UPROPERTY(BlueprintReadOnly, Category = "MMO Version")
FString ServerVersion;  // populated from /health response

UPROPERTY(BlueprintReadOnly, Category = "MMO Version")
FString MinClientVersion;  // populated from /health response

UPROPERTY(BlueprintAssignable, Category = "MMO Events")
FOnVersionMismatch OnVersionMismatch;  // declare delegate above class
```

Declare the delegate above the class:
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVersionMismatch, const FString&, Reason);
```

File: `client/SabriMMO/Source/SabriMMO/MMOGameInstance.cpp`

Top of file:
```cpp
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/GeneralProjectSettings.h"
#include "Kismet/GameplayStatics.h"
```

Modify `Init()` (near line 11):
```cpp
void UMMOGameInstance::Init()
{
    Super::Init();
    LoadRememberedUsername();
    LoadGameOptions();

    // Create the event router (persists for the lifetime of the game instance)
    EventRouter = NewObject<USocketEventRouter>(this);

    // Resolve ServerBaseUrl: CLI > SaveGame > compile-time default
    ResolveServerBaseUrl();

    // Record client version from project settings
    ClientVersion = UGeneralProjectSettings::StaticClass()
        ->GetDefaultObject<UGeneralProjectSettings>()
        ->ProjectVersion;
    UE_LOG(LogTemp, Log, TEXT("[MMO] ClientVersion=%s, ServerBaseUrl=%s"),
        *ClientVersion, *ServerBaseUrl);
}

void UMMOGameInstance::ResolveServerBaseUrl()
{
    // 1. Command-line override: -server=host:port
    FString CliServer;
    if (FParse::Value(FCommandLine::Get(), TEXT("server="), CliServer) && !CliServer.IsEmpty())
    {
        // If user passes a bare host:port, default to https://
        if (!CliServer.StartsWith(TEXT("http://")) && !CliServer.StartsWith(TEXT("https://")))
        {
            CliServer = FString::Printf(TEXT("https://%s"), *CliServer);
        }
        ServerBaseUrl = CliServer;
        UE_LOG(LogTemp, Log, TEXT("[MMO] ServerBaseUrl from CLI: %s"), *ServerBaseUrl);
        return;
    }

    // 2. SaveGame override
    if (UGameplayStatics::DoesSaveGameExist(TEXT("SabriMMO_Options"), 0))
    {
        if (UOptionsSaveGame* Save = Cast<UOptionsSaveGame>(
                UGameplayStatics::LoadGameFromSlot(TEXT("SabriMMO_Options"), 0)))
        {
            if (!Save->CustomServerUrl.IsEmpty())
            {
                ServerBaseUrl = Save->CustomServerUrl;
                UE_LOG(LogTemp, Log, TEXT("[MMO] ServerBaseUrl from SaveGame: %s"), *ServerBaseUrl);
                return;
            }
        }
    }

    // 3. Compile-time default (from SabriMMO.Build.cs PublicDefinitions)
    ServerBaseUrl = FString(SABRI_DEFAULT_MASTER_URL);
    UE_LOG(LogTemp, Log, TEXT("[MMO] ServerBaseUrl from compile default: %s"), *ServerBaseUrl);
}
```

Modify `GetServerSocketUrl()` to produce `wss://` or `ws://`:
```cpp
FString UMMOGameInstance::GetServerSocketUrl() const
{
    // Prefer selected server from /api/servers list, if set
    FString Url;
    if (!SelectedServer.Host.IsEmpty())
    {
        // Mirror scheme of current ServerBaseUrl (master URL)
        const bool bTls = ServerBaseUrl.StartsWith(TEXT("https://"));
        Url = FString::Printf(TEXT("%s://%s:%d"),
            bTls ? TEXT("https") : TEXT("http"),
            *SelectedServer.Host, SelectedServer.Port);
    }
    else
    {
        Url = ServerBaseUrl;
    }

    // Convert http:// -> ws://, https:// -> wss://
    if (Url.StartsWith(TEXT("https://")))
    {
        Url.ReplaceInline(TEXT("https://"), TEXT("wss://"));
    }
    else if (Url.StartsWith(TEXT("http://")))
    {
        Url.ReplaceInline(TEXT("http://"), TEXT("ws://"));
    }
    return Url;
}
```

Update `SelectServer()` to preserve scheme:
```cpp
void UMMOGameInstance::SelectServer(const FServerInfo& Server)
{
    SelectedServer = Server;
    const bool bTls = ServerBaseUrl.StartsWith(TEXT("https://"));
    ServerBaseUrl = FString::Printf(TEXT("%s://%s:%d"),
        bTls ? TEXT("https") : TEXT("http"),
        *Server.Host, Server.Port);
    UE_LOG(LogTemp, Log, TEXT("Selected server: %s (%s)"), *Server.Name, *ServerBaseUrl);
}
```

Update `ConnectSocket()` — set TLS flags on the native socket, **conditional on scheme** so dev-mode `ws://` skips the OpenSSL path entirely:
```cpp
void UMMOGameInstance::ConnectSocket()
{
    if (NativeSocket.IsValid() && NativeSocket->bIsConnected)
    {
        UE_LOG(LogMMOSocket, Warning, TEXT("ConnectSocket called but socket is already connected."));
        return;
    }

    NativeSocket = ISocketIOClientModule::Get().NewValidNativePointer();
    if (!NativeSocket.IsValid())
    {
        UE_LOG(LogMMOSocket, Error, TEXT("Failed to create FSocketIONative"));
        return;
    }

    NativeSocket->bCallbackOnGameThread = true;
    NativeSocket->bUnbindEventsOnDisconnect = false;
    NativeSocket->MaxReconnectionAttempts = 0;
    NativeSocket->ReconnectionDelay = 3000;
    NativeSocket->VerboseLog = false;

    // Compute URL first so TLS flags match scheme
    const FString SocketUrl = GetServerSocketUrl();
    const bool bUseTls = SocketUrl.StartsWith(TEXT("wss://"));

    // --- Scheme-conditional TLS: wss:// turns on, ws:// stays off ---
    // Keeps local dev (ws://localhost:3001) on the plain-WS path.
    NativeSocket->bShouldUseTlsLibraries = bUseTls;
    // Plugin issue #303 — cert verification is broken in v2.11.0.
    // Leave skip = true when TLS is on; it's a no-op when off.
    NativeSocket->bShouldSkipCertificateVerification = bUseTls;

    NativeSocket->OnConnectedCallback = [this](const FString& SocketId, const FString& SessionId)
    {
        OnSocketConnected(SocketId, SessionId);
    };
    NativeSocket->OnDisconnectedCallback = [this](const ESIOConnectionCloseReason Reason)
    {
        OnSocketDisconnected((int32)Reason);
    };
    NativeSocket->OnReconnectionCallback = [this](const uint32 AttemptCount, const uint32 DelayInMs)
    {
        OnSocketReconnecting(AttemptCount, DelayInMs);
    };

    if (EventRouter)
    {
        EventRouter->BindToNativeClient(NativeSocket);
    }

    UE_LOG(LogMMOSocket, Log, TEXT("Connecting persistent socket to: %s (TLS=%s)"),
        *SocketUrl, bUseTls ? TEXT("on") : TEXT("off"));
    NativeSocket->Connect(SocketUrl);
}
```

### 3.4 `UHttpManager::HealthCheck` — parse version fields

File: `client/SabriMMO/Source/SabriMMO/MMOHttpManager.cpp`

Modify `HealthCheck` (around line 58) to parse and store version info, and emit a version-mismatch event if needed:

```cpp
void UHttpManager::HealthCheck(UObject* WorldContextObject)
{
    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda(
        [WeakContext](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful)
    {
        if (!WeakContext.IsValid()) return;
        UMMOGameInstance* GI = Cast<UMMOGameInstance>(
            UGameplayStatics::GetGameInstance(WeakContext.Get()));
        if (!GI) return;

        if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] Server health check failed"));
            GI->OnHealthCheckFailed.Broadcast(TEXT("Cannot reach server"));
            return;
        }

        // Parse server version + minClientVersion
        TSharedPtr<FJsonObject> Json;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
        if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
        {
            Json->TryGetStringField(TEXT("serverVersion"), GI->ServerVersion);
            Json->TryGetStringField(TEXT("minClientVersion"), GI->MinClientVersion);
        }

        UE_LOG(LogTemp, Display, TEXT("[HTTP] Server OK. version=%s, minClient=%s, myVersion=%s"),
            *GI->ServerVersion, *GI->MinClientVersion, *GI->ClientVersion);

        // Version gate: block login if client is too old.
        // IMPORTANT: skip the gate if ClientVersion is empty (DefaultGame.ini ProjectVersion
        // not set). Otherwise an empty version parses to 0.0.0 and fails every min check —
        // this will break local dev if someone forgets to set ProjectVersion.
        if (!GI->ClientVersion.IsEmpty()
            && !GI->MinClientVersion.IsEmpty()
            && FVersion::CompareLess(GI->ClientVersion, GI->MinClientVersion))
        {
            const FString Reason = FString::Printf(
                TEXT("Your client (v%s) is older than the required minimum (v%s). Please update."),
                *GI->ClientVersion, *GI->MinClientVersion);
            UE_LOG(LogTemp, Warning, TEXT("[HTTP] %s"), *Reason);
            GI->OnVersionMismatch.Broadcast(Reason);
            return;
        }

        GI->OnHealthCheckPassed.Broadcast();
    });
    Request->SetURL(GetServerBaseUrl(WorldContextObject) + TEXT("/health"));
    Request->SetVerb(TEXT("GET"));
    Request->SetTimeout(10.0f);
    Request->ProcessRequest();
}
```

Add `OnHealthCheckPassed` / `OnHealthCheckFailed` delegates to `UMMOGameInstance` alongside the other dispatchers.

### 3.5 `FVersion` util

New small utility file `client/SabriMMO/Source/SabriMMO/VersionUtil.h`:

```cpp
#pragma once

#include "CoreMinimal.h"

/**
 * Semantic version comparison (MAJOR.MINOR.PATCH).
 * Treats missing components as 0. Ignores -prerelease suffixes.
 */
struct FVersion
{
    int32 Major = 0;
    int32 Minor = 0;
    int32 Patch = 0;

    static FVersion Parse(const FString& Str)
    {
        FVersion V;
        TArray<FString> Parts;
        // Strip any -prerelease suffix first (e.g. "0.4.0-beta" -> "0.4.0")
        FString Clean = Str;
        int32 DashIdx;
        if (Clean.FindChar('-', DashIdx)) { Clean = Clean.Left(DashIdx); }
        Clean.ParseIntoArray(Parts, TEXT("."));
        if (Parts.Num() > 0) V.Major = FCString::Atoi(*Parts[0]);
        if (Parts.Num() > 1) V.Minor = FCString::Atoi(*Parts[1]);
        if (Parts.Num() > 2) V.Patch = FCString::Atoi(*Parts[2]);
        return V;
    }

    static bool CompareLess(const FString& A, const FString& B)
    {
        const FVersion VA = Parse(A);
        const FVersion VB = Parse(B);
        if (VA.Major != VB.Major) return VA.Major < VB.Major;
        if (VA.Minor != VB.Minor) return VA.Minor < VB.Minor;
        return VA.Patch < VB.Patch;
    }
};
```

### 3.6 `LoginFlowSubsystem` — show "Please update" modal

File: `client/SabriMMO/Source/SabriMMO/UI/LoginFlowSubsystem.cpp`

In `OnWorldBeginPlay`, after binding other GI delegates, bind version-mismatch:
```cpp
if (UMMOGameInstance* GI = GetGI())
{
    GI->OnVersionMismatch.AddDynamic(this, &ULoginFlowSubsystem::HandleVersionMismatch);
    GI->OnHealthCheckFailed.AddDynamic(this, &ULoginFlowSubsystem::HandleHealthCheckFailed);
    // ... existing bindings ...
}

// Kick off a health check as soon as the login flow starts
UHttpManager::HealthCheck(this);
```

Handler:
```cpp
void ULoginFlowSubsystem::HandleVersionMismatch(const FString& Reason)
{
    ShowLoadingOverlay(Reason + TEXT("\n\nDownload the latest version from:\nhttps://sabrimmo.com/download"));
    // Disable the login button if it exists
    if (LoginWidget.IsValid())
    {
        LoginWidget->SetLoginButtonEnabled(false);
    }
}
```

Add `SetLoginButtonEnabled(bool)` to `SLoginWidget` if not already present — trivial `SetEnabled(false)` on the login button slot.

### 3.7 Optional: Options menu entry for `CustomServerUrl`

For QA/invitee support, add a hidden Options panel entry so tech-savvy users can override the URL without rebuilding:

File: `client/SabriMMO/Source/SabriMMO/UI/OptionsSubsystem.cpp` (or wherever the SOptionsWidget is built)

Add a collapsible "Advanced — Network" section with a text input bound to `UOptionsSaveGame::CustomServerUrl`. Save triggers a notice "Restart the game to apply."

Skip this for the initial Shipping build if you want — the command-line override and compile-time default cover 99% of cases.

---

## 4. Server-Side Changes

### 4.1 Extend `/health` with version fields

File: `server/src/index.js` (current handler at line 34142).

```javascript
const SERVER_VERSION = process.env.SERVER_VERSION || '0.4.0';
const MIN_CLIENT_VERSION = process.env.MIN_CLIENT_VERSION || '0.4.0';

app.get('/health', async (req, res) => {
    try {
        logger.debug('Health check requested');
        const result = await pool.query('SELECT NOW()');
        logger.info('Health check passed');
        res.json({
            status: 'OK',
            serverVersion: SERVER_VERSION,
            minClientVersion: MIN_CLIENT_VERSION,
            timestamp: result.rows[0].now,
            message: 'Server is running and connected to database'
        });
    } catch (err) {
        logger.error('Health check failed:', err.message);
        res.status(500).json({
            status: 'ERROR',
            serverVersion: SERVER_VERSION,
            message: 'Database connection failed',
            error: err.message
        });
    }
});
```

### 4.2 Tighten CORS (optional, Scenario C hardening)

Currently `app.use(cors())` at line 34049 is wide open. If you never have a browser client, this is moot — but if/when you add one, tighten:
```javascript
app.use(cors({
    origin: process.env.NODE_ENV === 'production'
        ? ['https://sabrimmo.com']
        : true,  // permissive in dev
    methods: ['GET', 'POST', 'PUT', 'DELETE'],
    credentials: true,
}));
```

### 4.3 Rotate secrets (mandatory before production)

`server/.env` currently has:
- `DB_PASSWORD=goku22` — weak, committed in plaintext in the repo (via `.env`, which is gitignored but worth confirming).
- `JWT_SECRET=your-super-secret-jwt-key-change-this-in-production-2026` — placeholder.

**Before exposing the server to any friend**: rotate both.

```bash
# Generate strong secrets
openssl rand -base64 24     # use for DB_PASSWORD
openssl rand -base64 48     # use for JWT_SECRET
```

Rotating `JWT_SECRET` invalidates all existing tokens — users re-login on next session start. One-time cost.

Verify `.env` is in `.gitignore` (it is):
```bash
grep -l ".env" C:\Sabri_MMO\.gitignore
```

### 4.4 Populate `SERVER_HOST` in production `.env`

Currently unset, so `/api/servers` returns `localhost`. On production, add:
```
SERVER_HOST=sabrimmo.com
```

---

## 5. Testing Matrix

| Scenario | Expected behavior |
|---|---|
| `SabriMMO.exe` with **no args** in dev build | ServerBaseUrl = `http://localhost:3001` (from compile default) |
| `SabriMMO.exe` with no args in **Shipping** build | ServerBaseUrl = `https://sabrimmo.com` (from compile default) |
| `SabriMMO.exe -server=staging.sabrimmo.com:443` | ServerBaseUrl = `https://staging.sabrimmo.com:443` (CLI override) |
| User sets `CustomServerUrl="https://dev.sabrimmo.com"` in Options, restarts | ServerBaseUrl = `https://dev.sabrimmo.com` (SaveGame override) |
| CLI + SaveGame both set | CLI wins |
| Server returns `minClientVersion=0.5.0`, client is `0.4.0` | Client shows "Please update" modal, login disabled |
| Server returns `minClientVersion=0.4.0`, client is `0.4.2` | Login proceeds normally |
| `/health` times out (server down) | Client shows "Cannot reach server" modal, no proceed |
| Socket URL is `wss://sabrimmo.com`, TLS handshake fails | `bShouldSkipCertificateVerification = true` should prevent this (any cert accepted); actual failure would indicate an OpenSSL / ASIO issue in the plugin — rare |

Add automated unit tests for `FVersion::CompareLess`:
```cpp
// Tests/VersionUtilTests.cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVersionCompareTest,
    "SabriMMO.Version.Compare",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FVersionCompareTest::RunTest(const FString& Parameters)
{
    TestTrue("0.4.0 < 0.5.0", FVersion::CompareLess(TEXT("0.4.0"), TEXT("0.5.0")));
    TestFalse("0.5.0 < 0.4.0", FVersion::CompareLess(TEXT("0.5.0"), TEXT("0.4.0")));
    TestFalse("0.4.0 < 0.4.0", FVersion::CompareLess(TEXT("0.4.0"), TEXT("0.4.0")));
    TestTrue("0.4.0 < 0.4.1", FVersion::CompareLess(TEXT("0.4.0"), TEXT("0.4.1")));
    TestTrue("0.4.9 < 0.5.0", FVersion::CompareLess(TEXT("0.4.9"), TEXT("0.5.0")));
    TestTrue("0.4.0-beta == 0.4.0", !FVersion::CompareLess(TEXT("0.4.0-beta"), TEXT("0.4.0")));
    return true;
}
```

---

## 6. Implementation Sequence

1. Add `UOptionsSaveGame::CustomServerUrl` field.
2. Add `SABRI_DEFAULT_MASTER_URL` to `SabriMMO.Build.cs`.
3. Replace `ServerBaseUrl` default, add `ResolveServerBaseUrl()` in `UMMOGameInstance`.
4. Rewrite `GetServerSocketUrl()` and `SelectServer()` to preserve scheme.
5. Set TLS flags on `NativeSocket` in `ConnectSocket()`.
6. Add `FVersion` util.
7. Add `ClientVersion`, `ServerVersion`, `MinClientVersion`, `OnVersionMismatch` to `UMMOGameInstance`.
8. Extend `UHttpManager::HealthCheck` to parse + compare versions.
9. Bind `OnVersionMismatch` in `ULoginFlowSubsystem`, show modal.
10. Add `/health` version fields on server.
11. Rotate `DB_PASSWORD` and `JWT_SECRET` in production `.env`.
12. Set `SERVER_HOST=sabrimmo.com` in production `.env`.
13. Update `Config/DefaultGame.ini` ProjectName + ProjectVersion.
14. Unit test `FVersion`.
15. Build Development target → verify editor login works against local dev server.
16. Build Shipping target with prod URL → verify it connects to production.

---

## 6.5 Does this break local development?

**No.** The Development and DebugGame build configurations (used by the UE5 Editor, PIE, and any non-Shipping build) retain `http://localhost:3001` as the compile-time default because of the conditional in `SabriMMO.Build.cs`:

```csharp
if (Target.Configuration == UnrealTargetConfiguration.Development ||
    Target.Configuration == UnrealTargetConfiguration.DebugGame)
{
    DefaultMasterUrl = System.Environment.GetEnvironmentVariable("SABRI_DEFAULT_MASTER_URL")
                       ?? "http://localhost:3001";
}
```

Only **Shipping** builds bake the production URL.

Dev workflow stays: `npm run dev` in `server/` → `Play` in UE5 editor → client hits `http://localhost:3001` → login, character select, spawn. Identical to pre-change behaviour.

### Dev-safe defaults to verify

- `Config/DefaultGame.ini` has `ProjectVersion=0.4.0` (or whatever matches `MIN_CLIENT_VERSION` in local `server/.env`). Otherwise `ClientVersion` is empty and the version-gate skip above kicks in — you still log in, but without the gate protecting you.
- Local `server/.env` does NOT need `MIN_CLIENT_VERSION` or `SERVER_VERSION` set — the server defaults them to `'0.4.0'` in code.
- `SocketIOClient` plugin's OpenSSL path is now skipped for `ws://` URLs, so dev doesn't drag in TLS libraries that can mask compile issues.

### Testing a Shipping build against a local server

You'll want this for reproducing invitee bugs. Three options, most to least convenient:

1. **CLI override on launch** (easiest):
   ```bat
   SabriMMO-Shipping.exe -server=localhost:3001
   ```
2. **Build-time override** (make a "local-Shipping" build to hand to a LAN friend):
   ```bat
   set SABRI_DEFAULT_MASTER_URL=http://localhost:3001
   scripts\build_ship.bat
   ```
3. **SaveGame override** — set `CustomServerUrl` via an in-game options panel. Persists across launches; remember to clear it before shipping a real build.

---

## 7. Rollout Notes

- **First rollout**: ship with `MIN_CLIENT_VERSION = current ClientVersion`. No one gets blocked.
- **Breaking server change**: bump `MIN_CLIENT_VERSION` on the server to force clients to update. Push a new client build to itch.io simultaneously.
- **Rollback**: drop `MIN_CLIENT_VERSION` back in `server/.env` and restart — old clients can reconnect while you investigate.
- **Diagnostic**: include `ClientVersion`, `ServerBaseUrl`, `BUILD_ID.txt` on the client debug footer so invitees can paste them into bug reports.

---

**End of design.** Next step is implementation. The Implementation Sequence above is ordered so you can compile and test after each step.
