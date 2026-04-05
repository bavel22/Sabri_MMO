"""
Generate a Ragnarok Online Classic style tree concept image via ComfyUI API.
Uses Illustrious-XL checkpoint + ROSprites LoRA.
"""
import json
import urllib.request
import urllib.parse
import time
import os
import sys
import random

COMFYUI_URL = "http://127.0.0.1:8188"
OUTPUT_PATH = "C:/Sabri_MMO/_tools/concept_art/tree_01.png"

# Build the workflow as a prompt dict (API format)
def build_workflow():
    client_id = f"tree_gen_{random.randint(0, 999999):06d}"

    workflow = {
        # KSampler
        "3": {
            "class_type": "KSampler",
            "inputs": {
                "seed": 12345,
                "steps": 30,
                "cfg": 7.0,
                "sampler_name": "euler",
                "scheduler": "normal",
                "denoise": 1.0,
                "model": ["10", 0],
                "positive": ["6", 0],
                "negative": ["7", 0],
                "latent_image": ["5", 0]
            }
        },
        # CheckpointLoaderSimple
        "4": {
            "class_type": "CheckpointLoaderSimple",
            "inputs": {
                "ckpt_name": "Illustrious-XL-v0.1.safetensors"
            }
        },
        # EmptyLatentImage
        "5": {
            "class_type": "EmptyLatentImage",
            "inputs": {
                "width": 1024,
                "height": 1024,
                "batch_size": 1
            }
        },
        # CLIP Text Encode (positive)
        "6": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "text": "no_humans, scenery, one single large deciduous tree centered in frame, thick brown trunk, big round puffy green canopy, lush green leaves, fantasy RPG style, white background, clean isolated object, concept art, digital painting, anime coloring, warm lighting, simple shading, game asset",
                "clip": ["4", 1]
            }
        },
        # CLIP Text Encode (negative)
        "7": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "text": "photorealistic, 3d_render, realistic, person, character, face, text, watermark, blurry, low quality, multiple trees, forest, complex background, dark, nsfw, signature, logo, top_down, overhead view, aerial view, stump, cross section, wood grain, cut log, pixel art, pixelated, sprite sheet, multiple views, tiled, grid",
                "clip": ["4", 1]
            }
        },
        # VAE Decode
        "8": {
            "class_type": "VAEDecode",
            "inputs": {
                "samples": ["3", 0],
                "vae": ["4", 2]
            }
        },
        # SaveImage
        "9": {
            "class_type": "SaveImage",
            "inputs": {
                "filename_prefix": "tree_concept",
                "images": ["8", 0]
            }
        },
        # LoraLoader
        "10": {
            "class_type": "LoraLoader",
            "inputs": {
                "lora_name": "ROSprites-v2.1C.safetensors",
                "strength_model": 0.25,
                "strength_clip": 0.25,
                "model": ["4", 0],
                "clip": ["4", 1]
            }
        }
    }

    return workflow, client_id


def queue_prompt(workflow, client_id):
    """Queue a prompt via the ComfyUI API."""
    payload = json.dumps({
        "prompt": workflow,
        "client_id": client_id
    }).encode("utf-8")

    req = urllib.request.Request(
        f"{COMFYUI_URL}/prompt",
        data=payload,
        headers={"Content-Type": "application/json"}
    )
    resp = urllib.request.urlopen(req)
    return json.loads(resp.read())


def get_history(prompt_id):
    """Get the execution history for a prompt."""
    url = f"{COMFYUI_URL}/history/{prompt_id}"
    try:
        resp = urllib.request.urlopen(url)
        return json.loads(resp.read())
    except Exception:
        return {}


def download_image(filename, subfolder, output_path):
    """Download a generated image from ComfyUI."""
    params = urllib.parse.urlencode({
        "filename": filename,
        "subfolder": subfolder,
        "type": "output"
    })
    url = f"{COMFYUI_URL}/view?{params}"
    resp = urllib.request.urlopen(url)

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(resp.read())
    print(f"Image saved to: {output_path}")


def main():
    print("=== Generating RO-style tree concept art via ComfyUI ===")

    # Build and queue the workflow
    workflow, client_id = build_workflow()
    print(f"Queueing prompt with client_id: {client_id}")

    result = queue_prompt(workflow, client_id)
    prompt_id = result.get("prompt_id")
    if not prompt_id:
        print(f"ERROR: Failed to queue prompt: {result}")
        sys.exit(1)

    print(f"Prompt queued: {prompt_id}")
    print("Waiting for generation...")

    # Poll for completion
    max_wait = 300  # 5 minutes max
    start = time.time()
    while time.time() - start < max_wait:
        history = get_history(prompt_id)
        if prompt_id in history:
            outputs = history[prompt_id].get("outputs", {})
            # Node "9" is our SaveImage node
            if "9" in outputs:
                images = outputs["9"].get("images", [])
                if images:
                    img_info = images[0]
                    print(f"Generation complete! File: {img_info['filename']}")
                    download_image(
                        img_info["filename"],
                        img_info.get("subfolder", ""),
                        OUTPUT_PATH
                    )
                    return
            # Check for errors
            status = history[prompt_id].get("status", {})
            if status.get("status_str") == "error":
                print(f"ERROR: Generation failed: {status}")
                sys.exit(1)
            break
        time.sleep(2)
    else:
        print("ERROR: Timed out waiting for generation")
        sys.exit(1)

    # If we broke out of loop (history found but no images yet), keep polling
    while time.time() - start < max_wait:
        history = get_history(prompt_id)
        if prompt_id in history:
            outputs = history[prompt_id].get("outputs", {})
            if "9" in outputs:
                images = outputs["9"].get("images", [])
                if images:
                    img_info = images[0]
                    print(f"Generation complete! File: {img_info['filename']}")
                    download_image(
                        img_info["filename"],
                        img_info.get("subfolder", ""),
                        OUTPUT_PATH
                    )
                    return
        time.sleep(2)

    print("ERROR: Timed out waiting for image output")
    sys.exit(1)


if __name__ == "__main__":
    main()
