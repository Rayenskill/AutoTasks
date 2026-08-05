#pragma once

// Review — the steps the engine was not sure about.
//
// Layout: app/ui/ReviewPage.ui (open it in Qt Designer).
// Data:   AppModel, which files a review entry for every low-confidence step.
//
// This is the view that makes the project different from a plain macro
// recorder. The stub engine already produces its input — one step in four comes
// back below the review threshold — so the whole loop is testable today.

#include <QWidget>

#include <memory>

namespace Ui {
class ReviewPage;
}

class QStandardItemModel;

namespace autotasks {

class AppModel;

class ReviewPage : public QWidget {
    Q_OBJECT

public:
    explicit ReviewPage(AppModel* model, QWidget* parent = nullptr);
    ~ReviewPage() override;

    ReviewPage(const ReviewPage&) = delete;
    ReviewPage& operator=(const ReviewPage&) = delete;
    ReviewPage(ReviewPage&&) = delete;
    ReviewPage& operator=(ReviewPage&&) = delete;

signals:
    void statusMessage(const QString& text);
    void openInEditor(const QString& scriptId, int stepIndex);

private slots:
    void reload();
    void onSelectionChanged();
    void onConfirmClicked();
    void onSkipClicked();
    void onOpenInEditorClicked();

private:
    int selectedRow() const;

    std::unique_ptr<Ui::ReviewPage> m_ui;

    AppModel* m_model = nullptr;
    QStandardItemModel* m_queue = nullptr;
};

}  // namespace autotasks
