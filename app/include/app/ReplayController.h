#pragma once

// The bridge between the Qt UI and the engine.
//
// WHY THIS CLASS EXISTS: Engine::replay() blocks for seconds or minutes.
// Called on the UI thread it would freeze the window solid — no repaint, no
// Abort button, "application not responding". This runs it on a worker thread
// and turns the engine's callbacks into Qt signals, which Qt then delivers
// back on the UI thread automatically.
//
// The rule that follows, and the only one that matters here: NO WIDGET IS EVER
// TOUCHED FROM THE ENGINE'S THREAD. Connect to the signals below and do the UI
// work in the slots.
//
// ===== FOR THE FRONTEND DEVELOPER =====
// This is scaffolding, not a finished design. Wire it up like this:
//
//     m_replay = new ReplayController(this);
//     connect(m_replay, &ReplayController::runStarted,  this, &MyView::onRunStarted);
//     connect(m_replay, &ReplayController::stepReported, this, &MyView::onStep);
//     connect(m_replay, &ReplayController::runFinished, this, &MyView::onRunFinished);
//     m_replay->start(/*useStub=*/true);
//
// Change it freely — it lives in app/, it is yours. Just keep replay() off the
// UI thread.

#include <QObject>
#include <QString>

#include <memory>

class QThread;

namespace autotasks {

class Engine;

class ReplayController : public QObject {
    Q_OBJECT

public:
    explicit ReplayController(QObject* parent = nullptr);
    ~ReplayController() override;

    ReplayController(const ReplayController&) = delete;
    ReplayController& operator=(const ReplayController&) = delete;
    ReplayController(ReplayController&&) = delete;
    ReplayController& operator=(ReplayController&&) = delete;

    /// Starts a replay of the demo script. No-op if one is already running.
    /// useStub selects the simulated engine instead of the real one.
    ///
    /// TODO (Phase 4): take a Script instead, once the store can supply one.
    void start(bool useStub);

    /// Asks the running replay to stop. Safe from the UI thread.
    void abort();

    /// Note: turns false slightly AFTER the run ends, because the notification
    /// is queued through the event loop. Drive button states from the
    /// runStarted / runFinished signals rather than polling this.
    bool isRunning() const;

signals:
    void runStarted(QString engineName);

    /// outcome maps to autotasks::StepOutcome: 0 = Ok, 1 = LowConfidence, 2 = Failed.
    /// Sent as int because queued connections only carry types Qt knows about,
    /// and this keeps engine/Engine.h out of every UI header.
    void stepReported(int index, int outcome, double confidence, QString message,
                      QString screenshotPath);

    void runFinished(bool success);

private slots:
    void onThreadFinished();

private:
    QThread* m_thread = nullptr;

    // shared_ptr, not unique_ptr: the worker lambda holds its own copy, so the
    // engine stays alive until the thread is genuinely done with it — even if
    // this controller drops its reference first.
    std::shared_ptr<Engine> m_engine;
};

}  // namespace autotasks
