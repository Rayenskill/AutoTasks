#include "app/LibraryPage.h"

#include "app/AppModel.h"

#include "ui_LibraryPage.h"

// CHANGEMENT : QHeaderView etait deja utilise plus bas sans etre inclus ici.
// Ca compilait par transitivite via ui_LibraryPage.h, ce qui casse des que le
// .ui change.
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace autotasks {

namespace {

// Column layout of the scripts table.
enum Column { Name = 0, Tags = 1, Steps = 2, LastRun = 3, Status = 4, ColumnCount = 5 };

// Role carrying the script id on every row, so the view never has to guess
// which script a selected row means.
constexpr int kIdRole = Qt::UserRole + 1;

const char* outcomeLabel(int outcome) {
    switch (outcome) {
        case 0: return "OK  ";
        case 1: return "WARN";
        case 2: return "FAIL";
        default: return "????";
    }
}

}  // namespace

LibraryPage::LibraryPage(AppModel* model, QWidget* parent)
    : QWidget(parent), m_ui(std::make_unique<Ui::LibraryPage>()), m_model(model) {
    m_ui->setupUi(this);

    // Splitter stretch cannot be expressed in a .ui file — it belongs here.
    m_ui->mainSplitter->setStretchFactor(0, 3);
    m_ui->mainSplitter->setStretchFactor(1, 2);

    m_source = new QStandardItemModel(this);
    m_source->setHorizontalHeaderLabels(
        {tr("Name"), tr("Tags"), tr("Steps"), tr("Last run"), tr("Status")});

    // The proxy is what makes the search box work — no manual filtering.
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_source);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(-1);  // match against every column

    m_ui->scriptsTable->setModel(m_proxy);
    m_ui->scriptsTable->horizontalHeader()->setStretchLastSection(true);

    // =====================================================================
    // CHANGEMENTS — bande de selection uniforme sur toute la ligne.
    //
    // Aucune couleur n'est inventee : la seule couleur utilisee est lue dans
    // la palette, c'est-a-dire la couleur d'accentuation de Windows.
    // =====================================================================

    // 1. Selection de la ligne entiere au lieu de la cellule cliquee.
    //
    // QTableView herite du defaut SelectItems de QAbstractItemView ; QTreeView
    // (stepsView, page Editor) bascule seul sur SelectRows. C'est toute la
    // difference de comportement entre les deux pages.
    //
    // Ce n'est pas que cosmetique : selectedScriptId() appelle selectedRows(),
    // qui ne renvoie une ligne que si toutes ses colonnes sont selectionnees.
    // En SelectItems, un clic utilisateur laissait la liste vide, donc les
    // boutons Run et Delete restaient grises. Le bug etait masque par reload(),
    // qui utilise selectRow() et selectionne bien la ligne complete.
    m_ui->scriptsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 2. Une seule ligne a la fois : selectedScriptId() ne lit de toute facon
    //    que selected.first().
    m_ui->scriptsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // 3. La grille. QTableView dessine un trait entre chaque cellule, ce qui
    //    decoupait la bande de selection en cinq rectangles. QTreeView n'en a
    //    pas, d'ou le rendu continu de stepsView.
    m_ui->scriptsTable->setShowGrid(false);

    // 4. La colonne de numeros a gauche. Le modele n'a pas d'en-tetes de
    //    lignes, donc QTableView affichait 1, 2, 3... alors que Name est deja
    //    la premiere colonne.
    m_ui->scriptsTable->verticalHeader()->setVisible(false);

    // 5. L'alternance de couleurs, activee dans le .ui. Elle survit sous la
    //    selection et reintroduit une variation de teinte au milieu de la
    //    bande.
    m_ui->scriptsTable->setAlternatingRowColors(false);

    // 6. Hauteur de ligne. QTableView est plus haut que QTreeView par defaut ;
    //    on s'aligne sur stepsView.
    m_ui->scriptsTable->verticalHeader()->setDefaultSectionSize(24);

    // enlève le focus sur la table pour que la selection ne disparaisse pas quand on clique sur
    // searchEdit ou un bouton. Le focus reste sur le bouton cliqué, ce qui est exactement le
    // comportement de Windows.
    m_ui->scriptsTable->setFocusPolicy(Qt::NoFocus);

    // 7. Le remplissage continu.
    //
    //    Sans stylesheet, le style natif Windows peint la selection cellule par
    //    cellule et laisse des ruptures aux jointures, meme sans grille. La
    //    stylesheet force un remplissage uniforme sur toute la largeur.
    //
    //    Les quatre couleurs viennent de la palette, pas de valeurs en dur :
    //    Active = vue au premier plan, Inactive = focus parti sur searchEdit ou
    //    un bouton, cas frequent ici. C'est exactement ce que Windows aurait
    //    peint, en un seul bloc.
    m_ui->scriptsTable->setStyleSheet(
        QStringLiteral("QTableView::item {"
                       "    border: none;"
                       "}"
                       "QTableView::item:hover {"
                       "    background: transparent;"
                       "}"
                       "QTableView::item:focus {"
                       "    outline: none;"
                       "    border: none;"
                       "}"
                       "QTableView::item:selected {"
                       "    background: %1;"
                       "    color: %2;"
                       "}"
                       "QTableView::item:selected:!active {"
                       "    background: %3;"
                       "    color: %4;"
                       "}")
            .arg(palette().color(QPalette::Active, QPalette::Highlight).name(),
                 palette().color(QPalette::Active, QPalette::HighlightedText).name(),
                 palette().color(QPalette::Inactive, QPalette::Highlight).name(),
                 palette().color(QPalette::Inactive, QPalette::HighlightedText).name()));

    // =====================================================================
    // Fin des changements.
    // =====================================================================

    connect(m_ui->searchEdit, &QLineEdit::textChanged, m_proxy,
            &QSortFilterProxyModel::setFilterFixedString);
    connect(m_ui->tagFilterCombo, &QComboBox::currentTextChanged, this, [this](const QString& tag) {
        m_ui->searchEdit->setText(tag == tr("All tags") ? QString() : tag);
    });

    connect(m_ui->scriptsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &LibraryPage::onSelectionChanged);

    connect(m_ui->runButton, &QPushButton::clicked, this, &LibraryPage::onRunClicked);
    connect(m_ui->abortButton, &QPushButton::clicked, this, &LibraryPage::onAbortClicked);
    connect(m_ui->newScriptButton, &QPushButton::clicked, this, &LibraryPage::onNewScriptClicked);
    connect(m_ui->deleteScriptButton, &QPushButton::clicked, this,
            &LibraryPage::onDeleteScriptClicked);

    connect(m_model, &AppModel::scriptsChanged, this, &LibraryPage::reload);
    connect(m_model, &AppModel::runStarted, this, &LibraryPage::onRunStarted);
    connect(m_model, &AppModel::stepReported, this, &LibraryPage::onStepReported);
    connect(m_model, &AppModel::runFinished, this, &LibraryPage::onRunFinished);

    // Import needs a script file format, which arrives with JSON serialisation.
    m_ui->importButton->setEnabled(false);
    m_ui->importButton->setToolTip(tr("Needs the JSON script format (Phase 2)."));

    reload();
}

LibraryPage::~LibraryPage() = default;

void LibraryPage::reload() {
    const QString keep = selectedScriptId();

    m_source->removeRows(0, m_source->rowCount());

    QStringList tags{tr("All tags")};

    for (const ScriptEntry& entry : m_model->scripts()) {
        QList<QStandardItem*> row;
        row.reserve(ColumnCount);

        auto* name = new QStandardItem(entry.name);
        name->setData(entry.id, kIdRole);
        row << name;
        row << new QStandardItem(entry.tags);
        row << new QStandardItem(QString::number(entry.steps.size()));
        row << new QStandardItem(entry.lastRunAt.isValid()
                                     ? entry.lastRunAt.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                                     : QStringLiteral("—"));
        row << new QStandardItem(entry.lastStatus.isEmpty() ? QStringLiteral("—")
                                                            : entry.lastStatus);

        for (QStandardItem* item : row) {
            item->setEditable(false);
        }
        m_source->appendRow(row);

        for (const QString& tag : entry.tags.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QString trimmed = tag.trimmed();
            if (!trimmed.isEmpty() && !tags.contains(trimmed)) {
                tags << trimmed;
            }
        }
    }

    {
        const QSignalBlocker blocker(m_ui->tagFilterCombo);
        const QString current = m_ui->tagFilterCombo->currentText();
        m_ui->tagFilterCombo->clear();
        m_ui->tagFilterCombo->addItems(tags);
        const int found = m_ui->tagFilterCombo->findText(current);
        m_ui->tagFilterCombo->setCurrentIndex(found >= 0 ? found : 0);
    }

    m_ui->scriptsTable->resizeColumnsToContents();

    // Restore the selection so a run finishing does not move the user's cursor.
    if (!keep.isEmpty()) {
        for (int i = 0; i < m_proxy->rowCount(); ++i) {
            const QModelIndex index = m_proxy->index(i, Column::Name);
            if (index.data(kIdRole).toString() == keep) {
                m_ui->scriptsTable->selectRow(i);
                break;
            }
        }
    }

    onSelectionChanged();
}

QString LibraryPage::selectedScriptId() const {
    const QModelIndexList selected =
        m_ui->scriptsTable->selectionModel()->selectedRows(Column::Name);
    return selected.isEmpty() ? QString() : selected.first().data(kIdRole).toString();
}

void LibraryPage::onSelectionChanged() {
    const bool hasSelection = !selectedScriptId().isEmpty();
    const bool idle = !m_model->isRunning();

    m_ui->runButton->setEnabled(hasSelection && idle);
    m_ui->deleteScriptButton->setEnabled(hasSelection && idle);
}

void LibraryPage::setRunningState(bool running) {
    m_ui->abortButton->setEnabled(running);
    m_ui->stubCheckBox->setEnabled(!running);
    m_ui->newScriptButton->setEnabled(!running);
    onSelectionChanged();
}

void LibraryPage::onRunClicked() {
    const QString id = selectedScriptId();
    if (id.isEmpty()) {
        return;
    }

    m_ui->logView->clear();
    m_model->runScript(id, m_ui->stubCheckBox->isChecked());
}

void LibraryPage::onAbortClicked() {
    m_model->abortRun();
    emit statusMessage(tr("Abort requested..."));
}

void LibraryPage::onNewScriptClicked() {
    bool accepted = false;
    const QString name = QInputDialog::getText(this, tr("New script"), tr("Script name"),
                                               QLineEdit::Normal, tr("Untitled script"), &accepted);
    if (!accepted || name.isEmpty()) {
        return;
    }

    m_model->createScript(name, QString());
    emit statusMessage(tr("Created \"%1\" — add its steps from the Editor").arg(name));
}

void LibraryPage::onDeleteScriptClicked() {
    const QString id = selectedScriptId();
    const ScriptEntry* entry = m_model->script(id);
    if (entry == nullptr) {
        return;
    }

    const auto answer =
        QMessageBox::question(this, tr("Delete script"),
                              tr("Delete \"%1\"? Its schedules are removed too.").arg(entry->name));
    if (answer != QMessageBox::Yes) {
        return;
    }

    m_model->removeScript(id);
}

void LibraryPage::onRunStarted(const QString& scriptName, const QString& engineName,
                               int stepCount) {
    m_stepCount = 0;
    m_ui->runProgress->setRange(0, std::max(stepCount, 1));
    m_ui->runProgress->setValue(0);

    setRunningState(true);
    m_ui->logView->appendPlainText(tr("%1 — %2").arg(scriptName, engineName));
    emit statusMessage(tr("Running \"%1\"...").arg(scriptName));
}

void LibraryPage::onStepReported(int index, int outcome, double confidence, const QString& message,
                                 const QString& screenshotPath) {
    QString line = QStringLiteral("[%1] step %2  conf=%3  %4")
                       .arg(QString::fromUtf8(outcomeLabel(outcome)))
                       .arg(index, 2)
                       .arg(confidence, 0, 'f', 2)
                       .arg(message);

    // Populated on failure and low confidence — this is what feeds the Review page.
    if (!screenshotPath.isEmpty()) {
        line += QStringLiteral("  [%1]").arg(screenshotPath);
    }

    m_ui->logView->appendPlainText(line);
    m_ui->runProgress->setValue(++m_stepCount);
}

void LibraryPage::onRunFinished(bool success) {
    setRunningState(false);
    m_ui->logView->appendPlainText(success ? tr("--- Run completed ---")
                                           : tr("--- Run did not complete ---"));
    emit statusMessage(success ? tr("Run completed — see History")
                               : tr("Run failed — see History"));
}

}  // namespace autotasks
