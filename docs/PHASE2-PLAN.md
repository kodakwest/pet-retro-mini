# PET Retro Mini — Phase 2: ROM Sourcing & Real Emulation

## Goal
Replace the visual shell with a real emulated PET. Source or build a fully open ROM stack, integrate VICE's emulation core, and make the 5 games actually load and play.

## Principle
This is a novelty/learning project. The ROMs don't need to be museum-accurate. They need to boot BASIC, read the keyboard, display text, and load .prg files. Everything else is faked or omitted.

## ROM Stack (all open, all shippable)

### 1. BASIC ROM — mist64/msbasic (BSD license)
- Repo: https://github.com/mist64/msbasic
- License: 2-clause BSD
- Fetches: clone repo, build with `./make.sh`, extract `bin/commodore/basic1` (BASIC 1.0 for PET 2001)
- Can also build BASIC 2 for 4032/8032 models
- This is the core BASIC interpreter — the language the PET speaks

### 2. Character ROM — Recreated PETSCII font data
- 256 characters × 8 bytes per character row × 8 rows = 16 KB (for an 8×8 font)
- Actually the PET chargen is 8×8 pixels per character = 8 bytes per char, 256 chars = 2048 bytes
- PETSCII character bitmaps are well-documented online
- Can extract from VICE's `pet-` chargen file or recreate from published tables
- This is pixel data — no copyright on the bitmap representation of a 'A' or a heart symbol

### 3. KERNAL + Editor ROM — Custom minimal (~2-3 KB each)
Write from scratch in 6502 assembly. Must do:

**Reset/Init (~50 bytes):**
- Set stack pointer to $FF
- Initialize zero-page vectors
- Clear/set up video pointers
- Initialize PIA #1 ($E810-13) for keyboard scanning
- Initialize PIA #2 ($E820-23) for IEEE-488 (stubbed, unused)
- Initialize VIA ($E840-4F) for timers/user port
- Initialize CRTC ($E880-81) for CRTC-based models (4032, 8032)
- Jump to BASIC cold start vector

**CHROUT — Character Output (~200 bytes):**
- Write byte to screen at current cursor position
- Advance cursor
- Handle line wrap / scrolling
- Handle control characters (CR, LF, HOME, CLR, cursor moves)
- PET: screen memory at $8000-$83FF (40-col) or $8000-$87FF (80-col)
- Color memory not used (single-color PET)

**CHRIN — Keyboard Input (~300 bytes):**
- Scan PIA matrix: write row select to $E810, read column from $E812
- Map matrix position to PETSCII value (using lookup table)
- Handle shift lock, shift, commodore key modifiers
- Handle keyboard repeat
- Queue character, return on CHRIN call

**Screen Editor (~400 bytes):**
- Cursor rendering (blinking block)
- Cursor movement (left, right, up, down, home)
- Screen clear
- Insert/delete line (scrolling)
- BASIC input line handling (buffer at $0200, 80 chars)

**IRQ Handler (~150 bytes):**
- Scan keyboard on timer interrupt
- Update cursor blink
- Return to BASIC

**Total: ~1.2-1.5 KB of code + ~512 bytes of lookup tables = ~2 KB**

### ROM Memory Map (PET 2001)

| Address | Size | Content |
|---|---|---|
| $C000-$CFFF | 4 KB | BASIC ROM (Microsoft BASIC 1.0, from msbasic) |
| $D000-$DFFF | 4 KB | Editor + KERNAL ROM (custom) |
| $E000-$E7FF | 2 KB | Editor extensions (merged into $D000) |
| $E800-$EFFF | 2 KB | I/O space + unused (not a ROM) |
| $F000-$FFFF | 4 KB | Kernel ROM (merged into $D000) |
| $8000-$83FF | 1 KB | Screen memory (RAM) |

Actually, the original PET ROM layout is:
- $C000-$DFFF: BASIC (8 KB, two 4K ROMs)
- $E000-$E7FF: Screen Editor (2 KB)
- $F000-$FFFF: Kernel (4 KB)

For our minimal build, we can compress this:
- $C000-$DFFF: BASIC 1.0 ROM (8 KB, from msbasic)
- $E000-$E7FF: Custom Editor (2 KB max, but we'll pad to 2K)
- $F000-$FFFF: Custom Kernel (4 KB, but we'll only use ~1 KB)

For 4032/8032 (BASIC 4.0):
- $C000-$DFFF: BASIC 4.0 ROM (8 KB)
- $E000-$E7FF: Editor (2 KB)
- $F000-$F7FF: Kernel (2 KB)
- $F800-$FFFF: unused

## Sourcing Steps

### Step 1: Clone and build mist64/msbasic
```bash
git clone https://github.com/mist64/msbasic.git /tmp/msbasic
cd /tmp/msbasic
# Need cc65 assembler
sudo apt install cc65
make
# Output: bin/commodore/basic1, bin/commodore/basic2
```

### Step 2: Extract PETSCII chargen
From VICE source (which distributes chargen in its ROM set with documentation):
```bash
# VICE's chargen file is called "chargen" or "chargen.901447-10"
# We can either extract from VICE's ROM set or recreate from scratch
# PETSCII bitmap data is published in multiple places online
```

### Step 3: Write minimal KERNAL + Editor
6502 assembly source code custom-written. Files:
- `roms/src/kernal.s` — Reset, IRQ, CHROUT, CHRIN
- `roms/src/editor.s` — Cursor management, screen scrolling, input line buffer
- `roms/src/keyboard.s` — PET keyboard matrix scan + PETSCII mapping
- `roms/src/Makefile` — Build with cc65 or xa65 assembler

### Step 4: Assemble ROM images
```bash
cd roms/src && make
# Output: roms/open/kernal.bin (4 KB)
#         roms/open/editor.bin (2 KB)
#         roms/open/chargen.bin (2 KB, PETSCII chars)
```

## VICE Integration

### Step 5: Configure VICE as static library
- Use VICE 3.10 source as git submodule
- Configure with `--enable-embedded EMUTYPE=xpet --with-sdl2`
- Build just the core library (libxpet.a)
- Patch: point ROM file paths to our roms/open/ directory

### Step 6: Bridge layer
- Replace the placeholder PET screen in Phase 1 launcher with VICE's video output
- Route SDL2 keyboard events → VICE's keyboard matrix
- Route VICE's CRT output texture → our SDL2 renderer
- .prg loader: inject bytes into VICE's RAM at load address, call BASIC's LOAD/RUN

### Step 7: Verify on all 3 models
- PET 2001 (BASIC 1.0, 8K RAM, TTL video)
- CBM 4032 (BASIC 4.0, 32K RAM, CRTC video)
- CBM 8032 (BASIC 4.0, 32K RAM, 80-column CRTC video)

## Game Loading Flow (how it actually works now)
1. User clicks "Miner" or drops a .prg
2. Launcher reads the .prg file, extracts first 2 bytes (load address, LE)
3. Launcher calls VICE API to inject bytes into PET RAM at that address
4. Launcher sets up simulated keyboard buffer with `RUN\n`
5. VICE PET executes RUN → program starts
6. Screen shows the game running

## Tasks for Codex

### Task 1: Source & build BASIC ROM
- Clone mist64/msbasic
- Install cc65 assembler
- Build, extract BASIC 1.0 and BASIC 2 ROM images
- Copy to roms/open/

### Task 2: Write minimal KERNAL
- Reset vector → init hardware → jump to BASIC cold start
- CHROUT: write char to screen at cursor
- CHRIN: scan keyboard matrix, return PETSCII
- Stub out IEEE-488 and tape (return immediately)
- Build with ca65/xa65

### Task 3: Write minimal Screen Editor
- Cursor management (blink, position tracking)
- Screen scrolling
- Line input buffer ($0200)
- CLR, HOME, cursor control codes

### Task 4: Recreate PETSCII Character ROM
- Generate 256 character bitmaps (8×8 each)
- Include alphanumeric, PETSCII graphics characters
- Output as binary file

### Task 5: VICE integration
- Add VICE 3.10 as submodule
- Configure for PET-only static lib build
- Create bridge layer (video output → SDL2 texture, keyboard → key matrix)
- Wire .prg loader to VICE memory injection
- Replace placeholder screen with real emulated display

## Verification
1. Emulator boots to `*** COMMODORE BASIC *** READY.` prompt
2. Keyboard input works (type BASIC commands, they appear on screen)
3. `PRINT "HELLO"` works and displays correctly
4. Each of the 5 games loads and runs when its chip is clicked
5. Drag-and-drop .prg loads and runs
6. .prg files load correctly on the correct PET model (auto-switch)
7. BASIC prompt returns after game exits or reset

## ROM/Code status summary

| Component | Size | Source | Legal |
|---|---|---|---|
| BASIC 1.0/2.0 | 8 KB each | mist64/msbasic (BSD) | ✅ Ship freely |
| Character ROM | 2 KB | Recreated from public bitmap data | ✅ Ship freely |
| KERNAL | 2 KB | Custom-written 6502 asm | ✅ Own code |
| Screen Editor | 2 KB | Custom-written 6502 asm | ✅ Own code |
| **Total** | **~14 KB** | | **100% open** |
