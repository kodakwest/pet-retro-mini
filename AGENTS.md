# AGENTS.md — PET Retro Mini

## Project Context

PET Retro Mini is a compact (~3–5 MB) standalone Windows .exe that emulates the Commodore PET series (2001, 4032, 8032). **MVP ships 5 iconic, legally-clear games.** Game library grows via user .prg import — not by bundling more.

## MVP Principle

Stability over library size. Five great games that work perfectly is better than fifty that sorta work. The import path (drag-and-drop .prg files) lets users add thousands of games without licensing risk to us.

## Agent Role

You configure VICE for PET-only SDL2 builds, write the C launcher/frontend, handle the 5 bundled games, and build the .prg import system. No emulator core writing — VICE team maintains that. Your patches are build-system only.

## MVP Games (5 Bundled)

| Game | Genre | Legal |
|---|---|---|
| Zork I | Adventure | MIT License |
| Miner | Arcade | Public domain (Cursor #19) |
| Lunar Lander | Simulation | Public domain |
| Dungeon | RPG | Public domain (Cursor #15) |
| PET Invaders | Arcade | Freeware (masswerk) |

## Key Constraints

### No core emulator writing
Patches to VICE are limited to build system, feature stripping (PET-only), UI backend forcing (SDL2), and embedding assets.

### ROMs
- Open-source replacements in `roms/open/` (bundled)
- Original Commodore ROMs are user-supplied (NOT in repo)
- First-launch boots on open ROMs immediately — no setup required

### Game import (future expansion)
- Drag-and-drop .prg onto window loads it into VICE
- Auto-detect load address from .prg header (2 bytes, LE)
- Remember imported files in config.ini favorites
- No metadata system — filename + load address is sufficient

### Code Style
- C99/C11 (MSVC-compatible subset)
- Snake_case, UPPER_CASE constants
- SDL2 API conventions for UI
- Static link SDL2 (MIT) — no external DLLs

### Windows Target
- x64 only, Windows 10+
- Single .exe, no installer, portable app pattern
- No UPX compression (antivirus false positives)

## Build Pipeline

```bash
git submodule update --init src/vice/
cd src/vice && patch -p1 < ../patches/*.diff
./configure --enable-embedded EMUTYPE=xpet --with-sdl2 --host=x86_64-w64-mingw32
make -j$(nproc)
cd ../..
make -C src/launcher
make dist
# Output: dist/pet-retro-mini-v0.1.zip
```

## Project Structure

```
pet-retro-mini/
├── AGENTS.md              # This file
├── README.md              # Quick start
├── LICENSE                # GPL v2
├── docs/
│   ├── SPEC.md            # Full spec
│   └── IMPORTING_GAMES.md # How to add .prg files
├── src/
│   ├── vice/              # VICE 3.10 (submodule + patches)
│   │   └── patches/
│   │       ├── 01-pet-only.diff
│   │       ├── 02-strip-gtk.diff
│   │       └── 03-embed-roms.diff
│   ├── launcher/
│   │   ├── main.c         # SDL2 init, main loop
│   │   ├── menu.c/h       # Menu bar
│   │   ├── game_launcher.c/h # 5-game loader + .prg importer
│   │   ├── config.c/h     # Settings
│   │   ├── crt_effect.c/h # Green phosphor + scanlines
│   │   └── resources.rc
│   └── common/
│       └── compress.c/h   # LZ4 decompression
├── games/
│   ├── manifest.json      # 5-game catalog
│   ├── zork1/             # Zork I data
│   ├── miner.prg
│   ├── lander.prg
│   ├── dungeon.prg
│   └── petinvaders.prg
├── roms/open/             # Open-source ROM replacements
├── scripts/               # Build, fetch, release
├── tests/                 # Smoke tests
├── Makefile
├── CMakeLists.txt
└── CHANGELOG.md
```

## MVP UI (Concise)

```
Window: 2x scaled PET display (green phosphor)
Bottom bar: [Zork I] [Miner] [Lunar] [Dungeon] [PET Invaders] [Load .prg...]
Menu: File (Load .prg, Reset, Exit) | Machine (Model, Speed) | Help
Settings: Model selector, CRT toggle, speed slider
```

## What I've Learned

*(Auto-populated)*

- VICE configure: `--enable-embedded EMUTYPE=xpet` for standalone PET-only
- SDL2 static link: `-static-libgcc -static-libstdc++` flags
- .prg import: first 2 bytes = LE load address, remaining = payload
- MVP is 5 games, not 30 — simplifies manifest, browser, testing
- Zork I runs on PET 4032/8032 best (32K RAM required)
- ROM replacements must match originals' exact sizes (Kernal 4K, BASIC 8K+4K, Editor 4K, Chargen 1K)
