#pragma once

// Editor — a script's steps as a reorderable list, with a property panel.
//
// Layout: app/ui/EditorPage.ui (open it in Qt Designer).
// Data:   AppModel.
//
// Edits happen on a working copy and only reach the model on Save, so Revert
// is always possible and a half-finished edit never corrupts the library.

#include "engine/Engine.h"

#include <QWidget>

#include <memory>
#include <vector>

namespace Ui {
class EditorPage;
}

class QStandardItemModel;

namespace autotasks {

class AppModel;

class EditorPage : public QWidget {
    Q_OBJECT

public:
    explicit EditorPage(AppModel* model, QWidget* parent = nullptr);
    ~EditorPage() override;

    EditorPage(const EditorPage&) = delete;
    EditorPage& operator=(const EditorPage&) = delete;
    EditorPage(EditorPage&&) = delete;
    EditorPage& operator=(EditorPage&&) = delete;

    /// Opens a script and selects one of its steps. Used by the Review page.
    void showStep(const QString& scriptId, int stepIndex);

signals:
    void statusMessage(const QString& text);

private slots:
    void reloadScripts();
    void onScriptChanged();
    void onStepSelected();
    void onStepTypeChanged(int index);
    void onFieldEdited();

    void onAddStep();
    void onDuplicateStep();
    void onDeleteStep();
    void onMoveUp();
    void onMoveDown();
    void onSave();
    void onRevert();

private:
    void loadWorkingCopy();
    void refreshStepList(int selectRow);
    void bindStepToForm(int row);
    void setDirty(bool dirty);
    int currentStepRow() const;

    std::unique_ptr<Ui::EditorPage> m_ui;

    AppModel* m_model = nullptr;
    QStandardItemModel* m_steps = nullptr;

    QString m_scriptId;
    std::vector<Step> m_working;  ///< edits live here until Save
    bool m_dirty = false;
    bool m_binding = false;  ///< guards the form against feedback loops
};

}  // namespace autotasks
