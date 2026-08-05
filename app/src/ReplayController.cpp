#include "app/ReplayController.h"

#include "engine/Engine.h"

#include <QThread>

namespace autotasks {

namespace {

// How long the destructor waits for a running replay before giving up.
// The stub takes ~400 ms per step, so this is generous by design.
constexpr int kShutdownWaitMs = 5000;

}  // namespace

ReplayController::ReplayController(QObject* parent) : QObject(parent) {}

ReplayController::~ReplayController() {
    if (m_thread == nullptr) {
        return;
    }

    abort();
    m_thread->wait(kShutdownWaitMs);

    // deleteLater() would never fire here: the event loop that would process
    // it is already gone by the time a top-level widget's children are
    // destroyed. Delete directly instead.
    delete m_thread;
    m_thread = nullptr;
}

bool ReplayController::isRunning() const {
    return m_thread != nullptr;
}

void ReplayController::start(const Script& script, bool useStub) {
    if (m_thread != nullptr) {
        return;  // one replay at a time (SRS: FR-REP.5)
    }

    m_engine = useStub ? createStubEngine() : createEngine();

    emit runStarted(QString::fromStdString(m_engine->name()));

    // Captured by value: the worker keeps the engine alive on its own, so the
    // run cannot be cut short by this object releasing its reference.
    auto engine = m_engine;

    m_thread = QThread::create([this, engine, script]() {
        const bool success = engine->replay(script, [this](const StepResult& result) {
            // Emitted from the WORKER thread. Because `this` lives on the UI
            // thread, Qt queues the delivery automatically — that is the whole
            // reason for going through signals instead of calling widgets.
            emit stepReported(result.index, static_cast<int>(result.outcome), result.confidence,
                              QString::fromStdString(result.message),
                              QString::fromStdString(result.screenshotPath));
        });

        emit runFinished(success);
    });

    connect(m_thread, &QThread::finished, this, &ReplayController::onThreadFinished);
    m_thread->start();
}

void ReplayController::abort() {
    if (m_engine) {
        m_engine->abort();  // atomic flag — safe to call from this thread
    }
}

void ReplayController::onThreadFinished() {
    if (m_thread != nullptr) {
        m_thread->deleteLater();
        m_thread = nullptr;
    }
    m_engine.reset();
}

}  // namespace autotasks
