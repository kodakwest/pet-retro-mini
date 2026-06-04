# PET Retro Mini — Creature Comforts: Follow-Up to Session 4044508094963981381

## What We Have
The first pass delivered a solid mockup: dark window, CRT screen with BASIC prompt, 5 game chips, toast notifications, drag-and-drop import. Good bones.

## What's Missing (Creature Comforts)

Modern users who don't know 1977 BASIC need help. Here's what to add to the mockup. Think of it as "old school assisted UX" — guide without getting in the way.

### 1. Command Drawer (Slide-Out Right Panel)

A thin pull-tab on the right edge of the window. Click it or press `Ctrl+Shift+C` to slide out a panel (~260px wide) over the display area.

**Content:**

**Section: Quick BASIC Commands**
Large, readable buttons that simulate typing into the PET:
- `RUN` — executes current program
- `LIST` — lists program lines
- `LOAD` — shows load prompt
- `SAVE` — shows save prompt  
- `SYS` — call machine code (with address input)
- `POKE` — memory poke (with addr,val inputs)
- `PRINT` — prints text
- `CLEAR` — clears screen (CLR/HOME)

**Section: Type to PET**
A text input field at the top of the drawer. Whatever the user types here gets "typed" into the emulated PET's keyboard buffer. Big textarea with a SEND button. This is the "I don't know PET BASIC so I'll just type" bridge.

**Section: PETSCII Quick Pick**
A 4×8 grid of common PETSCII graphics characters (hearts, cards, shapes, lines). Click one and it sends that keystroke to the PET. Each cell shows the character and its CHR$ code. This is the "I want to draw something" bridge.

**Section: Shortcuts**
A compact reference list:
```
Ctrl+O    Load Program
Ctrl+R    Reset
F11        Fullscreen
Ctrl+Shift+C  Toggle Command Drawer
1-5        Launch Game 1-5
```

**Visual style:**
- Dark surface matching the panel bg (`#222222`)
- Thin 1px left border (`#3a3a3a`)
- Smooth slide animation (~200ms ease-out)
- Semi-transparent backdrop overlay when open
- Close button (✕) in top-right corner
- Drag-handle tab on the edge when closed

### 2. Search/Filter Bar Above Game Chips

Between the PET display and the game chips, a compact search bar:
- Placeholder: "Search games or type a command..."
- Filters the 5 game chips in real-time as you type
- Can also type BASIC commands here (ENTER sends them to PET)
- Minimal height (~36px), glass-morphism background

### 3. Recent Programs List

In the File menu, a "Recent" submenu showing the last 5 loaded .prg files. When empty, show "No recent programs" in gray.

### 4. Screenshot Button

A small camera icon in the status bar (right side, next to speed). Click it:
- Flashes the screen white for 100ms (shutter effect)
- Toast: "Screenshot saved to Desktop/PET-Retro-Mini-Screenshots/"
- Adds a satisfying retro camera shutter click feel

### 5. Keyboard Shortcut Overlay

Press `Ctrl+/` or `?` to show a centered modal overlay with all keyboard shortcuts listed in a clean grid:
- Grouped by category: General, Games, Emulation, Display
- Dismiss with Escape or click outside

### Design Integration

All these additions should feel like they belong in the original design:
- Same color tokens (`#1a1a1a`, `#222222`, `#2a2a2a`, `#33ff33`, `#e0e0e0`)
- Same font (Inter for UI, Courier New for PET display)
- Same rounded-rect aesthetic
- No modal dialogs for routine actions (except the shortcut overlay)
- The command drawer is the main "deep" feature — everything else is shallow UI

## Deliverable

Update the existing `pet-retro-mini-mockup.html` to include all of the above. The file should still be self-contained, interactive, and demonstrate the new features working via JavaScript simulation.

Key interactions to mock:
1. Click the right-edge tab → drawer slides open with BASIC commands
2. Click a BASIC command button (e.g., RUN) → toast shows "RUN sent to PET" + screen updates
3. Type in the search bar → game chips filter
4. Click camera icon in status bar → flash + toast
5. Press Ctrl+/ (or click Help > Shortcuts) → overlay appears
6. File > Recent shows submenu items

DO NOT remove anything from the existing mockup. Add to it.
