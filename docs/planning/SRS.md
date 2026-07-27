# Software Requirements Specification — *Reprise*

**Desktop GUI Automation & Macro Orchestration Tool**

> *"Reprise" is a working codename (to reprise = to perform again). Rename freely.*

| | |
|---|---|
| **Document** | Software Requirements Specification (SRS) |
| **Version** | 1.0 (initial) |
| **Status** | Draft for a personal project |
| **Audience** | The author (you), plus anyone you show the project to |

---

## 1. Introduction

### 1.1 Purpose
This document specifies the requirements for *Reprise*, a desktop application that records a user's mouse and keyboard actions and replays them reliably, with a management interface for organizing, editing, scheduling, and auditing those automations. It defines what the system must do (functional requirements) and the qualities it must have (non-functional requirements), so that development can proceed against a clear, testable target.

### 1.2 Scope
Reprise lets a single user automate repetitive, deterministic GUI tasks — the kind that "never change," such as the same sequence of clicks through an internal tool or a routine data-entry pass. The system covers four capabilities:

1. **Capture** — record pointer movement, clicks, and keystrokes into an editable, structured script.
2. **Replay** — reproduce a script reliably, using image recognition rather than blind screen coordinates so that automations survive moved windows and timing variation.
3. **Manage** — a desktop interface to organize scripts in a library, edit them step by step, build new ones, and schedule runs.
4. **Audit** — run history, step-level logs with screenshots, and a *review queue* ("doubts") surfacing steps the engine was uncertain about so the user can confirm or correct them.

Reprise is **Windows-first** and intended for **personal, single-user, local** use. It is not a multi-user server product, a web service, or a tool for defeating anti-automation protections.

### 1.3 Definitions, Acronyms, and Abbreviations

| Term | Meaning |
|---|---|
| **Script** | An ordered, named, persisted list of steps that can be replayed. |
| **Step** | One unit of action in a script (e.g., *move*, *click*, *type*, *wait-for-image*, *verify*). |
| **Recording** | The act of capturing input events into a script. |
| **Replay** | Executing a script by synthesizing input events. |
| **Template** | A reference image of an on-screen element used to locate it during replay. |
| **Template match** | Finding a template on the current screen; produces a location and a **confidence** score (0–1). |
| **Confidence threshold** | The minimum match confidence allowed to act. |
| **Review threshold** | A confidence band above the action threshold but below "certain"; matches in this band are flagged for review. |
| **Run** | A single execution of a script, with a start/end time, status, and event log. |
| **Doubt / review item** | A step flagged `needs_review` because of low match confidence or a failed verification. |
| **Engine** | The component that performs OS-level capture and replay. |
| **Store** | The local database of scripts, runs, logs, and schedules. |
| **Orchestrator** | The component that schedules and launches runs and mediates between UI, engine, and store. |
| **Secure field** | An OS-protected input (e.g., UAC prompt, some password boxes) that blocks synthetic input by design. |
| **SendInput / low-level hook** | Windows APIs for synthesizing and capturing input, respectively. |

### 1.4 References
- IEEE Std 830-1998 — Recommended Practice for Software Requirements Specifications (structure basis).
- OpenCV documentation — template matching (`matchTemplate`, `minMaxLoc`).
- Microsoft Win32 API — `SendInput`, `SetWindowsHookEx` (`WH_MOUSE_LL`, `WH_KEYBOARD_LL`).
- Companion documents: *Reprise — Pitch* and *Reprise — Architecture & Roadmap*.

### 1.5 Overview
Section 2 describes the product context and constraints. Section 3 lists functional requirements grouped by feature, each with a stable identifier (FR-x.y). Section 4 covers external interfaces. Section 5 covers non-functional requirements (NFR-x). Section 6 records what is explicitly out of scope for the first version and what is reserved for later.

---

## 2. Overall Description

### 2.1 Product Perspective
Reprise is a new, self-contained desktop application, not an extension of an existing system. Internally it is organized into three loosely coupled parts so that responsibilities stay separate and the system remains testable and extensible:

- **Engine** — the only component that touches OS-level input. It accepts a script, replays it, and emits events (step started, step succeeded, low confidence, error + screenshot). It also performs recording.
- **Store** — a local database holding scripts, version snapshots, runs, run events, and schedules. It is the single source of truth.
- **Orchestrator + UI** — schedules runs, commands the engine, persists the engine's events to the store, and renders the interface.

A defining architectural rule: **the UI never synthesizes input itself.** It commands the engine and reads the store. This is what allows live logs, run history, and the review queue to all be drawn from one consistent record, and it keeps a future headless mode possible.

### 2.2 Product Functions (summary)
- Record mouse/keyboard activity into a structured, editable script.
- Replay scripts using image-anchored targeting and smart waits, not fixed coordinates alone.
- Organize scripts in a searchable, taggable library with last-run status.
- Edit scripts as a reorderable list of steps; re-capture a single step without redoing the whole macro.
- Schedule runs (interval/cron) with enable/disable, and trigger runs via global hotkeys.
- Persist run history with step-level logs and failure screenshots.
- Surface low-confidence or failed-verification steps in a review queue, and let the user confirm or correct (re-capture) them.

### 2.3 User Characteristics
A single technical user (the author): a software-engineering student comfortable with programming, the command line, and SQL, automating their own repetitive desktop tasks. No accessibility or localization obligations beyond the author's preference. No untrusted or anonymous users.

### 2.4 Operating Environment
- **Primary OS:** Windows 10/11 (x64).
- **Hardware:** the author's personal laptop; standard mouse/keyboard/display.
- **Display:** must account for resolution and DPI scaling (template matching is resolution-sensitive).
- **Session:** replay requires an active, unlocked, logged-in desktop session (see constraints).
- **Portability target:** the chosen UI framework should not foreclose a future Linux/macOS build, even though only Windows is supported in v1.

### 2.5 Design and Implementation Constraints
- **C-1 — Active session required.** Synthetic input cannot be delivered to a locked screen or a session with no user logged in. Unattended overnight runs therefore require the machine to be awake and unlocked. The system must not promise locked-session execution.
- **C-2 — Elevation.** To interact with an elevated (administrator) window, Reprise must itself run elevated. A non-elevated process cannot click into an elevated window.
- **C-3 — Protected input.** Secure fields, UAC prompts, and anti-cheat-protected applications may block synthetic input by design. Reprise must not attempt to circumvent these protections and should fail such steps gracefully.
- **C-4 — Single active replay.** Only one replay may run at a time per session; the engine controls the one physical pointer/keyboard, so concurrent replays are nonsensical and must be prevented.
- **C-5 — Local only.** All data is stored locally. No network service, account, or cloud component in v1.
- **C-6 — Resolution/DPI sensitivity.** Because targeting uses image matching, captured templates are tied to the display configuration under which they were recorded; the system must record enough context to detect, and warn on, a mismatch.

### 2.6 Assumptions and Dependencies
- The target applications present a reasonably stable visual interface (the premise of "tasks that never change").
- The user has rights to automate the target applications and is not violating their terms of use.
- Third-party dependencies are available: an image-processing library (OpenCV), an embedded database (SQLite), a JSON library, and the chosen UI framework.

---

## 3. System Features and Functional Requirements

> Convention: **FR-<feature>.<n>**. "Shall" = mandatory for v1; "should" = strongly desired; "may" = optional/stretch.

### 3.1 Recording (Capture)
*Description:* Capture the user's actions into a structured script.

- **FR-REC.1** The system shall start and stop recording on an explicit user action (UI control and/or global hotkey).
- **FR-REC.2** The system shall capture mouse button events (down/up/click, button identity) and keyboard key events (key, modifiers).
- **FR-REC.3** The system shall capture pointer movement sufficient to reproduce the task; it should record **intent** (clicks and key targets with positions) rather than every raw pixel of the path, to reduce noise. Full-path capture may be offered as an option.
- **FR-REC.4** During recording, the system should capture, for each click target, a small screenshot region around the click point, to serve as a candidate template for image-anchored replay.
- **FR-REC.5** On stopping, the system shall produce a named script persisted as an ordered list of structured steps (see FR-MODEL).
- **FR-REC.6** The system shall record the display resolution and DPI scaling in effect at capture time (supports C-6).

### 3.2 Replay Engine
*Description:* Execute a script by synthesizing input.

- **FR-REP.1** The system shall replay a script by executing its steps in order.
- **FR-REP.2** The system shall synthesize mouse movement, clicks, and keystrokes through OS input APIs.
- **FR-REP.3** The system shall support configurable replay speed (e.g., 0.5×–2×) and a fixed inter-step delay.
- **FR-REP.4** The user shall be able to abort a running replay immediately via a global "panic" hotkey (e.g., `Esc`-based or a dedicated key), which halts input synthesis.
- **FR-REP.5** The system shall prevent more than one replay from running concurrently in a session (supports C-4).
- **FR-REP.6** On any step failure, the engine shall stop (default) or continue (if the step is marked non-critical), and in both cases record the failure with a screenshot.
- **FR-REP.7** The engine shall emit structured events for each step: started, succeeded, succeeded-with-low-confidence, failed (with reason and screenshot).

### 3.3 Image Matching and Smart Waits
*Description:* Make replay robust against moved windows and timing variation.

- **FR-IMG.1** A step shall be able to target an on-screen element by **template image** rather than fixed coordinates.
- **FR-IMG.2** For an image-targeted step, the engine shall capture the current screen, locate the template, and act at the matched location.
- **FR-IMG.3** Each match shall produce a confidence score; the engine shall act only if confidence ≥ the **action threshold**, and shall flag the step `needs_review` if confidence falls in the **review band** (between review and action thresholds). Both thresholds shall be configurable.
- **FR-IMG.4** The system shall support a **wait-for-image** step: pause until a template appears (or a window/title/pixel condition is met), up to a timeout, before continuing.
- **FR-IMG.5** On match failure or timeout, the engine shall treat the step as failed per FR-REP.6.
- **FR-IMG.6** The system should support fixed-coordinate steps as a fallback for cases where image matching is unsuitable, clearly marked as such.

### 3.4 Script Management (Library)
*Description:* Organize scripts.

- **FR-LIB.1** The system shall present a library listing all scripts with name, tags, and last-run status.
- **FR-LIB.2** The user shall be able to search/filter scripts by name and tag.
- **FR-LIB.3** The user shall be able to run a script immediately ("run now") from the library.
- **FR-LIB.4** The user shall be able to rename, duplicate, and delete scripts.
- **FR-LIB.5** The user shall be able to import and export a script as a portable file (e.g., JSON), so scripts can be backed up or moved.

### 3.5 Script Editing and Building
*Description:* Modify existing scripts and create new ones without always re-recording.

- **FR-EDIT.1** The system shall render a script's steps as an ordered, **reorderable** list, each step showing its action type, target, and parameters.
- **FR-EDIT.2** The user shall be able to insert, delete, and edit individual steps.
- **FR-EDIT.3** The user shall be able to **re-capture a single step's** target/template in place, without re-recording the whole script.
- **FR-EDIT.4** The "build new" entry point shall start a recording (FR-REC) and deposit the result in the library.
- **FR-EDIT.5** Every save shall create a **version snapshot** so that editing never destroys a previously working version (supports rollback).
- **FR-EDIT.6** The editor shall validate steps (e.g., a missing template, an unreachable target) and warn before saving an invalid script.

### 3.6 Scheduling and Triggers
*Description:* Run scripts on a schedule or by hotkey.

- **FR-SCH.1** The user shall be able to create a schedule for a script using an interval or cron-like expression.
- **FR-SCH.2** Each schedule shall be individually enabled/disabled and shall display its next run time.
- **FR-SCH.3** The system shall execute scheduled runs only while the orchestrator is running and the session is active/unlocked (consequence of C-1); the UI shall make this limitation clear.
- **FR-SCH.4** The system shall run as a persistent background/tray application so schedules and hotkeys can fire without the main window open, and should support launching at OS login.
- **FR-SCH.5** The user shall be able to bind a global hotkey to launch a specific script.

### 3.7 Logging and Run History (Audit)
*Description:* See what happened.

- **FR-LOG.1** The system shall record every run with start/finish times, trigger type (manual/scheduled/hotkey), and final status (success / failed / needs-review).
- **FR-LOG.2** For each run, the system shall record per-step events: step index, timestamp, level, message, match confidence (if applicable), and a screenshot path on failure or low confidence.
- **FR-LOG.3** The system shall present a run-history view filterable by script and status, drillable into the step-by-step trace and screenshots.
- **FR-LOG.4** Failure screenshots shall be retained and viewable so a failed unattended run can be diagnosed after the fact.

### 3.8 Review Queue ("Doubts")
*Description:* Surface and resolve uncertain steps. This is the system's distinguishing feature.

- **FR-REV.1** The system shall maintain a queue of steps flagged `needs_review`, populated by low-confidence matches (FR-IMG.3) and failed verification steps (FR-VER.x).
- **FR-REV.2** For each review item, the system shall show the step, the captured screen at the time, the expected target/template, and the observed result (with confidence where applicable).
- **FR-REV.3** The user shall be able to **confirm** an item (the action was correct) or **correct** it.
- **FR-REV.4** Correcting an item shall allow **re-capturing the template** on the spot, and the corrected template shall be written back into the script (creating a version snapshot per FR-EDIT.5).
- **FR-REV.5** Resolving a review item shall update its status and remove it from the active queue.

### 3.9 Verification (supports the Review Queue)
*Description:* Optionally assert expected screen state.

- **FR-VER.1** A step may be marked as a **verification** step that asserts an expected on-screen state (template present/absent, or a region matches a reference).
- **FR-VER.2** A failed verification shall not necessarily abort the run; it shall flag the step `needs_review` (FR-REV.1) and record evidence.

### 3.10 Data Model (cross-cutting)
*Description:* The persisted shape that the above features share.

- **FR-MODEL.1** A **script** shall have: id, name, description, tags, ordered steps (structured), created/updated timestamps, current version.
- **FR-MODEL.2** A **script version** shall be an immutable snapshot of a script's steps at a point in time.
- **FR-MODEL.3** A **step** shall have: type (move / click / type / wait-for-image / verify / fixed-coordinate / …), target (template reference and/or coordinates), parameters, and a critical/non-critical flag.
- **FR-MODEL.4** A **run** shall have: id, script id + version, start/finish, status, trigger.
- **FR-MODEL.5** A **run event** shall have: run id, step index, timestamp, level, message, match confidence, screenshot path.
- **FR-MODEL.6** A **schedule** shall have: id, script id, interval/cron expression, enabled flag, next-run time.

---

## 4. External Interface Requirements

### 4.1 User Interfaces
- A desktop GUI with these primary views: **Library/Dashboard**, **Step Editor**, **Recorder entry**, **Schedules**, **Run History/Logs**, **Review Queue**, **Settings** (thresholds, speed, hotkeys, autostart).
- A **system-tray** presence with quick actions (start/stop a known script, open the window, quit) and a status indicator.
- Global hotkeys: start/stop recording, panic-abort replay, and per-script launch bindings.

### 4.2 Hardware Interfaces
- Standard pointing device and keyboard. The engine reads from and writes to these via OS input APIs; no special hardware.

### 4.3 Software Interfaces
- **OS input APIs** (Windows `SendInput` for synthesis; low-level hooks for capture).
- **Screen capture API** for screenshots and template matching input.
- **OpenCV** for template matching.
- **SQLite** as the embedded store.
- A **JSON** library for script serialization and import/export.

### 4.4 Communications Interfaces
- None external in v1 (local only, C-5). Internal communication between UI, orchestrator, engine, and store is in-process or local IPC depending on the architecture chosen (see Architecture document).

---

## 5. Non-Functional Requirements

### 5.1 Performance
- **NFR-PERF.1** A single template match against a full-screen capture should typically complete within a few hundred milliseconds on the target hardware, so step-to-step latency stays usable.
- **NFR-PERF.2** Replay timing should be accurate enough that configured delays are honored within a small tolerance; the engine should not introduce unbounded drift over a long script.
- **NFR-PERF.3** The UI shall remain responsive (no frozen window) while a replay or a match is in progress — i.e., long-running engine work must not block the interface thread.

### 5.2 Reliability
- **NFR-REL.1** Image-anchored replay shall succeed across window repositioning and ordinary timing variation for a stable target UI.
- **NFR-REL.2** A failed step shall always be recorded with enough evidence (reason + screenshot) to diagnose it later.
- **NFR-REL.3** The store shall be transactional; an interrupted run shall not corrupt the database.
- **NFR-REL.4** Editing a script shall never destroy a prior working version (FR-EDIT.5).

### 5.3 Usability
- **NFR-USE.1** Recording → save → replay shall be achievable in a few clear steps without reading documentation.
- **NFR-USE.2** Steps shall be presented in human-readable form, not as opaque blobs.
- **NFR-USE.3** Session/elevation/secure-field limitations (C-1, C-2, C-3) shall be surfaced as understandable messages rather than silent failures.

### 5.4 Security and Safety
- **NFR-SEC.1** The system shall not attempt to bypass OS input protections (C-3).
- **NFR-SEC.2** The system shall avoid persisting obviously sensitive captured input (e.g., it should warn when recording into what appears to be a password field, and should not store raw keystrokes longer than necessary). *(Best-effort; secure fields generally block capture anyway.)*
- **NFR-SEC.3** A panic-abort (FR-REP.4) shall always be available so a misbehaving replay can be stopped immediately.

### 5.5 Maintainability
- **NFR-MNT.1** The engine, store, and orchestrator/UI shall be separable, with the UI never synthesizing input directly (§2.1).
- **NFR-MNT.2** The project shall use version control (git) from the outset, with build artifacts excluded.
- **NFR-MNT.3** Scripts shall be stored in a documented, portable, human-inspectable format (structured JSON in the store and on export).

### 5.6 Portability
- **NFR-PORT.1** Windows is the only supported platform in v1, but the UI framework and the engine's OS-specific code shall be isolated so that a future cross-platform build is feasible without a rewrite.

---

## 6. Out of Scope (v1) and Future Work

**Explicitly out of scope for v1:**
- Multi-user, accounts, cloud sync, or any network service.
- Locked-session / no-login unattended execution (precluded by C-1).
- Defeating anti-cheat, DRM, CAPTCHAs, or secure fields (C-3).
- Mobile platforms.

**Reserved for later versions (see Roadmap document):**
- Parameterized runs driven by a CSV/data source (run one macro over many rows).
- Control flow within scripts: loops, conditionals, "repeat until."
- Self-healing targeting (fall back from coordinate → image → OCR automatically).
- Emitting browser automation (Playwright/Selenium) when the target is a web page.
- A visual block/drag-and-drop macro builder.
- Cross-platform (Linux/macOS) support.
- Packaging as a signed installer and a CI build pipeline.

---

*End of SRS v1.0.*
