#include "app/HistoryPage.h"

#include "app/AppModel.h"

#include "ui_HistoryPage.h"

#include <QStandardItemModel>

namespace autotasks {

namespace {

constexpr int kRunRole = Qt::UserRole + 1;

QString outcomeName(int outcome) {
    switch (outcome) {
        case 0: return QStringLiteral("OK");
        case 1: return QStringLiteral("Low confidence");
        case 2: return QStringLiteral("Failed");
        default: return QStringLiteral("Unknown");
    }
}

}  // namespace

HistoryPage::HistoryPage(AppModel* model, QWidget* parent)
    : QWidget(parent), m_ui(std::make_unique<Ui::HistoryPage>()), m_model(model) {
    m_ui->setupUi(this);

    m_ui->historySplitter->setStretchFactor(0, 2);
    m_ui->historySplitter->setStretchFactor(1, 3);
    m_ui->detailSplitter->setStretchFactor(0, 3);
    m_ui->detailSplitter->setStretchFactor(1, 2);

    m_runs = new QStandardItemModel(this);
    m_runs->setHorizontalHeaderLabels(
        {tr("Started"), tr("Script"), tr("Status"), tr("Steps"), tr("Engine")});
    m_ui->runsTable->setModel(m_runs);
    m_ui->runsTable->horizontalHeader()->setStretchLastSection(true);

    m_steps = new QStandardItemModel(this);
    m_steps->setHorizontalHeaderLabels({tr("#"), tr("Outcome"), tr("Confidence"), tr("Message")});
    m_ui->stepsTable->setModel(m_steps);
    m_ui->stepsTable->horizontalHeader()->setStretchLastSection(true);

    connect(m_ui->runsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &HistoryPage::onRunSelected);
    connect(m_ui->stepsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &HistoryPage::onStepSelected);

    connect(m_ui->refreshButton, &QPushButton::clicked, this, &HistoryPage::reload);
    connect(m_ui->scriptFilterCombo, &QComboBox::currentIndexChanged, this, &HistoryPage::reload);
    connect(m_ui->statusFilterCombo, &QComboBox::currentIndexChanged, this, &HistoryPage::reload);

    connect(m_model, &AppModel::runsChanged, this, &HistoryPage::reload);
    connect(m_model, &AppModel::scriptsChanged, this, &HistoryPage::reloadFilters);

    // The date filter and export need persisted runs to be worth anything.
    m_ui->sinceDateEdit->setEnabled(false);
    m_ui->sinceDateEdit->setToolTip(tr("Needs persisted runs (Phase 4)."));
    m_ui->exportButton->setToolTip(tr("Needs persisted runs (Phase 4)."));

    reloadFilters();
    reload();
}

HistoryPage::~HistoryPage() = default;

void HistoryPage::reloadFilters() {
    const QSignalBlocker blocker(m_ui->scriptFilterCombo);
    const QString keep = m_ui->scriptFilterCombo->currentData().toString();

    m_ui->scriptFilterCombo->clear();
    m_ui->scriptFilterCombo->addItem(tr("All scripts"), QString());
    for (const ScriptEntry& entry : m_model->scripts()) {
        m_ui->scriptFilterCombo->addItem(entry.name, entry.id);
    }

    const int found = m_ui->scriptFilterCombo->findData(keep);
    m_ui->scriptFilterCombo->setCurrentIndex(found >= 0 ? found : 0);
}

void HistoryPage::reload() {
    const QString scriptFilter = m_ui->scriptFilterCombo->currentData().toString();
    const QString statusFilter = m_ui->statusFilterCombo->currentIndex() == 0
                                     ? QString()
                                     : m_ui->statusFilterCombo->currentText();

    m_runs->removeRows(0, m_runs->rowCount());

    for (int i = 0; i < m_model->runs().size(); ++i) {
        const RunRecord& run = m_model->runs()[i];

        if (!scriptFilter.isEmpty() && run.scriptId != scriptFilter) {
            continue;
        }
        if (!statusFilter.isEmpty() && run.status != statusFilter) {
            continue;
        }

        QList<QStandardItem*> row;
        auto* first =
            new QStandardItem(run.startedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        first->setData(i, kRunRole);  // index into the model's run list
        row << first;
        row << new QStandardItem(run.scriptName);
        row << new QStandardItem(run.status);
        row << new QStandardItem(QString::number(run.steps.size()));
        row << new QStandardItem(run.engineName);

        for (QStandardItem* item : row) {
            item->setEditable(false);
        }
        m_runs->appendRow(row);
    }

    m_ui->runsTable->resizeColumnsToContents();

    m_steps->removeRows(0, m_steps->rowCount());
    m_ui->screenshotView->setText(tr("Select a step to see what the engine saw."));
    m_ui->stepMessageLabel->clear();

    if (m_runs->rowCount() > 0) {
        m_ui->runsTable->selectRow(0);
    }
}

void HistoryPage::onRunSelected() {
    m_steps->removeRows(0, m_steps->rowCount());
    m_ui->screenshotView->setText(tr("Select a step to see what the engine saw."));
    m_ui->stepMessageLabel->clear();

    const QModelIndexList selected = m_ui->runsTable->selectionModel()->selectedRows(0);
    if (selected.isEmpty()) {
        return;
    }

    const int runIndex = selected.first().data(kRunRole).toInt();
    if (runIndex < 0 || runIndex >= m_model->runs().size()) {
        return;
    }

    for (const StepRecord& step : m_model->runs()[runIndex].steps) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::number(step.index));
        row << new QStandardItem(outcomeName(step.outcome));
        row << new QStandardItem(QString::number(step.confidence, 'f', 2));
        row << new QStandardItem(step.message);

        for (QStandardItem* item : row) {
            item->setEditable(false);
        }
        m_steps->appendRow(row);
    }

    m_ui->stepsTable->resizeColumnsToContents();
}

void HistoryPage::onStepSelected() {
    const QModelIndexList runSelected = m_ui->runsTable->selectionModel()->selectedRows(0);
    const QModelIndexList stepSelected = m_ui->stepsTable->selectionModel()->selectedRows(0);
    if (runSelected.isEmpty() || stepSelected.isEmpty()) {
        return;
    }

    const int runIndex = runSelected.first().data(kRunRole).toInt();
    const int stepRow = stepSelected.first().row();
    if (runIndex < 0 || runIndex >= m_model->runs().size()) {
        return;
    }

    const QVector<StepRecord>& steps = m_model->runs()[runIndex].steps;
    if (stepRow < 0 || stepRow >= steps.size()) {
        return;
    }

    const StepRecord& step = steps[stepRow];

    // The engine does not capture screenshots yet (Phase 3), so the path is
    // shown rather than an image. Once it does:
    //     m_ui->screenshotView->setPixmap(QPixmap(step.screenshotPath)
    //         .scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_ui->screenshotView->setText(
        step.screenshotPath.isEmpty()
            ? tr("No screenshot for this step.")
            : tr("%1\n\n(the engine does not capture images yet — Phase 3)")
                  .arg(step.screenshotPath));

    m_ui->stepMessageLabel->setText(
        tr("Step %1 — %2 at %3 confidence")
            .arg(step.index)
            .arg(outcomeName(step.outcome), QString::number(step.confidence, 'f', 2)));
}

}  // namespace autotasks
