#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class QString;

namespace autotasks {

class AppModel;
class EditorPage;

/// Application shell: sidebar navigation on the left, stacked pages on the right.
///
/// This window owns no layout of its own beyond that split. Each view is its own
/// widget class with its own Qt Designer form in app/ui/ — to change how a page
/// looks, open the matching .ui, not this file.
///
/// Architectural rule: this window NEVER synthesizes input. It commands the
/// engine (through each page's ReplayController) and reads from the store.
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

    /// Pages announce; the window decides. Keeps pages from touching the
    /// status bar directly.
    void onPageStatusMessage(const QString& text);

    /// Switches to the Editor on a given step. The route Review and Recorder
    /// use — a page never reaches into another page itself.
    void onOpenInEditor(const QString& scriptId, int stepIndex);

private:
    void buildUi();
    void buildMenus();

    /// Adds a nav entry and its page, and connects the page's statusMessage
    /// signal when it has one.
    void addPage(const QString& label, QWidget* page);

    AppModel* m_model = nullptr;     ///< shared data layer, handed to every page
    EditorPage* m_editor = nullptr;  ///< kept for cross-page navigation
    QListWidget* m_navigation = nullptr;
    QStackedWidget* m_pages = nullptr;
};

}  // namespace autotasks
