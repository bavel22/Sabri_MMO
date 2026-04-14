"""
render_monster.py - Non-humanoid Monster Sprite Renderer for Sabri_MMO
======================================================================
Renders blob/amorphous monsters (Poring, Drops, etc.) using shape key
animations instead of skeletal (Mixamo) animations. Uses the same
cel-shading, camera, and lighting as blender_sprite_render_v2.py.

Usage:
  "C:/Blender 5.1/blender.exe" --background --python \
    "C:/Sabri_MMO/2D animations/scripts/render_monster.py" -- \
    "C:/Sabri_MMO/2D animations/3d_models/characters/poring/poring.glb" \
    "C:/Sabri_MMO/2D animations/sprites/render_output/poring" \
    --monster-type blob \
    --render-size 1024 --camera-angle 10 --camera-target-z 0.7

Output matches the folder structure expected by pack_atlas.py (v2):
  output_dir/
    Idle Bounce/
      Idle Bounce_S_f00.png ... Idle Bounce_SE_f07.png
    Hop Forward/
      ...
    Lunge Attack/
      ...
    Hit Squish/
      ...
    Flatten Death/
      ...
"""

import bpy
import math
import os
import sys
import argparse
import mathutils


# =============================================================
# Constants
# =============================================================

DIRECTION_NAMES_8 = ["S", "SW", "W", "NW", "N", "NE", "E", "SE"]
DIRECTION_NAMES_4 = ["S", "W", "N", "E"]

DEFAULT_FRAME_TARGETS = {
    "idle": 8, "walk": 12, "attack": 10, "hit": 6, "death": 8,
}


# =============================================================
# Shape Key Presets
# =============================================================

BLOB_SHAPE_KEYS = {
    # Deformation keys: scale from ground plane (z_min anchor)
    'Squash':  {'z_scale': 0.6,  'xy_scale': 1.25},
    'Stretch': {'z_scale': 1.35, 'xy_scale': 0.85},
    'Flatten': {'z_scale': 0.15, 'xy_scale': 2.0},
    # Translation key: uniform vertical offset (hop)
    'HopUp':   {'z_offset': 0.25},  # max hop = 25% of body height
}

BLOB_ANIMATIONS = {
    'Idle Bounce': {
        'total_frames': 56,
        'classified_type': 'idle',
        'keyframes': [
            # (frame, {shape_key: value, ...})
            # Unspecified keys default to 0.0
            (0,  {}),
            (14, {'Squash': 0.12}),
            (28, {'Stretch': 0.1, 'HopUp': 0.15}),
            (42, {'Squash': 0.05}),
            (56, {}),  # loops back to start
        ],
    },
    'Hop Forward': {
        'total_frames': 36,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.3}),                       # crouch
            (6,  {'Stretch': 0.3, 'HopUp': 0.5}),       # launch
            (14, {'Stretch': 0.15, 'HopUp': 1.0}),      # peak of hop
            (22, {'Stretch': 0.1, 'HopUp': 0.5}),       # descending
            (28, {'Squash': 0.3}),                       # landing
            (33, {'Squash': 0.1}),                       # recover
            (36, {}),                                    # rest (loop)
        ],
    },
    'Lunge Attack': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                    # rest
            (5,  {'Squash': 0.35}),                      # wind up
            (10, {'Stretch': 0.4, 'HopUp': 0.4}),       # lunge upward
            (15, {'Stretch': 0.3, 'HopUp': 0.2}),       # extending
            (20, {'Squash': 0.15}),                      # recoil
            (25, {'Squash': 0.05}),                      # settle
            (30, {}),                                    # rest
        ],
    },
    'Hit Squish': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.4}),                       # impact
            (5,  {'Squash': 0.5}),                       # max compression
            (10, {'Stretch': 0.2, 'HopUp': 0.2}),       # rebound
            (14, {'Stretch': 0.05}),                     # settling
            (18, {}),                                    # rest
        ],
    },
    'Flatten Death': {
        'total_frames': 24,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                    # normal
            (4,  {'Squash': 0.3}),                       # initial squash
            (8,  {'Stretch': 0.2, 'HopUp': 0.25}),      # bounce up
            (12, {'Flatten': 0.3}),                      # start flattening
            (18, {'Flatten': 0.7}),                      # mostly flat
            (24, {'Flatten': 1.0}),                      # fully flat
        ],
    },
}

# --- Caterpillar/Worm preset (Fabre, etc.) ---
CATERPILLAR_SHAPE_KEYS = {
    'Squash':  {'z_scale': 0.5,  'xy_scale': 1.3},       # Deeper curl
    'Stretch': {'z_scale': 1.4,  'xy_scale': 0.82},      # Bigger bite extension
    'Flatten': {'z_scale': 0.2,  'xy_scale': 1.8},       # Unused in death — curls instead
    'HopUp':   {'z_offset': 0.25},                        # Higher rear-up
}

CATERPILLAR_ANIMATIONS = {
    'Idle Wiggle': {
        'total_frames': 48,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),
            (12, {'Squash': 0.1}),
            (24, {'Stretch': 0.08, 'HopUp': 0.05}),
            (36, {'Squash': 0.06}),
            (48, {}),
        ],
    },
    'Crawl Forward': {
        'total_frames': 36,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.35}),                        # compressed tight
            (4,  {'Squash': 0.4}),                         # max scrunch
            (8,  {'Stretch': 0.3, 'HopUp': 0.4}),         # push forward + lift
            (14, {'Stretch': 0.4, 'HopUp': 0.6}),         # full extension, peak height
            (18, {'Stretch': 0.2, 'HopUp': 0.3}),         # descending
            (22, {'Squash': 0.3}),                         # land + compress
            (28, {'Squash': 0.35}),                        # re-scrunch
            (32, {'Squash': 0.2}),                         # easing
            (36, {'Squash': 0.35}),                        # loop
        ],
    },
    'Bite Attack': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                      # rest
            (3,  {'Squash': 0.35}),                        # coil body tight
            (6,  {'Squash': 0.45}),                        # deep wind-up crouch
            (9,  {'Stretch': 0.6, 'HopUp': 0.6}),         # REAR UP — big and obvious
            (11, {'Stretch': 0.65, 'HopUp': 0.65}),       # peak rear-up (hold)
            (14, {'Squash': 0.5}),                         # SLAM DOWN — fast strike
            (16, {'Squash': 0.55}),                        # max impact compression
            (20, {'Squash': 0.3}),                         # bounce
            (24, {'Squash': 0.1}),                         # recovering
            (30, {}),                                      # rest
        ],
    },
    'Hit Recoil': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.35}),
            (4,  {'Squash': 0.45}),
            (9,  {'Stretch': 0.2, 'HopUp': 0.2}),
            (13, {'Stretch': 0.05}),
            (18, {}),
        ],
    },
    'Curl Death': {
        'total_frames': 28,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # alive
            (3,  {'Squash': 0.3}),                         # initial hit
            (6,  {'Stretch': 0.25, 'HopUp': 0.3}),        # death spasm — rears up
            (9,  {'Squash': 0.45}),                        # snaps back down
            (12, {'Stretch': 0.1, 'HopUp': 0.15}),        # smaller spasm
            (16, {'Squash': 0.6}),                         # curling up
            (20, {'Squash': 0.8}),                         # tightly curled
            (24, {'Squash': 0.9}),                         # almost fully curled
            (28, {'Squash': 1.0}),                         # dead — curled into ball (NOT flat)
        ],
    },
}

# --- Rabbit/Critter preset (Lunatic, etc.) ---
RABBIT_SHAPE_KEYS = {
    'Squash':   {'z_scale': 0.5,  'xy_scale': 1.3},       # Deeper crouch
    'Stretch':  {'z_scale': 1.4,  'xy_scale': 0.8},       # Bigger stretch
    'HopUp':    {'z_offset': 0.45},                        # Higher hop
    'TiltOver': {'tilt': True, 'amount': 0.9},             # Strong diagonal fall
}

RABBIT_ANIMATIONS = {
    'Idle Twitch': {
        'total_frames': 56,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),
            (14, {'Squash': 0.06}),
            (28, {'Stretch': 0.06, 'HopUp': 0.12}),
            (42, {'Squash': 0.04}),
            (56, {}),
        ],
    },
    'Bunny Hop': {
        'total_frames': 30,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.5}),                         # deep crouch
            (4,  {'Stretch': 0.35, 'HopUp': 0.7}),        # explosive launch
            (9,  {'Stretch': 0.25, 'HopUp': 1.0}),        # peak — fully airborne
            (14, {'Stretch': 0.1, 'HopUp': 0.6}),         # descending
            (18, {'Squash': 0.45}),                        # heavy landing squash
            (22, {'Squash': 0.3, 'HopUp': 0.15}),         # small bounce
            (26, {'Squash': 0.2}),                         # settling
            (30, {'Squash': 0.5}),                         # loop (back to crouch)
        ],
    },
    'Headbutt Attack': {
        'total_frames': 24,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                      # rest
            (2,  {'Squash': 0.4}),                         # fast crouch
            (4,  {'Squash': 0.5}),                         # deep wind-up
            (7,  {'Stretch': 0.6, 'HopUp': 0.5}),         # LAUNCH forward — explosive
            (9,  {'Stretch': 0.5, 'HopUp': 0.3}),         # impact moment
            (12, {'Squash': 0.35}),                        # hard recoil
            (16, {'Squash': 0.15}),                        # recovering
            (20, {'Squash': 0.05}),                        # settling
            (24, {}),                                      # rest
        ],
    },
    'Hit Flinch': {
        'total_frames': 16,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.4}),
            (4,  {'Squash': 0.45, 'HopUp': 0.2}),
            (8,  {'Stretch': 0.2, 'HopUp': 0.3}),
            (12, {'Squash': 0.1}),
            (16, {}),
        ],
    },
    'Topple Death': {
        'total_frames': 28,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # alive
            (2,  {'Squash': 0.3}),                         # hit
            (5,  {'Stretch': 0.3, 'HopUp': 0.85}),        # LAUNCHED airborne
            (8,  {'Squash': 0.25, 'HopUp': 0.6}),         # tumbling
            (11, {'Stretch': 0.15, 'HopUp': 0.3}),        # coming down
            (14, {'Squash': 0.4}),                         # CRASH landing
            (17, {'Squash': 0.25, 'HopUp': 0.08}),        # small bounce
            (20, {'Squash': 0.2, 'TiltOver': 0.3}),       # tipping on side
            (24, {'Squash': 0.25, 'TiltOver': 0.7}),      # falling over
            (28, {'Squash': 0.3, 'TiltOver': 1.0}),       # on side — dead
        ],
    },
}

# --- Egg/Cocoon preset (Pupa, Thief Bug Egg, etc.) ---
EGG_SHAPE_KEYS = {
    'Squash':  {'z_scale': 0.8,  'xy_scale': 1.12},      # Compression
    'Stretch': {'z_scale': 1.15, 'xy_scale': 0.92},      # Elongation
    'Flatten': {'z_scale': 0.15, 'xy_scale': 2.0},       # Cracked open
    'HopUp':   {'z_offset': 0.08},                        # Wobble lift
}

EGG_ANIMATIONS = {
    'Idle Wobble': {
        'total_frames': 64,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),
            (16, {'Squash': 0.06}),
            (32, {'Stretch': 0.05, 'HopUp': 0.4}),
            (48, {'Squash': 0.03}),
            (64, {}),
        ],
    },
    'Hit Jiggle': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.35}),                        # hard impact
            (3,  {'Stretch': 0.2, 'HopUp': 0.6}),         # big jiggle up
            (6,  {'Squash': 0.25}),                        # bounce down
            (9,  {'Stretch': 0.12, 'HopUp': 0.35}),       # smaller jiggle
            (12, {'Squash': 0.1}),                         # bounce
            (15, {'Stretch': 0.04}),                       # settling
            (18, {}),
        ],
    },
    'Crack Death': {
        'total_frames': 28,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # normal
            (2,  {'Squash': 0.25}),                        # hit
            (4,  {'Stretch': 0.15, 'HopUp': 0.5}),        # violent wobble up
            (6,  {'Squash': 0.3}),                         # wobble down
            (8,  {'Stretch': 0.1, 'HopUp': 0.4}),         # wobble up
            (10, {'Squash': 0.35}),                        # wobble down — cracking
            (13, {'Stretch': 0.08, 'HopUp': 0.2}),        # weaker wobble
            (16, {'Squash': 0.25}),                        # structure failing
            (20, {'Flatten': 0.4}),                        # shell breaking open
            (24, {'Flatten': 0.7}),                        # collapsing
            (28, {'Flatten': 1.0}),                        # cracked open flat
        ],
    },
}

# --- Frog preset (Roda Frog, etc.) ---
FROG_SHAPE_KEYS = {
    'Squash':   {'z_scale': 0.4,  'xy_scale': 1.4},       # Very deep frog crouch
    'Stretch':  {'z_scale': 1.35, 'xy_scale': 0.8},       # Bigger tongue snap
    'HopUp':    {'z_offset': 0.4},                         # Higher leap
    'TiltOver': {'tilt': True, 'amount': 0.85},            # Strong belly-up roll
}

FROG_ANIMATIONS = {
    'Idle Croak': {
        'total_frames': 56,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),
            (10, {'Squash': 0.12}),                        # throat in
            (18, {'Stretch': 0.15, 'HopUp': 0.08}),       # throat PUFFS out
            (24, {'Stretch': 0.12}),                       # hold puff
            (30, {'Squash': 0.08}),                        # deflate
            (42, {}),
            (56, {}),
        ],
    },
    'Frog Leap': {
        'total_frames': 32,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.55}),                        # very deep crouch
            (3,  {'Squash': 0.6}),                         # max crouch (loading)
            (6,  {'Stretch': 0.4, 'HopUp': 0.8}),         # EXPLOSIVE launch
            (11, {'Stretch': 0.25, 'HopUp': 1.0}),        # peak — full airborne
            (16, {'Stretch': 0.1, 'HopUp': 0.6}),         # descending
            (20, {'Squash': 0.5}),                         # SPLAT landing — big squash
            (24, {'Squash': 0.35, 'HopUp': 0.1}),         # small bounce
            (28, {'Squash': 0.25}),                        # settling
            (32, {'Squash': 0.55}),                        # loop (back to crouch)
        ],
    },
    'Tongue Snap': {
        'total_frames': 24,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                      # rest
            (2,  {'Squash': 0.35}),                        # fast crouch
            (4,  {'Squash': 0.45}),                        # deep wind-up
            (6,  {'Stretch': 0.65, 'HopUp': 0.3}),        # WHIP forward — explosive
            (8,  {'Stretch': 0.6, 'HopUp': 0.15}),        # fully extended (tongue out)
            (11, {'Squash': 0.35}),                        # SNAP back — fast recoil
            (14, {'Squash': 0.2}),                         # recovering
            (18, {'Squash': 0.08}),                        # settling
            (24, {}),                                      # rest
        ],
    },
    'Hit Flinch': {
        'total_frames': 16,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.4}),
            (4,  {'Squash': 0.45, 'HopUp': 0.25}),
            (8,  {'Stretch': 0.2, 'HopUp': 0.35}),
            (12, {'Squash': 0.12}),
            (16, {}),
        ],
    },
    'Belly Up Death': {
        'total_frames': 32,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # alive
            (3,  {'Squash': 0.2}),                         # hit
            (6,  {'Stretch': 0.5}),                        # BLOATING up — frog puffs
            (9,  {'Stretch': 0.65}),                       # MAX BLOAT — big and round
            (12, {'Stretch': 0.6}),                        # holding bloat
            (15, {'Stretch': 0.5}),                        # slightly deflating
            (18, {'Stretch': 0.4, 'TiltOver': 0.2}),      # starting to tip
            (22, {'Stretch': 0.35, 'TiltOver': 0.5}),     # rolling belly-up
            (26, {'Stretch': 0.3, 'TiltOver': 0.8}),      # on back — bloated
            (32, {'Stretch': 0.25, 'TiltOver': 1.0}),     # belly up — puffed dead frog
        ],
    },
}

# --- Tree/Plant preset (Willow, Elder Willow, etc.) ---
TREE_SHAPE_KEYS = {
    'Squash':   {'z_scale': 0.65, 'xy_scale': 1.18},      # Bigger trunk slam
    'Stretch':  {'z_scale': 1.25, 'xy_scale': 0.88},      # Taller reach
    'HopUp':    {'z_offset': 0.18},                        # Bigger root lift
    'TiltOver': {'tilt': True, 'amount': 1.0},             # Full timber fall
}

TREE_ANIMATIONS = {
    'Idle Sway': {
        'total_frames': 64,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),
            (12, {'Stretch': 0.1}),                        # sway up
            (24, {'Squash': 0.08}),                        # sway down
            (36, {'Stretch': 0.06, 'HopUp': 0.1}),        # sway up + slight root lift
            (48, {'Squash': 0.05}),                        # sway down
            (64, {}),
        ],
    },
    'Root Waddle': {
        'total_frames': 32,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.15}),                        # lean down
            (3,  {'Squash': 0.2}),                         # deeper lean — loading
            (6,  {'HopUp': 0.75, 'Stretch': 0.15}),       # BIG root lift — uproot right
            (9,  {'HopUp': 0.5, 'Stretch': 0.1}),         # coming down
            (12, {'Squash': 0.2}),                         # PLANT — thud
            (15, {'Squash': 0.12}),                        # wobble settle
            (18, {'Squash': 0.2}),                         # lean again — loading
            (21, {'HopUp': 0.75, 'Stretch': 0.15}),       # BIG root lift — uproot left
            (24, {'HopUp': 0.5, 'Stretch': 0.1}),         # coming down
            (27, {'Squash': 0.2}),                         # PLANT — thud
            (30, {'Squash': 0.12}),                        # wobble
            (32, {'Squash': 0.15}),                        # loop
        ],
    },
    'Branch Swipe': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                      # standing
            (3,  {'Stretch': 0.2}),                        # reaching UP
            (6,  {'Stretch': 0.35}),                       # TALL wind-up — branches raised
            (8,  {'Stretch': 0.4}),                        # maximum height (hold)
            (10, {'Squash': 0.4, 'HopUp': 0.4}),          # SLAM DOWN — fast + roots lift
            (12, {'Squash': 0.45}),                        # max impact — ground shakes
            (15, {'Squash': 0.3}),                         # rebound
            (18, {'Stretch': 0.15}),                       # recovering upright
            (22, {'Stretch': 0.06}),                       # settling
            (26, {}),                                      # settling
            (30, {}),                                      # rest
        ],
    },
    'Trunk Shudder': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.2}),                         # impact
            (3,  {'Stretch': 0.15}),                       # shudder up
            (6,  {'Squash': 0.18}),                        # shudder down
            (9,  {'Stretch': 0.1}),                        # shudder up
            (12, {'Squash': 0.06}),                        # settling
            (18, {}),
        ],
    },
    'Timber Death': {
        'total_frames': 36,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # standing
            (4,  {'Stretch': 0.12}),                       # stiffening — creak
            (8,  {'Stretch': 0.08, 'TiltOver': 0.1}),     # starting to lean
            (12, {'TiltOver': 0.25, 'Squash': 0.05}),     # tipping point — creaking
            (16, {'TiltOver': 0.4, 'Squash': 0.1}),       # falling — gaining speed
            (20, {'TiltOver': 0.6, 'Squash': 0.15}),      # falling faster
            (24, {'TiltOver': 0.8, 'Squash': 0.2}),       # almost down
            (28, {'TiltOver': 0.95, 'Squash': 0.25}),     # TIMBER! impact
            (32, {'TiltOver': 1.0, 'Squash': 0.2}),       # bounce settle
            (36, {'TiltOver': 1.0, 'Squash': 0.2}),       # fallen tree — dead
        ],
    },
}

# --- Bird preset (Condor, Peco Peco, etc.) ---
BIRD_SHAPE_KEYS = {
    'Squash':   {'z_scale': 0.6,  'xy_scale': 1.25},      # Bigger peck range
    'Stretch':  {'z_scale': 1.3,  'xy_scale': 0.82},      # Taller rear-up
    'HopUp':    {'z_offset': 0.3},                         # Higher strut step
    'TiltOver': {'tilt': True, 'amount': 0.9},             # Strong keel-over
}

BIRD_ANIMATIONS = {
    'Idle Peck': {
        'total_frames': 48,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),
            (8,  {'Squash': 0.15}),                        # head dip — pecking ground
            (12, {'Stretch': 0.08}),                       # head snap up
            (20, {}),                                      # pause — looking around
            (28, {'Squash': 0.12}),                        # another peck
            (32, {'Stretch': 0.06}),                       # head up
            (40, {'Squash': 0.05}),                        # slight bob
            (48, {}),
        ],
    },
    'Strut Walk': {
        'total_frames': 28,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.15}),                        # head down
            (2,  {'Squash': 0.2}),                         # loading step
            (5,  {'Stretch': 0.15, 'HopUp': 0.6}),        # BIG step up — head bobs high
            (8,  {'Squash': 0.2}),                         # head dips on plant
            (10, {'Squash': 0.12}),                        # settle
            (13, {'Squash': 0.18}),                        # loading other step
            (16, {'Stretch': 0.15, 'HopUp': 0.6}),        # BIG step — head bobs
            (19, {'Squash': 0.2}),                         # head dips on plant
            (22, {'Squash': 0.12}),                        # settle
            (25, {'Squash': 0.08}),                        # transition
            (28, {'Squash': 0.15}),                        # loop
        ],
    },
    'Peck Attack': {
        'total_frames': 26,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                      # rest
            (3,  {'Stretch': 0.25}),                       # rearing UP
            (5,  {'Stretch': 0.4, 'HopUp': 0.2}),         # TALL wind-up — head high
            (7,  {'Stretch': 0.42}),                       # hold at peak (menacing)
            (9,  {'Squash': 0.45}),                        # SLAM peck DOWN — fast strike
            (11, {'Squash': 0.5}),                         # max impact — beak in target
            (14, {'Squash': 0.35}),                        # pulling back
            (17, {'Stretch': 0.15, 'HopUp': 0.1}),        # recovering upright
            (21, {'Stretch': 0.06}),                       # settling
            (26, {}),                                      # rest
        ],
    },
    'Wing Flinch': {
        'total_frames': 16,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.25}),
            (3,  {'Stretch': 0.25, 'HopUp': 0.4}),        # wing flap — big recoil up
            (6,  {'Squash': 0.2}),
            (10, {'Stretch': 0.08}),
            (16, {}),
        ],
    },
    'Keel Over Death': {
        'total_frames': 32,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # standing
            (2,  {'Stretch': 0.2, 'HopUp': 0.3}),         # frantic wing FLAP up
            (4,  {'Squash': 0.2}),                         # down
            (6,  {'Stretch': 0.15, 'HopUp': 0.2}),        # another FLAP — weaker
            (8,  {'Squash': 0.25}),                        # down — failing
            (10, {'Stretch': 0.08, 'HopUp': 0.1}),        # last weak flap
            (12, {'Squash': 0.3}),                         # buckle — wings give out
            (16, {'Squash': 0.25, 'TiltOver': 0.2}),      # starting to tip
            (20, {'Squash': 0.2,  'TiltOver': 0.5}),      # falling sideways
            (24, {'Squash': 0.25, 'TiltOver': 0.8}),      # almost down
            (28, {'Squash': 0.2,  'TiltOver': 0.95}),     # on side
            (32, {'Squash': 0.2,  'TiltOver': 1.0}),      # keeled over — dead
        ],
    },
}

# --- Flying Insect preset (Hornet, Chonchon, etc.) ---
FLYING_INSECT_SHAPE_KEYS = {
    'Squash':    {'z_scale': 0.7,  'xy_scale': 1.2},
    'Stretch':   {'z_scale': 1.25, 'xy_scale': 0.83},
    'HopUp':     {'z_offset': 0.5},                        # Higher hover
    'TiltOver':  {'tilt': True, 'amount': 0.9},            # Strong crash-on-side
    'WingUp':    {'wing': True, 'z_offset': 0.18, 'xy_scale': 0.8},
    'WingDown':  {'wing': True, 'z_offset': -0.12, 'xy_scale': 1.2},
}

FLYING_INSECT_ANIMATIONS = {
    'Hover Buzz': {
        'total_frames': 24,
        'classified_type': 'idle',
        'keyframes': [
            # Body hover bob + rapid wing beats every 3 frames
            (0,  {'HopUp': 0.6,  'Stretch': 0.08, 'WingDown': 0.8}),
            (3,  {'HopUp': 0.42, 'Squash': 0.1,  'WingUp': 0.85}),
            (6,  {'HopUp': 0.68, 'Stretch': 0.08, 'WingDown': 0.85}),
            (9,  {'HopUp': 0.45, 'Squash': 0.08, 'WingUp': 0.8}),
            (12, {'HopUp': 0.65, 'Stretch': 0.06, 'WingDown': 0.8}),
            (15, {'HopUp': 0.4,  'Squash': 0.1,  'WingUp': 0.85}),
            (18, {'HopUp': 0.62, 'Stretch': 0.07, 'WingDown': 0.85}),
            (21, {'HopUp': 0.44, 'Squash': 0.08, 'WingUp': 0.8}),
            (24, {'HopUp': 0.6,  'Stretch': 0.08, 'WingDown': 0.8}),
        ],
    },
    'Dart Forward': {
        'total_frames': 28,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'HopUp': 0.5,  'Squash': 0.12, 'WingDown': 0.8}),
            (2,  {'HopUp': 0.65, 'Stretch': 0.15, 'WingUp': 0.9}),
            (5,  {'HopUp': 0.75, 'Stretch': 0.2,  'WingDown': 0.85}),
            (7,  {'HopUp': 0.55, 'Squash': 0.08, 'WingUp': 0.85}),
            (10, {'HopUp': 0.7,  'Stretch': 0.15, 'WingDown': 0.8}),
            (12, {'HopUp': 0.48, 'Squash': 0.12, 'WingUp': 0.9}),
            (15, {'HopUp': 0.7,  'Stretch': 0.12, 'WingDown': 0.85}),
            (17, {'HopUp': 0.45, 'Squash': 0.1,  'WingUp': 0.85}),
            (20, {'HopUp': 0.65, 'Stretch': 0.08, 'WingDown': 0.8}),
            (23, {'HopUp': 0.48, 'Squash': 0.1,  'WingUp': 0.85}),
            (26, {'HopUp': 0.55, 'Stretch': 0.06, 'WingDown': 0.8}),
            (28, {'HopUp': 0.5,  'Squash': 0.12, 'WingDown': 0.8}),
        ],
    },
    'Sting Attack': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {'HopUp': 0.6,  'WingDown': 0.7}),
            (2,  {'HopUp': 0.8,  'Stretch': 0.2,  'WingUp': 0.9}),
            (4,  {'HopUp': 1.0,  'Stretch': 0.3,  'WingDown': 0.9}),    # MAX HEIGHT
            (6,  {'HopUp': 0.95, 'Stretch': 0.25, 'WingUp': 0.85}),     # hold peak
            (8,  {'HopUp': 0.35, 'Squash': 0.25, 'WingDown': 0.5}),     # DIVE — wings tuck
            (10, {'HopUp': 0.15, 'Squash': 0.35}),                       # IMPACT
            (12, {'HopUp': 0.12, 'Squash': 0.3}),                        # hold sting
            (15, {'HopUp': 0.35, 'Stretch': 0.15, 'WingUp': 0.7}),      # recovering
            (18, {'HopUp': 0.5,  'Stretch': 0.1,  'WingDown': 0.8}),
            (21, {'HopUp': 0.55, 'WingUp': 0.8}),
            (24, {'HopUp': 0.58, 'WingDown': 0.75}),
            (27, {'HopUp': 0.6,  'WingUp': 0.8}),
            (30, {'HopUp': 0.6,  'WingDown': 0.7}),
        ],
    },
    'Swat Flinch': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'HopUp': 0.25, 'Squash': 0.25}),                       # SWATTED
            (3,  {'HopUp': 0.15, 'Squash': 0.3}),                        # lowest
            (6,  {'HopUp': 0.45, 'Stretch': 0.15, 'WingUp': 0.9}),      # frantic flap
            (9,  {'HopUp': 0.5,  'WingDown': 0.8}),
            (12, {'HopUp': 0.55, 'Stretch': 0.05, 'WingUp': 0.8}),
            (15, {'HopUp': 0.58, 'WingDown': 0.7}),
            (18, {'HopUp': 0.6,  'WingUp': 0.75}),
        ],
    },
    'Swatted Death': {
        'total_frames': 32,
        'classified_type': 'death',
        'keyframes': [
            (0,  {'HopUp': 0.6,  'WingDown': 0.7}),                     # flying
            (2,  {'HopUp': 0.5,  'Squash': 0.15, 'WingUp': 0.6}),      # hit
            (4,  {'HopUp': 0.55, 'WingDown': 0.5}),                     # erratic
            (6,  {'HopUp': 0.38, 'WingUp': 0.4}),                       # losing it
            (8,  {'HopUp': 0.42, 'WingDown': 0.3}),                     # wings failing
            (10, {'HopUp': 0.28, 'Squash': 0.15}),                      # dropping — wings stop
            (12, {'HopUp': 0.2,  'Squash': 0.2}),                       # spiraling
            (14, {'HopUp': 0.1,  'Squash': 0.2}),                       # almost down
            (16, {'HopUp': 0.02, 'Squash': 0.25}),                      # CRASH
            (18, {'Squash': 0.2}),                                        # on ground
            (20, {'Squash': 0.15, 'TiltOver': 0.3}),                    # tipping over
            (24, {'Squash': 0.2,  'TiltOver': 0.7}),                    # falling on side
            (28, {'Squash': 0.25, 'TiltOver': 0.9}),                    # on side
            (32, {'Squash': 0.2,  'TiltOver': 1.0}),                    # dead on side
        ],
    },
}

# --- Bat preset (Familiar, etc.) ---
# Bats fly constantly like insects but with SLOW, LARGE wing beats.
# Swooping flight, dive attacks, plummet death.
BAT_SHAPE_KEYS = {
    'Squash':    {'z_scale': 0.6,  'xy_scale': 1.28},
    'Stretch':   {'z_scale': 1.35, 'xy_scale': 0.8},
    'HopUp':     {'z_offset': 0.45},
    'TiltOver':  {'tilt': True, 'amount': 0.9},            # Strong crash-on-side
    'WingUp':    {'wing': True, 'z_offset': 0.22, 'xy_scale': 0.78},  # Bigger flaps
    'WingDown':  {'wing': True, 'z_offset': -0.15, 'xy_scale': 1.22},
}

BAT_ANIMATIONS = {
    'Wing Beat Hover': {
        'total_frames': 32,
        'classified_type': 'idle',
        'keyframes': [
            # Slow dramatic wing beats (every 5 frames) + body bob
            (0,  {'HopUp': 0.55, 'Stretch': 0.15, 'WingUp': 0.8}),
            (5,  {'HopUp': 0.38, 'Squash': 0.15, 'WingDown': 0.85}),
            (10, {'HopUp': 0.6,  'Stretch': 0.12, 'WingUp': 0.75}),
            (16, {'HopUp': 0.4,  'Squash': 0.12, 'WingDown': 0.8}),
            (21, {'HopUp': 0.58, 'Stretch': 0.1,  'WingUp': 0.8}),
            (26, {'HopUp': 0.38, 'Squash': 0.14, 'WingDown': 0.85}),
            (32, {'HopUp': 0.55, 'Stretch': 0.15, 'WingUp': 0.8}),
        ],
    },
    'Swoop Flight': {
        'total_frames': 36,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'HopUp': 0.5,  'Squash': 0.15, 'WingDown': 0.7}),
            (4,  {'HopUp': 0.3,  'Squash': 0.2,  'WingDown': 0.85}),   # swooping DOWN
            (8,  {'HopUp': 0.2,  'Squash': 0.25, 'WingUp': 0.5}),      # lowest — wings start up
            (12, {'HopUp': 0.5,  'Stretch': 0.2,  'WingUp': 0.9}),     # BIG flap UP
            (16, {'HopUp': 0.75, 'Stretch': 0.25, 'WingDown': 0.3}),   # peak — wings tucking
            (20, {'HopUp': 0.6,  'Stretch': 0.12, 'WingDown': 0.7}),   # gliding
            (24, {'HopUp': 0.45, 'Squash': 0.12, 'WingUp': 0.6}),      # descending
            (28, {'HopUp': 0.35, 'Squash': 0.18, 'WingUp': 0.8}),      # swooping
            (32, {'HopUp': 0.42, 'WingDown': 0.6}),                     # transition
            (36, {'HopUp': 0.5,  'Squash': 0.15, 'WingDown': 0.7}),    # loop
        ],
    },
    'Dive Bite': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {'HopUp': 0.55, 'Stretch': 0.1,  'WingUp': 0.7}),
            (3,  {'HopUp': 0.75, 'Stretch': 0.25, 'WingUp': 0.9}),    # rising — big flap
            (5,  {'HopUp': 0.95, 'Stretch': 0.3,  'WingDown': 0.3}),   # MAX HEIGHT
            (7,  {'HopUp': 1.0,  'Stretch': 0.28}),                     # hold peak — wings tucked
            (9,  {'HopUp': 0.5,  'Squash': 0.2}),                       # DIVE — wings folded tight
            (11, {'HopUp': 0.2,  'Squash': 0.35}),                      # IMPACT — fangs in
            (13, {'HopUp': 0.15, 'Squash': 0.3}),                       # latched on
            (16, {'HopUp': 0.3,  'Stretch': 0.15, 'WingUp': 0.6}),     # releasing — flap
            (19, {'HopUp': 0.45, 'Stretch': 0.2,  'WingDown': 0.7}),
            (23, {'HopUp': 0.52, 'Stretch': 0.12, 'WingUp': 0.7}),
            (27, {'HopUp': 0.55, 'WingDown': 0.6}),
            (30, {'HopUp': 0.55, 'Stretch': 0.1,  'WingUp': 0.7}),
        ],
    },
    'Wing Recoil': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'HopUp': 0.25, 'Squash': 0.3}),                      # SMACKED
            (3,  {'HopUp': 0.18, 'Squash': 0.35}),                      # lowest
            (6,  {'HopUp': 0.45, 'Stretch': 0.2, 'WingUp': 0.9}),      # frantic flap
            (9,  {'HopUp': 0.35, 'Squash': 0.1, 'WingDown': 0.8}),
            (12, {'HopUp': 0.5,  'Stretch': 0.12, 'WingUp': 0.7}),
            (15, {'HopUp': 0.48, 'WingDown': 0.6}),
            (18, {'HopUp': 0.55, 'Stretch': 0.1, 'WingUp': 0.7}),
        ],
    },
    'Plummet Death': {
        'total_frames': 32,
        'classified_type': 'death',
        'keyframes': [
            (0,  {'HopUp': 0.55, 'Stretch': 0.1, 'WingUp': 0.7}),     # flying
            (3,  {'HopUp': 0.45, 'Squash': 0.15, 'WingDown': 0.5}),   # hit
            (5,  {'HopUp': 0.5,  'Stretch': 0.15, 'WingUp': 0.6}),    # desperate flap
            (7,  {'HopUp': 0.35, 'Squash': 0.2, 'WingDown': 0.3}),    # failing
            (9,  {'HopUp': 0.4,  'WingUp': 0.3}),                      # last weak flap
            (11, {'HopUp': 0.25, 'Squash': 0.25}),                     # wings FOLD — done
            (13, {'HopUp': 0.15, 'Squash': 0.3}),                      # plummeting
            (15, {'HopUp': 0.05, 'Squash': 0.35}),                     # almost ground
            (17, {'Squash': 0.3}),                                       # CRASH
            (19, {'Squash': 0.2}),                                       # on ground
            (21, {'Squash': 0.15, 'TiltOver': 0.3}),                   # tipping over
            (25, {'Squash': 0.2,  'TiltOver': 0.7}),                   # falling on side
            (28, {'Squash': 0.2,  'TiltOver': 0.9}),                   # on side
            (32, {'Squash': 0.2,  'TiltOver': 1.0}),                   # dead on side
        ],
    },
}

# --- Quadruped preset (Savage Babe, Wolf, Savage, etc.) ---
# Four-legged animals: trotting gait, charge attacks, stumble/collapse death
QUADRUPED_SHAPE_KEYS = {
    'Squash':   {'z_scale': 0.6,  'xy_scale': 1.25},      # Deep crouch before charge
    'Stretch':  {'z_scale': 1.25, 'xy_scale': 0.85},      # Body extends during charge
    'HopUp':    {'z_offset': 0.18},                        # Trot bounce (not a hop)
    'TiltOver': {'tilt': True, 'amount': 0.9},             # Collapse on side
}

QUADRUPED_ANIMATIONS = {
    'Idle Sniff': {
        'total_frames': 48,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),                                      # standing
            (8,  {'Squash': 0.1}),                         # head dips — sniffing
            (14, {'Stretch': 0.06}),                       # head up — alert
            (20, {}),                                      # pause
            (28, {'Squash': 0.12}),                        # another sniff
            (34, {'Stretch': 0.08}),                       # look up
            (40, {'Squash': 0.04}),                        # breathing
            (48, {}),                                      # loop
        ],
    },
    'Trot Forward': {
        'total_frames': 28,
        'classified_type': 'walk',
        'keyframes': [
            (0,  {'Squash': 0.15}),                        # front legs plant
            (3,  {'Stretch': 0.1, 'HopUp': 0.5}),         # push off — body UP
            (6,  {'Squash': 0.18}),                        # front legs land — bob down
            (9,  {'Stretch': 0.08, 'HopUp': 0.4}),        # push off again
            (12, {'Squash': 0.2}),                         # land — deeper bob
            (15, {'Stretch': 0.12, 'HopUp': 0.55}),       # bigger push — trotting faster
            (18, {'Squash': 0.15}),                        # land
            (21, {'Stretch': 0.1, 'HopUp': 0.45}),        # push
            (24, {'Squash': 0.18}),                        # land
            (28, {'Squash': 0.15}),                        # loop
        ],
    },
    'Charge Attack': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                      # standing
            (3,  {'Squash': 0.3}),                         # crouch — winding up
            (5,  {'Squash': 0.45}),                        # deep crouch — LOADING
            (7,  {'Stretch': 0.5, 'HopUp': 0.4}),         # CHARGE — body extends forward
            (9,  {'Stretch': 0.6, 'HopUp': 0.3}),         # full extension — IMPACT
            (11, {'Stretch': 0.55, 'HopUp': 0.15}),       # follow-through
            (14, {'Squash': 0.35}),                        # hard recoil — head down
            (17, {'Squash': 0.25}),                        # recovering
            (21, {'Squash': 0.12}),                        # settling
            (25, {'Squash': 0.05}),                        # almost rest
            (30, {}),                                      # rest
        ],
    },
    'Stumble Flinch': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.35}),                        # impact — buckles
            (3,  {'Squash': 0.4, 'HopUp': 0.15}),         # stumble — slight lift
            (6,  {'Stretch': 0.15, 'HopUp': 0.25}),       # recovering step
            (9,  {'Squash': 0.2}),                         # plants feet
            (12, {'Stretch': 0.08}),                       # steadying
            (18, {}),                                      # recovered
        ],
    },
    'Collapse Death': {
        'total_frames': 32,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # standing
            (3,  {'Squash': 0.25}),                        # hit — legs buckle
            (6,  {'Stretch': 0.15, 'HopUp': 0.2}),        # staggers up
            (9,  {'Squash': 0.35}),                        # legs GIVE OUT
            (12, {'Squash': 0.4}),                         # collapsing
            (15, {'Squash': 0.3, 'TiltOver': 0.2}),       # starting to tip
            (18, {'Squash': 0.25, 'TiltOver': 0.4}),      # falling sideways
            (22, {'Squash': 0.2, 'TiltOver': 0.65}),      # almost down
            (26, {'Squash': 0.25, 'TiltOver': 0.85}),     # on side
            (32, {'Squash': 0.2, 'TiltOver': 1.0}),       # dead — on side
        ],
    },
}

# --- Plant preset (Mandragora, Flora, etc.) ---
# Rooted/stationary monsters — no walk. Attack by thrashing tentacles/leaves.
PLANT_SHAPE_KEYS = {
    'Squash':   {'z_scale': 0.65, 'xy_scale': 1.25},      # Tentacle retraction / compression
    'Stretch':  {'z_scale': 1.35, 'xy_scale': 0.82},      # Tentacle extension / lashing out
    'HopUp':    {'z_offset': 0.12},                        # Slight lift for writhing effect
    'TiltOver': {'tilt': True, 'amount': 0.85},            # Wilting death
}

PLANT_ANIMATIONS = {
    'Idle Sway': {
        'total_frames': 56,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),                                      # resting
            (8,  {'Stretch': 0.1, 'HopUp': 0.2}),         # sway up — leaves rustle
            (16, {'Squash': 0.08}),                        # sway down
            (24, {'Stretch': 0.06, 'HopUp': 0.15}),       # sway up again
            (32, {'Squash': 0.05}),                        # settle
            (42, {'Stretch': 0.04, 'HopUp': 0.08}),       # gentle breath
            (56, {}),                                      # loop
        ],
    },
    'Tentacle Thrash': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                      # still
            (2,  {'Squash': 0.25}),                        # retract — coiling
            (4,  {'Squash': 0.35}),                        # deep retract — LOADING
            (6,  {'Stretch': 0.5, 'HopUp': 0.4}),         # LASH OUT — big extension
            (8,  {'Squash': 0.3}),                         # snap back
            (10, {'Stretch': 0.45, 'HopUp': 0.35}),       # LASH AGAIN — rapid thrash
            (12, {'Squash': 0.25}),                        # snap back
            (14, {'Stretch': 0.35, 'HopUp': 0.25}),       # third lash — weaker
            (17, {'Squash': 0.2}),                         # retract
            (20, {'Squash': 0.12}),                        # settling
            (25, {'Squash': 0.05}),                        # calming
            (30, {}),                                      # rest
        ],
    },
    'Recoil Flinch': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.35}),                        # impact — compressed
            (3,  {'Squash': 0.4, 'HopUp': 0.1}),          # max compress
            (6,  {'Stretch': 0.2, 'HopUp': 0.2}),         # rebound — springs back
            (10, {'Stretch': 0.08}),                       # settling
            (14, {'Squash': 0.03}),                        # almost rest
            (18, {}),                                      # recovered
        ],
    },
    'Wilt Death': {
        'total_frames': 32,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                      # alive
            (3,  {'Squash': 0.15}),                        # hit
            (6,  {'Stretch': 0.12, 'HopUp': 0.1}),        # last sway — still fighting
            (9,  {'Squash': 0.2}),                         # drooping
            (12, {'Squash': 0.25}),                        # wilting — leaves droop
            (16, {'Squash': 0.3, 'TiltOver': 0.2}),       # starting to lean
            (20, {'Squash': 0.3, 'TiltOver': 0.45}),      # falling sideways
            (24, {'Squash': 0.25, 'TiltOver': 0.7}),      # almost down
            (28, {'Squash': 0.25, 'TiltOver': 0.9}),      # on side
            (32, {'Squash': 0.2, 'TiltOver': 1.0}),       # wilted — dead
        ],
    },
}

# --- Biped Insect preset (Rocker, Thief Bug, etc.) ---
# Upright insects with arms and legs. Uses LeanForward for body tilt
# and ArmReach for arm extension. Can't truly move limbs independently,
# but rapid lean+squash+stretch cycles create convincing biped movement.
BIPED_INSECT_SHAPE_KEYS = {
    'Squash':      {'z_scale': 0.6,  'xy_scale': 1.25},   # Crouch/wind-up
    'Stretch':     {'z_scale': 1.3,  'xy_scale': 0.83},   # Reach up / stand tall
    'HopUp':       {'z_offset': 0.2},                      # Walking step bounce
    'TiltOver':    {'tilt': True, 'amount': 0.9},          # Death crumple sideways
    'LeanForward': {'lean': True, 'amount': 0.35},         # Upper body tilts forward
    'ArmReach':    {'wing': True, 'z_offset': 0.12, 'xy_scale': 1.2},  # Arms extend outward
}

BIPED_INSECT_ANIMATIONS = {
    'Idle Fidget': {
        'total_frames': 48,
        'classified_type': 'idle',
        'keyframes': [
            (0,  {}),                                      # standing upright
            (8,  {'Squash': 0.06, 'LeanForward': 0.1}),   # slight forward bob
            (14, {'Stretch': 0.05}),                       # straighten
            (20, {}),                                      # pause
            (26, {'LeanForward': 0.15, 'ArmReach': 0.1}), # look down — arms shift
            (32, {'Stretch': 0.08}),                       # back up
            (38, {'Squash': 0.04}),                        # subtle fidget
            (48, {}),                                      # loop
        ],
    },
    'Skitter Walk': {
        'total_frames': 24,
        'classified_type': 'walk',
        'keyframes': [
            # Rapid rhythmic walk: lean-bob-lean-bob
            (0,  {'Squash': 0.15, 'LeanForward': 0.2}),    # step — leaning forward
            (3,  {'Stretch': 0.1, 'HopUp': 0.5}),          # push off — body UP
            (6,  {'Squash': 0.18, 'LeanForward': 0.15}),   # land — bob down + lean
            (9,  {'Stretch': 0.08, 'HopUp': 0.45}),        # push off again
            (12, {'Squash': 0.2, 'LeanForward': 0.25}),    # deeper step — more lean
            (15, {'Stretch': 0.12, 'HopUp': 0.55}),        # big push off
            (18, {'Squash': 0.15, 'LeanForward': 0.18}),   # land
            (21, {'Stretch': 0.08, 'HopUp': 0.4}),         # push
            (24, {'Squash': 0.15, 'LeanForward': 0.2}),    # loop
        ],
    },
    'Claw Swipe': {
        'total_frames': 30,
        'classified_type': 'attack',
        'keyframes': [
            (0,  {}),                                       # standing
            (3,  {'Squash': 0.3}),                          # crouch — loading
            (5,  {'Squash': 0.4, 'ArmReach': 0.3}),        # deep wind-up — arms pull back
            (7,  {'Stretch': 0.5, 'LeanForward': 0.5,
                  'ArmReach': 0.8, 'HopUp': 0.3}),         # LUNGE — arms reach, body extends
            (9,  {'Stretch': 0.45, 'LeanForward': 0.6,
                  'ArmReach': 0.9}),                        # MAX REACH — claw swipe impact
            (11, {'Stretch': 0.3, 'LeanForward': 0.4,
                  'ArmReach': 0.6}),                        # follow through
            (14, {'Squash': 0.3, 'LeanForward': 0.15}),    # recoil back
            (17, {'Squash': 0.2}),                          # recovering
            (21, {'Squash': 0.1}),                          # settling
            (25, {'Squash': 0.04}),                         # almost rest
            (30, {}),                                       # rest
        ],
    },
    'Flinch Reel': {
        'total_frames': 18,
        'classified_type': 'hit',
        'keyframes': [
            (0,  {'Squash': 0.3}),                          # impact — crumples
            (3,  {'Squash': 0.35, 'HopUp': 0.15}),         # reeling back
            (6,  {'Stretch': 0.12, 'LeanForward': 0.15}),  # stumbles forward
            (9,  {'Squash': 0.15}),                         # catches self
            (12, {'Stretch': 0.06}),                        # steadying
            (18, {}),                                       # recovered
        ],
    },
    'Crumple Death': {
        'total_frames': 32,
        'classified_type': 'death',
        'keyframes': [
            (0,  {}),                                       # standing
            (3,  {'Squash': 0.2}),                          # hit — buckles
            (6,  {'Stretch': 0.1, 'LeanForward': 0.2}),    # staggers forward
            (9,  {'Squash': 0.3, 'LeanForward': 0.35}),    # legs giving out — pitching forward
            (12, {'Squash': 0.35, 'LeanForward': 0.4}),    # crumpling — almost down
            (15, {'Squash': 0.3, 'LeanForward': 0.3,
                  'TiltOver': 0.2}),                        # starting to tip sideways
            (19, {'Squash': 0.25, 'TiltOver': 0.45}),      # falling
            (23, {'Squash': 0.25, 'TiltOver': 0.7}),       # almost down
            (27, {'Squash': 0.2, 'TiltOver': 0.9}),        # on side
            (32, {'Squash': 0.2, 'TiltOver': 1.0}),        # crumpled — dead
        ],
    },
}

# Monster type registry
MONSTER_PRESETS = {
    'blob': {
        'shape_keys': BLOB_SHAPE_KEYS,
        'animations': BLOB_ANIMATIONS,
    },
    'caterpillar': {
        'shape_keys': CATERPILLAR_SHAPE_KEYS,
        'animations': CATERPILLAR_ANIMATIONS,
    },
    'rabbit': {
        'shape_keys': RABBIT_SHAPE_KEYS,
        'animations': RABBIT_ANIMATIONS,
    },
    'egg': {
        'shape_keys': EGG_SHAPE_KEYS,
        'animations': EGG_ANIMATIONS,
    },
    'frog': {
        'shape_keys': FROG_SHAPE_KEYS,
        'animations': FROG_ANIMATIONS,
    },
    'tree': {
        'shape_keys': TREE_SHAPE_KEYS,
        'animations': TREE_ANIMATIONS,
    },
    'bird': {
        'shape_keys': BIRD_SHAPE_KEYS,
        'animations': BIRD_ANIMATIONS,
    },
    'flying_insect': {
        'shape_keys': FLYING_INSECT_SHAPE_KEYS,
        'animations': FLYING_INSECT_ANIMATIONS,
    },
    'bat': {
        'shape_keys': BAT_SHAPE_KEYS,
        'animations': BAT_ANIMATIONS,
    },
    'quadruped': {
        'shape_keys': QUADRUPED_SHAPE_KEYS,
        'animations': QUADRUPED_ANIMATIONS,
    },
    'plant': {
        'shape_keys': PLANT_SHAPE_KEYS,
        'animations': PLANT_ANIMATIONS,
    },
    'biped_insect': {
        'shape_keys': BIPED_INSECT_SHAPE_KEYS,
        'animations': BIPED_INSECT_ANIMATIONS,
    },
}


# =============================================================
# Argument Parsing
# =============================================================

def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser(
        description="Render non-humanoid monsters to sprite sheets")
    parser.add_argument("input", help="Model file (GLB/FBX)")
    parser.add_argument("output", nargs="?",
                        default="C:/Sabri_MMO/2D animations/sprites/render_output",
                        help="Output directory")
    parser.add_argument("--monster-type", default="blob",
                        choices=list(MONSTER_PRESETS.keys()),
                        help="Monster type preset (default: blob). "
                             "Use 'caterpillar' for worm/insect enemies.")
    parser.add_argument("--directions", type=int, default=8, choices=[4, 8])
    parser.add_argument("--render-size", type=int, default=1024)
    parser.add_argument("--no-cel-shade", action="store_true")
    parser.add_argument("--no-outline", action="store_true")
    parser.add_argument("--outline-width", type=float, default=0.002)
    parser.add_argument("--cel-shadow", type=float, default=0.45)
    parser.add_argument("--cel-mid", type=float, default=0.78)
    parser.add_argument("--camera-angle", type=float, default=10.0,
                        help="Camera elevation degrees (default 10)")
    parser.add_argument("--camera-target-z", type=float, default=0.7,
                        help="Camera look-at Z (default 0.7)")
    parser.add_argument("--model-rotation", type=float, default=0.0,
                        help="Rotate model around Z axis (degrees) before rendering. "
                             "Use to align face with south camera (dir_idx=0).")
    parser.add_argument("--save-blend", action="store_true",
                        help="Save .blend file for manual tweaking")

    return parser.parse_args(argv)


# =============================================================
# Scene Setup
# =============================================================

def clear_scene():
    """Remove all default objects."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for block in bpy.data.meshes:
        if block.users == 0:
            bpy.data.meshes.remove(block)
    print("[OK] Scene cleared")


def import_model(filepath):
    """Import GLB/FBX model."""
    ext = os.path.splitext(filepath)[1].lower()
    if ext in ('.glb', '.gltf'):
        bpy.ops.import_scene.gltf(filepath=filepath)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=filepath)
    else:
        raise ValueError(f"Unsupported format: {ext}")
    print(f"[OK] Imported {os.path.basename(filepath)}")


def join_meshes():
    """Join all mesh objects into a single mesh."""
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    if not meshes:
        raise RuntimeError("No mesh objects found after import")
    if len(meshes) == 1:
        print(f"[OK] Single mesh: {meshes[0].name}")
        return meshes[0]

    # Select all meshes, make first one active
    bpy.ops.object.select_all(action='DESELECT')
    for m in meshes:
        m.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()

    result = bpy.context.active_object
    print(f"[OK] Joined {len(meshes)} meshes -> {result.name}")
    return result


def get_scene_bounds():
    """World-space bounding box of all mesh objects."""
    min_co = mathutils.Vector((float('inf'),) * 3)
    max_co = mathutils.Vector((float('-inf'),) * 3)

    depsgraph = bpy.context.evaluated_depsgraph_get()
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        eval_obj = obj.evaluated_get(depsgraph)
        try:
            mesh = eval_obj.to_mesh()
        except RuntimeError:
            continue
        for vert in mesh.vertices:
            world_co = obj.matrix_world @ vert.co
            min_co.x = min(min_co.x, world_co.x)
            min_co.y = min(min_co.y, world_co.y)
            min_co.z = min(min_co.z, world_co.z)
            max_co.x = max(max_co.x, world_co.x)
            max_co.y = max(max_co.y, world_co.y)
            max_co.z = max(max_co.z, world_co.z)
        eval_obj.to_mesh_clear()

    return min_co, max_co


def center_and_normalize():
    """Center model at origin, feet at Z=0, normalize height to 2.0 units."""
    min_co, max_co = get_scene_bounds()
    if min_co.x == float('inf'):
        print("[WARN] No mesh objects found for centering")
        return

    center = (min_co + max_co) / 2
    height = max_co.z - min_co.z

    offset = mathutils.Vector((-center.x, -center.y, -min_co.z))
    roots = [o for o in bpy.context.scene.objects if o.parent is None]
    for obj in roots:
        obj.location += offset
    bpy.context.view_layer.update()

    if height > 0.001:
        scale_factor = 2.0 / height
        for obj in roots:
            obj.scale *= scale_factor
        bpy.context.view_layer.update()

    # Apply transforms so shape key vertex coordinates are in world space
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            bpy.context.view_layer.objects.active = obj
            obj.select_set(True)
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            obj.select_set(False)

    print(f"[OK] Centered (height {height:.3f} -> 2.0)")


# =============================================================
# Shape Keys
# =============================================================

def compute_mesh_bounds(mesh_obj):
    """Get bounding box info from the mesh's vertex data."""
    verts = mesh_obj.data.vertices
    zs = [v.co.z for v in verts]
    xs = [v.co.x for v in verts]
    ys = [v.co.y for v in verts]
    center = mathutils.Vector((
        (min(xs) + max(xs)) / 2,
        (min(ys) + max(ys)) / 2,
        (min(zs) + max(zs)) / 2,
    ))
    return center, min(zs), max(zs)


def add_shape_keys(mesh_obj, preset):
    """Add shape keys to the mesh based on the preset definition.

    Shape key types:
      - Deformation (Squash/Stretch/Flatten): scale from z_min anchor
      - Translation (HopUp): uniform vertical offset
      - Wing-targeted (WingUp/WingDown): only affects vertices above center
        height AND beyond 35% of max XY extent (wing region)
      - Tilt (TiltOver): tilts model sideways — vertices on +X side drop,
        -X side rises. Creates "fallen over" death poses.
    """
    # Create Basis if needed
    if not mesh_obj.data.shape_keys:
        mesh_obj.shape_key_add(name='Basis', from_mix=False)

    basis = mesh_obj.data.shape_keys.key_blocks['Basis']
    center, z_min, z_max = compute_mesh_bounds(mesh_obj)
    height = z_max - z_min
    center_z = (z_min + z_max) / 2

    # Compute max XY distance from center (for wing detection)
    max_xy_dist = 0
    for v in mesh_obj.data.vertices:
        xy_dist = math.sqrt((v.co.x - center.x)**2 + (v.co.y - center.y)**2)
        max_xy_dist = max(max_xy_dist, xy_dist)

    for sk_name, params in preset.items():
        key = mesh_obj.shape_key_add(name=sk_name, from_mix=False)
        key.value = 0.0

        if params.get('wing'):
            # Wing-targeted: only deform vertices in the wing region
            # (above center height AND far from center XY)
            wing_z_off = params.get('z_offset', 0) * height
            wing_xy_scale = params.get('xy_scale', 1.0)
            wing_threshold = 0.35  # 35% of max XY extent

            for i in range(len(key.data)):
                bco = basis.data[i].co
                xy_dist = math.sqrt((bco.x - center.x)**2 +
                                    (bco.y - center.y)**2)
                is_wing = ((bco.z > center_z) and
                           (xy_dist > max_xy_dist * wing_threshold))

                if is_wing:
                    key.data[i].co = mathutils.Vector((
                        center.x + (bco.x - center.x) * wing_xy_scale,
                        center.y + (bco.y - center.y) * wing_xy_scale,
                        bco.z + wing_z_off,
                    ))
                # else: non-wing vertices stay at basis (no deformation)

        elif params.get('lean'):
            # Forward lean: higher vertices move forward (Y axis),
            # anchored at feet. Creates walking lean, attack lunge,
            # and backward recoil. Essential for biped enemies.
            lean_amount = params.get('amount', 0.3) * height
            for i in range(len(key.data)):
                bco = basis.data[i].co
                # Z ratio: 0 at feet, 1 at head
                z_ratio = ((bco.z - z_min) / height
                           if height > 0 else 0)
                key.data[i].co = mathutils.Vector((
                    bco.x,
                    bco.y + z_ratio * lean_amount,
                    bco.z,
                ))

        elif params.get('tilt'):
            # Diagonal tilt: combines X+Y so from ALL 8 camera angles the
            # fall is always partially sideways, never directly toward/away.
            # The SW corner drops, NE corner rises — from the south camera
            # this looks like falling forward-right (away from viewer).
            tilt_amount = params.get('amount', 0.4) * height
            for i in range(len(key.data)):
                bco = basis.data[i].co
                # Diagonal ratio: (X + Y) normalized by 2*max for safe range
                diag = ((bco.x - center.x) + (bco.y - center.y))
                diag_ratio = diag / (max_xy_dist * 2.0) if max_xy_dist > 0 else 0
                key.data[i].co = mathutils.Vector((
                    bco.x,
                    bco.y,
                    bco.z + diag_ratio * tilt_amount,
                ))

        elif 'z_offset' in params:
            # Uniform vertical translation (HopUp)
            offset = params['z_offset'] * height
            for i in range(len(key.data)):
                bco = basis.data[i].co
                key.data[i].co = mathutils.Vector((
                    bco.x, bco.y, bco.z + offset))
        else:
            # Scale deformation anchored to z_min (ground)
            z_scale = params['z_scale']
            xy_scale = params['xy_scale']
            for i in range(len(key.data)):
                bco = basis.data[i].co
                key.data[i].co = mathutils.Vector((
                    center.x + (bco.x - center.x) * xy_scale,
                    center.y + (bco.y - center.y) * xy_scale,
                    z_min + (bco.z - z_min) * z_scale,
                ))

    print(f"[OK] Added {len(preset)} shape keys: {list(preset.keys())}")


# =============================================================
# Animation Creation
# =============================================================

def create_animations(mesh_obj, anim_defs, all_sk_names):
    """Create one Blender action per animation with shape key keyframes.

    Returns dict: { anim_name: { 'action', 'total_frames', 'classified_type' } }
    """
    sk = mesh_obj.data.shape_keys
    if not sk.animation_data:
        sk.animation_data_create()

    animations = {}

    for anim_name, anim_def in anim_defs.items():
        action = bpy.data.actions.new(name=anim_name)
        action.use_fake_user = True
        sk.animation_data.action = action

        total = anim_def['total_frames']
        keyframes = anim_def['keyframes']

        # For each keyframe, set all shape key values and insert
        for frame, sk_values in keyframes:
            bpy.context.scene.frame_set(frame)
            for sk_name in all_sk_names:
                val = sk_values.get(sk_name, 0.0)
                sk.key_blocks[sk_name].value = val
                sk.key_blocks[sk_name].keyframe_insert('value', frame=frame)

        # Set interpolation to BEZIER for smooth transitions
        if hasattr(action, 'fcurves'):
            for fc in action.fcurves:
                for kp in fc.keyframe_points:
                    kp.interpolation = 'BEZIER'
        elif hasattr(action, 'layers'):
            # Blender 5.x layered action API
            for layer in action.layers:
                for strip in layer.strips:
                    for bag in strip.channelbags:
                        for fc in bag.fcurves:
                            for kp in fc.keyframe_points:
                                kp.interpolation = 'BEZIER'

        animations[anim_name] = {
            'action': action,
            'total_frames': total,
            'classified_type': anim_def['classified_type'],
        }
        print(f"  {anim_name}: {total} frames, "
              f"{len(keyframes)} keyframes, type={anim_def['classified_type']}")

    # Reset all shape keys to 0
    for sk_name in all_sk_names:
        sk.key_blocks[sk_name].value = 0.0

    print(f"[OK] Created {len(animations)} animations")
    return animations


# =============================================================
# Cel-Shading (identical to blender_sprite_render_v2.py)
# =============================================================

def setup_cel_shade(shadow_brightness=0.45, mid_brightness=0.78):
    """Color-preserving cel-shading with 4 brightness steps."""
    processed = 0
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        for slot in obj.material_slots:
            mat = slot.material
            if not mat or not mat.use_nodes:
                continue

            tree = mat.node_tree
            nodes = tree.nodes
            links = tree.links

            principled = None
            for node in nodes:
                if node.type == 'BSDF_PRINCIPLED':
                    principled = node
                    break
            if not principled:
                continue

            output_node = None
            for node in nodes:
                if node.type == 'OUTPUT_MATERIAL':
                    output_node = node
                    break
            if not output_node:
                continue

            base_color_source = None
            for link in links:
                if link.to_socket == principled.inputs['Base Color']:
                    base_color_source = link.from_socket
                    break
            base_color_value = principled.inputs['Base Color'].default_value[:]

            px = principled.location.x
            py = principled.location.y

            # Shader to RGB
            s2rgb = nodes.new('ShaderNodeShaderToRGB')
            s2rgb.location = (px + 300, py)
            links.new(principled.outputs['BSDF'], s2rgb.inputs['Shader'])

            # RGB to BW (luminance)
            rgb2bw = nodes.new('ShaderNodeRGBToBW')
            rgb2bw.location = (px + 500, py - 150)
            links.new(s2rgb.outputs['Color'], rgb2bw.inputs['Color'])

            # ColorRamp (4 cel-shade steps)
            ramp = nodes.new('ShaderNodeValToRGB')
            ramp.location = (px + 700, py - 150)
            ramp.color_ramp.interpolation = 'CONSTANT'

            cr = ramp.color_ramp
            ds = shadow_brightness * 0.65
            cr.elements[0].position = 0.0
            cr.elements[0].color = (ds, ds, ds, 1.0)
            cr.elements[1].position = 0.20
            cr.elements[1].color = (shadow_brightness, shadow_brightness,
                                    shadow_brightness, 1.0)
            e_mid = cr.elements.new(0.50)
            e_mid.color = (mid_brightness, mid_brightness, mid_brightness, 1.0)
            e_hi = cr.elements.new(0.75)
            e_hi.color = (1.0, 1.0, 1.0, 1.0)

            links.new(rgb2bw.outputs['Val'], ramp.inputs['Fac'])

            # Multiply: ramp x base color
            mix = nodes.new('ShaderNodeMix')
            mix.data_type = 'RGBA'
            mix.blend_type = 'MULTIPLY'
            mix.location = (px + 950, py)
            mix.inputs[0].default_value = 1.0

            links.new(ramp.outputs['Color'], mix.inputs[6])
            if base_color_source:
                links.new(base_color_source, mix.inputs[7])
            else:
                mix.inputs[7].default_value = base_color_value

            # Emission (avoid Eevee double-lighting)
            emission = nodes.new('ShaderNodeEmission')
            emission.location = (px + 1200, py)
            links.new(mix.outputs[2], emission.inputs['Color'])

            # Handle alpha transparency
            alpha_source = None
            for link in list(links):
                if link.to_socket == principled.inputs['Alpha']:
                    alpha_source = link.from_socket
                    break
            has_alpha = (alpha_source is not None or
                         principled.inputs['Alpha'].default_value < 0.999)

            for link in list(links):
                if (link.to_node == output_node and
                        link.to_socket == output_node.inputs['Surface']):
                    links.remove(link)

            if has_alpha:
                transparent = nodes.new('ShaderNodeBsdfTransparent')
                transparent.location = (px + 1200, py - 200)
                mix_shader = nodes.new('ShaderNodeMixShader')
                mix_shader.location = (px + 1450, py)
                if alpha_source:
                    links.new(alpha_source, mix_shader.inputs['Fac'])
                else:
                    mix_shader.inputs['Fac'].default_value = \
                        principled.inputs['Alpha'].default_value
                links.new(transparent.outputs['BSDF'], mix_shader.inputs[1])
                links.new(emission.outputs['Emission'], mix_shader.inputs[2])
                links.new(mix_shader.outputs['Shader'],
                          output_node.inputs['Surface'])
            else:
                links.new(emission.outputs['Emission'],
                          output_node.inputs['Surface'])

            processed += 1

    print(f"[OK] Cel-shade applied to {processed} materials")


def add_outline(thickness=0.002):
    """Add solidify modifier for 1px-style black outline."""
    outline_mat = bpy.data.materials.new(name="Outline_Black")
    outline_mat.use_nodes = True
    outline_mat.use_backface_culling = True
    nodes = outline_mat.node_tree.nodes
    lnk = outline_mat.node_tree.links
    nodes.clear()
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = (0.02, 0.02, 0.02, 1.0)
    out = nodes.new('ShaderNodeOutputMaterial')
    lnk.new(em.outputs['Emission'], out.inputs['Surface'])

    count = 0
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        obj.data.materials.append(outline_mat)
        idx = len(obj.data.materials) - 1
        mod = obj.modifiers.new(name="Outline", type='SOLIDIFY')
        mod.thickness = thickness
        mod.offset = -1
        mod.use_flip_normals = True
        mod.material_offset = idx
        count += 1

    print(f"[OK] Outline added to {count} meshes")


# =============================================================
# Camera, Lighting, Render Settings
# =============================================================

def setup_camera(elevation_deg=10.0):
    """Create orthographic camera."""
    cam_data = bpy.data.cameras.new(name="SpriteCamera")
    cam_data.type = 'ORTHO'
    cam_data.ortho_scale = 2.8

    cam_obj = bpy.data.objects.new("SpriteCamera", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj

    print(f"[OK] Camera created (ortho, {elevation_deg} deg)")
    return cam_obj


def auto_frame_camera(cam_obj, mesh_obj, sk_names, elevation_deg=10.0,
                       padding=1.2):
    """Frame camera to fit the model at its tallest pose (Stretch + HopUp).

    Temporarily activates shape keys to compute worst-case bounds, then resets.
    """
    sk = mesh_obj.data.shape_keys
    if sk:
        # Activate stretch + hop to find max bounds
        for name in sk_names:
            if name in ('Stretch', 'HopUp', 'TiltOver', 'WingUp',
                            'LeanForward', 'ArmReach'):
                sk.key_blocks[name].value = 1.0
        bpy.context.view_layer.update()

    min_co, max_co = get_scene_bounds()

    # Reset
    if sk:
        for name in sk_names:
            sk.key_blocks[name].value = 0.0
        bpy.context.view_layer.update()

    if min_co.x == float('inf'):
        return

    size = max_co - min_co
    ev = math.radians(elevation_deg)
    vis_h = size.z * math.cos(ev) + max(size.x, size.y) * math.sin(ev)
    vis_w = max(size.x, size.y)
    scale = max(vis_h, vis_w) * padding
    cam_obj.data.ortho_scale = max(scale, 2.5)

    print(f"[OK] Auto-frame ortho_scale={cam_obj.data.ortho_scale:.2f}")


def setup_lighting():
    """Three-point lighting for clear, consistent sprites."""
    key = bpy.data.lights.new("KeyLight", 'SUN')
    key.energy = 3.5
    key.color = (1.0, 0.98, 0.95)
    key_obj = bpy.data.objects.new("KeyLight", key)
    bpy.context.scene.collection.objects.link(key_obj)
    key_obj.rotation_euler = (math.radians(50), math.radians(10),
                              math.radians(-25))

    fill = bpy.data.lights.new("FillLight", 'SUN')
    fill.energy = 1.5
    fill.color = (0.95, 0.95, 1.0)
    fill_obj = bpy.data.objects.new("FillLight", fill)
    bpy.context.scene.collection.objects.link(fill_obj)
    fill_obj.rotation_euler = (math.radians(55), math.radians(-10),
                               math.radians(155))

    rim = bpy.data.lights.new("RimLight", 'SUN')
    rim.energy = 1.0
    rim_obj = bpy.data.objects.new("RimLight", rim)
    bpy.context.scene.collection.objects.link(rim_obj)
    rim_obj.rotation_euler = (math.radians(30), 0, math.radians(180))

    print("[OK] 3-point lighting")


def setup_render(render_size):
    """Configure EEVEE for transparent sprite rendering."""
    scene = bpy.context.scene

    for engine in ['BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE']:
        try:
            scene.render.engine = engine
            print(f"[OK] Engine: {engine}")
            break
        except Exception:
            continue

    scene.render.resolution_x = render_size
    scene.render.resolution_y = render_size
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.image_settings.compression = 15
    scene.render.use_stamp = False

    if hasattr(scene.render, 'use_motion_blur'):
        scene.render.use_motion_blur = False

    print(f"[OK] Render: {render_size}x{render_size} RGBA PNG")


# =============================================================
# Rendering
# =============================================================

def position_camera(cam_obj, dir_idx, num_dirs, elevation_deg, target_z=1.0):
    """Place camera at the given direction around the model."""
    dist = 10
    ev = math.radians(elevation_deg)
    angle_h = dir_idx * (2 * math.pi / num_dirs)

    cam_obj.location = (
        dist * math.sin(angle_h) * math.cos(ev),
        -dist * math.cos(angle_h) * math.cos(ev),
        dist * math.sin(ev) + target_z,
    )

    target = mathutils.Vector((0, 0, target_z))
    direction = target - cam_obj.location
    rot = direction.to_track_quat('-Z', 'Y')
    cam_obj.rotation_euler = rot.to_euler()


def sample_frames(start, end, target_count):
    """Evenly sample target_count frames from [start, end]."""
    total = end - start + 1
    if total <= target_count:
        return list(range(start, end + 1))
    step = (total - 1) / max(target_count - 1, 1)
    return [int(start + i * step) for i in range(target_count)]


def render_all(cam_obj, output_dir, mesh_obj, animations, args):
    """Render every animation x direction x sampled frame."""
    os.makedirs(output_dir, exist_ok=True)
    dir_names = DIRECTION_NAMES_8 if args.directions == 8 else DIRECTION_NAMES_4
    num_dirs = args.directions

    sk = mesh_obj.data.shape_keys
    total = 0

    for anim_name, anim_info in animations.items():
        atype = anim_info['classified_type']
        target_frames = DEFAULT_FRAME_TARGETS.get(atype, 8)
        total_frames = anim_info['total_frames']

        # Switch to this animation's action
        sk.animation_data.action = anim_info['action']

        frames = sample_frames(0, total_frames, target_frames)

        for dir_idx in range(num_dirs):
            dir_name = dir_names[dir_idx]
            position_camera(cam_obj, dir_idx, num_dirs,
                            args.camera_angle, args.camera_target_z)

            for fi, frame in enumerate(frames):
                bpy.context.scene.frame_set(frame)
                bpy.context.view_layer.update()

                # Output to subfolder per animation
                anim_dir = os.path.join(output_dir, anim_name)
                os.makedirs(anim_dir, exist_ok=True)

                filename = f"{anim_name}_{dir_name}_f{fi:02d}.png"
                filepath = os.path.join(anim_dir, filename)

                bpy.context.scene.render.filepath = filepath
                bpy.ops.render.render(write_still=True)
                total += 1

        print(f"  {anim_name}: {len(frames)} frames x {num_dirs} dirs")

    # Reset shape keys
    for kb in sk.key_blocks:
        if kb.name != 'Basis':
            kb.value = 0.0

    print(f"\n[OK] Rendered {total} sprites -> {output_dir}")
    return total


# =============================================================
# Main Pipeline
# =============================================================

def main():
    args = parse_args()

    preset = MONSTER_PRESETS[args.monster_type]
    sk_preset = preset['shape_keys']
    anim_defs = preset['animations']
    sk_names = list(sk_preset.keys())

    print(f"\n{'='*60}")
    print(f"Monster Sprite Renderer - {args.monster_type}")
    print(f"{'='*60}")
    print(f"  Input:  {args.input}")
    print(f"  Output: {args.output}")
    print(f"  Size:   {args.render_size}x{args.render_size}")
    print(f"  Dirs:   {args.directions}")
    print(f"  Anims:  {list(anim_defs.keys())}")
    print()

    # 1. Clear scene and import
    clear_scene()
    import_model(os.path.abspath(args.input))

    # 2. Remove any armatures from the import (not needed for shape keys)
    for obj in list(bpy.context.scene.objects):
        if obj.type == 'ARMATURE':
            bpy.data.objects.remove(obj, do_unlink=True)

    # 3. Join meshes and normalize
    mesh_obj = join_meshes()
    center_and_normalize()

    # 3b. Rotate model to align face with south camera (dir_idx=0)
    if args.model_rotation != 0.0:
        import bmesh
        rot_rad = math.radians(args.model_rotation)
        rot_matrix = mathutils.Matrix.Rotation(rot_rad, 4, 'Z')
        bm = bmesh.new()
        bm.from_mesh(mesh_obj.data)
        bmesh.ops.rotate(bm, verts=bm.verts, matrix=rot_matrix,
                         cent=mathutils.Vector((0, 0, 0)))
        bm.to_mesh(mesh_obj.data)
        bm.free()
        mesh_obj.data.update()
        bpy.context.view_layer.update()
        print(f"[OK] Rotated mesh vertices {args.model_rotation}° around Z")

    # 4. Add shape keys
    print("\n--- Shape Keys ---")
    add_shape_keys(mesh_obj, sk_preset)

    # 5. Create animations
    print("\n--- Animations ---")
    animations = create_animations(mesh_obj, anim_defs, sk_names)

    # 6. Visual setup
    print("\n--- Visual Setup ---")
    if not args.no_cel_shade:
        setup_cel_shade(args.cel_shadow, args.cel_mid)
    if not args.no_outline:
        add_outline(args.outline_width)
    setup_lighting()
    setup_render(args.render_size)

    cam = setup_camera(args.camera_angle)
    auto_frame_camera(cam, mesh_obj, sk_names, args.camera_angle)

    # 7. Save .blend for manual tweaking
    if args.save_blend:
        blend_path = os.path.join(args.output, "monster_scene.blend")
        os.makedirs(args.output, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=blend_path)
        print(f"[OK] Saved {blend_path}")

    # 8. Render
    print("\n--- Rendering ---")
    render_all(cam, args.output, mesh_obj, animations, args)

    print(f"\n{'='*60}")
    print(f"DONE. Output: {args.output}")
    print(f"Next: pack with pack_atlas.py using a v2 config")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
