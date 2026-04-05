"""
Tripo3D API Client — Convert 2D Reference Images to 3D GLB Models
===================================================================
Takes hero reference PNGs and converts them to 3D models via Tripo3D's API.

Setup:
  1. Create account at https://platform.tripo3d.ai
  2. Get API key from https://platform.tripo3d.ai/api-keys
  3. Set env var: set TRIPO_API_KEY=your_key_here
     Or pass --api-key on command line

Usage:
  python tripo_image_to_3d.py --input sprites/references/hero_refs/ --output 3d_models/characters/

  Single image:
  python tripo_image_to_3d.py --input hero_refs/novice_m.png --output 3d_models/characters/

  With explicit key:
  python tripo_image_to_3d.py --api-key tripo_xxx --input hero_refs/ --output 3d_models/

Notes:
  - Free tier: 300 credits/month. Image-to-3D costs ~100 credits per model.
  - So ~3 models per month on free tier.
  - Paid plans start at $9.90/month for 2000 credits (~20 models).
  - Each model takes ~30-60 seconds to generate.
  - Output is GLB format (ready for Blender or Mixamo).
  - Skips images that already have a .glb in the output directory.
"""

import os
import sys
import time
import json
import argparse

try:
    import urllib.request
    import urllib.error
except ImportError:
    pass

API_BASE = "https://api.tripo3d.ai/v2/openapi"


def api_request(method, path, api_key, data=None, headers=None):
    """Make an API request to Tripo3D."""
    url = f"{API_BASE}/{path}" if not path.startswith("http") else path

    if headers is None:
        headers = {}
    headers['Authorization'] = f'Bearer {api_key}'

    if data is not None and isinstance(data, dict):
        data = json.dumps(data).encode('utf-8')
        headers['Content-Type'] = 'application/json'

    req = urllib.request.Request(url, data=data, headers=headers, method=method)

    try:
        resp = urllib.request.urlopen(req, timeout=60)
        return json.loads(resp.read().decode())
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        print(f"  API error {e.code}: {body}")
        return None
    except urllib.error.URLError as e:
        print(f"  Connection error: {e.reason}")
        return None


def upload_image(api_key, image_path):
    """Upload image to Tripo3D, return file token."""
    with open(image_path, 'rb') as f:
        image_data = f.read()

    filename = os.path.basename(image_path)
    ext = os.path.splitext(filename)[1].lower().lstrip('.')
    content_type = {'png': 'image/png', 'jpg': 'image/jpeg',
                    'jpeg': 'image/jpeg', 'webp': 'image/webp'}.get(ext, 'image/png')

    boundary = f'----Boundary{int(time.time() * 1000)}'

    body = (
        f'--{boundary}\r\n'
        f'Content-Disposition: form-data; name="file"; filename="{filename}"\r\n'
        f'Content-Type: {content_type}\r\n\r\n'
    ).encode('utf-8') + image_data + f'\r\n--{boundary}--\r\n'.encode('utf-8')

    headers = {'Content-Type': f'multipart/form-data; boundary={boundary}'}

    req = urllib.request.Request(
        f"{API_BASE}/upload",
        data=body,
        headers={
            'Authorization': f'Bearer {api_key}',
            'Content-Type': f'multipart/form-data; boundary={boundary}',
        },
        method='POST'
    )

    try:
        resp = urllib.request.urlopen(req, timeout=60)
        result = json.loads(resp.read().decode())
    except (urllib.error.HTTPError, urllib.error.URLError) as e:
        print(f"  Upload failed: {e}")
        return None

    if result.get('code') != 0:
        print(f"  Upload error: {result.get('message', result)}")
        return None

    return result['data']['image_token']


def create_task(api_key, image_token):
    """Create image-to-model generation task."""
    result = api_request('POST', 'task', api_key, data={
        "type": "image_to_model",
        "file": {
            "type": "png",
            "file_token": image_token,
        },
    })

    if not result or result.get('code') != 0:
        msg = result.get('message', 'unknown') if result else 'no response'
        print(f"  Task creation failed: {msg}")
        return None

    return result['data']['task_id']


def poll_task(api_key, task_id, max_wait=300, poll_interval=5):
    """Poll task until success/failure or timeout."""
    start = time.time()
    last_progress = -1

    while time.time() - start < max_wait:
        result = api_request('GET', f'task/{task_id}', api_key)
        if not result:
            time.sleep(poll_interval)
            continue

        data = result.get('data', {})
        status = data.get('status', 'unknown')
        progress = data.get('progress', 0)

        if progress != last_progress:
            elapsed = int(time.time() - start)
            print(f"    {status}: {progress}% ({elapsed}s)", flush=True)
            last_progress = progress

        if status == 'success':
            return data
        elif status in ('failed', 'cancelled', 'unknown'):
            print(f"  Task {status}")
            return None

        time.sleep(poll_interval)

    print(f"  Timeout after {max_wait}s")
    return None


def download_glb(url, output_path):
    """Download GLB model from URL."""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    urllib.request.urlretrieve(url, output_path)
    size_mb = os.path.getsize(output_path) / (1024 * 1024)
    print(f"  Saved: {output_path} ({size_mb:.1f} MB)")


def process_one_image(api_key, image_path, output_dir):
    """Full pipeline for one reference image -> GLB model."""
    name = os.path.splitext(os.path.basename(image_path))[0]
    output_path = os.path.join(output_dir, f"{name}.glb")

    if os.path.exists(output_path):
        print(f"  [SKIP] {name}.glb exists")
        return True

    print(f"\n  [{name}] Uploading...")
    token = upload_image(api_key, image_path)
    if not token:
        return False

    print(f"  [{name}] Creating task...")
    task_id = create_task(api_key, token)
    if not task_id:
        return False

    print(f"  [{name}] Generating 3D model (task {task_id})...")
    result = poll_task(api_key, task_id)
    if not result:
        return False

    # Extract download URL (Tripo3D response format)
    output_data = result.get('output', {})
    glb_url = (output_data.get('model') or
               output_data.get('pbr_model') or
               output_data.get('base_model'))

    if not glb_url:
        print(f"  [{name}] No model URL in response")
        print(f"    Response keys: {list(output_data.keys())}")
        return False

    print(f"  [{name}] Downloading GLB...")
    download_glb(glb_url, output_path)
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Convert reference images to 3D models via Tripo3D API")
    parser.add_argument("--api-key",
                        default=os.environ.get("TRIPO_API_KEY"),
                        help="Tripo3D API key (or set TRIPO_API_KEY env var)")
    parser.add_argument("--input", required=True,
                        help="Input image file or directory of images")
    parser.add_argument("--output",
                        default="C:/Sabri_MMO/2D animations/3d_models/characters",
                        help="Output directory for GLB files")
    args = parser.parse_args()

    if not args.api_key:
        print("ERROR: No Tripo3D API key provided.")
        print()
        print("  Option 1: set TRIPO_API_KEY=your_key_here")
        print("  Option 2: python tripo_image_to_3d.py --api-key your_key ...")
        print()
        print("  Get your key at: https://platform.tripo3d.ai/api-keys")
        print("  Free tier: 300 credits/month (~3 models)")
        sys.exit(1)

    # Collect input images
    images = []
    if os.path.isdir(args.input):
        for f in sorted(os.listdir(args.input)):
            if f.lower().endswith(('.png', '.jpg', '.jpeg', '.webp')):
                images.append(os.path.join(args.input, f))
    elif os.path.isfile(args.input):
        images.append(args.input)
    else:
        print(f"ERROR: Not found: {args.input}")
        sys.exit(1)

    print(f"Tripo3D Image -> 3D Pipeline")
    print(f"  Images:  {len(images)}")
    print(f"  Output:  {args.output}")
    print(f"  API key: {args.api_key[:8]}...")
    print(f"  Credits: ~{len(images) * 100} needed (100 per model)")

    success = 0
    failed = 0
    skipped = 0

    for img in images:
        name = os.path.splitext(os.path.basename(img))[0]
        glb_path = os.path.join(args.output, f"{name}.glb")
        if os.path.exists(glb_path):
            skipped += 1
            print(f"  [SKIP] {name}")
            continue

        if process_one_image(args.api_key, img, args.output):
            success += 1
        else:
            failed += 1

    print(f"\n{'=' * 40}")
    print(f"  Done! {success} generated, {skipped} skipped, {failed} failed")
    print(f"  Output: {args.output}")
    print(f"{'=' * 40}")


if __name__ == "__main__":
    main()
