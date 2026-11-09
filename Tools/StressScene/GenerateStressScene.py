#!/usr/bin/env python3
"""Writes a stress scene: count objects on a square grid, each with a mesh picked at random and an oscillator, every
other one moving. It measures what GameObjects and components cost per object (the GetComponent searches, the
per-frame updates, a moved object's transform and bounds) at counts the shipped scenes are far from.

Seeded, so a count and seed always write the same scene: the plans record those rather than committing the
scenes, which run to tens of megabytes (RZE/Assets/Scenes/Stress*.scene is ignored).

    python3 Tools/StressScene/GenerateStressScene.py 100000            # RZE/Assets/Scenes/Stress100000.scene
    python3 Tools/StressScene/GenerateStressScene.py 25000 --seed 7 --out RZE/Assets/Scenes/Mine.scene

Then open it with -scene Stress100000.scene. The scene has no lights, so there are no shadow maps to render again as
objects move, and no reflection probes; turn global illumination off for the run (Engine.ini, GlobalIllumination=0),
or the irradiance probes are captured again as objects move. OscillatorComponent is a development component: the
scene is for Debug and Development builds.
"""

import argparse
import json
import math
import os
import random
import sys

# The burned meshes, with each one's extents at scale 1 (from its source .obj: min and max, in metres). Whole
# environments (Sponza, Hangar, Mountain) and the flat Quad are left out: they're places, not objects.
MESHES = {
    "Cube": ((-1.0, -1.0, -1.0), (1.0, 1.0, 1.0)),
    "Spaceship": ((-40.326, -10.972, -30.045), (40.326, 10.972, 30.045)),
    "FW190": ((-0.521, 0.005, -0.341), (0.522, 0.327, 0.538)),
    "Nanosuit": ((-3.994, 0.0, -1.72), (3.994, 15.412, 1.72)),
    "NeckMechWalker": ((-3.089, -0.008, -2.382), (3.25, 6.954, 2.404)),
    "Nyra": ((-10.297, -2.603, -12.672), (9.402, 38.759, 9.965)),
    "Panzer": ((-0.185, 0.0, -0.618), (0.192, 0.415, 0.384)),
    "M4A1": ((-1.083, -6.134, -16.701), (2.194, 5.136, 14.77)),
}

# Each mesh is scaled so its longest side is MESH_SIZE and centred in a CELL_SIZE square on the ground, so no two meshes
# overlap whatever is picked. The oscillation is up and down, so it never reaches the next cell.
CELL_SIZE = 3.0
MESH_SIZE = 2.0
AMPLITUDE = 0.5
PERIOD = 3.0

SCENE_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "RZE", "Assets", "Scenes")


def placement(mesh, cell_x, cell_z):
    """Position and uniform scale that put the mesh centred over the cell's middle, its base on the ground"""
    low, high = MESHES[mesh]
    scale = MESH_SIZE / max(h - l for l, h in zip(low, high))
    centre_x = (low[0] + high[0]) * 0.5
    centre_z = (low[2] + high[2]) * 0.5
    return [cell_x - centre_x * scale, -low[1] * scale, cell_z - centre_z * scale], scale


def generate(count, seed):
    rng = random.Random(seed)
    side = math.ceil(math.sqrt(count))
    # The grid is centred on the world origin
    first = -(side - 1) * CELL_SIZE * 0.5
    names = sorted(MESHES)

    used_ids = set()
    objects = []

    def new_id():
        while True:
            guid = rng.getrandbits(64)
            if guid != 0 and guid not in used_ids:
                used_ids.add(guid)
                return "%016x" % guid

    # Above a corner of the grid, looking in across it, for the Game; the Editor's own camera is set in its view
    objects.append({
        "id": new_id(),
        "name": "Camera",
        "components": {
            "TransformComponent": {"Position": [first - 10.0, 25.0, first - 10.0], "Scale": [1.0, 1.0, 1.0],
                                   "Rotation": [-30.0, -135.0, 0.0]},
            "CameraComponent": {"FOV": 60.0, "NearCull": 0.1, "FarCull": 1000.0, "ExposureCompensation": 0.0,
                                "MainCamera": True},
        },
    })

    for index in range(count):
        mesh = rng.choice(names)
        cell_x = first + (index % side) * CELL_SIZE
        cell_z = first + (index // side) * CELL_SIZE
        position, scale = placement(mesh, cell_x, cell_z)
        objects.append({
            "id": new_id(),
            "name": "%s_%d" % (mesh, index),
            # In this order: the oscillator moves the object before its mesh takes the new position, the same frame
            "components": {
                "TransformComponent": {"Position": position, "Scale": [scale, scale, scale],
                                       "Rotation": [0.0, 0.0, 0.0]},
                # The still half has one too, turned off, so both halves have the same components and differ only in
                # moving
                "OscillatorComponent": {"Enabled": index % 2 == 0, "Axis": [0.0, 1.0, 0.0], "Amplitude": AMPLITUDE,
                                        "Period": PERIOD, "Phase": round(rng.random(), 4), "Origin": position},
                "RenderComponent": {"ResourcePath": "ProjectData\\Mesh\\%s.meshasset" % mesh},
            },
        })

    return {"version": 2, "gameobjects": objects}


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("count", type=int, help="how many objects, besides the camera")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out", help="where to write it; RZE/Assets/Scenes/Stress<count>.scene by default")
    args = parser.parse_args()
    if args.count <= 0:
        parser.error("count must be positive")

    out = args.out or os.path.join(SCENE_FOLDER, "Stress%d.scene" % args.count)
    scene = generate(args.count, args.seed)
    with open(out, "w", newline="\n") as file:
        json.dump(scene, file, separators=(",", ":"))
    print("wrote %d objects to %s (%.1f MB)" % (args.count, os.path.normpath(out), os.path.getsize(out) / 1e6))
    return 0


if __name__ == "__main__":
    sys.exit(main())
