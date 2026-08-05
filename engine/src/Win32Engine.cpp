// The real engine.  ===== OWNED BY THE ENGINE DEVELOPER =====
//
// This is a SKELETON on purpose. It compiles, it links, it runs — and it
// reports every step as unimplemented rather than pretending to work. Filling
// it in is the critical path of the project.
//
// Two rules while working here:
//   - never #include a Qt header from this file or anything under engine/
//   - keep OS-specific calls in engine/src/win32/ once there is more than a
//     handful, so a future port stays feasible
//
// Nothing in app/ should ever need to be edited to make this work. If it does,
// the contract in engine/include/engine/Engine.h is wrong — fix it there, with
// the other developer, and update both sides at once.

#include "engine/Engine.h"

#include <atomic>

#ifdef _WIN32
// NOMINMAX: without it, <windows.h> defines min/max as macros and breaks
// std::min / std::max. WIN32_LEAN_AND_MEAN: skips the half of the API we will
// never touch. Both are the two things everyone forgets exactly once.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace autotasks {

namespace {

class Win32Engine final : public Engine {
public:
    bool replay(const Script& script, const ProgressCallback& onStep) override {
        m_abortRequested = false;

        for (std::size_t i = 0; i < script.steps.size(); ++i) {
            if (m_abortRequested) {
                return false;
            }

            const Step& step = script.steps[i];

            StepResult result;
            result.index = static_cast<int>(i);
            result.outcome = StepOutcome::Failed;
            result.message = "Not implemented yet — see engine/src/Win32Engine.cpp";

            // =============================================================
            // TODO — Phase 1: synthesize input with SendInput().
            //
            //   switch (step.type) {
            //       case StepType::Move:  ...
            //       case StepType::Click: ...
            //       case StepType::Type:  ...
            //       default: break;
            //   }
            //
            // Start with a single hardcoded click and check it actually lands
            // in Notepad before writing anything else.
            // =============================================================

            // =============================================================
            // TODO — Phase 2: driven by a script loaded from JSON rather than
            // built in code. Recording lives here too (SetWindowsHookEx).
            // =============================================================

            // =============================================================
            // TODO — Phase 3: image-anchored targeting. This is the phase that
            // makes the project worth building.
            //
            //   - capture the screen
            //   - cv::matchTemplate + cv::minMaxLoc against step.templatePath
            //   - fill result.confidence with the match score
            //   - below step.confidenceThreshold -> StepOutcome::Failed
            //   - above it but below step.reviewThreshold ->
            //     StepOutcome::LowConfidence, and save a screenshot to
            //     result.screenshotPath
            //
            // Those last two lines are what feeds the review queue. The UI
            // already displays them — see StubEngine.cpp for the shape of the
            // results it expects.
            // =============================================================

            (void)step;  // remove once the switch above uses it

            if (onStep) {
                onStep(result);
            }

            if (step.critical) {
                return false;
            }
        }

        return false;
    }

    void abort() override { m_abortRequested = true; }

    std::string name() const override { return "Win32 engine (skeleton)"; }

private:
    // atomic: abort() is called from the UI thread while replay() runs on a
    // worker thread. Keep it that way whatever else changes in this class.
    std::atomic<bool> m_abortRequested{false};
};

}  // namespace

std::unique_ptr<Engine> createEngine() {
    return std::make_unique<Win32Engine>();
}

}  // namespace autotasks
