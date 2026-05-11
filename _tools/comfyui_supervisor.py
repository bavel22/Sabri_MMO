"""
ComfyUI supervisor — auto-restarts ComfyUI if it dies.
Keeps trying indefinitely with backoff. Logs each restart to a file.

Run: C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/comfyui_supervisor.py
"""
import os
import subprocess
import sys
import time
from pathlib import Path

LOG_FILE = Path("C:/temp/comfyui_supervisor.log")


def log(msg):
    ts = time.strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line, flush=True)
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def main():
    log("=" * 60)
    log("ComfyUI Supervisor started")
    log("=" * 60)

    # Force UTF-8 everywhere
    env = os.environ.copy()
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUTF8"] = "1"

    restart_count = 0
    last_crash = 0
    backoff = 5  # seconds

    while True:
        log(f"Starting ComfyUI (restart #{restart_count})...")
        start_time = time.time()
        try:
            proc = subprocess.run(
                ["C:/ComfyUI/venv/Scripts/python.exe", "main.py"],
                cwd="C:/ComfyUI",
                env=env,
                stdout=open("C:/temp/comfyui_run.log", "ab"),
                stderr=subprocess.STDOUT,
            )
            uptime = time.time() - start_time
            log(f"ComfyUI exited with code {proc.returncode} after {uptime:.0f}s uptime")
        except KeyboardInterrupt:
            log("Supervisor interrupted by user")
            return 0
        except Exception as e:
            uptime = time.time() - start_time
            log(f"Supervisor error: {e}, uptime was {uptime:.0f}s")

        restart_count += 1
        # Backoff: if it crashes within 30s of starting, wait longer
        if uptime < 30:
            backoff = min(backoff * 2, 60)
            log(f"Crashed too quickly, backing off {backoff}s before restart")
        else:
            backoff = 5
        time.sleep(backoff)


if __name__ == "__main__":
    sys.exit(main())
