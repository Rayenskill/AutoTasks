# AutoTasks

**Record your repetitive desktop tasks, replay them by recognizing the screen instead of guessing pixel coordinates, and review the steps the engine wasn't sure about.**

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C)
![CMake](https://img.shields.io/badge/build-CMake%203.21%2B-064F8C)
![Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-0078D6)
![Status](https://img.shields.io/badge/status-early%20development-orange)

> ⚠️ **Early development — Phase 0.** The toolchain and build configuration are in place; the engine itself is not implemented yet. See the [roadmap](#roadmap) for what exists and what doesn't.

---

## Why

A lot of routine computer work is just the same clicks in the same order: stepping through an internal tool, running a verification pass, moving data between two windows that don't talk to each other. It's mechanical, and it's error-prone *because* it's boring.

Tools that record and replay clicks already exist. Most share one fragile assumption: they replay **fixed screen coordinates**. Record "click pixel (450, 300)" and the macro breaks the moment a window moves, the resolution changes, or a page loads half a second slower.

AutoTasks takes a different approach on two points:

1. **Image-anchored replay by default.** A step doesn't mean "click here" — it means "find this button and click it." That single change is what lets an automation survive a moved window or a slow load.
2. **A review-and-correct loop.** When the engine acts on a match it wasn't fully sure about (it found the target at 65% confidence against an 80% bar), or when a verification step expected one screen and saw another, it doesn't silently barrel ahead. It flags the step, saves a screenshot, and drops it into a **review queue**: here's the step, here's what it saw — was this right? You confirm, or you correct it and re-capture the target on the spot, and the fix feeds back into the script.

---

## Features

**Planned for v1** — see [status](#roadmap) for what's actually built:

- **Record** mouse and keyboard actions into an editable, structured script
- **Replay** with template matching and smart waits instead of blind `sleep()` calls
- **Library** — organize scripts with tags, search, and last-run status
- **Step editor** — insert, delete, reorder, or re-capture a single step without re-recording
- **Scheduling & hotkeys** — run scripts on a timer or from a global key binding, from the system tray
- **Run history** — per-step logs with failure screenshots, so a failed run can be diagnosed after the fact
- **Review queue** — resolve low-confidence matches and failed verifications, and teach the script to do better

**Deliberately out of scope:** multi-user or cloud sync, and anything that bypasses OS input protections (CAPTCHAs, anti-cheat, secure fields).

---

## Tech stack

| Layer | Technology |
|---|---|
| Language | C++20 |
| Build | CMake 3.21+ · Ninja · MSVC |
| Image matching | OpenCV (`matchTemplate`) |
| Storage | SQLite |
| Serialization | nlohmann/json |
| OS input | Win32 (`SendInput`, `SetWindowsHookEx`) |
| UI | Qt 6 (Widgets) |
| Tests | GoogleTest |
| Tooling | clang-format · clang-tidy · AddressSanitizer |

---

## Getting started

### Prerequisites

In the **Visual Studio Installer**, under *Desktop development with C++*, install:

- **MSVC v143 build tools** — the compiler
- **C++ CMake tools for Windows** — brings CMake and Ninja
- **C++ Clang tools for Windows** — brings clang-format and clang-tidy

Verify from a *Developer PowerShell for VS 2022*:

```powershell
cmake --version; ninja --version; clang-format --version; clang-tidy --version
```

### Build & run

```powershell
git clone <your-repo-url> autotasks
cd autotasks

cmake --preset dev          # configure
cmake --build --preset dev  # build

.\build\dev\bin\autotasks_engine.exe
```

### Available presets

| Preset | Purpose |
|---|---|
| `dev` | Everyday build — Ninja, Debug, warnings as errors |
| `lint` | Same, plus clang-tidy on every translation unit (slower; run before committing) |
| `asan` | AddressSanitizer build, for chasing memory bugs |
| `release` | Optimized (`RelWithDebInfo`) |
| `vs` | Visual Studio IDE solution — note: no `compile_commands.json`, so clang-tidy integration is limited |

---

## Project structure

```
autotasks/
├── .clang-format .clang-tidy .editorconfig .gitignore
├── CMakeLists.txt
├── CMakePresets.json
├── docs/                      Project documentation (see below)
├── engine/                    Recording, replay, image matching
│   ├── include/engine/
│   └── src/
│       └── win32/             All platform-specific code isolated here
├── store/                     SQLite persistence layer
│   ├── include/store/  src/  schema/
├── app/                       Qt UI + orchestrator
├── tests/                     GoogleTest
└── build/                     Generated — gitignored
```

The three top-level source folders mirror the architecture: the **engine** owns OS-level input and nothing else, the **store** is the single source of truth for scripts and run history, and the **app** schedules runs and renders the interface.

**The rule that holds it together:** the UI never synthesizes input. It commands the engine and reads the store. That's what lets live logs, run history, and the review queue all be drawn from one consistent record.

---

## Development

```powershell
# Format every source file in place
cmake --build --preset format

# Build with static analysis
cmake --preset lint
cmake --build --preset lint

# Lint a single file without a full build
clang-tidy engine/src/main.cpp -p build/dev
```

Code rules live in `.clang-format` (formatting), `.clang-tidy` (static analysis and naming), and the warning flags in `CMakeLists.txt` (`/W4 /permissive- /WX`). Warnings fail the build by design — if that blocks a spike, disable it for that build only:

```powershell
cmake --preset dev -D AUTOTASKS_WARNINGS_AS_ERRORS=OFF
```

See [`docs/TOOLING.md`](docs/TOOLING.md) for the full setup, gotchas, and the plan for tightening rules as the project grows.

---

## Roadmap

| Phase | Milestone | Status |
|---|---|---|
| 0 | Toolchain, build config, repo skeleton | ✅ Done |
| 1 | Minimal replay — hardcoded clicks via `SendInput` | ⬜ Not started |
| 2 | Record → save → replay round trip (JSON) | ⬜ Not started |
| 3 | Image matching + smart waits (OpenCV) | ⬜ Not started |
| 4 | SQLite persistence and the real data model | ⬜ Not started |
| 5 | Qt UI shell — library, run now | ⬜ Not started |
| 6 | Step editor and recorder entry point | ⬜ Not started |
| 7 | Run history and logs | ⬜ Not started |
| 8 | Scheduling, system tray, global hotkeys | ⬜ Not started |
| 9 | Review queue and verification steps | ⬜ Not started |

**Later:** parameterized runs from a CSV, control flow (loops and conditionals), OCR fallback, browser automation output for web targets, cross-platform support.

Phase 3 is the one that makes the project worth building — everything before it is a conventional macro recorder.

---

## Known limitations

These are properties of GUI automation on Windows, not bugs to be fixed:

- **An active, unlocked session is required.** Synthetic input can't be delivered to a locked screen or a session with no user logged in. Scheduled runs therefore need the machine awake and unlocked — there is no true unattended overnight mode.
- **Elevation is inherited.** To click into an administrator window, AutoTasks must itself run elevated.
- **Some input is protected by design.** UAC prompts, secure fields, and anti-cheat-protected applications block synthetic input. AutoTasks doesn't attempt to work around this; those steps fail cleanly.
- **Template matching is display-sensitive.** Captured templates are tied to the resolution and DPI scaling in effect when they were recorded.

---

## Data & privacy

AutoTasks stores everything locally — no accounts, no network calls. Note that its runtime output (the SQLite database, captured templates, and failure screenshots) is a record of **your screen**, which may include sensitive content. These paths are gitignored by default; keep them that way if you restructure the project.

---

## Documentation

| Document | What it covers |
|---|---|
| [`docs/Pitch.md`](docs/Pitch.md) | Why the project exists and what makes it different |
| [`docs/SRS.md`](docs/SRS.md) | Full requirements specification — every feature, testable |
| [`docs/Architecture_and_Roadmap.md`](docs/Architecture_and_Roadmap.md) | Component design, tech decisions, build order by difficulty |
| [`docs/Learning_Path.md`](docs/Learning_Path.md) | Resources per technology, sequenced by phase |
| [`docs/TOOLING.md`](docs/TOOLING.md) | Code rules, commands, and gotchas |

---

## License

Not yet chosen. Until a license file is added, all rights are reserved.

---

*A personal project, built to solve a real problem and to learn C++ properly along the way.*
