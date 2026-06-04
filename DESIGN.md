# PET Retro Mini — UI/UX Design Specifications

This document outlines the visual design, interaction states, and implementation notes for the PET Retro Mini Windows emulator frontend.

## 1. Visual Language & Tokens

The application follows a dark-themed, "retro appliance" aesthetic, aiming to feel like a modern, clean host for a vintage machine.

### Colors

*   **Background** (`#1a1a1a`): Window fill behind the PET screen. Provides a dark, moody environment.
*   **Panel Background** (`#222222`): Used for the launcher bar and status bar.
*   **Surface** (`#2a2a2a`): Used for interactive elements like game buttons, dropdowns, and toast backgrounds.
*   **Text Primary** (`#e0e0e0`): Main text color for labels, menu text, and active statuses.
*   **Text Muted** (`#888888`): Used for secondary status info and the "Load .prg..." button.
*   **Accent Green** (`#33ff33`): The core CRT phosphor color, cursor color, and active state highlight.
*   **Accent Amber** (`#ffb000`): Alternative warm phosphor mode color (if implemented).
*   **Border** (`#3a3a3a`): Subtle separators between panels.
*   **Error** (`#ff4444`): For error states like missing ROMs or crashes.

### Typography

*   **UI Font**: Inter (clean, modern, sans-serif). Fallbacks to system-ui.
*   **PET Screen Font**: A custom PETSCII font or rendered bitmap equivalent for accurate representation.

## 2. Component Hierarchy

The UI is divided into three primary vertical zones stacked within the main window frame.

1.  **Window Frame (Chrome)**
    *   Standard Windows dark mode title bar (Minimize, Maximize, Close).
    *   Menu Bar (File, Machine, View, Help).
2.  **Main View**
    *   **Display Area**: The core emulated screen. Includes the CRT overlay (scanlines, phosphor glow).
    *   **Toast Notifications**: Absolute positioned overlays within the display area for transient feedback.
3.  **Launcher Bar**
    *   5 x Game Chips (Zork I, Miner, Lunar Lander, Dungeon, PET Invaders).
    *   1 x "Load .prg..." Button.
4.  **Status Bar**
    *   Left: Machine Model (e.g., PET 2001).
    *   Center: Current Game or "BASIC".
    *   Right: Emulation Speed (e.g., 100%).

## 3. Interaction States & State Machines

### Game Chips

*   **Idle**: Surface color (`#2a2a2a`), primary text color, subtle border.
*   **Hover**: Lighter surface color (`#333333`), slight upward translation (`-2px`), subtle drop shadow.
*   **Active (Selected)**: Light green background tint (`rgba(51, 255, 51, 0.1)`), accent green border, accent green text.

### Toast Notifications

*   **State Machine**: `Hidden` -> `Visible` (on trigger) -> `Hidden` (after timeout, default 4s-5s).
*   **Animation**: Fade-in and fade-out opacity transitions.

### Import Flow (Drag & Drop)

*   **Drag Over**: Window frame highlights with a 2px inward accent green border (`box-shadow: inset`).
*   **Drop**: Highlight is removed. A toast notification appears ("Loaded '[filename]' at $1C00. Running..."). Emulation state resets to run the newly loaded program. Active game chips are deselected.

## 4. Animation Specifications

*   **Game Chip Hover**: `transform: translateY(-2px)`, `box-shadow` expansion, `background-color` change. Transition duration: `0.2s ease`.
*   **Cursor Blink**: Step animation alternating between 100% and 0% opacity every 0.5s (1s total cycle).
*   **Toast Visibility**: `opacity` change from 0 to 1 and vice-versa. Transition duration: `0.5s ease`.

## 5. Keyboard Shortcuts

*   `Ctrl+O`: File -> Load Program...
*   `Ctrl+R`: File -> Reset Machine
*   `Alt+F4`: File -> Exit
*   `F11`: View -> Fullscreen

## 6. SDL2 Implementation Notes

*   **Window Management**: Use `SDL_CreateWindow` with the `SDL_WINDOW_RESIZABLE` flag if maintaining aspect ratio, or specific fixed logical sizes to guarantee crisp integer scaling. The custom window chrome (if fully replacing standard OS chrome) requires handling `SDL_WINDOW_BORDERLESS` and implementing custom hit-testing (`SDL_SetWindowHitTest`). For a simpler implementation, standard OS dark mode borders are acceptable.
*   **Rendering Architecture**:
    *   Use `SDL_Renderer` for the UI elements (rectangles, text rendering with `SDL_ttf` or bitmap fonts).
    *   The emulated PET screen should be rendered to an `SDL_Texture` (Target Texture).
*   **CRT Effect**:
    *   **Simple approach**: Render the PET screen texture. Overlay a semi-transparent scanline texture. Overlay a semi-transparent radial gradient texture for the phosphor glow.
    *   **Advanced approach (Recommended if performance allows)**: Use an SDL2 custom shader (if using OpenGL/Direct3D backend directly) or pre-rendered overlay textures combined with blending modes (`SDL_SetTextureBlendMode` using `SDL_BLENDMODE_MOD` or `SDL_BLENDMODE_ADD`).
*   **Event Loop**: Standard `SDL_PollEvent` loop.
    *   Handle `SDL_DROPFILE` events to implement the `.prg` drag-and-drop feature.
    *   Map mouse clicks to the UI rects (Game Chips, Menu Bar) for interaction handling.
*   **UI State**: Maintain simple C structs or enums to track the currently active game index (-1 for none), current toast message and remaining display time, and hover states for elements based on current mouse coordinates (`SDL_GetMouseState`).
