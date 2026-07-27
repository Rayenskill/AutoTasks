# Reprise — Learning Path & Resource Checklist

Where to start from scratch on this stack, which resource to use for each piece, **how much of it to clear**, and when you'll actually need it. Built to be worked through over weeks, not read once.

> Pairs with the *Architecture & Roadmap* doc (§4 build phases, §5 what-to-learn). This one gives the concrete resources and the "do this much" guidance.

---

## 0. You are not starting from zero

Be honest about what already transfers — it changes where you start and how fast you move.

| You already know | Transfers to | So what's actually NEW |
|---|---|---|
| **C# / Unity** | C++ syntax, OOP, static typing | Value semantics vs. reference-default; manual resource management |
| **CTF / reverse engineering** | Pointers, stack, memory, builds, debuggers | Almost nothing here scares you — this is your edge |
| **PostgreSQL / INF3710** | The entire data layer, SQL, transactions | Only the C++ *binding* to SQLite |
| **Python / FastAPI / Angular** | App structure, async thinking | Keeps Path B (web UI) open; not needed for Path A |
| **git, VS Code** | Daily workflow | Just a more deliberate branching habit |

**The genuinely new list is short:** C++ language specifics beyond C#, CMake, the Win32 input calls, OpenCV's C++ API, the SQLite C++ binding, and Qt. That's it. Everything below targets exactly those.

---

## 1. Start here — the first move (before any engine code)

In C++, the toolchain is the first wall. Clear it first; it's the biggest early demotivator.

1. **Toolchain + hello world (Phase 0).** Install **Visual Studio Community** with the *"Desktop development with C++"* workload — it bundles the MSVC compiler, CMake, and a debugger. Write a trivial program, build it, run it. Don't proceed until the build loop works.
2. **Begin C++ fundamentals** on learncpp.com (below). Move fast through what you know from C#; slow down on the new parts.
3. **Skim modern CMake** enough to build an executable and link one library.

Then **stop reading and build Phase 1.** Pick up everything else just-in-time.

---

## 2. The rule

> **Learn just enough to build the phase in front of you, then build. Do not try to "finish learning" any technology first.**

Front-loading a whole language or framework kills momentum and you forget the unused parts. Every phase in the roadmap pulls in its own skills exactly when you hit it.

---

## 3. Resource checklist, by technology

Each entry: what it's for · the resource · **clear this much** · needed at.

### C++ language — the anchor
- **Primary:** **learncpp.com** — free, no signup, text-based, actively updated for modern C++ (C++20/23). The gold-standard free tutorial.
  - **Clear this much:** the fundamentals (types, control flow, functions), then the core new-to-you material: **pointers & references, dynamic memory, smart pointers (`unique_ptr`/`shared_ptr`) & RAII, C++ classes, and `std::vector`/`std::string`**. That's roughly the first third of the site — *not* all 28 chapters. Come back for the rest as needed.
  - **Needed at:** before Phase 1; keep going during Phases 1–4.
  - *Note:* learncpp is thorough to a fault — some chapters go into reference-level detail. Skim those the first time; don't get stuck.
- **Reference (not a tutorial):** **cppreference.com** — for looking up a function or type. Bookmark it; don't read it.
- **Optional book (good fit for you):** **"A Tour of C++" (Bjarne Stroustrup, 3rd ed.)** — short, written for people who *already* program, covers C++20. A fast, high-quality overview if you prefer a book to a website.
- **Prefer video?** There's a free ~31-hour "C++ Beginner to Advanced" video course by Daniel Gakwaya (LearnQtGuide) — a solid alternative to learncpp's text if you learn better by watching.

### CMake — the build system (non-negotiable)
- **Primary:** **"An Introduction to Modern CMake"** by Henry Schreiner — `https://cliutils.gitlab.io/modern-cmake/`. Free, living document, downloadable as PDF; teaches *modern* CMake (targets, not old globals).
- **Also:** the **official CMake tutorial** at `cmake.org/cmake/help/latest/guide/tutorial/`. And Jason Turner's *C++ Weekly Ep. 78 "Intro to CMake"* is a good 10-minute orientation.
  - **Clear this much:** define a target, link a library, use `find_package`. Skip the advanced/packaging chapters for now.
  - **Needed at:** Phase 0, deepened at Phase 4 (linking SQLite) and Phase 5 (Qt integration).

### Git workflow — you know the basics, tighten the habit
- **Resource:** **Pro Git** (free) — `git-scm.com/book`. Read only the branching/workflow chapters.
  - **Clear this much:** feature branches, meaningful commits, a C++-aware `.gitignore` (exclude `build/`). Push to GitHub from commit one.
  - **Needed at:** Phase 0.

### Win32 input — narrow, targeted, not a course
- **Resource:** **Microsoft Learn** reference pages for `SendInput` (synthesis) and `SetWindowsHookEx` with `WH_MOUSE_LL` / `WH_KEYBOARD_LL` (capture). Supplement with a blog walkthrough of low-level keyboard/mouse hooks.
  - **Clear this much:** *only* those specific functions and their structs. You are not learning "all of Win32."
  - **Needed at:** Phase 1 (SendInput), Phase 2 (hooks).
  - *Alternative:* use a cross-platform input library to abstract this away — less to learn now, and it isolates OS specifics for a future port.

### OpenCV (C++ API) — approachable
- **Resource:** **official OpenCV docs**, specifically the **Template Matching** tutorial at `docs.opencv.org`.
  - **Clear this much:** `Mat`, reading/capturing an image, `matchTemplate`, `minMaxLoc` (location + confidence). That's essentially the whole matching feature.
  - **Needed at:** Phase 3.

### SQLite C++ binding — the only new part of your data layer
- **Resource:** **SQLiteCpp** GitHub README (a clean C++ wrapper), *or* **Qt's `QtSql`** docs if you go Path A (Qt).
  - **Clear this much:** open a DB, run a parameterized query, read results, wrap it behind a small interface. Your SQL knowledge covers the rest.
  - **Needed at:** Phase 4.

### Qt 6 — the biggest new framework (Path A only)
- **Primary:** **official Qt docs** at `doc.qt.io/qt-6/` — start with the **Widgets Tutorial** and the **"Getting Started Programming with Qt Widgets"** notepad walkthrough (current series is Qt 6.11). Note: **use CMake, not qmake** — CMake is the recommended path for Qt projects now.
- **Video option:** **VoidRealms (Bryan Cairns) "Qt 6 Core for Beginners"** is a well-regarded free-leaning series; **LearnQtGuide's "Qt 6 C++ GUI Development for Beginners"** (Udemy, paid) is comprehensive if you want a structured course.
  - **Clear this much:** the event loop, **signals & slots**, common widgets, layouts, the **model/view** basics (for the script list and step editor), plus `QtSql`, `QTimer`, and `QSystemTrayIcon`. Budget real time — this is a genuine curve on top of the language.
  - **Needed at:** Phase 5, extended through Phases 6–8.
  - *(Path B instead: reuse Angular, which you already know, and add FastAPI endpoints.)*

### GoogleTest — introduce early, it pays off
- **Resource:** the **GoogleTest Primer** in the googletest docs.
  - **Clear this much:** writing a `TEST`, assertions, wiring it into CMake. Enough to test the engine's pure logic (step parsing, the confidence decision).
  - **Needed at:** around Phase 2–3.

---

## 4. Sequenced view — resource per phase

The compact "what am I learning right now" map. (Phases 0–4 are identical whether you later choose Qt or a web UI.)

| Phase | Focus | Learn now | Resource |
|---|---|---|---|
| **0** | Toolchain + skeleton | CMake basics, git hygiene | VS installer · Modern CMake · Pro Git |
| **1** | Minimal replay | C++ basics, `SendInput` | learncpp (fundamentals) · MS Learn |
| **2** | Record → save → replay | hooks, JSON, RAII, file I/O | MS Learn · nlohmann/json README · learncpp (pointers/RAII) |
| **3** | Image matching + waits | OpenCV `matchTemplate` | OpenCV Template Matching docs |
| **4** | Persistence (SQLite) | C++ SQLite binding | SQLiteCpp / QtSql docs |
| — | **★ decide Path A vs B** | — | *Architecture doc §2* |
| **5** | UI shell | Qt widgets, signals/slots, threading | Qt Widgets Tutorial · VoidRealms |
| **6** | Editor & builder | Qt model/view, drag-reorder | Qt model/view docs |
| **7** | Logs & history | filtering/displaying data in Qt | Qt docs |
| **8** | Scheduling, tray, hotkeys | `QTimer`, `QSystemTrayIcon`, global hotkeys | Qt docs + a hotkey lib |
| **9** | Review queue (capstone) | tying engine + store + UI together | (your own synthesis) |

---

## 5. Traps to avoid

- **Don't finish C++ before writing code.** Clear the fundamentals + pointers/RAII, then build Phase 1. The rest sticks better when applied.
- **Don't learn all of Qt up front.** It's huge. Learn the widgets and signals/slots you need for Phase 5, add model/view at Phase 6, tray/timers at Phase 8.
- **Don't learn "all of Win32."** Just `SendInput` and the two hook functions.
- **Don't fight qmake tutorials.** Use CMake for Qt from the start — it's the current recommended path and it's the build system you're already learning.
- **Don't skip git branching discipline.** One branch per phase/feature; your future self reviewing history will thank you.

---

## 6. If you prefer video over text

learncpp (C++) and the Qt/OpenCV/CMake official docs are text-first. Video equivalents exist for each: Daniel Gakwaya's free C++ course, VoidRealms for Qt, Jason Turner's *C++ Weekly* for bite-sized modern-C++ and CMake topics. Mix formats to taste — the sequence and "clear this much" guidance above stays the same.

---

*Companion documents: SRS, Pitch, Architecture & Roadmap.*
