#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class QString;

namespace autotasks {

/// Application shell: sidebar navigation on the left, stacked pages on the right.
///
/// Architectural rule: this window NEVER synthesizes input. It sends commands to
/// the engine and reads from the store. Every page below is a placeholder until
/// the corresponding roadmap phase lands.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

private slots:
    void onNavigationChanged(int index);

private:
    void buildUi();
    void buildMenus();

    /// Builds a stub page. Replace each one as its phase is implemented.
    static QWidget* createPlaceholderPage(const QString& title, const QString& description,
                                          const QString& phase);

    QListWidget* m_navigation = nullptr;
    QStackedWidget* m_pages = nullptr;
};

}  // namespace autotasks
