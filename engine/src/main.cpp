// AutoTasks — console entry point.  ===== ENGINE DEVELOPER'S TEST BENCH =====
//
// A thin CLI over autotasks_core. It exercises the library WITHOUT Qt, so the
// engine can be built, run and debugged on a machine where Qt is not installed
// at all:
//
//     cmake --preset engine
//     cmake --build --preset engine
//
// Usage:
//     autotasks_engine            run the demo script on the real engine
//     autotasks_engine --stub     run it on the simulated engine
//
// Keep this file thin. Anything worth more than a few lines belongs in
// autotasks_core, where the Qt application can reach it too.

#include "engine/Engine.h"

#include <iostream>
#include <string_view>

namespace {

const char* outcomeLabel(autotasks::StepOutcome outcome) {
    switch (outcome) {
        case autotasks::StepOutcome::Ok: return "OK  ";
        case autotasks::StepOutcome::LowConfidence: return "WARN";
        case autotasks::StepOutcome::Failed: return "FAIL";
    }
    return "????";
}

}  // namespace

int main(int argc, char* argv[]) {
    bool useStub = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--stub") {
            useStub = true;
        }
    }

    const auto engine = useStub ? autotasks::createStubEngine() : autotasks::createEngine();
    const autotasks::Script script = autotasks::makeDemoScript();

    std::cout << "AutoTasks — " << engine->name() << '\n'
              << "Script: " << script.name << " (" << script.steps.size() << " steps)\n"
              << "-----------------------------------------------------------\n";

    const bool success = engine->replay(script, [](const autotasks::StepResult& result) {
        std::cout << "  [" << outcomeLabel(result.outcome) << "] step " << result.index
                  << "  conf=" << result.confidence << "  " << result.message << '\n';
    });

    std::cout << "-----------------------------------------------------------\n"
              << (success ? "Run completed successfully." : "Run did not complete.") << '\n';

    return success ? 0 : 1;
}
