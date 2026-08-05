#pragma once

// The application's data layer — IN MEMORY, on purpose.
//
// WHY THIS EXISTS: the views need scripts, runs, schedules and a review queue
// to be worth anything, but the SQLite store is Phase 4. Rather than leave every
// page disabled until then, the data lives here and the pages work today.
//
// WHEN THE STORE ARRIVES: keep this class and its signals; replace the QVector
// members with queries. The pages never touch storage directly, so they will
// not change. Nothing here is persisted — closing the app loses everything, and
// that is the one thing Phase 4 fixes.
//
// This is also the orchestrator described in the architecture doc: it owns the
// ReplayController, so a run started from Library, from a schedule, or from
// anywhere else is recorded the same way and feeds History and Review.

#include "engine/Engine.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTime>
#include <QVector>

#include <vector>

class QTimer;

namespace autotasks {

class ReplayController;

/// A script as the UI sees it: the engine's Script plus the bookkeeping the
/// library needs.
struct ScriptEntry {
    QString id;
    QString name;
    QString tags;
    std::vector<Step> steps;
    QString lastStatus;  ///< empty, "Completed", "Failed" or "Needs review"
    QDateTime lastRunAt;
};

/// One line of a run's trace, as reported by the engine.
struct StepRecord {
    int index = 0;
    int outcome = 0;  ///< mirrors StepOutcome: 0 Ok, 1 LowConfidence, 2 Failed
    double confidence = 0.0;
    QString message;
    QString screenshotPath;
};

struct RunRecord {
    QString id;
    QString scriptId;
    QString scriptName;
    QString engineName;
    QDateTime startedAt;
    QString status;
    QVector<StepRecord> steps;
};

enum class TriggerKind { Interval, Daily, Cron };

struct ScheduleEntry {
    QString id;
    QString scriptId;
    TriggerKind kind = TriggerKind::Interval;
    int interval = 30;
    int unitIndex = 0;  ///< 0 minutes, 1 hours, 2 days
    QTime dailyTime{9, 0};
    QString cron;
    bool enabled = true;
    QDateTime nextRun;
    QDateTime lastRun;
};

/// A step the engine acted on but was not confident about.
struct ReviewItem {
    QString runId;
    QString scriptId;
    QString scriptName;
    int stepIndex = 0;
    double confidence = 0.0;
    double threshold = 0.95;
    QString message;
    QString screenshotPath;
    QString templatePath;
};

class AppModel : public QObject {
    Q_OBJECT

public:
    explicit AppModel(QObject* parent = nullptr);
    ~AppModel() override;

    AppModel(const AppModel&) = delete;
    AppModel& operator=(const AppModel&) = delete;
    AppModel(AppModel&&) = delete;
    AppModel& operator=(AppModel&&) = delete;

    // ---- Reading ---------------------------------------------------------
    const QVector<ScriptEntry>& scripts() const { return m_scripts; }

    const QVector<RunRecord>& runs() const { return m_runs; }

    const QVector<ScheduleEntry>& schedules() const { return m_schedules; }

    const QVector<ReviewItem>& review() const { return m_review; }

    const ScriptEntry* script(const QString& id) const;
    int scriptRow(const QString& id) const;

    // ---- Scripts ---------------------------------------------------------
    QString createScript(const QString& name, const QString& tags);
    void removeScript(const QString& id);
    /// Replaces name, tags and steps of an existing script.
    void updateScript(const ScriptEntry& entry);

    // ---- Schedules -------------------------------------------------------
    QString createSchedule(const QString& scriptId);
    void removeSchedule(const QString& id);
    void updateSchedule(const ScheduleEntry& entry);

    // ---- Review queue ----------------------------------------------------
    void resolveReview(int row);

    // ---- Running ---------------------------------------------------------
    bool isRunning() const { return m_running; }

    QString runningScriptId() const { return m_currentRun.scriptId; }

    /// Starts a replay. No-op if one is already running.
    void runScript(const QString& scriptId, bool useStub);
    void abortRun();

    /// Recomputes nextRun for a schedule from its trigger. Cron is not
    /// evaluated yet — those schedules stay dormant (Phase 8).
    static QDateTime computeNextRun(const ScheduleEntry& entry, const QDateTime& from);

signals:
    void scriptsChanged();
    void runsChanged();
    void schedulesChanged();
    void reviewChanged();

    void runStarted(const QString& scriptName, const QString& engineName, int stepCount);
    void stepReported(int index, int outcome, double confidence, const QString& message,
                      const QString& screenshotPath);
    void runFinished(bool success);

private slots:
    void onControllerRunStarted(const QString& engineName);
    void onControllerStep(int index, int outcome, double confidence, const QString& message,
                          const QString& screenshotPath);
    void onControllerFinished(bool success);
    void onScheduleTick();

private:
    void seedSampleScripts();

    QVector<ScriptEntry> m_scripts;
    QVector<RunRecord> m_runs;
    QVector<ScheduleEntry> m_schedules;
    QVector<ReviewItem> m_review;

    ReplayController* m_replay = nullptr;
    QTimer* m_scheduleTimer = nullptr;

    RunRecord m_currentRun;
    bool m_running = false;
    bool m_currentUsedStub = true;

    int m_nextId = 1;
};

}  // namespace autotasks
