#pragma once

// Recorder — entry point for creating a new script.
//
// Layout: app/ui/RecorderPage.ui (open it in Qt Designer).
// Data:   AppModel.
//
// Creating the script works today. Capturing its steps needs the engine's
// low-level hooks (Phase 2), so the page says so instead of pretending: it
// creates the entry and points you at the Editor.

#include <QWidget>

#include <memory>

namespace Ui {
class RecorderPage;
}

namespace autotasks {

class AppModel;

class RecorderPage : public QWidget {
    Q_OBJECT

public:
    explicit RecorderPage(AppModel* model, QWidget* parent = nullptr);
    ~RecorderPage() override;

    RecorderPage(const RecorderPage&) = delete;
    RecorderPage& operator=(const RecorderPage&) = delete;
    RecorderPage(RecorderPage&&) = delete;
    RecorderPage& operator=(RecorderPage&&) = delete;

signals:
    void statusMessage(const QString& text);

    /// Asks the window to switch to the Editor on the newly created script.
    void openInEditor(const QString& scriptId, int stepIndex);

private slots:
    void onCreateClicked();
    void onStopClicked();

private:
    std::unique_ptr<Ui::RecorderPage> m_ui;

    AppModel* m_model = nullptr;
};

}  // namespace autotasks
