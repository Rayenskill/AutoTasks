#pragma once

// Library — the script list, and the page that launches runs.
//
// Layout: app/ui/LibraryPage.ui (open it in Qt Designer).
// Data:   AppModel (in memory today, SQLite at Phase 4).

#include <QWidget>

#include <memory>

namespace Ui {
class LibraryPage;
}

class QSortFilterProxyModel;
class QStandardItemModel;

namespace autotasks {

class AppModel;

class LibraryPage : public QWidget {
    Q_OBJECT

public:
    explicit LibraryPage(AppModel* model, QWidget* parent = nullptr);
    ~LibraryPage() override;

    LibraryPage(const LibraryPage&) = delete;
    LibraryPage& operator=(const LibraryPage&) = delete;
    LibraryPage(LibraryPage&&) = delete;
    LibraryPage& operator=(LibraryPage&&) = delete;

signals:
    void statusMessage(const QString& text);

private slots:
    void reload();
    void onSelectionChanged();
    void onRunClicked();
    void onAbortClicked();
    void onNewScriptClicked();
    void onDeleteScriptClicked();

    void onRunStarted(const QString& scriptName, const QString& engineName, int stepCount);
    void onStepReported(int index, int outcome, double confidence, const QString& message,
                        const QString& screenshotPath);
    void onRunFinished(bool success);

private:
    QString selectedScriptId() const;
    void setRunningState(bool running);

    std::unique_ptr<Ui::LibraryPage> m_ui;

    AppModel* m_model = nullptr;
    QStandardItemModel* m_source = nullptr;
    QSortFilterProxyModel* m_proxy = nullptr;

    int m_stepCount = 0;
};

}  // namespace autotasks
