# PET Retro Mini — UI/UX Design Brief

## Overview

Design the user experience for a compact Windows desktop app that emulates the Commodore PET computer. Target binary: ~3–5 MB standalone .exe. Implementation in C with SDL2 (no HTML/CSS/JS runtime).

This brief is for **UX/visual concept design only** — mockups, layout exploration, visual language, user flows. Implementation will be in C/SDL2.

## Design Goals

1. **Feels native to Windows** — not a web-wrapper, not a skinned browser. Should sit comfortably among native Win32 apps. Familiar window chrome, menus, dialogs.
2. **Old-school assisted UX** — guides the user without manuals. First-launch should immediately make sense. Think retro appliance — warm, approachable, obvious. Helpful hints, clear affordances.
3. **Subtle modern flare** — Not a green-screen terminal replica. Modern typography, smooth edges, thoughtful spacing, micro-interactions. The PET is the *subject*, not the skin.

## User Personas

| Persona | Needs |
|---|---|
| **Retro gamer (primary)** | Launch Zork fast. Import obscure .prg files. CRT glow or GTFO. |
| **Curious modern user** | Wants to see what a PET is. Needs guidance. Will bounce if confused. |
| **Museum/kiosk mode** | Boots to game picker. No settings exposed. Auto-reset after exit. |

## UI Structure (Concept)

### Window Frame
- Standard Windows window with title bar, min/max/close
- Title: "PET Retro Mini" (no version in title)
- Default size: 2× native PET resolution (640×400 for 40-col, 640×400 for 80-col)
- Resizable with aspect-ratio lock toggle
- Dark gray window background (#1a1a1a or similar moody dark, not pure black)

### Main View (Three Zones, Vertical Stack)

**1. Display area (dominant, ~75% of window)**
- The emulated PET screen, centered
- Slightly inset from window edges with subtle shadow/border
- Green phosphor glow effect (toggleable)
- Scanline overlay (toggleable)
- When no game is running: shows the PET BASIC prompt `READY.` with blinking cursor — alive, waiting.

**2. Game launcher bar (thin strip, below display)**
- 5 equally-spaced game buttons with icon + name
- Icons: tiny (24×24) pixel-art-style representations of each game
- Button style: dark glassy chips with hover lift
- Active game highlighted
- + button at the end: "Load .prg..." with file icon
- "Drag .prg here" ghost text when no file is being dragged

**3. Status bar (one line, very bottom)**
- Left: Model name (PET 2001 / CBM 4032 / CBM 8032)
- Center: Game title (or "BASIC" if no game loaded)
- Right: Speed indicator (100% / 200% / 400%)

### Menu Bar
```
File ─┬─ Load Program...    Ctrl+O
      ├─ Quick Load Recent  ► [last 5 prgs]
      ├─ Reset Machine      Ctrl+R
      ├──────────────
      └─ Exit               Alt+F4

Machine ─┬─ PET 2001 (8K)
          ├─ CBM 4032 (32K)
          ├─ CBM 8032 (32K)
          └─────────────
              Speed ─┬─ 100% (Normal)
                     ├─ 200% (Fast)
                     └─ 400% (Turbo)

View ─┬─ CRT Effect    [✓]
      ├─ Fullscreen    F11
      └─ Aspect Ratio Lock [✓]

Help ─┬─ About PET Retro Mini
      └─ PET Keyboard Reference
```

### First-Launch Experience

First time the app runs:
1. Show the PET screen booting up (POST memory test → cursor appears)
2. Brief overlay toast: **"Welcome to PET Retro Mini. Pick a game below, or just explore BASIC."**
3. Toast fades after 5 seconds or click
4. No modal dialog. No registry setup. No first-run wizard.

### Game Launch Flow

1. User clicks game button (e.g., "Miner")
2. Button animates to "loading" state (subtle pulse)
3. PET screen shows BASIC prompt
4. Program LOADs visibly (cursor blinks, "SEARCHING" text appears if tape, or instant for bundled)
5. Programs RUNs automatically
6. Status bar updates to show game title
7. If game needs specific PET model, auto-switch with brief toast: *"Switching to PET 2001 for Miner"*

### Game Import Flow

1. User drags .prg file onto window (or File > Open)
2. Border highlights green to indicate drop zone
3. Brief parse animation (file header detected)
4. Toast: *"Loaded 'game.prg' at $1C00. Running..."*
5. Game launches
6. Added to Quick Load Recent list

### CRT Effect Visual Language

- Green phosphor: `#33ff33` on `#0a0a0a` with ~15% brightness decay from center
- Scanlines: alternating full-brightness and 70%-brightness horizontal lines
- Optional subtle bloom (post-processing glow on bright pixels)
- Slight screen curvature (barrel distortion, subtle)
- Configuration: On/Off toggle in View menu. No slider — simple.

## Visual Design Tokens (for mockup)

| Token | Value | Usage |
|---|---|---|
| Background | `#1a1a1a` | Window fill behind PET screen |
| Panel bg | `#222222` | Launcher bar, status bar |
| Surface | `#2a2a2a` | Buttons, dropdowns |
| Text primary | `#e0e0e0` | Labels, menu text |
| Text muted | `#888888` | Status info, hints |
| Accent (green) | `#33ff33` | CRT phosphor, active state |
| Accent (amber) | `#ffb000` | Alternative warm phosphor mode |
| Border | `#3a3a3a` | Subtle separators |
| Error | `#ff4444` | ROM missing, crash |

Font: **Inter** for UI (clean, modern, readable at small sizes). System-ui fallback.

Font: **PETSCII recreation** for the emulated display area (bundled as .ttf or rendered via bitmaps in SDL2 — note that a real TTF PETSCII font exists: "Petscii" by Slak[.] This is a design reference).

## Key UX Principles

1. **No dead PET.** The display is always alive — BASIC prompt or game running. Never a blank/black screen.
2. **No modal dialogs for routine actions.** Game loading, model switching, file import — all non-blocking toast notifications.
3. **Immediate boot.** Startup to playable state in under 2 seconds.
4. **Keyboard-first.** All menu items have keyboard shortcuts. Import supports Ctrl+O. Fullscreen is F11.
5. **Forgiving.** Wrong model for a game? Auto-detect and switch. Imported .prg crashes? Toaster: *"That program didn't work. Try a different PET model in Machine menu."*
6. **Good defaults.** CRT on. Speed 100%. Model auto (starts on PET 2001, switches per game).

## Priority Order for Design Exploration

1. **Main window layout** — display + launcher bar + status bar. Get the proportions right.
2. **Game launcher bar** — 5 game chips + import button. Icon style, hover states, selected state.
3. **Menu bar** — native-feeling menus with keyboard shortcut hints
4. **CRT effect on/off toggle** — visual before/after for the mockup
5. **First-launch toast** — welcome message design
6. **Import flow** — drag-and-drop visual feedback

## Deliverable Format

Since this app is implemented in C/SDL2 (not HTML/CSS), your mockup should be:
- **Interactive HTML mockup** — self-contained .html file(s) showing the window layout, color scheme, button states, and CRT display. Use CSS to simulate the window chrome. No backend needed. Clickable states for game buttons, menus (can use HTML menus as proxies).
- **Accompanying DESIGN.md** — describes the component hierarchy, state machine for each UI element, color tokens, and animation specs. This is what gets implemented in C/SDL2.
- **Two screenshots/mockups** at minimum: (a) idle state with BASIC prompt, (b) game running (e.g., Miner playing)

## Constraints

- **No web framework.** Pure HTML/CSS/JS for the mockup. Dark theme. Single HTML file.
- **No modal dialogs** in the final design except: About dialog and Keyboard Reference.
- **The PET screen itself cannot be restyled** — it's an emulated hardware display. The CRT effect is a post-process overlay, not a reskin.
- **Windows-only.** No macOS/Linux mockups needed. Use Windows window chrome conventions (close/maximize/minimize in upper-right).
- **SDL2 is the implementation target** — no Win32 API, no UWP, no WinUI. Design for what SDL2 can render (surfaces, textures, filled rects, blits).
