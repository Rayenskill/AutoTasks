#pragma once

// =========================================================================
// THE CONTRACT between the engine and the UI.
//
// This header is the agreement between the two developers:
//   - the UI codes against it and never looks at how it is implemented,
//   - the engine implements it and never looks at how it is consumed.
//
// Changing anything in this file breaks the other side, so it is the one
// file that must be modified BY AGREEMENT, never unilaterally. Everything
// else in engine/ and app/ belongs to one owner and can be changed freely.
//
// Two rules keep the separation honest:
//   - no #include <Q...> anywhere under engine/
//   - no SendInput / <windows.h> anywhere under app/
//
// Deliberately minimal for now. It will grow at Phase 3 (image matching)
// and Phase 9 (review queue) — grow it on purpose, together.
// =========================================================================

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace autotasks {

/// What a single step does. Extended as phases land.
enum class StepType {
    Move,          ///< move the pointer
    Click,         ///< press and release a mouse button
    Type,          ///< send keystrokes
    WaitForImage,  ///< pause until a template appears on screen (Phase 3)
    Verify,        ///< assert the screen looks as expected (Phase 9)
};

/// One action in a script.
///
/// Fixed coordinates are a FALLBACK. The whole point of the project is that a
/// step means "find this button and click it", not "click pixel (450, 300)" —
/// so prefer templatePath whenever there is one.
struct Step {
    StepType type = StepType::Move;

    int x = 0;
    int y = 0;

    std::string text;          ///< payload for StepType::Type
    std::string templatePath;  ///< image-anchored target (Phase 3)

    double confidenceThreshold = 0.80;  ///< act only at or above this match score
    double reviewThreshold = 0.95;      ///< acted, but below this -> flag for review
    bool critical = true;               ///< false = keep going when this step fails
};

/// A named, replayable sequence.
struct Script {
    std::string id;
    std::string name;
    std::vector<Step> steps;
};

/// How a single step turned out.
enum class StepOutcome {
    Ok,
    LowConfidence,  ///< acted, but below reviewThreshold -> goes to the review queue
    Failed,
};

/// What the engine reports back after each step.
struct StepResult {
    int index = 0;
    StepOutcome outcome = StepOutcome::Ok;
    double confidence = 0.0;     ///< 0..1, meaningful for image-anchored steps
    std::string message;         ///< human-readable, shown in logs and the UI
    std::string screenshotPath;  ///< filled on failure or low confidence
};

/// Called once per step as replay progresses.
///
/// IMPORTANT: invoked on whatever thread runs replay() — NOT the UI thread.
/// Qt code must therefore go through a queued signal and must never touch a
/// widget from here. See app/include/app/ReplayController.h, which exists
/// precisely so no one has to think about this twice.
using ProgressCallback = std::function<void(const StepResult&)>;

/// The engine interface. Everything the UI is allowed to ask of the engine.
class Engine {
public:
    virtual ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    /// Replays a script, calling onStep after each step.
    ///
    /// BLOCKING: returns only once the run is over. Never call it directly
    /// from the UI thread. Returns true if the run completed with no failed
    /// step.
    virtual bool replay(const Script& script, const ProgressCallback& onStep) = 0;

    /// Asks a running replay to stop as soon as it can.
    /// Safe to call from another thread — this is the panic-abort path.
    virtual void abort() = 0;

    /// Name of this implementation, for logs and the status bar.
    virtual std::string name() const = 0;

protected:
    Engine() = default;
};

/// The real engine: Win32 input plus OpenCV matching.
/// Implemented in engine/src/Win32Engine.cpp — OWNED BY THE ENGINE DEVELOPER.
std::unique_ptr<Engine> createEngine();

/// A fake engine that simulates a run without touching the mouse or keyboard.
/// This is what lets the UI be built and tested before the real engine exists.
/// Implemented in engine/src/StubEngine.cpp.
std::unique_ptr<Engine> createStubEngine();

/// A short demo script, so both sides have something to run from day one.
/// Replaced by real scripts loaded from the store at Phase 4.
Script makeDemoScript();

}  // namespace autotasks
