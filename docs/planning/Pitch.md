# Reprise — Project Pitch

**A desktop tool that records your repetitive mouse-and-keyboard tasks and replays them reliably — then tells you when it wasn't sure.**

---

## The problem

A surprising amount of routine computer work is just the same clicks in the same order, over and over: stepping through an internal tool, re-running a verification pass, moving data between two windows that don't talk to each other. It's mechanical, it's error-prone precisely *because* it's boring, and it eats time that could go anywhere more valuable.

There are tools that record and replay clicks. But the common ones share one fragile assumption: they replay **fixed screen coordinates**. Record "click pixel (450, 300)" and the macro breaks the moment a window moves, the resolution changes, or a page loads half a second slower. So the existing tools work in a demo and fail in real life, and people quietly go back to doing the task by hand.

## The idea

Reprise records what you do and replays it **by recognizing the screen, not memorizing pixel positions.** Instead of "click here," a step means "find this button and click it." That single change is what makes an automation survive a moved window or a slow load — the difference between a toy and something you actually trust to run.

On top of a robust engine sits a real control center: a library to organize your scripts, a step-by-step editor so you can fix one action without re-recording everything, scheduling and global hotkeys to launch runs, and a full run history with screenshots so you can see exactly what happened.

## What makes it different

The honest truth: record-and-replay is not a new category. AutoHotkey, PyAutoGUI, TinyTask, and Power Automate Desktop all exist. So Reprise doesn't win on "nobody has done this." It wins on **two things those tools largely don't do well together:**

1. **Image-anchored replay as the default**, not an afterthought — so reliability is built in, not bolted on.
2. **A review-and-correct loop.** This is the part I'm most interested in. When the engine acts on a match it wasn't fully sure about — it found the button at 65% confidence when the bar was 80% — or when a verification step expected one screen and saw another, it doesn't silently barrel ahead. It flags that step, saves a screenshot of what it saw, and drops it into a **review queue**: *here's the step, here's the screen, was this right?* You confirm, or you correct it and re-capture the target on the spot — and the fix feeds straight back into the script.

That loop turns a blind macro player into something closer to a self-improving assistant that knows the limits of its own confidence. It's also, not coincidentally, a verification mindset — the same instinct behind catching a formula error in a report before it ships — applied to automation.

## Who it's for

Initially: **me.** I have real, recurring, deterministic tasks to point it at, which is the best possible reason to build a personal tool — I'll feel every rough edge and fix the ones that actually matter. More broadly, it's for any technical person doing repeatable desktop work who's been burned by brittle coordinate-based recorders.

## Why it's worth building

Beyond saving myself time, this project is a deliberately good vehicle for going deeper as an engineer:

- **It's genuinely full-stack, in the real sense** — from OS-level input APIs and image processing at the bottom, through a transactional data layer, up to a responsive desktop UI. Few personal projects span that whole range.
- **It's a serious C++ project with a reason to exist.** Native input and OpenCV image matching are exactly where C++ is the right tool, so the language learning is anchored to real problems instead of toy exercises.
- **It demonstrably ties into the kind of work I already do** — quality assurance, verification, catching the case that doesn't match. The review queue is that discipline turned into software.
- **It has a clean growth path.** A working version is small; the ceiling is high. Parameterized runs from a spreadsheet, control flow, OCR fallback, even emitting real browser automation for web targets — each is a natural next chapter, not a rewrite.

## The shape of it

Three loosely coupled parts, so the system stays clean and testable:

- an **engine** that does the OS-level recording and replaying (and nothing else),
- a **store** — the single source of truth for scripts, runs, logs, and schedules,
- an **orchestrator + UI** that schedules runs, commands the engine, and renders the control center.

One rule holds the design together: the interface never clicks anything itself. It tells the engine what to do and reads back the record. That's what lets live logs, history, and the review queue all come from one consistent place.

## The one-line version

> **Reprise records your repetitive desktop tasks, replays them by recognizing the screen instead of guessing pixel coordinates, and flags the steps it wasn't sure about so you can teach it to do better.**

---

*Companion documents: the SRS (what it must do) and the Architecture & Roadmap (how to build it, in order).*
