#pragma once

// Schedules — interval and daily triggers with next-run times.
//
// Layout: app/ui/SchedulesPage.ui (open it in Qt Designer).
// Data:   AppModel, which also owns the timer that actually fires runs.
//
// Cron is parsed at Phase 8; a cron schedule has no next run and stays dormant
// rather than firing at the wrong time.

#include <QWidget>

#include <memory>

namespace Ui {
class SchedulesPage;
}

class QStandardItemModel;

namespace autotasks {

class AppModel;

class SchedulesPage : public QWidget {
    Q_OBJECT

public:
    explicit SchedulesPage(AppModel* model, QWidget* parent = nullptr);
    ~SchedulesPage() override;

    SchedulesPage(const SchedulesPage&) = delete;
    SchedulesPage& operator=(const SchedulesPage&) = delete;
    SchedulesPage(SchedulesPage&&) = delete;
    SchedulesPage& operator=(SchedulesPage&&) = delete;

signals:
    void statusMessage(const QString& text);

private slots:
    void reload();
    void reloadScripts();
    void onSelectionChanged();
    void onTriggerKindChanged();
    void onAddClicked();
    void onDeleteClicked();
    void onSaveClicked();
    void onRunNowClicked();

private:
    QString selectedScheduleId() const;
    void bindToForm();

    std::unique_ptr<Ui::SchedulesPage> m_ui;

    AppModel* m_model = nullptr;
    QStandardItemModel* m_source = nullptr;
    bool m_binding = false;
};

}  // namespace autotasks
