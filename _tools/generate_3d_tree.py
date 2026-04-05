"""
Generate a 3D tree model from a concept image using TRELLIS.2-4B.
Standalone script with ComfyUI mocks.

Usage:
  C:/Sabri_MMO/_tools/TRELLIS/venv/Scripts/python.exe generate_3d_tree.py
"""
import os
import sys
import types
import gc

# ---- Environment setup ----
os.environ['ATTN_BACKEND'] = 'sdpa'
os.environ['SPARSE_ATTN_BACKEND'] = 'sdpa'
os.environ['SPARSE_CONV_BACKEND'] = 'flex_gemm'

# ---- Mock ComfyUI modules ----
TRELLIS_DIR = r'C:\Sabri_MMO\_tools\TRELLIS'
TRELLIS2_DIR = r'C:\ComfyUI\custom_nodes\ComfyUI-Trellis2'
MODELS_DIR = os.path.join(TRELLIS_DIR, 'models')

folder_paths = types.ModuleType('folder_paths')
folder_paths.get_folder_paths = lambda x: []
folder_paths.models_dir = MODELS_DIR
sys.modules['folder_paths'] = folder_paths

comfy = types.ModuleType('comfy')
comfy_utils = types.ModuleType('comfy.utils')
comfy_utils.ProgressBar = type('ProgressBar', (), {
    '__init__': lambda s, x: None,
    'update': lambda s, x: None,
})
comfy_utils.load_torch_file = lambda *a, **kw: {}
comfy_utils.common_upscale = lambda *a, **kw: None
comfy.utils = comfy_utils
sys.modules['comfy'] = comfy
sys.modules['comfy.utils'] = comfy_utils

import torch
comfy_mm = types.ModuleType('comfy.model_management')
comfy_mm.get_torch_device = lambda: torch.device('cuda')
sys.modules['comfy.model_management'] = comfy_mm

node_helpers = types.ModuleType('node_helpers')
sys.modules['node_helpers'] = node_helpers

# Add trellis2 to path
if TRELLIS2_DIR not in sys.path:
    sys.path.insert(0, TRELLIS2_DIR)

# ---- Monkey-patch DinoV3FeatureExtractor for transformers API ----
# The code references self.model.layer but the HF model has self.model.model.layer
from trellis2.modules import image_feature_extractor as _ife
_orig_extract = _ife.DinoV3FeatureExtractor.extract_features
def _patched_extract(self, image):
    import torch.nn.functional as F
    image = image.to(self.model.embeddings.patch_embeddings.weight.dtype)
    hidden_states = self.model.embeddings(image, bool_masked_pos=None)
    position_embeddings = self.model.rope_embeddings(image)
    # Use model.model.layer instead of model.layer
    layers = self.model.model.layer if hasattr(self.model, 'model') and hasattr(self.model.model, 'layer') else self.model.layer
    for i, layer_module in enumerate(layers):
        hidden_states = layer_module(
            hidden_states,
            position_embeddings=position_embeddings,
        )
    return F.layer_norm(hidden_states, hidden_states.shape[-1:])
_ife.DinoV3FeatureExtractor.extract_features = _patched_extract

# ---- Paths ----
INPUT_IMAGE = r'C:\Sabri_MMO\_tools\concept_art\tree_01.png'
OUTPUT_GLB = r'C:\Sabri_MMO\_tools\3d_models\tree_01.glb'
MODEL_PATH = os.path.join(MODELS_DIR, 'microsoft', 'TRELLIS.2-4B')

# ---- Simple progress bar ----
class SimplePbar:
    def __init__(self, total):
        self.total = total
        self.current = 0
    def update(self, n):
        self.current += n
        print(f"  Progress: {self.current}/{self.total}")


def main():
    print("=" * 60)
    print("TRELLIS.2-4B Image-to-3D Pipeline")
    print("=" * 60)
    print(f"Input:  {INPUT_IMAGE}")
    print(f"Output: {OUTPUT_GLB}")
    print(f"Model:  {MODEL_PATH}")
    print()

    # Verify input exists
    if not os.path.exists(INPUT_IMAGE):
        print(f"ERROR: Input image not found: {INPUT_IMAGE}")
        sys.exit(1)

    # Load PIL image
    from PIL import Image
    image = Image.open(INPUT_IMAGE)
    print(f"Loaded image: {image.size}, mode={image.mode}")

    # Load pipeline
    print("\n--- Loading TRELLIS.2 pipeline ---")
    from trellis2.pipelines import Trellis2ImageTo3DPipeline

    pipeline = Trellis2ImageTo3DPipeline.from_pretrained(
        MODEL_PATH,
        keep_models_loaded=False,  # Save VRAM by unloading after each stage
    )
    pipeline.to(torch.device('cuda'))
    pipeline.low_vram = True  # Use low_vram mode for stage-by-stage GPU loading
    print("Pipeline loaded.")

    # Remove background using standard rembg (u2net, no gated model needed)
    print("\n--- Removing background with rembg (u2net) ---")
    import rembg
    import numpy as np

    rembg_session = rembg.new_session('u2net')
    rgba_image = rembg.remove(image.convert('RGB'), session=rembg_session)
    del rembg_session

    # Crop to bounding box and resize to 518x518 (DINOv2 input size)
    output_np = np.array(rgba_image)
    alpha = output_np[:, :, 3]
    bbox = np.argwhere(alpha > 0.8 * 255)
    bbox = np.min(bbox[:, 1]), np.min(bbox[:, 0]), np.max(bbox[:, 1]), np.max(bbox[:, 0])
    center = (bbox[0] + bbox[2]) / 2, (bbox[1] + bbox[3]) / 2
    size = max(bbox[2] - bbox[0], bbox[3] - bbox[1])
    size = int(size * 1.2)
    crop_box = (center[0] - size // 2, center[1] - size // 2,
                center[0] + size // 2, center[1] + size // 2)
    cropped = rgba_image.crop(crop_box)

    # Premultiply alpha and convert to RGB
    cropped_np = np.array(cropped).astype(np.float32) / 255
    cropped_np = cropped_np[:, :, :3] * cropped_np[:, :, 3:4]
    processed_image = Image.fromarray((cropped_np * 255).astype(np.uint8))

    # Save preprocessed for debugging
    processed_image.save(r'C:\Sabri_MMO\_tools\concept_art\tree_01_preprocessed.png')
    print(f"Preprocessed: {processed_image.size}")
    del rgba_image, output_np, cropped, cropped_np

    # Run the pipeline
    print("\n--- Running TRELLIS.2 3D generation ---")
    print("  Pipeline type: 512 (faster, sufficient for game assets)")

    pbar = SimplePbar(6)
    meshes = pipeline.run(
        processed_image,
        num_samples=1,
        seed=42,
        preprocess_image=False,  # Already preprocessed
        pipeline_type='512',     # Faster, good enough for game trees
        max_num_tokens=49152,
        generate_texture_slat=True,
        use_tiled=True,
        pbar=pbar,
        sampler='euler',
    )

    print(f"\nGeneration complete! Got {len(meshes)} mesh(es)")

    if not meshes:
        print("ERROR: No meshes generated")
        sys.exit(1)

    mesh = meshes[0]
    print(f"  Vertices: {mesh.vertices.shape[0]}")
    print(f"  Faces: {mesh.faces.shape[0]}")

    # Export to GLB using o_voxel postprocessing
    print("\n--- Exporting to GLB ---")
    import o_voxel.postprocess

    os.makedirs(os.path.dirname(OUTPUT_GLB), exist_ok=True)

    glb = o_voxel.postprocess.to_glb(
        vertices=mesh.vertices,
        faces=mesh.faces,
        attr_volume=mesh.attrs,
        coords=mesh.coords,
        attr_layout=mesh.layout,
        grid_size=512,
        aabb=[[-0.5, -0.5, -0.5], [0.5, 0.5, 0.5]],
        decimation_target=50000,  # Target 50k faces for game asset
        texture_size=1024,
        remesh=True,
        remesh_band=1,
        remesh_project=0,
        use_tqdm=True,
    )

    glb.export(OUTPUT_GLB, file_type='glb')
    print(f"GLB saved: {OUTPUT_GLB}")

    file_size = os.path.getsize(OUTPUT_GLB) / 1024 / 1024
    print(f"File size: {file_size:.2f} MB")

    # Cleanup
    del meshes, mesh, pipeline
    gc.collect()
    torch.cuda.empty_cache()

    print("\n" + "=" * 60)
    print("3D GENERATION COMPLETE")
    print("=" * 60)


if __name__ == '__main__':
    main()
