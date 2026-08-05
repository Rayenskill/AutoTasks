#include "app/AppModel.h"

#include "app/ReplayController.h"

#include <QTimer>

#include <algorithm>

namespace autotasks {

namespace {

// How often the scheduler checks whether something is due. Fine-grained enough
// for minute-level triggers without spinning.
constexpr int kScheduleTickMs = 15000;

QString outcomeToStatus(int worstOutcome) {
    switch (worstOutcome) {
        case 2: return QStringLiteral("Failed");
        case 1: return QStringLiteral("Needs review");
        default: return QStringLiteral("Completed");
    }
}

}  // namespace

AppModel::AppModel(QObject* parent) : QObject(parent) {
    m_replay = new ReplayController(this);

    connect(m_replay, &ReplayController::runStarted, this, &AppModel::onControllerRunStarted);
    connect(m_replay, &ReplayController::stepReported, this, &AppModel::onControllerStep);
    connect(m_replay, &ReplayController::runFinished, this, &AppModel::onControllerFinished);

    m_scheduleTimer = new QTimer(this);
    m_scheduleTimer->setInterval(kScheduleTickMs);
    connect(m_scheduleTimer, &QTimer::timeout, this, &AppModel::onScheduleTick);
    m_scheduleTimer->start();

    seedSampleScripts();
}

AppModel::~AppModel() = default;

// =========================================================================
// Sample content
//
// Without persistence there is nothing to load at startup, and an empty
// library makes every view impossible to try. These are ordinary editable
// scripts, not fake results — no run history and no review entries are
// invented; those only appear once you actually run something.
// =========================================================================
void AppModel::seedSampleScripts() {
    ScriptEntry demo;
    demo.id = QStringLiteral("s%1").arg(m_nextId++);
    demo.name = QStringLiteral("Demo script");
    demo.tags = QStringLiteral("demo");
    demo.steps = makeDemoScript().steps;  // the very script the CLI runs
    m_scripts.push_back(demo);

    ScriptEntry report;
    report.id = QStringLiteral("s%1").arg(m_nextId++);
    report.name = QStringLiteral("Weekly report export");
    report.tags = QStringLiteral("reporting, weekly");
    report.steps = {
        Step{.type = StepType::WaitForImage, .templatePath = "templates/app_ready.png"},
        Step{.type = StepType::Click,
             .x = 180,
             .y = 96,
             .templatePath = "templates/reports_tab.png"},
        Step{.type = StepType::Click,
             .x = 520,
             .y = 340,
             .templatePath = "templates/export_button.png"},
        Step{.type = StepType::Type, .text = "weekly"},
        Step{.type = StepType::Verify,
             .templatePath = "templates/export_done.png",
             .critical = false},
    };
    m_scripts.push_back(report);

    ScriptEntry cleanup;
    cleanup.id = QStringLiteral("s%1").arg(m_nextId++);
    cleanup.name = QStringLiteral("Clear staging folder");
    cleanup.tags = QStringLiteral("maintenance");
    cleanup.steps = {
        Step{.type = StepType::Move, .x = 640, .y = 400},
        Step{.type = StepType::Click, .x = 640, .y = 400, .templatePath = "templates/folder.png"},
        Step{.type = StepType::Type, .text = ""},  // Delete
    };
    m_scripts.push_back(cleanup);
}

// =========================================================================
// Scripts
// =========================================================================
const ScriptEntry* AppModel::script(const QString& id) const {
    for (const ScriptEntry& entry : m_scripts) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

int AppModel::scriptRow(const QString& id) const {
    for (int i = 0; i < m_scripts.size(); ++i) {
        if (m_scripts[i].id == id) {
            return i;
        }
    }
    return -1;
}

QString AppModel::createScript(const QString& name, const QString& tags) {
    ScriptEntry entry;
    entry.id = QStringLiteral("s%1").arg(m_nextId++);
    entry.name = name.isEmpty() ? tr("Untitled script") : name;
    entry.tags = tags;
    m_scripts.push_back(entry);

    emit scriptsChanged();
    return entry.id;
}

void AppModel::removeScript(const QString& id) {
    const int row = scriptRow(id);
    if (row < 0) {
        return;
    }
    m_scripts.remove(row);

    // A schedule pointing at a deleted script would fire into the void.
    const auto before = m_schedules.size();
    m_schedules.removeIf([&id](const ScheduleEntry& s) { return s.scriptId == id; });

    emit scriptsChanged();
    if (m_schedules.size() != before) {
        emit schedulesChanged();
    }
}

void AppModel::updateScript(const ScriptEntry& entry) {
    const int row = scriptRow(entry.id);
    if (row < 0) {
        return;
    }

    // Run bookkeeping belongs to the model, not to the editing page.
    ScriptEntry updated = entry;
    updated.lastStatus = m_scripts[row].lastStatus;
    updated.lastRunAt = m_scripts[row].lastRunAt;

    m_scripts[row] = updated;
    emit scriptsChanged();
}

// =========================================================================
// Schedules
// =========================================================================
QString AppModel::createSchedule(const QString& scriptId) {
    ScheduleEntry entry;
    entry.id = QStringLiteral("sch%1").arg(m_nextId++);
    entry.scriptId = scriptId;
    entry.nextRun = computeNextRun(entry, QDateTime::currentDateTime());
    m_schedules.push_back(entry);

    emit schedulesChanged();
    return entry.id;
}

void AppModel::removeSchedule(const QString& id) {
    const auto before = m_schedules.size();
    m_schedules.removeIf([&id](const ScheduleEntry& s) { return s.id == id; });
    if (m_schedules.size() != before) {
        emit schedulesChanged();
    }
}

void AppModel::updateSchedule(const ScheduleEntry& entry) {
    for (ScheduleEntry& existing : m_schedules) {
        if (existing.id != entry.id) {
            continue;
        }
        const QDateTime lastRun = existing.lastRun;
        existing = entry;
        existing.lastRun = lastRun;
        existing.nextRun =
            entry.enabled ? computeNextRun(existing, QDateTime::currentDateTime()) : QDateTime();
        emit schedulesChanged();
        return;
    }
}

QDateTime AppModel::computeNextRun(const ScheduleEntry& entry, const QDateTime& from) {
    if (!entry.enabled) {
        return {};
    }

    switch (entry.kind) {
        case TriggerKind::Interval: {
            switch (entry.unitIndex) {
                case 1: return from.addSecs(entry.interval * 3600LL);
                case 2: return from.addDays(entry.interval);
                default: return from.addSecs(entry.interval * 60LL);
            }
        }
        case TriggerKind::Daily: {
            QDateTime next(from.date(), entry.dailyTime);
            if (next <= from) {
                next = next.addDays(1);
            }
            return next;
        }
        case TriggerKind::Cron:
            // TODO (Phase 8): parse the expression. Until then a cron schedule
            // has no next run and never fires — better than firing wrongly.
            return {};
    }
    return {};
}

void AppModel::onScheduleTick() {
    if (m_running) {
        return;  // one replay at a time (SRS: FR-REP.5)
    }

    const QDateTime now = QDateTime::currentDateTime();
    for (ScheduleEntry& entry : m_schedules) {
        if (!entry.enabled || !entry.nextRun.isValid() || entry.nextRun > now) {
            continue;
        }

        entry.lastRun = now;
        entry.nextRun = computeNextRun(entry, now);
        emit schedulesChanged();

        runScript(entry.scriptId, /*useStub=*/true);
        return;  // at most one launch per tick
    }
}

// =========================================================================
// Review queue
// =========================================================================
void AppModel::resolveReview(int row) {
    if (row < 0 || row >= m_review.size()) {
        return;
    }
    m_review.remove(row);
    emit reviewChanged();
}

// =========================================================================
// Running
// =========================================================================
void AppModel::runScript(const QString& scriptId, bool useStub) {
    if (m_running) {
        return;
    }

    const ScriptEntry* entry = script(scriptId);
    if (entry == nullptr || entry->steps.empty()) {
        return;
    }

    Script script;
    script.id = entry->id.toStdString();
    script.name = entry->name.toStdString();
    script.steps = entry->steps;

    m_currentRun = RunRecord{};
    m_currentRun.id = QStringLiteral("r%1").arg(m_nextId++);
    m_currentRun.scriptId = entry->id;
    m_currentRun.scriptName = entry->name;
    m_currentRun.startedAt = QDateTime::currentDateTime();
    m_currentUsedStub = useStub;

    m_running = true;
    m_replay->start(script, useStub);
}

void AppModel::abortRun() {
    m_replay->abort();
}

void AppModel::onControllerRunStarted(const QString& engineName) {
    m_currentRun.engineName = engineName;

    const ScriptEntry* entry = script(m_currentRun.scriptId);
    const int stepCount = entry != nullptr ? static_cast<int>(entry->steps.size()) : 0;

    emit runStarted(m_currentRun.scriptName, engineName, stepCount);
}

void AppModel::onControllerStep(int index, int outcome, double confidence, const QString& message,
                                const QString& screenshotPath) {
    m_currentRun.steps.push_back(StepRecord{index, outcome, confidence, message, screenshotPath});

    emit stepReported(index, outcome, confidence, message, screenshotPath);
}

void AppModel::onControllerFinished(bool success) {
    m_running = false;

    // The run's status is its worst step: one failure fails the run, one
    // low-confidence step sends it to review.
    int worst = 0;
    for (const StepRecord& step : m_currentRun.steps) {
        worst = std::max(worst, step.outcome);
    }
    m_currentRun.status = success ? outcomeToStatus(worst) : QStringLiteral("Failed");

    const int row = scriptRow(m_currentRun.scriptId);
    if (row >= 0) {
        m_scripts[row].lastStatus = m_currentRun.status;
        m_scripts[row].lastRunAt = m_currentRun.startedAt;
        emit scriptsChanged();
    }

    // Every low-confidence step becomes a review entry. This is the loop the
    // whole project is built around, and the stub engine already feeds it.
    const ScriptEntry* entry = script(m_currentRun.scriptId);
    bool addedReview = false;
    for (const StepRecord& step : m_currentRun.steps) {
        if (step.outcome != 1) {
            continue;
        }

        ReviewItem item;
        item.runId = m_currentRun.id;
        item.scriptId = m_currentRun.scriptId;
        item.scriptName = m_currentRun.scriptName;
        item.stepIndex = step.index;
        item.confidence = step.confidence;
        item.message = step.message;
        item.screenshotPath = step.screenshotPath;

        if (entry != nullptr && step.index >= 0 &&
            step.index < static_cast<int>(entry->steps.size())) {
            const Step& source = entry->steps[static_cast<std::size_t>(step.index)];
            item.templatePath = QString::fromStdString(source.templatePath);
            item.threshold = source.reviewThreshold;
        }

        m_review.push_back(item);
        addedReview = true;
    }

    m_runs.prepend(m_currentRun);  // newest first
    emit runsChanged();
    if (addedReview) {
        emit reviewChanged();
    }

    emit runFinished(success);
}

}  // namespace autotasks
