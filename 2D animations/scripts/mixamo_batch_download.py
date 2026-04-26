#!/usr/bin/env python3
"""
Mixamo batch animation downloader for Sabri_MMO enemy sprite pipeline.

Uses Mixamo's internal (reverse-engineered) API to export all 16 standard enemy
animations for a single character that you have already uploaded to Mixamo.

Verified working on 40+ enemies as of 2026-04-25. For full API details and
debugging, see: C:/Users/pladr/.claude/projects/C--Sabri-MMO/memory/feedback-mixamo-batch-api.md

--------------------------------------------------------------------------------
ONE-TIME SETUP (per browser session — tokens expire after ~24 hours)
--------------------------------------------------------------------------------
1. Log in to https://www.mixamo.com in Chrome.
2. Open DevTools (F12) -> Network tab -> filter "api/v1".
3. Click any animation thumbnail to fire a request.
4. Click any /api/v1/* request -> "Request Headers" -> copy the value AFTER
   "authorization: Bearer " (the long JWT blob, no quotes, no "Bearer ").
5. Save it to: 2D animations/scripts/.mixamo_token  (one line, gitignored)

--------------------------------------------------------------------------------
PER-CHARACTER (per enemy)
--------------------------------------------------------------------------------
1. Upload the enemy FBX/GLB to Mixamo manually (still requires browser).
2. Select the uploaded character in Mixamo's left panel.
3. In DevTools Network tab, find any request with /characters/<UUID>/ in URL.
   Copy that UUID (8-4-4-4-12 hex format).
4. Run:
       python mixamo_batch_download.py --enemy <name> --character-id <UUID>

   <name> becomes the output folder. Use lowercase + underscores
   (e.g. orc_warrior, pirate_skel).

--------------------------------------------------------------------------------
USAGE
--------------------------------------------------------------------------------
    # Standard run (16-animation set, ~4 minutes)
    python mixamo_batch_download.py --enemy skeleton --character-id <UUID>

    # Re-run after transient failure — skips files already on disk
    python mixamo_batch_download.py --enemy skeleton --character-id <UUID>

    # Force redownload of everything
    python mixamo_batch_download.py --enemy skeleton --character-id <UUID> --force

    # Custom animation list (one "Mixamo Name" per line, # comments ok)
    python mixamo_batch_download.py --enemy skeleton --character-id <UUID> \
        --animations-file my_list.txt

    # Debug API shape — dumps first retarget response to .mixamo_retarget_response.json
    python mixamo_batch_download.py --enemy skeleton --character-id <UUID> --debug

Downloaded FBX files are saved to:
    2D animations/3d_models/animations/enemies/{enemy}/{Animation Name}.fbx

--------------------------------------------------------------------------------
COMMON FAILURES
--------------------------------------------------------------------------------
- 401 -> token expired, re-extract from DevTools
- 429 -> rate-limited, wait ~10 minutes (script aborts to avoid worse)
- 504 -> transient Mixamo timeout, just re-run (skips existing files)
- ConnectionResetError 10054 -> transient network, re-run (skips existing files)
- "invalid literal for int() ... model-id" -> API shape changed, run with --debug
  and inspect .mixamo_retarget_response.json against the API doc above
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

import requests


# --------------------------------------------------------------------------- #
# Paths
# --------------------------------------------------------------------------- #

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent  # .../Sabri_MMO
TOKEN_FILE = SCRIPT_DIR / ".mixamo_token"
ENEMY_ANIM_ROOT = PROJECT_ROOT / "2D animations" / "3d_models" / "animations" / "enemies"


# --------------------------------------------------------------------------- #
# API constants
# --------------------------------------------------------------------------- #

MIXAMO_API_BASE = "https://www.mixamo.com/api/v1"
MIXAMO_X_API_KEY = "mixamo2"  # public key used by the Mixamo site itself

COMMON_HEADERS = {
    "Accept": "application/json",
    "Content-Type": "application/json",
    "X-Api-Key": MIXAMO_X_API_KEY,
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Sabri_MMO/1.0",
}

# Default export preferences (matches the manual "With Skin, FPS=30, No Reduction" workflow)
EXPORT_PREFERENCES = {
    "format": "fbx7_2019",
    "skin": "true",
    "fps": "30",
    "reducekf": "0",
}


# --------------------------------------------------------------------------- #
# Standard 16-animation enemy set (from skeleton — ground truth for the pipeline)
# --------------------------------------------------------------------------- #

DEFAULT_ENEMY_ANIMATIONS = [
    "Idle",
    "2hand Idle",
    "Walking",
    "Run With Sword",
    "Mutant Punch",
    "Stable Sword Inward Slash",
    "Great Sword Slash",
    "Standing 1H Magic Attack 01",
    "Standing 2H Magic Area Attack 01",
    "Standing 2H Magic Area Attack 02",
    "Standing Draw Arrow",
    "Standing 1H Cast Spell 01",
    "Sword And Shield Block",
    "Reaction",
    "Sitting Idle",
    "Dying",
]


# --------------------------------------------------------------------------- #
# Helpers
# --------------------------------------------------------------------------- #

def load_token() -> str:
    if not TOKEN_FILE.exists():
        sys.exit(
            f"ERROR: token file not found: {TOKEN_FILE}\n"
            f"Follow the SETUP instructions at the top of this script."
        )
    token = TOKEN_FILE.read_text(encoding="utf-8").strip()
    if not token:
        sys.exit(f"ERROR: token file is empty: {TOKEN_FILE}")
    if token.lower().startswith("bearer "):
        token = token[7:].strip()
    return token


def make_session(token: str) -> requests.Session:
    s = requests.Session()
    s.headers.update(COMMON_HEADERS)
    s.headers["Authorization"] = f"Bearer {token}"
    return s


def read_animation_list(path: Path | None) -> list[str]:
    if path is None:
        return list(DEFAULT_ENEMY_ANIMATIONS)
    lines = path.read_text(encoding="utf-8").splitlines()
    names = [ln.strip() for ln in lines if ln.strip() and not ln.strip().startswith("#")]
    if not names:
        sys.exit(f"ERROR: animation list is empty: {path}")
    return names


# --------------------------------------------------------------------------- #
# Mixamo API calls
# --------------------------------------------------------------------------- #

def search_animation(session: requests.Session, query: str) -> dict | None:
    """Search Mixamo for an animation. Returns the best match or None."""
    params = {
        "type": "Motion,MotionPack",
        "query": query,
        "page": 1,
        "limit": 12,
    }
    r = session.get(f"{MIXAMO_API_BASE}/products", params=params, timeout=30)
    if r.status_code == 401:
        sys.exit("ERROR 401: token rejected. Extract a fresh token and try again.")
    r.raise_for_status()
    data = r.json()
    results = data.get("results") or []
    if not results:
        return None

    # Prefer exact case-insensitive match on "name"; fall back to first result.
    qlow = query.lower()
    for item in results:
        if (item.get("name") or "").lower() == qlow:
            return item
    for item in results:
        if qlow in (item.get("name") or "").lower():
            return item
    return results[0]


_retarget_dumped = False

def retarget_animation(session: requests.Session, character_id: str, anim_uuid: str,
                       debug: bool = False) -> dict:
    """GET /products/{uuid}?character_id=... — retargets the animation onto the character
    server-side and returns the gms_hash details needed for export."""
    global _retarget_dumped
    r = session.get(
        f"{MIXAMO_API_BASE}/products/{anim_uuid}",
        params={"similar": 0, "character_id": character_id},
        timeout=30,
    )
    if r.status_code == 401:
        sys.exit("ERROR 401: token rejected. Extract a fresh token and try again.")
    if r.status_code == 429:
        sys.exit("ERROR 429: rate-limited by Mixamo. Wait ~10 minutes before retrying.")
    r.raise_for_status()
    data = r.json() if r.content else {}
    # Dump the first retarget response to a debug file for API-shape discovery.
    if debug and not _retarget_dumped:
        debug_path = SCRIPT_DIR / ".mixamo_retarget_response.json"
        debug_path.write_text(json.dumps(data, indent=2), encoding="utf-8")
        print(f"        [debug] saved retarget response to {debug_path.name}")
        _retarget_dumped = True
    return data


def build_gms_hash(retarget_data: dict) -> list[dict]:
    """Transform the retarget response's details.gms_hash into the export payload shape.

    Response shape:
        details.gms_hash = {
            "model-id": 117780901,
            "mirror": false,
            "trim": [0.0, 100.0],
            "inplace": false,
            "arm-space": 0,
            "params": [["Overdrive", 0.0], ...]   # array of [name, value] pairs
        }
    Export needs:
        {"model-id": 117780901, "mirror": false, "trim": [0, 100],
         "overdrive": 0, "params": "0", "arm-space": 0, "inplace": false}
    Transformation:
        - "Overdrive" param is promoted to its own top-level "overdrive" field
        - Remaining params' values are comma-joined into the "params" string
          (empty collapses to "0" to match frontend behavior)
    """
    details = retarget_data.get("details") or {}
    src = details.get("gms_hash") or {}

    if "model-id" not in src:
        raise RuntimeError(
            f"No model-id in details.gms_hash. "
            f"top-level keys: {list(retarget_data.keys())}, "
            f"details keys: {list(details.keys())}"
        )

    overdrive = 0
    remaining: list[str] = []
    for pair in src.get("params") or []:
        if not (isinstance(pair, (list, tuple)) and len(pair) >= 2):
            continue
        name, value = pair[0], pair[1]
        val_int = int(value) if isinstance(value, (int, float)) else value
        if str(name).lower() == "overdrive":
            overdrive = val_int if isinstance(val_int, int) else 0
        else:
            remaining.append(str(val_int))
    params_str = ",".join(remaining) if remaining else "0"

    trim_src = src.get("trim") or [0, 100]
    trim_int = [int(v) for v in trim_src]

    return [{
        "model-id": int(src["model-id"]),
        "mirror": bool(src.get("mirror", False)),
        "trim": trim_int,
        "overdrive": overdrive,
        "params": params_str,
        "arm-space": src.get("arm-space", 0),
        "inplace": bool(src.get("inplace", False)),
    }]


def export_animation(session: requests.Session, character_id: str, anim: dict,
                     debug: bool = False) -> None:
    """Retarget the animation onto the character, then kick off an FBX export job."""
    anim_uuid = anim.get("id") or anim.get("uuid")
    anim_name = anim.get("name")
    anim_type = anim.get("type", "Motion")
    if not anim_uuid:
        raise RuntimeError(f"No UUID found for animation {anim_name!r}")

    retarget_data = retarget_animation(session, character_id, anim_uuid, debug=debug)
    gms_hash = build_gms_hash(retarget_data)

    payload = {
        "gms_hash": gms_hash,
        "preferences": EXPORT_PREFERENCES,
        "character_id": character_id,
        "type": anim_type,
        "product_name": anim_name,
    }
    r = session.post(f"{MIXAMO_API_BASE}/animations/export", json=payload, timeout=30)
    if r.status_code == 401:
        sys.exit("ERROR 401: token rejected. Extract a fresh token and try again.")
    if r.status_code == 429:
        sys.exit("ERROR 429: rate-limited by Mixamo. Wait ~10 minutes before retrying.")
    if r.status_code == 404:
        sys.exit(
            "ERROR 404 on /animations/export. The API shape has changed again.\n"
            "Capture a working Download request from mixamo.com DevTools Network tab."
        )
    # 202 Accepted is the expected success code — job is queued.
    r.raise_for_status()


def wait_for_export(
    session: requests.Session,
    character_id: str,
    *,
    timeout_seconds: int = 120,
    poll_interval: float = 1.5,
) -> str:
    """Poll the monitor endpoint until the export job is done. Returns the download URL."""
    deadline = time.time() + timeout_seconds
    last_status = None
    while time.time() < deadline:
        r = session.get(f"{MIXAMO_API_BASE}/characters/{character_id}/monitor", timeout=30)
        if r.status_code == 401:
            sys.exit("ERROR 401: token rejected during polling.")
        r.raise_for_status()
        data = r.json()
        status = data.get("status")
        if status != last_status:
            last_status = status
        if status == "completed":
            url = (
                data.get("job_result")
                or data.get("result")
                or data.get("download_url")
                or data.get("url")
            )
            if not url:
                raise RuntimeError(f"monitor: completed but no download URL. Fields: {list(data.keys())}")
            return url
        if status == "failed":
            raise RuntimeError(f"monitor: export failed -> {data!r}")
        time.sleep(poll_interval)
    raise TimeoutError(f"export did not complete in {timeout_seconds}s (last status: {last_status!r})")


def download_fbx(url: str, output_path: Path) -> int:
    """Stream the FBX to disk. Returns bytes written."""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with requests.get(url, stream=True, timeout=120) as r:
        r.raise_for_status()
        written = 0
        with output_path.open("wb") as f:
            for chunk in r.iter_content(chunk_size=1 << 16):
                if chunk:
                    f.write(chunk)
                    written += len(chunk)
    return written


# --------------------------------------------------------------------------- #
# Main
# --------------------------------------------------------------------------- #

def main() -> None:
    ap = argparse.ArgumentParser(
        description="Batch-download Mixamo animations for a single enemy character.",
    )
    ap.add_argument("--enemy", required=True, help="Enemy name (output folder under animations/enemies/)")
    ap.add_argument("--character-id", required=True, help="Mixamo character UUID (uploaded model ID)")
    ap.add_argument("--animations-file", type=Path, default=None,
                    help="Optional: text file with one animation name per line (overrides default 16-set)")
    ap.add_argument("--force", action="store_true", help="Overwrite existing .fbx files")
    ap.add_argument("--timeout", type=int, default=120, help="Per-animation export timeout (seconds)")
    ap.add_argument("--debug", action="store_true", help="Save first retarget response to a debug file")
    args = ap.parse_args()

    token = load_token()
    session = make_session(token)
    animations = read_animation_list(args.animations_file)
    output_dir = ENEMY_ANIM_ROOT / args.enemy

    print(f"Mixamo batch download")
    print(f"  enemy        : {args.enemy}")
    print(f"  character id : {args.character_id}")
    print(f"  output dir   : {output_dir}")
    print(f"  animations   : {len(animations)}")
    print()

    ok, skipped, missing, failed = 0, 0, 0, 0

    for i, query in enumerate(animations, start=1):
        output_path = output_dir / f"{query}.fbx"
        tag = f"[{i:2d}/{len(animations)}]"

        if output_path.exists() and not args.force:
            print(f"{tag} SKIP  {query}.fbx (exists)")
            skipped += 1
            continue

        print(f"{tag} ...   {query}")
        try:
            anim = search_animation(session, query)
        except requests.RequestException as e:
            print(f"        search error: {e}")
            failed += 1
            continue

        if not anim:
            print(f"        NOT FOUND on Mixamo (check spelling)")
            missing += 1
            continue

        found_name = anim.get("name")
        if found_name != query:
            print(f"        (matched: {found_name!r})")

        try:
            export_animation(session, args.character_id, anim, debug=args.debug)
            url = wait_for_export(session, args.character_id, timeout_seconds=args.timeout)
            size = download_fbx(url, output_path)
        except Exception as e:
            print(f"        export/download error: {e}")
            failed += 1
            continue

        print(f"        OK   {output_path.name}  ({size / 1024:.0f} KB)")
        ok += 1

        # Mixamo can reject fast bursts — small courtesy delay between jobs.
        time.sleep(1.0)

    print()
    print(f"Done. ok={ok} skipped={skipped} missing={missing} failed={failed}")
    print(f"Output: {output_dir}")
    if failed or missing:
        sys.exit(1)


if __name__ == "__main__":
    main()
