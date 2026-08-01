#include "app/RecorderPage.h"

#include "app/AppModel.h"

#include "ui_RecorderPage.h"

namespace autotasks {

RecorderPage::RecorderPage(AppModel* model, QWidget* parent)
    : QWidget(parent), m_ui(std::make_unique<Ui::RecorderPage>()), m_model(model) {
    m_ui->setupUi(this);

    connect(m_ui->recordButton, &QPushButton::clicked, this, &RecorderPage::onCreateClicked);
    connect(m_ui->stopButton, &QPushButton::clicked, this, &RecorderPage::onStopClicked);

    // A name is the minimum needed to create a script.
    connect(m_ui->nameEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_ui->recordButton->setEnabled(!text.trimmed().isEmpty());
    });

    m_ui->recordButton->setEnabled(false);
    m_ui->stopButton->setEnabled(false);

    // These describe how capture will behave; they cannot take effect until the
    // engine can record. Left visible so the intent is documented in the UI.
    m_ui->optionsGroup->setEnabled(false);
    m_ui->optionsGroup->setToolTip(tr("Applies once the engine can record (Phase 2)."));

    m_ui->statusLabel->setText(tr("Idle"));
}

RecorderPage::~RecorderPage() = default;

void RecorderPage::onCreateClicked() {
    const QString name = m_ui->nameEdit->text().trimmed();
    if (name.isEmpty()) {
        return;
    }

    const QString id = m_model->createScript(name, m_ui->tagsEdit->text().trimmed());

    m_ui->capturedView->clear();
    m_ui->capturedView->appendPlainText(tr("Created \"%1\".").arg(name));
    m_ui->capturedView->appendPlainText(
        tr("Live capture is not implemented — the engine needs SetWindowsHookEx (Phase 2)."));
    m_ui->capturedView->appendPlainText(tr("Add its steps from the Editor for now."));

    m_ui->statusLabel->setText(tr("Script created"));
    m_ui->stepCountLabel->setText(tr("0 steps"));
    m_ui->nameEdit->clear();
    m_ui->tagsEdit->clear();

    emit statusMessage(tr("Created \"%1\"").arg(name));
    emit openInEditor(id, 0);
}

void RecorderPage::onStopClicked() {
    m_ui->stopButton->setEnabled(false);
    m_ui->statusLabel->setText(tr("Idle"));
}

}  // namespace autotasks
