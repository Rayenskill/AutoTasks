# AutoTasks

**Record your repetitive desktop tasks, replay them by recognizing the screen instead of guessing pixel coordinates, and review the steps the engine wasn't sure about.**

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C)
![CMake](https://img.shields.io/badge/build-CMake%203.21%2B-064F8C)
![Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-0078D6)
![Status](https://img.shields.io/badge/status-early%20development-orange)

> ⚠️ **Early development.** The build is split into a reusable engine library, a CLI test bench, and a Qt shell — all three compile and run. What is *not* written yet is the engine itself: the real `Win32Engine` is a skeleton that reports every step as unimplemented. A simulated engine stands in for it so the UI can be built in parallel. See the [roadmap](#roadmap) for what exists and what doesn't.

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

Qt 6 is required **only** for the desktop app. The engine builds without it.

### Build & run — engine only (no Qt needed)

```powershell
git clone <your-repo-url> autotasks
cd autotasks

cmake --preset engine
cmake --build --preset engine

.\build\engine\bin\autotasks_engine.exe --stub   # simulated engine
.\build\engine\bin\autotasks_engine.exe          # real engine (skeleton)
```

### Build & run — desktop app

Copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json` and set your Qt path in it — see [`docs/guides/QT_SETUP.md`](docs/guides/QT_SETUP.md). Then:

```powershell
cmake --preset dev-qt
cmake --build --preset dev-qt

.\build\dev\bin\autotasks_app.exe
```

All commands must be run from a **Developer PowerShell for VS**, otherwise the compiler is not on the PATH.

### Available presets

| Preset | Purpose |
|---|---|
| `engine` | **Engine only, no Qt.** The engine developer's everyday build |
| `dev` | Everyday build — Ninja, Debug, warnings as errors, app included if Qt is found |
| `dev-qt` | `dev` plus a machine-local Qt path — defined in your own `CMakeUserPresets.json` |
| `lint` | Same as `dev`, plus clang-tidy on every translation unit (slower; run before committing) |
| `asan` | AddressSanitizer build, for chasing memory bugs |
| `release` | Optimized (`RelWithDebInfo`) |
| `vs` | Visual Studio IDE solution — note: no `compile_commands.json`, so clang-tidy integration is limited |

If Qt is not found, configuration still succeeds and only the engine targets are built — a collaborator without Qt can always clone and compile.

---

## Project structure

```
autotasks/
├── .clang-format .clang-tidy .editorconfig .gitattributes .gitignore
├── CMakeLists.txt
├── CMakePresets.json
├── CMakeUserPresets.json.example   Copy to CMakeUserPresets.json, set your Qt path
├── docs/                      Project documentation (see below)
├── engine/                    Recording, replay, image matching — no Qt, ever
│   ├── include/engine/
│   │   └── Engine.h           THE CONTRACT between the two halves
│   └── src/
│       ├── Win32Engine.cpp    The real engine (skeleton)
│       ├── StubEngine.cpp     Simulated engine — lets the UI be built in parallel
│       ├── main.cpp           CLI test bench
│       └── win32/             All platform-specific code isolated here (Phase 1)
├── store/                     SQLite persistence layer — Phase 4
├── app/                       Qt UI + orchestrator — no Win32, ever
│   ├── include/app/
│   └── src/
├── tests/                     GoogleTest — Phases 2-3
└── build/                     Generated — gitignored
```

### Build targets

| Target | Kind | Owner |
|---|---|---|
| `autotasks_core` | Static library — the whole engine, no `main()` | engine dev |
| `autotasks_engine` | CLI test bench, builds without Qt | engine dev |
| `autotasks_app` | Qt desktop application | frontend dev |

The engine is a **library**, not an executable. That is what lets the Qt app link against it, and what lets two developers work in parallel: the engine developer never installs Qt, the frontend developer never touches Win32. The one file they share is `engine/include/engine/Engine.h` — see [`docs/guides/TEAM_WORKFLOW.md`](docs/guides/TEAM_WORKFLOW.md).

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

See [`docs/guides/TOOLING.md`](docs/guides/TOOLING.md) for the full setup, gotchas, and the plan for tightening rules as the project grows.

---

## Roadmap

| Phase | Milestone | Status |
|---|---|---|
| 0 | Toolchain, build config, repo skeleton | ✅ Done |
| 0.5 | Engine split into a library, contract + simulated engine, Qt shell wired into the build | ✅ Done |
| 1 | Minimal replay — hardcoded clicks via `SendInput` | ⬜ Not started |
| 2 | Record → save → replay round trip (JSON) | ⬜ Not started |
| 3 | Image matching + smart waits (OpenCV) | ⬜ Not started |
| 4 | SQLite persistence and the real data model | ⬜ Not started |
| 5 | Qt UI shell — library, run now | 🟨 Shell and threading in place; views are placeholders |
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

Index: [`docs/README.md`](docs/README.md)

| Document | What it covers |
|---|---|
| [`docs/guides/TEAM_WORKFLOW.md`](docs/guides/TEAM_WORKFLOW.md) | **Start here if there are two of you.** How the work splits, why the simulated engine exists, file ownership, Git |
| [`docs/guides/QT_SETUP.md`](docs/guides/QT_SETUP.md) | Installing and configuring Qt, per machine |
| [`docs/guides/TOOLING.md`](docs/guides/TOOLING.md) | Code rules, commands, and gotchas |
| [`docs/planning/Pitch.md`](docs/planning/Pitch.md) | Why the project exists and what makes it different |
| [`docs/planning/SRS.md`](docs/planning/SRS.md) | Full requirements specification — every feature, testable |
| [`docs/planning/Architecture_and_Roadmap.md`](docs/planning/Architecture_and_Roadmap.md) | Component design, tech decisions, build order by difficulty |
| [`docs/planning/Learning_Path.md`](docs/planning/Learning_Path.md) | Resources per technology, sequenced by phase |

---

## License

Not yet chosen. Until a license file is added, all rights are reserved.

---

*A personal project, built to solve a real problem and to learn C++ properly along the way.*
