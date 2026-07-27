// The simulated engine. Pretends to replay a script without touching the
// mouse or the keyboard.
//
// WHY THIS EXISTS: the real engine is weeks of work (Win32 input, low-level
// hooks, OpenCV matching). The UI needs something that ANSWERS today. This
// implements the exact same Engine interface, so the day the real engine
// works, the UI switches to it by calling a different factory — one line.
//
// It is not throwaway code: it stays useful afterwards as a way to exercise
// the UI without moving the real mouse, and as the engine developer's
// reference for what a well-behaved implementation looks like.

#include "engine/Engine.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace autotasks {

namespace {

// A real step takes real time. Making the stub instant would hide exactly the
// bug this is meant to expose: a UI that freezes because replay() was called
// on the UI thread.
constexpr std::chrono::milliseconds kSimulatedStepDuration{400};

// Deterministic, not random: every UI path must be reproducible on demand.
constexpr std::size_t kLowConfidenceEvery = 4;
constexpr std::size_t kFailureEvery = 7;

class StubEngine final : public Engine {
public:
    bool replay(const Script& script, const ProgressCallback& onStep) override {
        m_abortRequested = false;
        bool allOk = true;

        for (std::size_t i = 0; i < script.steps.size(); ++i) {
            if (m_abortRequested) {
                if (onStep) {
                    onStep(StepResult{.index = static_cast<int>(i),
                                      .outcome = StepOutcome::Failed,
                                      .confidence = 0.0,
                                      .message = "Aborted by user",
                                      .screenshotPath = {}});
                }
                return false;
            }

            std::this_thread::sleep_for(kSimulatedStepDuration);

            StepResult result;
            result.index = static_cast<int>(i);

            // The variety is the point: the UI needs a low-confidence step to
            // build the review queue against, and a failure to build error
            // handling against, long before the real engine can produce either.
            if (i > 0 && i % kFailureEvery == 0) {
                result.outcome = StepOutcome::Failed;
                result.confidence = 0.31;
                result.message = "Template not found before timeout";
                result.screenshotPath = "runs/stub/failure.png";
                allOk = false;
            } else if (i > 0 && i % kLowConfidenceEvery == 0) {
                result.outcome = StepOutcome::LowConfidence;
                result.confidence = 0.68;
                result.message = "Matched below review threshold";
                result.screenshotPath = "runs/stub/low_confidence.png";
            } else {
                result.outcome = StepOutcome::Ok;
                result.confidence = 0.97;
                result.message = "Step completed";
            }

            if (onStep) {
                onStep(result);
            }

            // A non-critical step is allowed to fail without ending the run.
            if (result.outcome == StepOutcome::Failed && script.steps[i].critical) {
                return false;
            }
        }

        return allOk;
    }

    void abort() override { m_abortRequested = true; }

    std::string name() const override { return "Stub engine (simulated)"; }

private:
    // atomic because abort() is called from the UI thread while replay() runs
    // on a worker thread. This is the only cross-thread state in the class.
    std::atomic<bool> m_abortRequested{false};
};

}  // namespace

std::unique_ptr<Engine> createStubEngine() {
    return std::make_unique<StubEngine>();
}

Script makeDemoScript() {
    Script script;
    script.id = "demo";
    script.name = "Demo script";

    // Designated initialisers (C++20): every field not named keeps the default
    // declared in Step. Adding a field to Step will not silently shift values
    // here, unlike positional initialisation.
    script.steps = {
        Step{.type = StepType::Move, .x = 400, .y = 300},
        Step{.type = StepType::Click,
             .x = 400,
             .y = 300,
             .templatePath = "templates/open_button.png"},
        Step{.type = StepType::WaitForImage, .templatePath = "templates/dialog.png"},
        Step{.type = StepType::Type, .text = "AutoTasks"},
        Step{.type = StepType::Click, .x = 620, .y = 480, .templatePath = "templates/confirm.png"},
        Step{.type = StepType::Verify,
             .templatePath = "templates/success_banner.png",
             .critical = false},
    };

    return script;
}

}  // namespace autotasks
