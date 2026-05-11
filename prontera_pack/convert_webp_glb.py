"""
Strips the EXT_texture_webp extension from Moonlake AI / Tripo3D GLBs by
decoding each embedded WebP image and re-encoding it as PNG. Also rewrites
the JSON header so textures reference the image directly via `source`
(removing the EXT_texture_webp pointer) and clears the extension from
`extensionsRequired`/`extensionsUsed`. UE5's glTF importer accepts the
result.

Pure Python — no Blender dependency. Requires Pillow.

Usage:
    python convert_webp_glb.py [--src DIR] [--dst DIR] [--inplace]

Defaults: converts every *.glb in prontera_pack/models/ into
prontera_pack/models_fixed/. Pass --inplace to overwrite originals
(originals back up to prontera_pack/models_webp_backup/).
"""

import argparse
import io
import json
import os
import shutil
import struct
import sys

from PIL import Image


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_SRC = os.path.join(SCRIPT_DIR, "prontera_pack", "models")
DEFAULT_DST = os.path.join(SCRIPT_DIR, "prontera_pack", "models_fixed")
BACKUP_DIR = os.path.join(SCRIPT_DIR, "prontera_pack", "models_webp_backup")

WEBP_EXT = "EXT_texture_webp"


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument("--src", default=DEFAULT_SRC)
    p.add_argument("--dst", default=DEFAULT_DST)
    p.add_argument("--inplace", action="store_true",
                   help="Overwrite originals (backs up to models_webp_backup/)")
    return p.parse_args()


def _pad4(buf: bytearray, fill: int = 0) -> None:
    while len(buf) % 4 != 0:
        buf.append(fill)


def convert_glb(src_path: str, dst_path: str) -> int:
    with open(src_path, "rb") as f:
        data = f.read()

    magic, version, _total_len = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF" or version != 2:
        raise RuntimeError(f"Not a GLB v2: {src_path}")

    json_len, json_type = struct.unpack_from("<I4s", data, 12)
    if json_type != b"JSON":
        raise RuntimeError("Missing JSON chunk")
    json_bytes = data[20:20 + json_len]
    gltf = json.loads(json_bytes.rstrip(b" \x00"))

    bin_off = 20 + json_len
    bin_len, bin_type = struct.unpack_from("<I4s", data, bin_off)
    if bin_type != b"BIN\x00":
        raise RuntimeError("Missing BIN chunk")
    original_bin = data[bin_off + 8:bin_off + 8 + bin_len]

    new_bin = bytearray(original_bin)
    images = gltf.get("images", [])
    buffer_views = gltf.get("bufferViews", [])

    converted = 0
    for img in images:
        if img.get("mimeType") != "image/webp":
            continue
        if "bufferView" not in img:
            continue
        bv = buffer_views[img["bufferView"]]
        off = bv.get("byteOffset", 0)
        length = bv["byteLength"]
        webp_bytes = bytes(original_bin[off:off + length])

        pil = Image.open(io.BytesIO(webp_bytes))
        if pil.mode not in ("RGB", "RGBA"):
            pil = pil.convert("RGBA")
        out_buf = io.BytesIO()
        pil.save(out_buf, format="PNG", optimize=False)
        png_bytes = out_buf.getvalue()

        _pad4(new_bin)
        bv["byteOffset"] = len(new_bin)
        bv["byteLength"] = len(png_bytes)
        new_bin.extend(png_bytes)

        img["mimeType"] = "image/png"
        converted += 1

    for tex in gltf.get("textures", []):
        ext = tex.get("extensions", {}).get(WEBP_EXT)
        if not ext:
            continue
        tex.setdefault("source", ext["source"])
        del tex["extensions"][WEBP_EXT]
        if not tex["extensions"]:
            del tex["extensions"]

    for key in ("extensionsRequired", "extensionsUsed"):
        if key in gltf and WEBP_EXT in gltf[key]:
            gltf[key].remove(WEBP_EXT)
            if not gltf[key]:
                del gltf[key]

    if gltf.get("buffers"):
        gltf["buffers"][0]["byteLength"] = len(new_bin)

    _pad4(new_bin)

    new_json_bytes = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    pad = (4 - (len(new_json_bytes) % 4)) % 4
    new_json_bytes += b" " * pad

    total = 12 + 8 + len(new_json_bytes) + 8 + len(new_bin)
    out = bytearray()
    out.extend(struct.pack("<4sII", b"glTF", 2, total))
    out.extend(struct.pack("<I4s", len(new_json_bytes), b"JSON"))
    out.extend(new_json_bytes)
    out.extend(struct.pack("<I4s", len(new_bin), b"BIN\x00"))
    out.extend(new_bin)

    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    with open(dst_path, "wb") as f:
        f.write(out)
    return converted


def main():
    args = parse_args()
    src_dir = os.path.abspath(args.src)
    if not os.path.isdir(src_dir):
        print(f"[ERR] Source dir not found: {src_dir}")
        sys.exit(1)

    glbs = sorted(f for f in os.listdir(src_dir) if f.lower().endswith(".glb"))
    if not glbs:
        print(f"[ERR] No .glb files in {src_dir}")
        sys.exit(1)

    if args.inplace:
        os.makedirs(BACKUP_DIR, exist_ok=True)
        out_dir = src_dir
    else:
        out_dir = os.path.abspath(args.dst)
        os.makedirs(out_dir, exist_ok=True)

    print(f"[INFO] Converting {len(glbs)} GLB(s)")
    print(f"       src: {src_dir}")
    print(f"       dst: {out_dir}{' (inplace)' if args.inplace else ''}")

    failed = []
    total_imgs = 0
    for name in glbs:
        src_path = os.path.join(src_dir, name)
        try:
            if args.inplace:
                shutil.copy2(src_path, os.path.join(BACKUP_DIR, name))
                tmp_path = os.path.join(out_dir, name + ".tmp")
                n = convert_glb(src_path, tmp_path)
                os.replace(tmp_path, src_path)
            else:
                dst_path = os.path.join(out_dir, name)
                n = convert_glb(src_path, dst_path)
            total_imgs += n
            print(f"  OK  {name} ({n} image{'s' if n != 1 else ''})")
        except Exception as e:
            print(f"  FAIL {name}: {e}")
            failed.append(name)

    print(f"[DONE] {len(glbs) - len(failed)}/{len(glbs)} GLBs, "
          f"{total_imgs} images re-encoded")
    if failed:
        print("[FAILED] " + ", ".join(failed))
        sys.exit(2)


if __name__ == "__main__":
    main()
