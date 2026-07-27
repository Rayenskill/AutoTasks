# Reprise — Tooling & Code Rules

How the "lint config" of this project works, what to run, and what to tighten as you go.

---

## The mapping (coming from TypeScript)

| TypeScript | Here | File |
|---|---|---|
| Prettier | clang-format | `.clang-format` |
| ESLint | clang-tidy | `.clang-tidy` |
| `tsconfig` `strict: true` | compiler warning flags + warnings-as-errors | `CMakeLists.txt` |
| `tsconfig` `target` / `lib` | `CMAKE_CXX_STANDARD`, `CXX_EXTENSIONS OFF` | `CMakeLists.txt` |
| `.editorconfig` | `.editorconfig` (identical) | `.editorconfig` |
| npm scripts | CMake presets / targets | `CMakePresets.json` |
| — (no equivalent) | sanitizers (ASan/UBSan) | `-D REPRISE_ENABLE_SANITIZERS=ON` |

The important structural difference: TS puts it all in one or two JSON files. C++ splits it across formatter, analyser, and compiler — and CMake is the glue that makes all three run automatically.

---

## Files in this drop

```
.clang-format        formatting rules
.clang-tidy          static-analysis rules + naming conventions
.editorconfig        editor baseline (line endings, final newline, indents)
.gitignore           C++/CMake/Qt/VS + Reprise's own runtime data
CMakeLists.txt       build + strict-mode warning flags + tooling hookups
CMakePresets.json    named build configurations (dev / lint / asan / release / vs)
engine/src/main.cpp  Phase 0 stub so the build actually runs
```

Drop them at the repo root (with `main.cpp` at `engine/src/`) and commit them **first**, before any real code. Rules added to an empty project cost nothing; rules added to 5,000 lines cost a weekend.

---

## Prerequisites

In the **Visual Studio Installer**, under *Desktop development with C++*, tick:

- **MSVC v143 build tools** (the compiler)
- **C++ CMake tools for Windows** (brings CMake **and Ninja**)
- **C++ Clang tools for Windows** (brings **clang-format** and **clang-tidy**)

Nothing else to install. Verify from a *Developer PowerShell for VS 2022*:

```powershell
cmake --version ; ninja --version ; clang-format --version ; clang-tidy --version
```

---

## Everyday commands

```powershell
# Configure once per preset
cmake --preset dev

# Build (this is the loop you'll live in)
cmake --build --preset dev

# Run
.\build\dev\bin\reprise_engine.exe

# Format every source file in place
cmake --build --preset format

# Build WITH clang-tidy running on every file (slower — run before committing)
cmake --preset lint
cmake --build --preset lint

# Chase a memory bug
cmake --preset asan
cmake --build --preset asan
```

Run clang-tidy on a single file without a full build:

```powershell
clang-tidy engine/src/main.cpp -p build/dev
```

Auto-apply its fixes (review the diff afterwards — it's usually right, not always):

```powershell
clang-tidy engine/src/main.cpp -p build/dev --fix
```

---

## Gotchas, in the order you'll hit them

**1. Use the Ninja presets, not the Visual Studio generator.**
clang-tidy and clangd need `compile_commands.json`, and the VS generator doesn't produce one. The `dev`/`lint`/`asan` presets all use Ninja for this reason. The `vs` preset exists only if you want the IDE solution — you lose tidy integration with it.

**2. clang-tidy can choke on MSVC flags.**
clang-tidy is a Clang tool being handed cl.exe-style command lines. Usually fine; when it isn't, the clean fix is to build the lint preset with **clang-cl** (MSVC-compatible Clang driver, ships with the same VS component):

```powershell
cmake --preset lint -D CMAKE_CXX_COMPILER=clang-cl
```

Your normal `dev` builds stay on MSVC. Alternatively, Visual Studio has built-in clang-tidy integration (*Project Properties → Code Analysis → Clang-Tidy*) that handles this for you.

**3. Warnings-as-errors will stop you mid-flow, on purpose.**
That's the point — a warning you can ignore is a warning you will ignore. If it's blocking a spike, turn it off for that build only:

```powershell
cmake --preset dev -D REPRISE_WARNINGS_AS_ERRORS=OFF
```

Turn it back on before committing. Don't edit the default in `CMakeLists.txt`.

**4. Don't run clang-tidy on every save at first.**
It's slow — it compiles the file to analyse it. Keep it as a pre-commit / pre-push step (the `lint` preset) until the codebase is small and clean enough that per-save is comfortable.

**5. Third-party headers will trigger warnings you can't fix.**
When OpenCV and Qt arrive, link them with `SYSTEM` so their headers are exempt:

```cmake
target_include_directories(reprise_engine SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
```

`HeaderFilterRegex` in `.clang-tidy` already restricts analysis to your own headers.

**6. `qt_add_executable` + AUTOMOC generates code that clang-tidy will flag.**
At Phase 5, exclude generated files — the usual approach is a `.clang-tidy` in the build directory containing `Checks: '-*'`, or setting `CMAKE_CXX_CLANG_TIDY` only on your own targets rather than globally.

---

## Tightening plan, by phase

Start loose enough to keep moving, tighten as the codebase earns it.

| Phase | Change |
|---|---|
| **0** | Commit all config as-is. `bugprone-*` + `performance-*` are already errors. |
| **2** | Add GoogleTest; start running the `lint` preset before each commit. |
| **2–3** | Add `cppcoreguidelines-*` to `.clang-tidy` — **on its own branch.** It's noisy, and working through its output is the single best C++ tutoring you'll get. |
| **3** | Mark OpenCV includes `SYSTEM`. |
| **4** | Add `concurrency-*` once threads appear. |
| **5** | Exclude Qt-generated sources from tidy; add `Qt6::Widgets`/`Qt6::Sql`. |
| **anytime** | Add `clang-analyzer-*` for deeper analysis — consider CI-only, it's slow. |
| **stretch** | GitHub Actions running `cmake --preset lint` + tests on every push. |

---

## Why this is worth more in C++ than it was in TypeScript

Two reasons, and the second is the real one.

**Bugs.** TS catches a type error; C++ hands you a use-after-free that works fine on your machine for three months. `bugprone-*` and ASan exist to close that gap, which is why `bugprone-*` is set to fail the build.

**Teaching.** Coming from C#, the `modernize-*` and (later) `cppcoreguidelines-*` families act as an always-on tutor: *use `nullptr` not `NULL`; this loop should be a range-for; this member belongs in the initialiser list; this raw pointer should be a `unique_ptr`; this should be `const`.* You'll absorb idiomatic modern C++ from build output rather than from a chapter you read once and forgot. That's a faster feedback loop than any tutorial, and it's the main reason to set this up on day one rather than day thirty.

---

*Companion documents: SRS · Pitch · Architecture & Roadmap · Learning Path.*
