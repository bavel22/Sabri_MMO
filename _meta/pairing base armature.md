import base t pose fbx from mixamo into blender

import CLASS t pose fbx from mixamo into blender  (only need to download t-pose fbx after rigging)

If the class mesh has an Armature modifier: Select mesh → Properties → Modifiers (wrench icon) → delete the
Armature modifier
      - b. If the class mesh is parented to another armature: Select mesh → Alt+P → Clear Parent, Keep Transform
      - c. Apply all transforms: Select mesh → Ctrl+A → All Transforms
      - 
reparent the CLASS mesh to the base_f armature (relation - > parent -> armature, and parent type - > object)



position/scale the CLASS mesh overlapping the base_f mesh

Select class mesh → Shift+Select armature → Ctrl+P → Armature Deform → With Automatic Weights

position/scale the CLASS mesh overlapping the base_f mesh AGAIN

select class mesh
Apply all transforms: Select mesh → Ctrl+A → All Transforms

4. Delete the base_f body mesh (keep the armature):
      - Select the base_f mesh → X → Delete
      - The shared armature remains in the scene
    7. Delete the class armature (if imported from Mixamo FBX):
      - Select the class's armature → X → Delete

    7. Verify: Enter Pose Mode, rotate some bones — class mesh should deform correctly.
    8. Save as:
    3d_models/characters/{class}_{gender}/{class}_{gender}_shared.blend

    Key reminder: Use the Tripo3D GLB for --texture-from at render time (has the actual textures), since the Mixamo FBX
    strips textures.


if there is wierd stretching after parenting:

  1. Select the mesh → go to Edit Mode
  2. Open Properties → Object Data (green triangle) → Vertex Groups
  3. Click the down arrow next to the vertex group list → Delete All Groups
  4. Go back to Object Mode
  5. Re-parent: select mesh, Shift+select base_f armature → Ctrl+P → Armature Deform with Automatic Weights