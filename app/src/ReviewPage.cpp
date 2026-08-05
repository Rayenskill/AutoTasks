#include "app/ReviewPage.h"

#include "app/AppModel.h"

#include "ui_ReviewPage.h"

#include <QStandardItemModel>

namespace autotasks {

ReviewPage::ReviewPage(AppModel* model, QWidget* parent)
    : QWidget(parent), m_ui(std::make_unique<Ui::ReviewPage>()), m_model(model) {
    m_ui->setupUi(this);

    m_ui->reviewSplitter->setStretchFactor(0, 1);
    m_ui->reviewSplitter->setStretchFactor(1, 3);

    // Confidence is a 0..1 score; the bar shows it as a percentage.
    m_ui->confidenceBar->setRange(0, 100);
    m_ui->confidenceBar->setFormat(QStringLiteral("%p%"));

    m_queue = new QStandardItemModel(this);
    m_ui->queueList->setModel(m_queue);

    connect(m_ui->queueList->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ReviewPage::onSelectionChanged);

    connect(m_ui->confirmButton, &QPushButton::clicked, this, &ReviewPage::onConfirmClicked);
    connect(m_ui->skipButton, &QPushButton::clicked, this, &ReviewPage::onSkipClicked);
    connect(m_ui->openInEditorButton, &QPushButton::clicked, this,
            &ReviewPage::onOpenInEditorClicked);

    connect(m_model, &AppModel::reviewChanged, this, &ReviewPage::reload);

    // Re-capturing means grabbing a region of the screen — engine territory.
    m_ui->recaptureButton->setEnabled(false);
    m_ui->recaptureButton->setToolTip(
        tr("Needs screen capture from the engine (Phase 3). Use \"Open in Editor\" to fix the "
           "template path by hand in the meantime."));

    reload();
}

ReviewPage::~ReviewPage() = default;

void ReviewPage::reload() {
    m_queue->clear();

    for (const ReviewItem& item : m_model->review()) {
        m_queue->appendRow(new QStandardItem(QStringLiteral("%1 — step %2  (%3)")
                                                 .arg(item.scriptName)
                                                 .arg(item.stepIndex)
                                                 .arg(QString::number(item.confidence, 'f', 2))));
    }

    m_ui->queueCountLabel->setText(m_queue->rowCount() == 1
                                       ? tr("1 step waiting")
                                       : tr("%1 steps waiting").arg(m_queue->rowCount()));

    if (m_queue->rowCount() > 0) {
        m_ui->queueList->setCurrentIndex(m_queue->index(0, 0));
    } else {
        onSelectionChanged();
    }
}

int ReviewPage::selectedRow() const {
    const QModelIndex index = m_ui->queueList->currentIndex();
    return index.isValid() ? index.row() : -1;
}

void ReviewPage::onSelectionChanged() {
    const int row = selectedRow();
    const bool valid = row >= 0 && row < m_model->review().size();

    m_ui->confirmButton->setEnabled(valid);
    m_ui->skipButton->setEnabled(valid);
    m_ui->openInEditorButton->setEnabled(valid);

    if (!valid) {
        m_ui->stepHeaderLabel->setText(m_model->review().isEmpty()
                                           ? tr("Nothing to review — run a script first")
                                           : tr("Select a step from the queue"));
        m_ui->confidenceBar->setValue(0);
        m_ui->thresholdLabel->setText(QString());
        m_ui->expectedView->setText(tr("No template"));
        m_ui->observedView->setText(tr("No screenshot"));
        m_ui->messageLabel->clear();
        return;
    }

    const ReviewItem& item = m_model->review()[row];

    m_ui->stepHeaderLabel->setText(tr("%1 — step %2").arg(item.scriptName).arg(item.stepIndex));
    m_ui->confidenceBar->setValue(static_cast<int>(item.confidence * 100));
    m_ui->thresholdLabel->setText(tr("threshold %1").arg(QString::number(item.threshold, 'f', 2)));

    // Both panes show paths rather than images: the engine does not capture
    // screenshots yet (Phase 3). Once it does:
    //     setPixmap(QPixmap(path).scaled(..., Qt::KeepAspectRatio));
    m_ui->expectedView->setText(item.templatePath.isEmpty()
                                    ? tr("This step has no template —\nit targets fixed "
                                         "coordinates, which is what makes it fragile.")
                                    : item.templatePath);
    m_ui->observedView->setText(item.screenshotPath.isEmpty() ? tr("No screenshot")
                                                              : item.screenshotPath);
    m_ui->messageLabel->setText(item.message);
}

void ReviewPage::onConfirmClicked() {
    const int row = selectedRow();
    if (row < 0) {
        return;
    }

    // TODO (Phase 9): confirming should also raise the step's reviewThreshold
    // so the same match stops being questioned. For now it clears the queue
    // entry, which is the visible half of the loop.
    m_model->resolveReview(row);
    emit statusMessage(tr("Confirmed — step cleared from the queue"));
}

void ReviewPage::onSkipClicked() {
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    m_model->resolveReview(row);
    emit statusMessage(tr("Skipped"));
}

void ReviewPage::onOpenInEditorClicked() {
    const int row = selectedRow();
    if (row < 0 || row >= m_model->review().size()) {
        return;
    }

    const ReviewItem& item = m_model->review()[row];
    emit openInEditor(item.scriptId, item.stepIndex);
}

}  // namespace autotasks
