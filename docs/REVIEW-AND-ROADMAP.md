# PET Retro Mini — Full Repo Review & Roadmap (June 2026)

This review was produced by building the emulator core headless and actually booting it,
not just reading the code. Verdict up front:

**The app "never quite worked right" because the emulated machine never boots.**
The launcher UI runs fine, but behind it the PET dies during reset, the five bundled
games don't exist in the repo, and the keyboard/interrupt hardware model is an invented
dialect that only the bundled custom ROMs speak. Each section below has the evidence
and the fix.

---

## 1. Root causes (verified by execution)

### 1.1 The bundled BASIC ROM images are broken — boot dies after one instruction

A headless boot test (`tests/boot_test.c`) shows: reset vector fetches `$F000`, the open
kernal runs, clears the screen — then jumps to `BASIC_COLD = $C000` and executes exactly
**one** instruction in BASIC ROM before wedging in a BRK loop in zeroed RAM
(PC oscillating `$2020..$F18F`). The screen never shows `### COMMODORE BASIC ###`,
`READY.`, or anything else. Keyboard input does nothing.

Two independent defects compound:

1. **`$C000` is not the BASIC cold-start entry.** The first bytes of `basic2.bin` are
   `40 c7 57 c6 1f cc ...` — a pointer/dispatch table. Byte `$40` is `RTI`, so
   `JMP $C000` executes `RTI` into garbage. The real cold-start entry is elsewhere in
   the image (a `JMP $C389` is visible near the `BYTES FREE` banner code).
2. **The committed images are the wrong size.** `roms/src/Makefile` asserts
   `basic1.bin`/`basic2.bin` must be exactly **8192** bytes. The committed files are
   **8673** and **8670** bytes. `pet.c` truncates them into an 8 KB buffer, cutting off
   the tail of the image — which is where msbasic places the init code and banner.
   The commit message "all 5 ROMs compiled and verified" cannot be true; the repo's own
   size checks would have failed.

**Fix:** rebuild BASIC from msbasic with a PET memory config that produces a true
`$C000–$DFFF` 8 KB image (or relocate/pad deliberately), determine the actual cold-start
address from the link map, and point `BASIC_COLD` in `kernal.s` at it. Then make the
boot test (see §6) a CI gate so this class of failure can never land silently again.

### 1.2 All five bundled games are missing

`games/` contains only `manifest.json`. Every `file:` path in it
(`games/zork1/zork1.prg`, `games/miner.prg`, …) points at a file that does not exist.
Clicking any game chip can only ever produce "failed: Could not open PRG". The product's
headline feature — "5 iconic games ready to play" — has never been in the repo.

**Fix:** add a `scripts/fetch-games.sh` (with checksums and license provenance per
title) or commit the .prg files directly where licenses allow.

### 1.3 Zork I is architecturally impossible in the current design

Separate from the missing file: Zork I's story file is ~84 KB of Z-machine data. A PET
has at most 32 KB of RAM; the real Infocom PET release paged data from an IEEE-488 disk
drive at runtime. This emulator has **no disk/IEEE-488 support at all**, so no single
`.prg` can ever contain a working Zork I. (The MIT relicensing of the Zork sources is
real, but doesn't help with this.)

**Fix (choose one):**
- Replace Zork I in the MVP with a feasible flagship (a self-contained BASIC or
  machine-code adventure that fits in 32 KB), and move Zork to a stretch phase behind
  virtual-disk support; or
- Implement a kernal LOAD/disk-sector trap backed by a host-side file (the clean
  long-term answer, see roadmap Phase 3).

### 1.4 The hardware model is self-consistent but not a PET

The emulator and the custom kernal agree with each other and disagree with real
hardware. This silently destroys the *other* headline feature, "import any .prg":

- **Keyboard row select:** real PETs put a row *number* on PIA1 PA0–3 through a 74145
  BCD decoder. The emulator (`read_keyboard_port`) and `keyboard.s` (`row_masks:
  .byte $fe,$fd,…`) use one-low-bit-per-row masks instead — and can only address 8 of
  10 rows. Any imported machine-code game that scans the keyboard directly (most of
  them) reads garbage.
- **Keyboard matrix layout is invented.** The row/col assignments in `pet.c`/
  `emulator_runtime.c` (`'1'` at 0,0; `'Q'` at 1,0; …) match no real PET matrix
  (graphics or business). Imported games that decode the matrix themselves get wrong
  keys even after the decoder fix.
- **No shift key exists anywhere** in the matrix maps, so `"`, `(`, `)`, `$` etc.
  cannot be typed. You cannot even enter `PRINT "HELLO"` — on a machine whose
  primary interface is BASIC.
- **VIA 6522 semantics are wrong.** Writing the IER on a real 6522 uses bit 7 as a
  set/clear control; `kernal.s` writes `$7F`, which on real hardware *disables* all
  interrupts, but this emulator treats the register as plain storage, so it "works".
  Timers don't count, IFR is never set by hardware, and the jiffy-clock IRQ is
  hard-wired in `pet_execute` to fire whenever `(i & 4095) == 0` — i.e. twice per
  5000-instruction slice (~120 Hz, irregular) instead of 60 Hz.
- **No PIA1 CB1 retrace interrupt** — the mechanism original Commodore kernals use for
  the 60 Hz jiffy/keyscan IRQ. So the documented "user-supplied original ROMs" path
  (AGENTS.md) cannot work even if a loading path for them existed — and it doesn't:
  `pet_load_roms` only knows the open-ROM filenames.

**Fix:** make the *hardware* honest (BCD row decode, real graphics-keyboard matrix,
shift key, 6522 IER/IFR set/clear + counting T1, CB1 retrace IRQ at 60 Hz), then adapt
the open kernal to the real interface. The open ROMs should be a software stand-in,
never a different machine.

---

## 2. Emulator core — additional defects

| # | Severity | Issue | Location |
|---|---|---|---|
| E1 | High | No cycle accounting: `cycles++` once per *instruction*; `pet_execute` budgets instructions, not cycles. All timing-sensitive software runs at the wrong, data-dependent speed. Add a per-opcode cycle table and budget ~16 667 cycles per 60 Hz frame. | `cpu.c`, `pet.c:205` |
| E2 | High | Undocumented opcodes decoded as **1-byte** NOPs. Many real NOPs are 2–3 bytes (`$04`, `$0C`, `$14`, `$1C`, …); treating them as 1 byte desynchronizes the instruction stream and turns operands into opcodes. Implement correct lengths at minimum. | `cpu.c:269` |
| E3 | Med | Decimal-mode SBC is implemented as BCD-ADC of the one's complement — not how 6502 BCD subtraction works. Run the Klaus Dormann functional test suite in CI to pin the whole core down. | `cpu.c:76` |
| E4 | Med | Model selection is cosmetic: every "model" has full RAM to `$BFFF`, 40×25 screen, same kernal. PET 2001 (8 KB) and 8032 (80-column, different editor, 12 KB BASIC 4 at `$B000`) are not modeled; the 8 KB `basic_rom` buffer can't even hold BASIC 4. Either model these or stop offering the selector. | `pet.h`, `pet.c` |
| E5 | Med | Inverse video: real hardware inverts the glyph of `code & 0x7f` when bit 7 is set; the renderer instead indexes 256 distinct glyphs. Works only with the custom chargen layout, breaks real chargen ROMs. | `render.c:20` |
| E6 | Low | `cpu_init` sets `sp = 0xff` but `cpu_reset` sets `0xfd`; harmless today, confusing later. | `cpu.c:97,103` |
| E7 | Low | `pet_load_prg` pokes BASIC pointers at `$28–$2F` — correct for BASIC 2/4 but BASIC 1 (the "PET 2001" choice) uses a different zero page, and the msbasic build's layout should be verified, not assumed. | `pet.c:293` |
| E8 | Low | The ASCII→matrix `key_queue` injection in `read_keyboard_port` only releases a key when the scan happens to hit the queued row while `out == 0xff`; fast typists and repeated characters will drop. Replace with a timed press/release driven from the frame loop. | `pet.c:144` |

---

## 3. Launcher — bugs

| # | Severity | Issue | Location |
|---|---|---|---|
| L1 | **Critical** | `render_screen` allocates `uint32_t pixels[640*400]` = **1 024 000 bytes on the stack**. MSVC's default stack is 1 MB; MinGW's is 2 MB. This is an instant or latent stack overflow on the only target platform. Use `SDL_LockTexture` (zero-copy) or a static/heap buffer. | `render.c:13` |
| L2 | High | Emulation speed is coupled to display refresh: `pet_runtime_frame()` runs 5000 instructions per *render*. On a 144 Hz monitor with vsync the PET runs 2.4× fast; on a slow machine it runs slow. Decouple: accumulate wall-clock time, execute a cycle budget, render whatever is current. | `emulator_runtime.c:77`, `main.c:708` |
| L3 | High | File-menu index math is wrong when there are no recent files: the items are `[Load, "Recent: none", Reset, Exit]`, but the conditions make "Recent: none" trigger Reset, and **clicking Reset both resets and quits the app** (`index==2` satisfies both the reset clause and `recent_count+2`). Replace positional arithmetic with explicit per-item action IDs built alongside the labels. | `main.c:541-555` |
| L4 | High | The speed setting (100/200/400%) is saved to config and drawn in the status bar but **never used** — nothing reads `speed_index` into the execution budget. | `main.c`, `emulator_runtime.c` |
| L5 | Med | All asset/config paths (`roms/open`, `games/manifest.json`, `config.ini`) are CWD-relative. Launched from a shortcut, a different directory, or "Open with", the app finds nothing. Resolve assets against `SDL_GetBasePath()` and put `config.ini` in `SDL_GetPrefPath()`. | `main.c:671,672,696` |
| L6 | Med | Every key event is forwarded to the PET, including the Ctrl+O/Ctrl+R chords and F11 — so opening a file also types `o` into BASIC. Swallow app shortcuts before forwarding; ideally only forward keys when the screen has focus. | `main.c:582-599` |
| L7 | Low | The hand-rolled JSON scanner (`strchr('{')`/`strchr('}')`) breaks on any `}` or escaped quote inside a string. Since `manifest_defaults()` already hard-codes the identical five games, the JSON file is pure liability — delete it, or keep it and parse it with a real minimal parser. | `game_launcher.c:91` |
| L8 | Low | `set_running_screen`/`screen_line_*` fake-screen text is dead code now that the runtime always returns a texture; remove it. | `main.c:209-221,483-490` |
| L9 | Low | No minimum window size; shrinking the window makes `screen_rect`/chip widths go negative or overlap. `SDL_SetWindowMinimumSize` + clamp. | `main.c:234` |
| L10 | Low | CRT effect = 56 outlined rects + ~100 scanline fills per frame through the (possibly software) renderer, and `draw_text` issues one `FillRect` per pixel. Pre-render the glow/scanlines once to a texture and bake the font into a texture atlas. | `main.c:464-498,173` |

---

## 4. Repo hygiene & documentation drift

- **Stale VICE submodule:** `.gitmodules` still references `src/vice` (the VICE svn
  mirror — enormous), but the directory is gone. Anyone running
  `git submodule update --init` pays for it. Delete `.gitmodules`.
- **Phantom docs:** README links `docs/BUILDING.md` and `docs/GAMES.md`; AGENTS.md
  promises `docs/SPEC.md`, `docs/IMPORTING_GAMES.md`, `CMakeLists.txt`, `CHANGELOG.md`,
  and `tests/`. None exist. Write them or stop referencing them — AGENTS.md is the
  agent's map and it currently lies about the territory.
- **License claims:** README says "SDL2 (MIT) statically linked" — SDL2 is **zlib**
  licensed, not MIT. Also document per-game provenance in `docs/GAMES.md` when the
  games actually land (the "Cursor #15/#19 public domain" claims deserve a citation).
- **`roms/open/.gitkeep`** is pointless now that real files live there.
- **No CI of any kind.** See §6.

---

## 5. What's already good

Credit where due — the bones are worth building on:

- The 6502 core's documented-opcode coverage is complete and the code is compact and
  readable; with a cycle table and the Dormann suite it can become trustworthy.
- The launcher is dependency-light (SDL2 only), idiomatic C99, and the UI matches
  DESIGN.md closely: menus, chips, toasts, drag-and-drop, recents, config persistence
  all exist and are wired.
- Building ROMs from source (ca65 + msbasic) with size assertions is exactly the right
  open-ROM strategy — the failure was process (committing unverified bins), not design.
- `pet_load_prg` does proper bounds checking; the config/manifest string handling is
  consistently length-guarded. No memory-safety red flags outside L1.

---

## 6. Testing & CI (currently: none)

Add, in this order:

1. **Headless boot test** (`tests/boot_test.c`, included with this review): builds
   `cpu.c + pet.c` with no SDL, boots the machine, runs ~1.2 M instructions, and
   asserts `READY.` appears in screen RAM. **This test is red today** — it is the
   executable form of finding 1.1, and turns green when Phase 0 is done.
2. **CPU conformance:** run the Klaus Dormann 6502 functional test binary in CI
   (it needs only RAM + the CPU core; minutes to wire up).
3. **Per-game smoke tests:** for each bundled .prg — load, run N frames, assert the
   screen changed and the CPU isn't in a BRK loop.
4. **GitHub Actions:** Linux build + tests on every push; `x86_64-w64-mingw32` cross
   build producing the actual `pet-retro-mini.exe` artifact; release workflow zipping
   exe + roms + games on tags.

---

## 7. Roadmap

### Phase 0 — Make it boot (the credibility phase)
1. Rebuild `basic1.bin`/`basic2.bin` to exact 8 KB with a known cold-start entry; fix
   `BASIC_COLD`; commit only bins that pass the size checks.
2. Land the boot test + CI so green CI ≡ "the PET reaches READY."
3. Fix L1 (stack overflow) and L3 (reset-quits-app).
4. Source and commit the five game .prg files — replacing Zork I with a feasible
   flagship (see 1.3).

**Exit criterion:** fresh clone → `make` → pick any game → it plays. On Windows.

### Phase 1 — Make it correct
- Real keyboard: BCD row decode, authentic graphics-PET matrix, shift handling,
  timed key injection (fixes both typing and imported games).
- Real timing: per-opcode cycles, 16 667-cycle frame budget decoupled from vsync,
  wire the speed setting, 60 Hz CB1 retrace IRQ + honest 6522 (IER/IFR semantics,
  counting T1).
- Dormann suite green in CI.
- Path resolution via `SDL_GetBasePath`/`SDL_GetPrefPath` (fixes double-click launch).
- Original-ROM support: `roms/original/` drop-in with auto-detect — now feasible
  because the hardware is honest. This single feature multiplies compatibility.

**Exit criterion:** original Commodore ROMs boot; a random PD machine-code .prg from
the internet runs with working keyboard at correct speed.

### Phase 2 — Make it complete
- **Sound** (CB2 shift-register square wave → SDL audio). Games feel broken without it.
- Model fidelity: per-model RAM sizes, BASIC 4 at `$B000` (grow the ROM buffer),
  80-column 8032 with CRTC-derived geometry.
- **Save states** — `pet_t` is already one flat struct; this is ~50 lines for a
  huge UX win, and it's the foundation for rewind.
- Kernal LOAD trap → host file picker, so multi-part games load their second stage;
  grows later into virtual IEEE-488 disk (which unlocks Zork properly).
- Creature comforts already spec'd in `docs/CREATURE-COMFORTS.md`: the "Type to PET"
  drawer is nearly free — `pet_queue_text()` already exists.

### Phase 3 — Make it exceptional
- CRT as prebaked textures or a real shader; amber/white phosphor modes (DESIGN.md
  already reserves the palette).
- Rewind (ring buffer of save states — the machine state is ~70 KB, so seconds of
  rewind cost nothing).
- Joystick→matrix mapping, per-game input presets in the manifest.
- Screenshot/GIF capture, keyboard-reference overlay, About box with version.
- Signed, reproducible release builds from CI; auto-generated CHANGELOG.

### Process rule that falls out of all of this
Every "it works" claim gets an executable witness. The repo's history shows two commits
declaring victory ("compiled and verified", "real 6502 CPU + PET emulator core") over a
machine that has never printed `READY.` — the boot test in CI is the structural fix for
that, not better intentions.
