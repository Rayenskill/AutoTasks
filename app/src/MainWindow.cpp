#include "app/MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include <array>

namespace autotasks {

namespace {

struct NavigationEntry {
    const char* label;
    const char* description;
    const char* phase;
};

// Mirrors the views listed in the SRS (section 4.1).
constexpr std::array<NavigationEntry, 6> kNavigationEntries{{
    {"Library",
     "Every recorded script, searchable and taggable, with its last-run status "
     "and a run-now button.",
     "Phase 5"},
    {"Editor",
     "A script's steps as a reorderable list. Insert, delete, edit, or re-capture "
     "a single step without re-recording the whole macro.",
     "Phase 6"},
    {"Recorder",
     "Entry point for capturing a new script. Recording is performed by the "
     "engine; this view only starts and stops it.",
     "Phase 6"},
    {"Schedules",
     "Interval and cron triggers with next-run times. Only fires while the app "
     "runs and the session is unlocked.",
     "Phase 8"},
    {"History",
     "Run history filterable by script and status, drillable into the per-step "
     "trace with failure screenshots.",
     "Phase 7"},
    {"Review",
     "Steps the engine was not sure about. Confirm the action, or correct it and "
     "re-capture the template on the spot.",
     "Phase 9"},
}};

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    buildMenus();

    setWindowTitle(QStringLiteral("AutoTasks"));
    resize(1000, 640);
    statusBar()->showMessage(QStringLiteral("Ready — engine not yet implemented (Phase 0)"));
}

void MainWindow::buildUi() {
    m_navigation = new QListWidget(this);
    m_navigation->setMaximumWidth(200);
    m_navigation->setSpacing(2);

    m_pages = new QStackedWidget(this);

    for (const auto& entry : kNavigationEntries) {
        m_navigation->addItem(QString::fromUtf8(entry.label));
        m_pages->addWidget(createPlaceholderPage(QString::fromUtf8(entry.label),
                                                 QString::fromUtf8(entry.description),
                                                 QString::fromUtf8(entry.phase)));
    }

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_navigation);
    splitter->addWidget(m_pages);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    // Signals and slots: the navigation list drives the stacked pages.
    connect(m_navigation, &QListWidget::currentRowChanged, this, &MainWindow::onNavigationChanged);

    m_navigation->setCurrentRow(0);
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

QWidget* MainWindow::createPlaceholderPage(const QString& title, const QString& description,
                                           const QString& phase) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(12);

    auto* titleLabel = new QLabel(title, page);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto* phaseLabel = new QLabel(QStringLiteral("Not implemented — %1").arg(phase), page);
    phaseLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));

    auto* descriptionLabel = new QLabel(description, page);
    descriptionLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(phaseLabel);
    layout->addSpacing(8);
    layout->addWidget(descriptionLabel);
    layout->addStretch();

    return page;
}

void MainWindow::onNavigationChanged(int index) {
    if (index < 0 || index >= m_pages->count()) {
        return;
    }
    m_pages->setCurrentIndex(index);
}

}  // namespace autotasks
