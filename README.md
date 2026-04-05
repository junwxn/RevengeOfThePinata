# Revenge of the Piñata

A 2.5D hack-and-slash game with roguelike elements, built in C/C++ using a custom engine provided by DigiPen Institute of Technology Singapore.

You play as a piñata fighting back against waves of enemies using a melee combat system built around attack charges and a parry mechanic. Time your parries to defend against incoming attacks and replenish your charges, collect upgrades between waves, and smash your way through increasingly challenging encounters — all wrapped in a colourful, candy-coated art style.

<!-- TODO: Add a gameplay screenshot or GIF here -->
<!-- ![Gameplay Screenshot](screenshots/gameplay.png) -->

## Features

- **Charge-based melee combat** — Manage a limited number of attack charges; every swing counts.
- **Parry-to-recharge system** — Well-timed parries block damage and restore spent charges, keeping the action flowing.
- **Roguelike upgrades** — Choose from a variety of upgrades between waves to shape your build.
- **Satisfying knockback physics** — Send enemies flying with impactful hit feedback.
- **Wave-based encounters** — Face progressively tougher enemy compositions that test your timing and positioning.
- **Colourful 2.5D art style** — A vibrant, hand-crafted aesthetic that contrasts with the intensity of combat.

## Tech Highlights

Built in **C/C++** on a custom engine by DigiPen Institute of Technology Singapore. The team's work focused on gameplay systems on top of the engine, including:

- **Isometric rendering** — Screen/grid coordinate transforms with Y-sorted render queue for correct 2.5D depth ordering
- **A\* pathfinding** — Size-aware clearance, stuck detection, and waypoint look-ahead for smooth enemy navigation
- **Frame-based combat** — Startup/active/recovery frame windows with cone-based hit detection and a 3-stage combo chain
- **Multi-phase boss** — 4-phase state machine with growth scaling, minion spawning, ranged transformation, and HP-gated transitions
- **Knockback physics** — Exponential velocity decay with wall bounce, dampening, and impact damage on high-speed collisions
- **Event-driven augments** — Pub/sub system lets 9 augments hook into gameplay events without coupling to combat code
- **Projectile reflection** — Parry sweep detection with reflection grace period to prevent bounce loops
- **Isometric collision** — Binary grid from TMX layers with axis-decomposed wall-sliding for smooth diagonal movement
- **TMX map pipeline** — RapidXML `.tmx` loader with pre-generated per-tile meshes and multi-layer support
- **Shared resource pooling** — Reference-counted static meshes/textures across enemy instances

## Running the Game

1. Go to the [Releases](../../releases) page
2. Download the latest release
3. Extract the zip and run the executable

## Building from Source

1. Clone the repository
2. Open the `.sln` file in Visual Studio 2022
3. Press F5 to build and run

## Controls

| Input | Action |
|-------|--------|
| WASD | Move |
| Left Click | Attack (consumes a charge) |
| Right Click | Parry |
| Space | Dash |
| X | Interact |

## Screenshots

<!-- TODO: Add 2-4 screenshots showcasing gameplay, combat, and the upgrade screen -->
<!-- ![Combat](screenshots/combat.png) -->
<!-- ![Upgrades](screenshots/upgrades.png) -->

## Team — 4head

- Jun Wen
- Timothy
- Nigel
- Charles

## Built With

- **Language:** C / C++
- **Engine:** Custom engine provided by DigiPen

## Acknowledgements

Developed as a student project at [DigiPen Institute of Technology Singapore](https://www.digipen.edu.sg/)

All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
