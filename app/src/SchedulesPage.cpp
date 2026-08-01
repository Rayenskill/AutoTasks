#include "app/SchedulesPage.h"

#include "app/AppModel.h"

#include "ui_SchedulesPage.h"

#include <QStandardItemModel>

namespace autotasks {

namespace {

constexpr int kIdRole = Qt::UserRole + 1;

QString triggerSummary(const ScheduleEntry& entry) {
    switch (entry.kind) {
        case TriggerKind::Interval: {
            const QString unit = entry.unitIndex == 1   ? QStringLiteral("h")
                                 : entry.unitIndex == 2 ? QStringLiteral("d")
                                                        : QStringLiteral("min");
            return QStringLiteral("every %1 %2").arg(entry.interval).arg(unit);
        }
        case TriggerKind::Daily:
            return QStringLiteral("daily at %1")
                .arg(entry.dailyTime.toString(QStringLiteral("HH:mm")));
        case TriggerKind::Cron: return QStringLiteral("cron: %1").arg(entry.cron);
    }
    return {};
}

QString formatWhen(const QDateTime& when) {
    return when.isValid() ? when.toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QStringLiteral("—");
}

}  // namespace

SchedulesPage::SchedulesPage(AppModel* model, QWidget* parent)
    : QWidget(parent), m_ui(std::make_unique<Ui::SchedulesPage>()), m_model(model) {
    m_ui->setupUi(this);

    m_ui->schedulesSplitter->setStretchFactor(0, 2);
    m_ui->schedulesSplitter->setStretchFactor(1, 3);

    m_source = new QStandardItemModel(this);
    m_source->setHorizontalHeaderLabels(
        {tr("Script"), tr("Trigger"), tr("Enabled"), tr("Next run"), tr("Last run")});
    m_ui->schedulesTable->setModel(m_source);
    m_ui->schedulesTable->horizontalHeader()->setStretchLastSection(true);

    connect(m_ui->schedulesTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &SchedulesPage::onSelectionChanged);

    connect(m_ui->intervalRadio, &QRadioButton::toggled, this,
            &SchedulesPage::onTriggerKindChanged);
    connect(m_ui->dailyRadio, &QRadioButton::toggled, this, &SchedulesPage::onTriggerKindChanged);
    connect(m_ui->cronRadio, &QRadioButton::toggled, this, &SchedulesPage::onTriggerKindChanged);

    connect(m_ui->addScheduleButton, &QPushButton::clicked, this, &SchedulesPage::onAddClicked);
    connect(m_ui->deleteScheduleButton, &QPushButton::clicked, this,
            &SchedulesPage::onDeleteClicked);
    connect(m_ui->saveScheduleButton, &QPushButton::clicked, this, &SchedulesPage::onSaveClicked);
    connect(m_ui->runNowButton, &QPushButton::clicked, this, &SchedulesPage::onRunNowClicked);

    // Any edit in the trigger form arms the Save button.
    const auto markDirty = [this]() {
        if (!m_binding) {
            m_ui->saveScheduleButton->setEnabled(true);
        }
    };
    connect(m_ui->scriptCombo, &QComboBox::currentIndexChanged, this, markDirty);
    connect(m_ui->intervalSpin, &QSpinBox::valueChanged, this, markDirty);
    connect(m_ui->intervalUnitCombo, &QComboBox::currentIndexChanged, this, markDirty);
    connect(m_ui->dailyTimeEdit, &QTimeEdit::timeChanged, this, markDirty);
    connect(m_ui->cronEdit, &QLineEdit::textEdited, this, markDirty);
    connect(m_ui->enabledCheck, &QCheckBox::toggled, this, markDirty);
    connect(m_ui->intervalRadio, &QRadioButton::toggled, this, markDirty);
    connect(m_ui->dailyRadio, &QRadioButton::toggled, this, markDirty);
    connect(m_ui->cronRadio, &QRadioButton::toggled, this, markDirty);

    connect(m_model, &AppModel::schedulesChanged, this, &SchedulesPage::reload);
    connect(m_model, &AppModel::scriptsChanged, this, &SchedulesPage::reloadScripts);
    connect(m_model, &AppModel::runFinished, this, [this](bool) { onSelectionChanged(); });

    m_ui->cronEdit->setToolTip(tr("Parsed at Phase 8 — a cron schedule stays dormant for now."));

    reloadScripts();
    reload();
}

SchedulesPage::~SchedulesPage() = default;

void SchedulesPage::reloadScripts() {
    const QSignalBlocker blocker(m_ui->scriptCombo);
    const QString keep = m_ui->scriptCombo->currentData().toString();

    m_ui->scriptCombo->clear();
    for (const ScriptEntry& entry : m_model->scripts()) {
        m_ui->scriptCombo->addItem(entry.name, entry.id);
    }

    const int found = m_ui->scriptCombo->findData(keep);
    m_ui->scriptCombo->setCurrentIndex(found >= 0 ? found : 0);

    m_ui->addScheduleButton->setEnabled(!m_model->scripts().isEmpty());
}

void SchedulesPage::reload() {
    const QString keep = selectedScheduleId();

    m_source->removeRows(0, m_source->rowCount());

    for (const ScheduleEntry& entry : m_model->schedules()) {
        const ScriptEntry* script = m_model->script(entry.scriptId);

        QList<QStandardItem*> row;
        auto* first = new QStandardItem(script != nullptr ? script->name : tr("(deleted)"));
        first->setData(entry.id, kIdRole);
        row << first;
        row << new QStandardItem(triggerSummary(entry));
        row << new QStandardItem(entry.enabled ? tr("yes") : tr("no"));
        row << new QStandardItem(formatWhen(entry.nextRun));
        row << new QStandardItem(formatWhen(entry.lastRun));

        for (QStandardItem* item : row) {
            item->setEditable(false);
        }
        m_source->appendRow(row);
    }

    m_ui->schedulesTable->resizeColumnsToContents();

    if (!keep.isEmpty()) {
        for (int i = 0; i < m_source->rowCount(); ++i) {
            if (m_source->item(i, 0)->data(kIdRole).toString() == keep) {
                m_ui->schedulesTable->selectRow(i);
                break;
            }
        }
    }

    onSelectionChanged();
}

QString SchedulesPage::selectedScheduleId() const {
    const QModelIndexList selected = m_ui->schedulesTable->selectionModel()->selectedRows(0);
    return selected.isEmpty() ? QString() : selected.first().data(kIdRole).toString();
}

void SchedulesPage::onSelectionChanged() {
    const bool hasSelection = !selectedScheduleId().isEmpty();

    m_ui->triggerGroup->setEnabled(hasSelection);
    m_ui->statusGroup->setEnabled(hasSelection);
    m_ui->deleteScheduleButton->setEnabled(hasSelection);
    m_ui->runNowButton->setEnabled(hasSelection && !m_model->isRunning());

    bindToForm();
}

void SchedulesPage::bindToForm() {
    const QString id = selectedScheduleId();
    const ScheduleEntry* found = nullptr;
    for (const ScheduleEntry& entry : m_model->schedules()) {
        if (entry.id == id) {
            found = &entry;
            break;
        }
    }
    if (found == nullptr) {
        m_ui->saveScheduleButton->setEnabled(false);
        return;
    }

    // Writing into the widgets fires their signals; the guard stops those from
    // arming Save as if the user had typed.
    m_binding = true;
    const int scriptIndex = m_ui->scriptCombo->findData(found->scriptId);
    m_ui->scriptCombo->setCurrentIndex(scriptIndex >= 0 ? scriptIndex : 0);

    m_ui->intervalRadio->setChecked(found->kind == TriggerKind::Interval);
    m_ui->dailyRadio->setChecked(found->kind == TriggerKind::Daily);
    m_ui->cronRadio->setChecked(found->kind == TriggerKind::Cron);

    m_ui->intervalSpin->setValue(found->interval);
    m_ui->intervalUnitCombo->setCurrentIndex(found->unitIndex);
    m_ui->dailyTimeEdit->setTime(found->dailyTime);
    m_ui->cronEdit->setText(found->cron);
    m_ui->enabledCheck->setChecked(found->enabled);

    m_ui->nextRunLabel->setText(tr("Next run: %1").arg(formatWhen(found->nextRun)));
    m_ui->lastRunLabel->setText(tr("Last run: %1").arg(formatWhen(found->lastRun)));
    m_binding = false;

    onTriggerKindChanged();
    m_ui->saveScheduleButton->setEnabled(false);
}

void SchedulesPage::onTriggerKindChanged() {
    // Only one trigger kind is active at a time — greying out the other inputs
    // means an unused field can never be mistaken for a configured one.
    m_ui->intervalSpin->setEnabled(m_ui->intervalRadio->isChecked());
    m_ui->intervalUnitCombo->setEnabled(m_ui->intervalRadio->isChecked());
    m_ui->dailyTimeEdit->setEnabled(m_ui->dailyRadio->isChecked());
    m_ui->cronEdit->setEnabled(m_ui->cronRadio->isChecked());
}

void SchedulesPage::onAddClicked() {
    if (m_model->scripts().isEmpty()) {
        return;
    }

    const QString scriptId = m_ui->scriptCombo->currentData().toString();
    const QString created =
        m_model->createSchedule(scriptId.isEmpty() ? m_model->scripts().first().id : scriptId);

    // createSchedule emits schedulesChanged, so reload() has already run and
    // the row exists. Select it: adding something you then have to hunt for in
    // the table is a small cruelty.
    for (int i = 0; i < m_source->rowCount(); ++i) {
        if (m_source->item(i, 0)->data(kIdRole).toString() == created) {
            m_ui->schedulesTable->selectRow(i);
            break;
        }
    }

    emit statusMessage(tr("Schedule added — set its trigger on the right"));
}

void SchedulesPage::onDeleteClicked() {
    const QString id = selectedScheduleId();
    if (id.isEmpty()) {
        return;
    }
    m_model->removeSchedule(id);
    emit statusMessage(tr("Schedule deleted"));
}

void SchedulesPage::onSaveClicked() {
    const QString id = selectedScheduleId();
    if (id.isEmpty()) {
        return;
    }

    ScheduleEntry entry;
    entry.id = id;
    entry.scriptId = m_ui->scriptCombo->currentData().toString();
    entry.kind = m_ui->dailyRadio->isChecked()  ? TriggerKind::Daily
                 : m_ui->cronRadio->isChecked() ? TriggerKind::Cron
                                                : TriggerKind::Interval;
    entry.interval = m_ui->intervalSpin->value();
    entry.unitIndex = m_ui->intervalUnitCombo->currentIndex();
    entry.dailyTime = m_ui->dailyTimeEdit->time();
    entry.cron = m_ui->cronEdit->text();
    entry.enabled = m_ui->enabledCheck->isChecked();

    m_model->updateSchedule(entry);
    emit statusMessage(tr("Schedule saved"));
}

void SchedulesPage::onRunNowClicked() {
    const QString id = selectedScheduleId();
    for (const ScheduleEntry& entry : m_model->schedules()) {
        if (entry.id == id) {
            m_model->runScript(entry.scriptId, /*useStub=*/true);
            emit statusMessage(tr("Run started — see Library or History"));
            return;
        }
    }
}

}  // namespace autotasks
