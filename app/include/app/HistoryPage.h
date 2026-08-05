#pragma once

// History — every run, drillable into the per-step trace.
//
// Layout: app/ui/HistoryPage.ui (open it in Qt Designer).
// Data:   AppModel, which records a run every time one finishes.
//
// Three linked views: runs on top, that run's steps bottom-left, the selected
// step's detail bottom-right.

#include <QWidget>

#include <memory>

namespace Ui {
class HistoryPage;
}

class QStandardItemModel;

namespace autotasks {

class AppModel;

class HistoryPage : public QWidget {
    Q_OBJECT

public:
    explicit HistoryPage(AppModel* model, QWidget* parent = nullptr);
    ~HistoryPage() override;

    HistoryPage(const HistoryPage&) = delete;
    HistoryPage& operator=(const HistoryPage&) = delete;
    HistoryPage(HistoryPage&&) = delete;
    HistoryPage& operator=(HistoryPage&&) = delete;

private slots:
    void reload();
    void reloadFilters();
    void onRunSelected();
    void onStepSelected();

private:
    std::unique_ptr<Ui::HistoryPage> m_ui;

    AppModel* m_model = nullptr;
    QStandardItemModel* m_runs = nullptr;
    QStandardItemModel* m_steps = nullptr;
};

}  // namespace autotasks
