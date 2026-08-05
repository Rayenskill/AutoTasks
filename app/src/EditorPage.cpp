#include "app/EditorPage.h"

#include "app/AppModel.h"

#include "ui_EditorPage.h"

#include <QStandardItemModel>

namespace autotasks {

namespace {

// Same order as the typeCombo entries in EditorPage.ui, which itself mirrors
// autotasks::StepType. Keep the three in sync.
int typeToIndex(StepType type) {
    switch (type) {
        case StepType::Move: return 0;
        case StepType::Click: return 1;
        case StepType::Type: return 2;
        case StepType::WaitForImage: return 3;
        case StepType::Verify: return 4;
    }
    return 0;
}

StepType indexToType(int index) {
    switch (index) {
        case 1: return StepType::Click;
        case 2: return StepType::Type;
        case 3: return StepType::WaitForImage;
        case 4: return StepType::Verify;
        default: return StepType::Move;
    }
}

QString typeName(StepType type) {
    switch (type) {
        case StepType::Move: return QStringLiteral("Move");
        case StepType::Click: return QStringLiteral("Click");
        case StepType::Type: return QStringLiteral("Type");
        case StepType::WaitForImage: return QStringLiteral("Wait for image");
        case StepType::Verify: return QStringLiteral("Verify");
    }
    return {};
}

/// One-line summary of what a step targets, for the list.
QString targetSummary(const Step& step) {
    if (!step.templatePath.empty()) {
        return QString::fromStdString(step.templatePath);
    }
    if (step.type == StepType::Type) {
        return QStringLiteral("\"%1\"").arg(QString::fromStdString(step.text));
    }
    return QStringLiteral("(%1, %2)").arg(step.x).arg(step.y);
}

}  // namespace

EditorPage::EditorPage(AppModel* model, QWidget* parent)
    : QWidget(parent), m_ui(std::make_unique<Ui::EditorPage>()), m_model(model) {
    m_ui->setupUi(this);

    m_ui->editorSplitter->setStretchFactor(0, 2);
    m_ui->editorSplitter->setStretchFactor(1, 3);

    m_steps = new QStandardItemModel(this);
    m_steps->setHorizontalHeaderLabels({tr("#"), tr("Type"), tr("Target"), tr("Critical")});
    m_ui->stepsView->setModel(m_steps);

    connect(m_ui->scriptCombo, &QComboBox::currentIndexChanged, this, &EditorPage::onScriptChanged);
    connect(m_ui->stepsView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &EditorPage::onStepSelected);

    connect(m_ui->typeCombo, &QComboBox::currentIndexChanged, this, &EditorPage::onStepTypeChanged);

    // Every editable field writes straight into the working copy.
    connect(m_ui->textEdit, &QLineEdit::textEdited, this, &EditorPage::onFieldEdited);
    connect(m_ui->templatePathEdit, &QLineEdit::textEdited, this, &EditorPage::onFieldEdited);
    connect(m_ui->criticalCheck, &QCheckBox::toggled, this, &EditorPage::onFieldEdited);
    connect(m_ui->xSpin, &QSpinBox::valueChanged, this, &EditorPage::onFieldEdited);
    connect(m_ui->ySpin, &QSpinBox::valueChanged, this, &EditorPage::onFieldEdited);
    connect(m_ui->confidenceThresholdSpin, &QDoubleSpinBox::valueChanged, this,
            &EditorPage::onFieldEdited);
    connect(m_ui->reviewThresholdSpin, &QDoubleSpinBox::valueChanged, this,
            &EditorPage::onFieldEdited);

    connect(m_ui->addStepButton, &QPushButton::clicked, this, &EditorPage::onAddStep);
    connect(m_ui->duplicateStepButton, &QPushButton::clicked, this, &EditorPage::onDuplicateStep);
    connect(m_ui->deleteStepButton, &QPushButton::clicked, this, &EditorPage::onDeleteStep);
    connect(m_ui->moveUpButton, &QPushButton::clicked, this, &EditorPage::onMoveUp);
    connect(m_ui->moveDownButton, &QPushButton::clicked, this, &EditorPage::onMoveDown);
    connect(m_ui->saveButton, &QPushButton::clicked, this, &EditorPage::onSave);
    connect(m_ui->revertButton, &QPushButton::clicked, this, &EditorPage::onRevert);

    connect(m_model, &AppModel::scriptsChanged, this, &EditorPage::reloadScripts);

    // Capturing a template needs the engine to grab the screen (Phase 3).
    m_ui->recaptureButton->setEnabled(false);
    m_ui->recaptureButton->setToolTip(tr("Needs screen capture from the engine (Phase 3)."));

    reloadScripts();
}

EditorPage::~EditorPage() = default;

void EditorPage::reloadScripts() {
    const QString keep = m_scriptId;

    {
        const QSignalBlocker blocker(m_ui->scriptCombo);
        m_ui->scriptCombo->clear();
        for (const ScriptEntry& entry : m_model->scripts()) {
            m_ui->scriptCombo->addItem(entry.name, entry.id);
        }
        const int found = m_ui->scriptCombo->findData(keep);
        m_ui->scriptCombo->setCurrentIndex(found >= 0 ? found : 0);
    }

    loadWorkingCopy();
}

void EditorPage::onScriptChanged() {
    loadWorkingCopy();
}

void EditorPage::loadWorkingCopy() {
    m_scriptId = m_ui->scriptCombo->currentData().toString();

    const ScriptEntry* entry = m_model->script(m_scriptId);
    m_working = entry != nullptr ? entry->steps : std::vector<Step>{};

    setDirty(false);
    refreshStepList(m_working.empty() ? -1 : 0);
}

void EditorPage::refreshStepList(int selectRow) {
    m_steps->removeRows(0, m_steps->rowCount());

    for (std::size_t i = 0; i < m_working.size(); ++i) {
        const Step& step = m_working[i];

        QList<QStandardItem*> row;
        row << new QStandardItem(QString::number(i + 1));
        row << new QStandardItem(typeName(step.type));
        row << new QStandardItem(targetSummary(step));
        row << new QStandardItem(step.critical ? tr("yes") : tr("no"));

        for (QStandardItem* item : row) {
            item->setEditable(false);
        }
        m_steps->appendRow(row);
    }

    m_ui->stepsView->resizeColumnToContents(0);
    m_ui->stepsView->resizeColumnToContents(1);

    if (selectRow >= 0 && selectRow < m_steps->rowCount()) {
        m_ui->stepsView->setCurrentIndex(m_steps->index(selectRow, 0));
    } else {
        bindStepToForm(-1);
    }
}

int EditorPage::currentStepRow() const {
    const QModelIndex index = m_ui->stepsView->currentIndex();
    return index.isValid() ? index.row() : -1;
}

void EditorPage::onStepSelected() {
    bindStepToForm(currentStepRow());
}

void EditorPage::bindStepToForm(int row) {
    const bool valid = row >= 0 && row < static_cast<int>(m_working.size());

    m_ui->stepGroup->setEnabled(valid);
    m_ui->targetGroup->setEnabled(valid);
    m_ui->confidenceGroup->setEnabled(valid);
    m_ui->duplicateStepButton->setEnabled(valid);
    m_ui->deleteStepButton->setEnabled(valid);
    m_ui->moveUpButton->setEnabled(valid && row > 0);
    m_ui->moveDownButton->setEnabled(valid && row < static_cast<int>(m_working.size()) - 1);

    if (!valid) {
        return;
    }

    // Writing into the widgets fires their signals; the guard stops those from
    // being mistaken for user edits and marking the script dirty.
    m_binding = true;
    const Step& step = m_working[static_cast<std::size_t>(row)];

    m_ui->typeCombo->setCurrentIndex(typeToIndex(step.type));
    m_ui->textEdit->setText(QString::fromStdString(step.text));
    m_ui->templatePathEdit->setText(QString::fromStdString(step.templatePath));
    m_ui->criticalCheck->setChecked(step.critical);
    m_ui->xSpin->setValue(step.x);
    m_ui->ySpin->setValue(step.y);
    m_ui->confidenceThresholdSpin->setValue(step.confidenceThreshold);
    m_ui->reviewThresholdSpin->setValue(step.reviewThreshold);
    m_binding = false;

    onStepTypeChanged(m_ui->typeCombo->currentIndex());
}

void EditorPage::onStepTypeChanged(int index) {
    const bool usesText = (index == 2);
    const bool usesCoordinates = (index == 0 || index == 1);
    const bool usesTemplate = (index != 2);

    // Greying out a field that has no meaning for the current type beats a
    // tooltip: an empty box the user cannot fill is never mistaken for a value
    // they forgot to set.
    m_ui->textLabel->setEnabled(usesText);
    m_ui->textEdit->setEnabled(usesText);
    m_ui->xLabel->setEnabled(usesCoordinates);
    m_ui->xSpin->setEnabled(usesCoordinates);
    m_ui->yLabel->setEnabled(usesCoordinates);
    m_ui->ySpin->setEnabled(usesCoordinates);
    m_ui->templatePathEdit->setEnabled(usesTemplate);

    if (!m_binding) {
        onFieldEdited();
    }
}

void EditorPage::onFieldEdited() {
    if (m_binding) {
        return;
    }

    const int row = currentStepRow();
    if (row < 0 || row >= static_cast<int>(m_working.size())) {
        return;
    }

    Step& step = m_working[static_cast<std::size_t>(row)];
    step.type = indexToType(m_ui->typeCombo->currentIndex());
    step.text = m_ui->textEdit->text().toStdString();
    step.templatePath = m_ui->templatePathEdit->text().toStdString();
    step.critical = m_ui->criticalCheck->isChecked();
    step.x = m_ui->xSpin->value();
    step.y = m_ui->ySpin->value();
    step.confidenceThreshold = m_ui->confidenceThresholdSpin->value();
    step.reviewThreshold = m_ui->reviewThresholdSpin->value();

    // Keep the list in sync without losing the selection.
    m_steps->item(row, 1)->setText(typeName(step.type));
    m_steps->item(row, 2)->setText(targetSummary(step));
    m_steps->item(row, 3)->setText(step.critical ? tr("yes") : tr("no"));

    setDirty(true);
}

void EditorPage::setDirty(bool dirty) {
    m_dirty = dirty;
    m_ui->saveButton->setEnabled(dirty);
    m_ui->revertButton->setEnabled(dirty);
}

void EditorPage::onAddStep() {
    const int row = currentStepRow();
    const auto at =
        row < 0 ? m_working.end() : m_working.begin() + static_cast<std::ptrdiff_t>(row) + 1;

    m_working.insert(at, Step{});
    setDirty(true);
    refreshStepList(row < 0 ? static_cast<int>(m_working.size()) - 1 : row + 1);
}

void EditorPage::onDuplicateStep() {
    const int row = currentStepRow();
    if (row < 0) {
        return;
    }

    const Step copy = m_working[static_cast<std::size_t>(row)];
    m_working.insert(m_working.begin() + static_cast<std::ptrdiff_t>(row) + 1, copy);
    setDirty(true);
    refreshStepList(row + 1);
}

void EditorPage::onDeleteStep() {
    const int row = currentStepRow();
    if (row < 0) {
        return;
    }

    m_working.erase(m_working.begin() + static_cast<std::ptrdiff_t>(row));
    setDirty(true);
    refreshStepList(std::min(row, static_cast<int>(m_working.size()) - 1));
}

void EditorPage::onMoveUp() {
    const int row = currentStepRow();
    if (row <= 0) {
        return;
    }

    std::swap(m_working[static_cast<std::size_t>(row)],
              m_working[static_cast<std::size_t>(row) - 1]);
    setDirty(true);
    refreshStepList(row - 1);
}

void EditorPage::onMoveDown() {
    const int row = currentStepRow();
    if (row < 0 || row >= static_cast<int>(m_working.size()) - 1) {
        return;
    }

    std::swap(m_working[static_cast<std::size_t>(row)],
              m_working[static_cast<std::size_t>(row) + 1]);
    setDirty(true);
    refreshStepList(row + 1);
}

void EditorPage::onSave() {
    const ScriptEntry* existing = m_model->script(m_scriptId);
    if (existing == nullptr) {
        return;
    }

    ScriptEntry updated = *existing;
    updated.steps = m_working;
    m_model->updateScript(updated);

    setDirty(false);
    emit statusMessage(tr("Saved \"%1\" (%2 steps)").arg(updated.name).arg(m_working.size()));
}

void EditorPage::onRevert() {
    loadWorkingCopy();
    emit statusMessage(tr("Reverted to the saved version"));
}

void EditorPage::showStep(const QString& scriptId, int stepIndex) {
    const int found = m_ui->scriptCombo->findData(scriptId);
    if (found < 0) {
        return;
    }

    m_ui->scriptCombo->setCurrentIndex(found);
    if (stepIndex >= 0 && stepIndex < m_steps->rowCount()) {
        m_ui->stepsView->setCurrentIndex(m_steps->index(stepIndex, 0));
    }
}

}  // namespace autotasks
