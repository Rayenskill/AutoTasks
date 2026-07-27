# Reprise — Architecture & Build Roadmap

How the system is put together, what to build in what order, the languages and frameworks for each part, and what to learn before you start — calibrated to where you already are.

---

## 1. Architecture overview

Three loosely coupled components, with one hard rule.

```
        ┌─────────────────────────────────────────────┐
        │              Orchestrator + UI               │
        │  (library, editor, schedules, logs, review)  │
        │   - schedules runs (timer)                   │
        │   - commands the engine                      │
        │   - writes engine events to the store        │
        │   - renders the interface + tray             │
        └───────────────┬──────────────┬──────────────┘
                        │              │
              commands  │              │  reads/writes
                        ▼              ▼
        ┌───────────────────────┐   ┌──────────────────────┐
        │        Engine         │   │        Store         │
        │  - record (hooks)     │   │  SQLite:             │
        │  - replay (SendInput) │   │  scripts, versions,  │
        │  - template matching  │   │  runs, run_events,   │
        │  - emits step events  │   │  schedules           │
        └───────────────────────┘   └──────────────────────┘
                 │   ▲
   OS input APIs │   │ screen capture
                 ▼   │
        ┌───────────────────────┐
        │   Operating System    │
        └───────────────────────┘
```

**The rule:** the UI never synthesizes input. It commands the engine and reads the store. Everything good about the design — live logs, run history, and the review queue all drawn from one consistent record, plus a possible future headless mode — follows from keeping that boundary clean.

**Responsibilities, sharply divided:**

| Component | Owns | Must NOT |
|---|---|---|
| **Engine** | OS input (capture + synthesis), screenshots, template matching, emitting step events | Know about the UI, scheduling, or how data is presented |
| **Store** | Persisting scripts, versions, runs, events, schedules; transactions | Contain logic; it's data, queried by the others |
| **Orchestrator + UI** | Scheduling, launching runs, persisting events, rendering views, tray/hotkeys | Touch OS input directly |

---

## 2. The language decision (read this first)

Earlier, when the priority was shipping fast, the pragmatic recommendation was a Python-first stack (FastAPI + your existing Angular skills). **You've now said you want to learn more C++ — so this document deliberately takes the C++-centric path instead.** Here's the honest reasoning, because it changes the whole plan.

### Where C++ is genuinely the right tool (not bolted on)
The **engine** is a real C++ problem, not a contrived one:
- **OS-level input** — capturing global mouse/keyboard hooks and synthesizing input with `SendInput` is native Win32 territory. C++ talks to it directly.
- **Image matching** — OpenCV has a first-class, fast C++ API. Template matching against full-screen captures is exactly the kind of performance-sensitive work C++ suits.

So the part where you'll learn the most C++ is also the part where C++ is the correct choice. That alignment is rare and worth taking advantage of.

### The choice that actually matters: what to write the UI in
Two viable paths. Be deliberate here.

**Path A — All-in C++ with Qt (recommended for your stated goal).**
The engine *and* the UI in C++/Qt. One language, one toolchain, no boundary between components to engineer around. Qt gives you the whole desktop story in one framework: widgets, a SQL module (`QtSql`), timers (`QTimer`), system tray (`QSystemTrayIcon`), and cross-platform reach.
- **Why it fits you:** your goal is to learn C++. This maximizes time *in* C++, and Qt is a genuinely valuable, employable skill. It's also architecturally the cleanest — no inter-language plumbing.
- **The honest cost:** Qt has a real learning curve, and GUI code in C++ is more verbose and slower to iterate on than web UI. This is the **harder, slower** road. You're choosing depth over speed — which is the right trade *when the point is to learn the language.*

**Path B — C++ engine + Python/web UI (the fallback).**
Keep the engine in C++ for the learning and the fit, but build the orchestrator/UI in Python (FastAPI) with your existing Angular skills, talking to the engine across a boundary (a local socket, a subprocess exchanging JSON, or bindings via `pybind11`).
- **Why you might want it:** much faster UI iteration; reuses what you already know; you still get substantial C++ from the engine.
- **The honest cost:** that boundary between a C++ engine and a Python UI is *itself* real complexity, and it's complexity that doesn't teach you much C++. For a project whose explicit purpose is learning C++, you'd be spending effort on plumbing that's orthogonal to the goal.

### Recommendation
**Go Path A (all-in C++/Qt), and accept the slower pace.** It serves your actual goal, it's the cleanest architecture, and your background makes it feasible (more on that in §5). Crucially, **the early phases are identical either way** — the engine starts as a standalone C++ console program with no UI at all. So you can begin building immediately and defer the A-vs-B commitment until you reach the UI phase, by which point you'll have real C++ under your belt and a better-informed opinion. If Qt becomes a genuine blocker rather than just a learning curve, Path B is your escape hatch with no wasted work.

The rest of this document assumes **Path A**, and flags where Path B would differ.

---

## 3. Technology stack

| Concern | Choice | Why |
|---|---|---|
| **Language** | **C++ (C++17 or C++20)** | Your learning goal; right tool for the engine. C++17 is the safe, well-supported baseline; C++20 adds niceties you can adopt gradually. |
| **Build system** | **CMake** | The de-facto standard for modern C++. Non-negotiable to learn; everything (OpenCV, Qt, tests) integrates through it. |
| **Compiler / toolchain** | **MSVC** (Visual Studio Build Tools) | Native Windows toolchain; you already have Visual Studio in your environment, so this is the path of least resistance. |
| **UI framework** | **Qt 6** (Widgets) | One framework covers UI + SQL + timers + tray + cross-platform. *(Path B: FastAPI + Angular instead.)* |
| **Image matching** | **OpenCV** (C++ API) | `matchTemplate` + `minMaxLoc`; screen-capture frames in, location + confidence out. |
| **Database** | **SQLite** | Local, single-file, ACID, fully queryable, no server. Access via Qt's `QtSql`, or `SQLiteCpp` if not using Qt. |
| **Serialization** | **nlohmann/json** | Ergonomic JSON for C++; for script export/import and the JSON `steps` column. |
| **OS input** | **Win32 API** (`SendInput`, `SetWindowsHookEx`) directly, *or* a cross-platform input library to abstract it | Direct Win32 teaches you the most; a library isolates OS specifics and eases a future port. |
| **Scheduling** | **`QTimer`** (Qt) or a small scheduler | Fire schedules while the app runs. (Path B: APScheduler in Python.) |
| **Version control** | **git** + **GitHub** | From commit one. Branch per feature; `.gitignore` excludes the `build/` directory and other artifacts. |
| **Testing** | **GoogleTest** (or **Catch2**) | Unit-test the engine's pure logic (step parsing, confidence decisions) early; it pays for itself. |
| **CI (stretch)** | **GitHub Actions** | Build the project on push once it stabilizes — great learning, optional. |

**Suggested repo layout** (the `/docs` folder is where these three documents live):

```
reprise/
├── docs/            # SRS.md, Pitch.md, Architecture_and_Roadmap.md
├── engine/          # C++ engine: capture, replay, matching
├── store/           # schema + DB access
├── app/             # Qt UI + orchestrator  (Path B: separate ui/ + orchestrator/)
├── tests/           # GoogleTest
├── third_party/     # or pull deps via package manager
├── CMakeLists.txt
├── .gitignore
└── README.md
```

---

## 4. Build order — first to last, by difficulty

Each phase produces something runnable. Difficulty is **relative to your current level**. "New skills" lists what that phase forces you to pick up — this is how you learn C++ *just in time* instead of front-loading all of it.

> **Phases 0–4 are pure engine** and are **identical under Path A or Path B.** The language decision only bites at Phase 5.

### Phase 0 — Toolchain & skeleton · ★☆☆☆☆ (easy, but unfamiliar)
Get a C++ "hello world" building with CMake under MSVC. Initialize the git repo with the layout above and a proper C++ `.gitignore`. Get **one** dependency (start with OpenCV) to actually configure, build, and link — in C++, *making libraries link* is often the first real wall, so clear it early on something low-stakes.
**New skills:** CMake basics; the compile/link model; git repo hygiene for C++.
**Done when:** `cmake --build` produces a runnable binary that uses an OpenCV function.

### Phase 1 — Minimal replay · ★★☆☆☆
Hardcode a short sequence of moves/clicks in C++ and play it back through `SendInput` (or your input library). No recording, no UI, no files. This is the first "it moved by itself" moment — keep it tiny.
**New skills:** calling OS input APIs from C++; the basics of C++ program structure (headers vs. source, `main`).
**Done when:** running the binary reproduces a fixed click sequence.

### Phase 2 — Recording & round-trip · ★★★☆☆
Capture global mouse/keyboard events via low-level hooks; serialize them to a JSON file as an ordered list of steps (the FR-MODEL shape); replay *from that file*. Now you have **record → save → replay** as a console tool — the engine MVP.
**New skills:** global input hooks; `nlohmann/json`; file I/O; designing the step data structure; RAII for cleaning up hook handles.
**Done when:** you can record a task, close the program, reopen it, and replay from the saved file.

### Phase 3 — Robust replay (image matching + waits) · ★★★★☆
Integrate OpenCV properly. A step can target a **template image**: on replay, screenshot → `matchTemplate` → `minMaxLoc` → act at the match, gated by a confidence threshold. Add **wait-for-image** with a timeout. Record the confidence on each match — this quietly seeds the review queue later.
**New skills:** OpenCV `Mat`, template matching, confidence scoring; screen capture; turning fragile coordinate steps into resilient image steps.
**Done when:** a recorded macro still works after you move the target window, and waits replace blind sleeps.
*This is the phase that makes the whole project worth doing. Don't rush past it.*

### Phase 4 — Persistence layer · ★★★☆☆
Replace loose JSON files with SQLite and the real data model (`scripts`, `script_versions`, `runs`, `run_events`, `schedules`). Your INF3710 background makes the SQL itself easy; the new part is the **C++ database binding**. Wrap DB access behind a small interface so the rest of the code doesn't see raw SQL.
**New skills:** a C++ SQLite binding (`QtSql` or `SQLiteCpp`); transactions in code; schema migrations in a tiny way.
**Done when:** scripts and run history persist in the database, and editing a script writes a version snapshot.

> **★ Decision point:** before Phase 5, commit to **Path A (Qt)** or **Path B (Python/web UI)**. Everything above transfers either way.

### Phase 5 — UI shell · ★★★★★ (the single biggest step under Path A)
A Qt application with a **Library/Dashboard**: list scripts from the DB, show last-run status, and a **Run now** button that invokes the engine. Basic navigation between (mostly empty) views. The hard, important part is wiring the engine and the UI together in one app **without freezing the interface** — long engine work runs off the UI thread.
**New skills (Path A):** Qt widgets; the signals/slots model; running work off the UI thread (Qt threads / worker objects); Qt's project integration with CMake.
**(Path B):** FastAPI endpoints (run/list/logs/schedules), Angular wired to localhost, and the engine-to-UI boundary (socket / subprocess+JSON / `pybind11`).
**Done when:** you launch a script from the window and watch its status update live.

### Phase 6 — Editor & builder · ★★★★☆
Render a script's steps as a **reorderable list** (action / target / params per row); insert, delete, edit, and **re-capture a single step** in place. Wire the "record new" entry point into the UI so recordings land in the library.
**New skills:** Qt list/model-view widgets, drag-to-reorder; round-tripping structured steps between UI and store.
**Done when:** you can fix or insert one step without re-recording the whole macro.

### Phase 7 — Logs & run history · ★★★☆☆
A run-history view filterable by script and status, drillable into the per-step event trace with failure/low-confidence screenshots. Mostly UI plus querying the store you already built.
**New skills:** presenting and filtering tabular data in Qt; displaying images.
**Done when:** after a failed run you can open it and see exactly which step failed and what the screen looked like.

### Phase 8 — Scheduling, tray & hotkeys · ★★★★☆ (fiddly, platform-specific)
A scheduler (`QTimer`) firing runs; a **system-tray** presence (`QSystemTrayIcon`) so the app lives in the background; global hotkeys (start/stop record, **panic-abort**, per-script launch); optional launch-at-login. Surface the hard limits honestly in the UI: schedules only fire while the app runs and the session is **unlocked** (constraint C-1), and admin targets need elevation (C-2).
**New skills:** background/tray app lifecycle; global hotkey registration; scheduling; the OS realities of unattended GUI automation.
**Done when:** a scheduled run fires on its own with the window closed (machine awake/unlocked), and the panic key always stops a replay.

### Phase 9 — Review queue ("doubts") + verification · ★★★★★ (the capstone)
The distinguishing feature. Add **verification** steps (assert expected screen state). Flag low-confidence matches and failed verifications as `needs_review`, saving evidence. Build the **review queue**: show the step, the captured screen, the expected vs. observed result; let the user **confirm** or **correct** — and on correct, **re-capture the template on the spot** and write it back (new version snapshot). This closes the learning loop that makes Reprise more than a macro player.
**New skills:** bringing engine confidence data, the store, and the UI together into one workflow; the correct-and-recapture round trip.
**Done when:** a low-confidence run lands in the queue, and correcting it there visibly improves the next run.

### Phase 10 — Stretch goals · varies
Pick what's useful, in any order: **parameterized runs** from a CSV (★★★); **control flow** — loops/conditionals/repeat-until (★★★★); **self-healing** targeting, coordinate→image→OCR fallback (★★★★); **browser automation** output (Playwright/Selenium) for web targets (★★★★); a **visual block builder** (★★★★, plays to your UI instincts); **packaging/installer** and a **GitHub Actions** build (★★★).

**The natural milestones:** a real artifact by the end of **Phase 1**, a genuinely useful engine MVP by **Phase 3**, a usable application by **Phase 7**, and the feature that makes it *yours* at **Phase 9**.

---

## 5. What to learn before you start — calibrated to your skills

You're not starting cold. Here's where you stand and exactly what the deltas are, so you don't waste time relearning things you know or get blindsided by things you don't.

### What you already have (and how it transfers)
- **C# / Unity** → object-orientation, classes, C-family syntax, static typing. The OOP and syntax transfer to C++ almost directly. *The deltas* are the memory and compilation models below.
- **CTF: binary exploitation & reverse engineering** → this is your secret weapon. Pointers, the stack, memory layout, builds, debuggers — none of this is scary to you, and that's exactly the part of C++ that intimidates most newcomers. You're past the hardest psychological hurdle before you begin.
- **PostgreSQL / INF3710** (triggers, transactions, indexes, CTEs) → the entire Store layer is comfortable territory. Only the C++ *binding* to SQLite is new.
- **FastAPI + Angular** → app/API architecture and async thinking. Directly relevant if you take Path B; still useful instincts for structuring the orchestrator under Path A.
- **Python** (CTF scripting) → keeps Path B fully open and is handy for quick prototyping and tooling.
- **git, VS Code, Windows toolchain** → already in place. You just need a slightly more deliberate git workflow (below).

### Learn *before* you write real code (the must-haves so you're not lost)
Keep this short and practical — enough to build small programs, then learn the rest just-in-time per phase.

1. **C++ fundamentals, framed as the delta from C#:**
   - References vs. pointers; **value semantics** vs. C#'s reference-by-default (this trips up C# developers more than memory does).
   - **RAII and smart pointers** (`std::unique_ptr`, `std::shared_ptr`) — the modern C++ way to manage resources; you'll rarely write `new`/`delete` by hand.
   - `const`-correctness; headers vs. translation units and why both exist; the preprocessor/`#include` at a basic level.
   - The **STL essentials**: `std::vector`, `std::string`, `std::map`/`unordered_map`, `std::optional`.
   - *Defer:* deep move semantics, template metaprogramming, custom allocators — not needed for v1.
2. **CMake basics** — enough to define a target, link a library, and build. This is the chore that unlocks everything; do it early and on purpose.
3. **Your Windows toolchain end to end** — MSVC build/run/debug; what compiling vs. linking actually means when an error appears.
4. **A deliberate git workflow** — feature branches, meaningful commits, a C++-aware `.gitignore` (exclude `build/`). You know git; just be intentional with it here.

### Learn *as you reach the phase that needs it* (just-in-time)
- **OpenCV C++ basics** → at **Phase 3** (`Mat`, `imread`/capture, `matchTemplate`, `minMaxLoc`). Approachable.
- **Win32 input specifics** → at **Phases 1–2** (`SendInput`; `SetWindowsHookEx` with `WH_MOUSE_LL`/`WH_KEYBOARD_LL`). Learn just these functions, not all of Win32.
- **A SQLite C++ binding** → at **Phase 4** (`QtSql` or `SQLiteCpp`).
- **Qt 6** → at **Phase 5** (widgets, signals/slots, the meta-object idea, `QtSql`, `QTimer`, `QSystemTrayIcon`). This is a framework curve on top of the language — budget real time for it. *(Path B: Angular you already know.)*
- **`nlohmann/json`** → at **Phase 2**. Trivial to pick up.
- **Threads/concurrency basics** (`std::thread`, `std::mutex`, or Qt's threading) → at **Phase 5**, only as much as you need to keep the UI responsive during replay.
- **GoogleTest** → introduce around **Phase 2–3**, once there's pure logic worth testing.

### The trap to avoid
Don't try to "finish learning C++" before writing anything — that kills momentum and you'll forget it unused. Learn the must-haves in the list above, build Phase 0 and Phase 1, and let each later phase pull in its skills as you hit them. Your CTF background means you can afford to be aggressive about diving in.

---

## 6. One-paragraph summary

Build it in **C++** because that's your goal and it genuinely fits the engine; structure it as **engine / store / orchestrator+UI** with the rule that the UI never clicks anything itself; use **CMake, OpenCV, SQLite, and Qt 6** (Path A) — or a Python/web UI over a C++ engine (Path B) if Qt becomes a wall. Start with a console engine (record → replay → image-matched replay → database), which is identical under either path, then build the UI, then the scheduling and tray, and finish with the **review queue** that makes the project distinctively yours. Learn just enough C++ and CMake to get moving, lean hard on your existing SQL and low-level/CTF instincts, and pick up OpenCV, Qt, and the rest exactly when the phase in front of you needs them.

---

*Companion documents: the SRS (what it must do) and the Pitch (why it's worth doing).*
