#include "app/MainWindow.h"

#include "app/AppModel.h"
#include "app/EditorPage.h"
#include "app/HistoryPage.h"
#include "app/LibraryPage.h"
#include "app/RecorderPage.h"
#include "app/ReviewPage.h"
#include "app/SchedulesPage.h"

#include <QApplication>
#include <QListWidget>
#include <QMenuBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>

namespace autotasks {

namespace {

// Row order in the sidebar. Used by the cross-page navigation below.
enum Page { Library = 0, Editor = 1, Recorder = 2, Schedules = 3, History = 4, Review = 5 };

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // One data layer, shared by every page. Owned by the window, so it lives
    // exactly as long as the UI does.
    m_model = new AppModel(this);

    buildUi();
    buildMenus();

    setWindowTitle(QStringLiteral("AutoTasks"));
    resize(1100, 700);
    statusBar()->showMessage(QStringLiteral("Ready"));
}

void MainWindow::buildUi() {
    m_navigation = new QListWidget(this);
    m_navigation->setMaximumWidth(200);
    m_navigation->setSpacing(2);

    m_pages = new QStackedWidget(this);

    // Every page's layout lives in app/ui/<Name>.ui — open those in Qt Designer
    // to change how a view looks. This window only assembles them.
    auto* library = new LibraryPage(m_model, this);
    m_editor = new EditorPage(m_model, this);
    auto* recorder = new RecorderPage(m_model, this);
    auto* schedules = new SchedulesPage(m_model, this);
    auto* history = new HistoryPage(m_model, this);
    auto* review = new ReviewPage(m_model, this);

    addPage(QStringLiteral("Library"), library);
    addPage(QStringLiteral("Editor"), m_editor);
    addPage(QStringLiteral("Recorder"), recorder);
    addPage(QStringLiteral("Schedules"), schedules);
    addPage(QStringLiteral("History"), history);
    addPage(QStringLiteral("Review"), review);

    // Cross-page navigation goes through the window: a page never reaches into
    // another one directly.
    connect(review, &ReviewPage::openInEditor, this, &MainWindow::onOpenInEditor);
    connect(recorder, &RecorderPage::openInEditor, this, &MainWindow::onOpenInEditor);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_navigation);
    splitter->addWidget(m_pages);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    // Signals and slots: the navigation list drives the stacked pages.
    connect(m_navigation, &QListWidget::currentRowChanged, this, &MainWindow::onNavigationChanged);

    m_navigation->setCurrentRow(Page::Library);
}

void MainWindow::addPage(const QString& label, QWidget* page) {
    m_navigation->addItem(label);
    m_pages->addWidget(page);

    // Pages that have something to say route it here rather than touching the
    // status bar themselves. Pages without the signal simply do not connect.
    if (page->metaObject()->indexOfSignal("statusMessage(QString)") >= 0) {
        connect(page, SIGNAL(statusMessage(QString)), this, SLOT(onPageStatusMessage(QString)));
    }
}

void MainWindow::buildMenus() {
    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&Quit"), QKeySequence::Quit, this, &QMainWindow::close);

    QMenu* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("&About"), this, [this]() {
        statusBar()->showMessage(QStringLiteral("AutoTasks %1 — desktop GUI automation")
                                     .arg(QApplication::applicationVersion()),
                                 5000);
    });
}

void MainWindow::onNavigationChanged(int index) {
    if (index < 0 || index >= m_pages->count()) {
        return;
    }
    m_pages->setCurrentIndex(index);
}

void MainWindow::onPageStatusMessage(const QString& text) {
    statusBar()->showMessage(text, 5000);
}

void MainWindow::onOpenInEditor(const QString& scriptId, int stepIndex) {
    m_navigation->setCurrentRow(Page::Editor);
    m_editor->showStep(scriptId, stepIndex);
}

}  // namespace autotasks
