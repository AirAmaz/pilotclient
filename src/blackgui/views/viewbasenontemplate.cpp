/* Copyright (C) 2018
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackgui/components/texteditdialog.h"
#include "blackgui/dockwidgetinfoarea.h"
#include "blackgui/filters/filterdialog.h"
#include "blackgui/filters/filterwidget.h"
#include "blackgui/guiutility.h"
#include "blackgui/loadindicator.h"
#include "blackgui/menus/fontmenus.h"
#include "blackgui/menus/menudelegate.h"
#include "blackgui/views/viewbase.h"
#include "blackgui/shortcut.h"
#include "blackmisc/logmessage.h"

#include <QApplication>
#include <QAction>
#include <QDesktopWidget>
#include <QMetaMethod>
#include <QShortcut>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackGui;
using namespace BlackGui::Menus;
using namespace BlackGui::Models;
using namespace BlackGui::Filters;
using namespace BlackGui::Settings;
using namespace BlackGui::Components;

namespace BlackGui
{
    namespace Views
    {
        CViewBaseNonTemplate::CViewBaseNonTemplate(QWidget *parent) :
            QTableView(parent)
        {
            this->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &QWidget::customContextMenuRequested, this, &CViewBaseNonTemplate::customMenuRequested);
            connect(this, &QTableView::clicked, this, &CViewBaseNonTemplate::ps_clicked);
            connect(this, &QTableView::doubleClicked, this, &CViewBaseNonTemplate::ps_doubleClicked);
            this->horizontalHeader()->setSortIndicatorShown(true);

            // setting resize mode rowsResizeModeToContent() causes extremly slow views
            // default, see: m_rowResizeMode

            // scroll modes
            this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            this->setWordWrap(true);
            this->setTextElideMode(Qt::ElideMiddle);

            // shortcuts
            QShortcut *filter = new QShortcut(CShortcut::keyDisplayFilter(), this);
            connect(filter, &QShortcut::activated, this, &CViewBaseNonTemplate::displayFilterDialog);
            filter->setObjectName("Filter shortcut for " + this->objectName());
            filter->setContext(Qt::WidgetShortcut);

            QShortcut *clearSelection = new QShortcut(CShortcut::keyClearSelection(), this);
            connect(clearSelection, &QShortcut::activated, this, &CViewBaseNonTemplate::clearSelection);
            clearSelection->setObjectName("Clear selection shortcut for " + this->objectName());
            clearSelection->setContext(Qt::WidgetShortcut);

            QShortcut *saveJson = new QShortcut(CShortcut::keySaveViews(), this);
            connect(saveJson, &QShortcut::activated, this, &CViewBaseNonTemplate::saveJsonAction);
            saveJson->setObjectName("Save JSON for " + this->objectName());
            saveJson->setContext(Qt::WidgetShortcut);

            QShortcut *deleteRow = new QShortcut(CShortcut::keyDelete(), this);
            connect(deleteRow, &QShortcut::activated, this, &CViewBaseNonTemplate::removeSelectedRowsChecked);
            deleteRow->setObjectName("Remove selected rows for " + this->objectName());
            deleteRow->setContext(Qt::WidgetShortcut);

            QShortcut *copy = new QShortcut(CShortcut::keyCopy(), this);
            connect(copy, &QShortcut::activated, this, &CViewBaseNonTemplate::copy);
            copy->setObjectName("Copy selection shortcut for " + this->objectName());
            copy->setContext(Qt::WidgetShortcut);

            QShortcut *resize = new QShortcut(CShortcut::keyResizeView(), this);
            connect(resize, &QShortcut::activated, this, &CViewBaseNonTemplate::fullResizeToContents);
            resize->setObjectName("Resize view shortcut for " + this->objectName());
            resize->setContext(Qt::WidgetShortcut);
        }

        CViewBaseNonTemplate::~CViewBaseNonTemplate()
        {
            // dtor
        }

        bool CViewBaseNonTemplate::setParentDockWidgetInfoArea(CDockWidgetInfoArea *parentDockableWidget)
        {
            const bool c = CEnableForDockWidgetInfoArea::setParentDockWidgetInfoArea(parentDockableWidget);
            return c;
        }

        void CViewBaseNonTemplate::resizeToContents()
        {
            this->performModeBasedResizeToContent();
        }

        void CViewBaseNonTemplate::setFilterWidgetImpl(QWidget *filterWidget)
        {
            if (filterWidget == m_filterWidget) { return; }

            // dialog or filter widget
            if (m_filterWidget)
            {
                disconnect(m_filterWidget);
                this->menuRemoveItems(MenuFilter);
                if (m_filterWidget->parent() == this) { m_filterWidget->deleteLater(); }
                m_filterWidget = nullptr;
            }

            if (filterWidget)
            {
                this->menuAddItems(MenuFilter);
                m_filterWidget = filterWidget;
            }
        }

        void CViewBaseNonTemplate::setFilterDialog(CFilterDialog *filterDialog)
        {
            if (filterDialog == m_filterWidget) { return; }
            this->setFilterWidgetImpl(filterDialog);
            if (filterDialog)
            {
                const bool s = connect(filterDialog, &CFilterDialog::finished, this, &CViewBaseNonTemplate::ps_filterDialogFinished);
                Q_ASSERT_X(s, Q_FUNC_INFO, "filter dialog connect");
                Q_UNUSED(s);
            }
        }

        void CViewBaseNonTemplate::setFilterWidget(CFilterWidget *filterWidget)
        {
            if (filterWidget == m_filterWidget) { return; }
            this->setFilterWidgetImpl(filterWidget);
            if (filterWidget)
            {
                bool s = connect(filterWidget, &CFilterWidget::changeFilter, this, &CViewBaseNonTemplate::ps_filterWidgetChangedFilter);
                Q_ASSERT_X(s, Q_FUNC_INFO, "filter connect");
                s = connect(this, &CViewBaseNonTemplate::modelDataChanged, filterWidget, &CFilterWidget::onRowCountChanged);
                Q_ASSERT_X(s, Q_FUNC_INFO, "filter connect");
                Q_UNUSED(s);
            }
        }

        void CViewBaseNonTemplate::enableLoadIndicator(bool enable)
        {
            m_enabledLoadIndicator = enable;
        }

        bool CViewBaseNonTemplate::isShowingLoadIndicator() const
        {
            return m_loadIndicator && m_enabledLoadIndicator && m_showingLoadIndicator;
        }

        void CViewBaseNonTemplate::setSelectionModel(QItemSelectionModel *model)
        {
            if (this->selectionModel()) { disconnect(this->selectionModel()); }
            QTableView::setSelectionModel(model);
            if (this->selectionModel())
            {
                connect(this->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &CViewBaseNonTemplate::ps_rowSelected);
            }
        }

        QWidget *CViewBaseNonTemplate::mainApplicationWindowWidget() const
        {
            return CGuiUtility::mainApplicationWidget();
        }

        CStatusMessage CViewBaseNonTemplate::showFileLoadDialog(const QString &directory)
        {
            return this->ps_loadJson(directory);
        }

        CStatusMessage CViewBaseNonTemplate::showFileSaveDialog(bool selectedOnly, const QString &directory)
        {
            return this->ps_saveJson(selectedOnly, directory);
        }

        IMenuDelegate *CViewBaseNonTemplate::setCustomMenu(IMenuDelegate *menu, bool nestPreviousMenu)
        {
            if (menu && nestPreviousMenu)
            {
                // new menu with nesting
                menu->setNestedDelegate(m_menu);
                m_menu = menu;
            }
            else if (!menu && nestPreviousMenu)
            {
                // nested new menu
                m_menu = m_menu->getNestedDelegate();
            }
            else
            {
                // no nesting
                m_menu = menu;
            }
            return menu;
        }

        CMenuActions CViewBaseNonTemplate::initMenuActions(CViewBaseNonTemplate::MenuFlag menu)
        {
            if (m_menuFlagActions.contains(menu)) { return m_menuFlagActions.value(menu); }

            CMenuActions ma;
            switch (menu)
            {
            case MenuRefresh:
                {
                    static const QMetaMethod requestSignal = QMetaMethod::fromSignal(&CViewBaseNonTemplate::requestUpdate);
                    if (!this->isSignalConnected(requestSignal)) break;
                    ma.addAction(CIcons::refresh16(), "Update", CMenuAction::pathViewUpdates(), { this, &CViewBaseNonTemplate::triggerReload }); break;
                }
            case MenuBackend:
                {
                    static const QMetaMethod requestSignal = QMetaMethod::fromSignal(&CViewBaseNonTemplate::requestNewBackendData);
                    if (!this->isSignalConnected(requestSignal)) break;
                    ma.addAction(CIcons::refresh16(), "Reload from backend", CMenuAction::pathViewUpdates(), { this, &CViewBaseNonTemplate::triggerReloadFromBackend }); break;
                }
            case MenuDisplayAutomatically:
                {
                    QAction *a = ma.addAction(CIcons::appMappings16(), "Automatically display (when loaded)", CMenuAction::pathViewUpdates(), { this, &CViewBaseNonTemplate::toggleAutoDisplay });
                    a->setCheckable(true);
                    a->setChecked(this->displayAutomatically());
                    break;
                }
            case MenuRemoveSelectedRows: { ma.addAction(CIcons::delete16(), "Remove selected rows", CMenuAction::pathViewAddRemove(), { this, &CViewBaseNonTemplate::removeSelectedRowsChecked }, CShortcut::keyDelete()); break; }
            case MenuClear: { ma.addAction(CIcons::delete16(), "Clear", CMenuAction::pathViewAddRemove(), { this, &CViewBaseNonTemplate::clear }); break; }
            case MenuFilter:
                {
                    if (m_filterWidget)
                    {
                        const bool dialog = qobject_cast<QDialog *>(m_filterWidget);
                        if (dialog) ma.addAction(CIcons::filter16(), "Show filter " + CShortcut::toParenthesisString(CShortcut::keyDisplayFilter()), CMenuAction::pathViewFilter(), { this, &CViewBaseNonTemplate::displayFilterDialog }, CShortcut::keyDisplayFilter());
                        ma.addAction(CIcons::filter16(), "Remove Filter", CMenuAction::pathViewFilter(), { this, &CViewBaseNonTemplate::ps_removeFilter });
                    }
                    break;
                }
            case MenuMaterializeFilter: { ma.addAction(CIcons::tableRelationship16(), "Materialize filtered data", CMenuAction::pathViewFilter(), { this, &CViewBaseNonTemplate::materializeFilter }); break; }
            case MenuLoad: { ma.addAction(CIcons::disk16(), "Load from file ", CMenuAction::pathViewLoadSave(), { this, &CViewBaseNonTemplate::loadJsonAction }); break; }
            case MenuSave:
                {
                    ma.addAction(CIcons::disk16(), "Save data in file " + CShortcut::toParenthesisString(CShortcut::keySaveViews()), CMenuAction::pathViewLoadSave(), { this, &CViewBaseNonTemplate::saveJsonAction }, CShortcut::keySaveViews());
                    if (this->hasSelection())
                    {
                        ma.addAction(CIcons::disk16(), "Save selected data in file", CMenuAction::pathViewLoadSave(), { this, &CViewBaseNonTemplate::saveSelectedJsonAction }); break;
                    }
                    break;
                }
            case MenuCut:
                {
                    if (!QApplication::clipboard()) break;
                    ma.addAction(CIcons::cut16(), "Cut", CMenuAction::pathViewCutPaste(), { this, &CViewBaseNonTemplate::cut }, QKeySequence(QKeySequence::Paste));
                    break;
                }
            case MenuPaste:
                {
                    if (!QApplication::clipboard()) break;
                    ma.addAction(CIcons::paste16(), "Paste", CMenuAction::pathViewCutPaste(), { this, &CViewBaseNonTemplate::paste }, QKeySequence(QKeySequence::Paste));
                    break;
                }
            case MenuCopy:
                {
                    if (!QApplication::clipboard()) break;
                    ma.addAction(CIcons::copy16(), "Copy", CMenuAction::pathViewCutPaste(), { this, &CViewBaseNonTemplate::copy }, QKeySequence(QKeySequence::Copy));
                    break;
                }
            default:
                break;
            }
            m_menuFlagActions.insert(menu, ma);
            return ma;
        }

        void CViewBaseNonTemplate::settingsChanged()
        {
            if (!this->allowsMultipleSelectedRows()) { return; }
            const CGeneralGuiSettings settings = m_guiSettings.getThreadLocal();
            m_originalSelectionMode = settings.getPreferredSelection();
            if (this->isCurrentlyAllowingMultipleRowSelections())
            {
                this->setSelectionMode(settings.getPreferredSelection());
            }
        }

        void CViewBaseNonTemplate::rememberLastJsonDirectory(const QString &selectedFileOrDir)
        {
            if (selectedFileOrDir.isEmpty()) { return; }
            const QString dir = CDirectories::fileNameToDirectory(selectedFileOrDir);
            QDir d(dir);
            if (!d.exists()) { return; }

            // existing dir
            CDirectories directories = m_dirSettings.get();
            directories.setPropertyByIndex(m_dirSettingsIndex, CVariant::fromValue(dir));
            const CStatusMessage msg = m_dirSettings.setAndSave(directories);
            CLogMessage::preformatted(msg);
        }

        QString CViewBaseNonTemplate::getRememberedLastJsonDirectory() const
        {
            const CDirectories directories = m_dirSettings.get();
            return directories.propertyByIndex(m_dirSettingsIndex).toQString();
        }

        Components::CTextEditDialog *CViewBaseNonTemplate::textEditDialog()
        {
            if (!m_textEditDialog)
            {
                m_textEditDialog = new CTextEditDialog(this);
            }
            return m_textEditDialog;
        }

        void CViewBaseNonTemplate::customMenu(CMenuActions &menuActions)
        {
            // delegate?
            if (m_menu) { m_menu->customMenu(menuActions); }

            // standard view menus
            if (m_menus.testFlag(MenuRefresh)) { menuActions.addActions(this->initMenuActions(MenuRefresh)); }
            if (m_menus.testFlag(MenuBackend)) { menuActions.addActions(this->initMenuActions(MenuBackend)); }
            if (m_showingLoadIndicator)
            {
                // just in case, if this ever will be dangling
                menuActions.addAction(CIcons::preloader16(), "Hide load indicator", CMenuAction::pathViewUpdates(), nullptr, { this, &CViewBaseNonTemplate::hideLoacIndicatorForced });
            }

            if (m_menus.testFlag(MenuClear)) { menuActions.addActions(this->initMenuActions(MenuClear)); }
            if (m_menus.testFlag(MenuDisplayAutomatically))
            {
                // here I expect only one action
                QAction *a = menuActions.addActions(this->initMenuActions(MenuDisplayAutomatically)).first();
                a->setChecked(this->displayAutomatically());
            }
            if (m_menus.testFlag(MenuRemoveSelectedRows))
            {
                if (this->hasSelection())
                {
                    menuActions.addActions(this->initMenuActions(MenuRemoveSelectedRows));
                }
            }

            if (m_menus.testFlag(MenuCopy))  { menuActions.addActions(this->initMenuActions(MenuCopy)); }
            if (m_menus.testFlag(MenuCut))   { menuActions.addActions(this->initMenuActions(MenuCut)); }
            if (m_menus.testFlag(MenuPaste)) { menuActions.addActions(this->initMenuActions(MenuPaste)); }
            if (m_menus.testFlag(MenuFont) && m_fontMenu)
            {
                menuActions.addActions(m_fontMenu->getActions(), CMenuAction::pathFont());
            }

            if (m_menus.testFlag(MenuFilter) && m_filterWidget)
            {
                menuActions.addActions(this->initMenuActions(MenuFilter));
                if (m_menus.testFlag(MenuMaterializeFilter))
                {
                    menuActions.addActions(this->initMenuActions(MenuMaterializeFilter));
                }
            }

            // selection menus, not in menu action list because it depends on current selection
            const SelectionMode sm = this->selectionMode();
            if (sm == MultiSelection || sm == ExtendedSelection)
            {
                menuActions.addAction("Select all", CMenuAction::pathViewSelection(), nullptr, { this, &CViewBaseNonTemplate::selectAll }, Qt::CTRL + Qt::Key_A);
            }
            if (sm != NoSelection)
            {
                menuActions.addAction("Clear selection " + CShortcut::toParenthesisString(CShortcut::keyClearSelection()), CMenuAction::pathViewSelection(), nullptr, { this, &CViewBaseNonTemplate::clearSelection }, CShortcut::keyClearSelection());
            }
            if ((m_originalSelectionMode == MultiSelection || m_originalSelectionMode == ExtendedSelection) && m_menus.testFlag(MenuToggleSelectionMode))
            {
                if (sm != MultiSelection)
                {
                    menuActions.addAction("Switch to multi selection", CMenuAction::pathViewSelection(), nullptr, { this, &CViewBaseNonTemplate::setMultiSelection });
                }

                if (sm != ExtendedSelection)
                {
                    menuActions.addAction("Switch to extended selection", CMenuAction::pathViewSelection(), nullptr, { this, &CViewBaseNonTemplate::setExtendedSelection });
                }

                if (sm != SingleSelection)
                {
                    menuActions.addAction("Switch to single selection", CMenuAction::pathViewSelection(), nullptr, { this, &CViewBaseNonTemplate::setSingleSelection });
                }
            }

            // load/save
            if (m_menus.testFlag(MenuLoad)) { menuActions.addActions(this->initMenuActions(MenuLoad)); }
            if (m_menus.testFlag(MenuSave) && !isEmpty()) { menuActions.addActions(this->initMenuActions(MenuSave)); }

            // resizing
            menuActions.addAction(CIcons::resize16(), "&Resize " + CShortcut::toParenthesisString(CShortcut::keyResizeView()), CMenuAction::pathViewResize(), nullptr, { this, &CViewBaseNonTemplate::presizeOrFullResizeToContents });

            // resize to content might decrease performance,
            // so I only allow changing to "content resizing" if size matches
            const bool enabled = !this->reachedResizeThreshold();
            const bool autoResize = (m_resizeMode == ResizingAuto);

            // when not set to auto, then lets set how we want to resize rows
            if (m_rowResizeMode == Interactive)
            {
                QAction *a = menuActions.addAction(CIcons::resizeVertical16(), " Resize rows to content (auto), can be slow", CMenuAction::pathViewResize(), nullptr, { this, &CViewBaseNonTemplate::rowsResizeModeToContent });
                a->setEnabled(enabled && !autoResize);
            }
            else
            {
                QAction *a = menuActions.addAction(CIcons::resizeVertical16(), "Resize rows interactively", CMenuAction::pathViewResize(), nullptr, { this, &CViewBaseNonTemplate::rowsResizeModeToInteractive });
                a->setEnabled(!autoResize);
            }

            // export actions, display in text edit
            if (CBuildConfig::isLocalDeveloperDebugBuild())
            {
                menuActions.addAction(CIcons::tableSheet16(), "Display as JSON", CMenuAction::pathViewLoadSave(), { this, &CViewBaseNonTemplate::displayJsonPopup });
                if (this->hasSelection())
                {
                    menuActions.addAction(CIcons::tableSheet16(), "Display selected as JSON", CMenuAction::pathViewLoadSave(), { this, &CViewBaseNonTemplate::displaySelectedJsonPopup });;
                }
            }


            QAction *actionInteractiveResize = menuActions.addAction(CIcons::viewMultiColumn(), "Resize (auto)", CMenuAction::pathViewResize(), nullptr);
            actionInteractiveResize->setObjectName(this->objectName().append("ActionResizing"));
            actionInteractiveResize->setCheckable(true);
            actionInteractiveResize->setChecked(autoResize);
            actionInteractiveResize->setEnabled(enabled);
            connect(actionInteractiveResize, &QAction::toggled, this, &CViewBaseNonTemplate::toggleResizeMode);
        }

        void CViewBaseNonTemplate::resizeEvent(QResizeEvent *event)
        {
            if (this->isShowingLoadIndicator())
            {
                // re-center
                this->centerLoadIndicator();
            }
            QTableView::resizeEvent(event);
        }

        int CViewBaseNonTemplate::getHorizontalHeaderFontHeight() const
        {
            const QFontMetrics m(this->getHorizontalHeaderFont());
            const int h = m.height();
            return h;
        }

        bool CViewBaseNonTemplate::hasSelection() const
        {
            return this->selectionModel()->hasSelection();
        }

        QModelIndexList CViewBaseNonTemplate::selectedRows() const
        {
            return this->selectionModel()->selectedRows();
        }

        QModelIndexList CViewBaseNonTemplate::unselectedRows() const
        {
            QModelIndexList selected = this->selectedRows();
            QModelIndexList unselected;
            const int rows = this->rowCount();
            for (int r = 0; r < rows; r++)
            {
                const QModelIndex mi = this->model()->index(r, 0);
                if (selected.contains(mi)) { continue; }
                unselected.push_back(mi);
            }
            return unselected;
        }

        void CViewBaseNonTemplate::selectRows(const QSet<int> &rows)
        {
            if (!this->selectionModel()) { return; }

            // multiple times faster than multiple than this->selectRow()
            this->clearSelection();
            QItemSelection selectedItems;
            const int columns = this->model()->columnCount() - 1;
            for (int r : rows)
            {
                selectedItems.select(this->model()->index(r, 0), this->model()->index(r, columns));
            }
            this->selectionModel()->select(selectedItems, QItemSelectionModel::Select);
        }

        int CViewBaseNonTemplate::selectedRowCount() const
        {
            if (!this->hasSelection()) { return 0;}
            return this->selectedRows().count();
        }

        int CViewBaseNonTemplate::unselectedRowCount() const
        {
            return this->rowCount() - this->selectedRowCount();
        }

        bool CViewBaseNonTemplate::hasSingleSelectedRow() const
        {
            return this->selectedRowCount() == 1;
        }

        bool CViewBaseNonTemplate::hasMultipleSelectedRows() const
        {
            return this->selectedRowCount() > 1;
        }

        bool CViewBaseNonTemplate::allowsMultipleSelectedRows() const
        {
            return m_originalSelectionMode == ExtendedSelection || m_originalSelectionMode == MultiSelection;
        }

        bool CViewBaseNonTemplate::isCurrentlyAllowingMultipleRowSelections() const
        {
            QAbstractItemView::SelectionMode m = this->selectionMode();
            return m == QAbstractItemView::MultiSelection || m == QAbstractItemView::ExtendedSelection;
        }

        void CViewBaseNonTemplate::init()
        {
            this->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); // faster mode
            this->horizontalHeader()->setStretchLastSection(true);
            const int fh = qRound(1.5 * this->getHorizontalHeaderFontHeight());
            this->verticalHeader()->setDefaultSectionSize(fh); // for height
            this->verticalHeader()->setMinimumSectionSize(fh); // for height

            switch (m_rowResizeMode)
            {
            case Interactive: this->rowsResizeModeToInteractive(); break;
            case Content: this->rowsResizeModeToContent(); break;
            default:
                Q_ASSERT_X(false, Q_FUNC_INFO, "wrong resize mode");
                break;
            }

            // call this deferred, otherwise the values are overridden with any values
            // from the UI builder
            const QPointer<CViewBaseNonTemplate> guard(this);
            QTimer::singleShot(500, this, [ = ]()
            {
                if (!guard) { return; }
                CViewBaseNonTemplate::settingsChanged();
            });
        }

        QString CViewBaseNonTemplate::getFileDialogFileName(bool load) const
        {
            // some logic to find a useful default name
            if (load)
            {
                return CFileUtils::appendFilePaths(this->getRememberedLastJsonDirectory(), CFileUtils::jsonWildcardAppendix());
            }

            // Save file path
            const QString dir = m_dirSettings.get().propertyByIndex(m_dirSettingsIndex).toQString();
            QString name(m_saveFileName);
            if (name.isEmpty())
            {
                // create a name
                if (this->getDockWidgetInfoArea())
                {
                    name = this->getDockWidgetInfoArea()->windowTitle();
                }
                else
                {
                    name = this->metaObject()->className();
                }
            }
            if (!name.endsWith(CFileUtils::jsonAppendix(), Qt::CaseInsensitive))
            {
                name += CFileUtils::jsonAppendix();
            }
            return CFileUtils::appendFilePaths(dir, name);
        }

        void CViewBaseNonTemplate::menuRemoveItems(Menu menusToRemove)
        {
            m_menus &= (~menusToRemove);
        }

        void CViewBaseNonTemplate::menuAddItems(Menu menusToAdd)
        {
            m_menus |= menusToAdd;
            if (menusToAdd.testFlag(MenuRemoveSelectedRows))
            {
                m_enableDeleteSelectedRows = true;
            }
        }

        void CViewBaseNonTemplate::displayFilterDialog()
        {
            if (!m_menus.testFlag(MenuFilter)) { return; }
            if (!m_filterWidget) { return; }
            m_filterWidget->show();
        }

        void CViewBaseNonTemplate::loadJsonAction()
        {
            if (!m_menus.testFlag(MenuLoad)) { return; }
            const CStatusMessage m = this->ps_loadJson();
            if (!m.isEmpty())
            {
                CLogMessage::preformatted(m);
            }
        }

        void CViewBaseNonTemplate::saveJsonAction()
        {
            if (this->isEmpty()) { return; }
            if (!m_menus.testFlag(MenuSave)) { return; }
            const CStatusMessage m = this->ps_saveJson(false);
            if (!m.isEmpty())
            {
                CLogMessage::preformatted(m);
            }
        }

        void CViewBaseNonTemplate::saveSelectedJsonAction()
        {
            if (this->isEmpty()) { return; }
            if (!m_menus.testFlag(MenuSave)) { return; }
            const CStatusMessage m = this->ps_saveJson(true);
            if (!m.isEmpty())
            {
                CLogMessage::preformatted(m);
            }
        }

        void CViewBaseNonTemplate::triggerReload()
        {
            this->showLoadIndicatorWithTimeout(m_loadIndicatorTimeoutMsDefault);
            emit this->requestUpdate();
        }

        void CViewBaseNonTemplate::triggerReloadFromBackend()
        {
            this->showLoadIndicatorWithTimeout(m_loadIndicatorTimeoutMsDefault);
            emit this->requestNewBackendData();
        }

        void CViewBaseNonTemplate::onModelChanged()
        {
            this->updateSortIndicator();
        }

        void CViewBaseNonTemplate::rowsResizeModeToInteractive()
        {
            const int height = this->verticalHeader()->minimumSectionSize();
            QHeaderView *verticalHeader = this->verticalHeader();
            Q_ASSERT_X(verticalHeader, Q_FUNC_INFO, "Missing vertical header");
            verticalHeader->setSectionResizeMode(QHeaderView::Interactive);
            verticalHeader->setDefaultSectionSize(height);
            m_rowResizeMode = Interactive;
        }

        void CViewBaseNonTemplate::rowsResizeModeToContent()
        {
            QHeaderView *verticalHeader = this->verticalHeader();
            Q_ASSERT(verticalHeader);
            verticalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
            m_rowResizeMode = Content;
        }

        void CViewBaseNonTemplate::rowsResizeModeBasedOnThreshold(int elements)
        {
            if (elements > ResizeRowsToContentThreshold)
            {
                this->rowsResizeModeToInteractive();
            }
            else
            {
                this->rowsResizeModeToContent();
            }
        }

        int CViewBaseNonTemplate::showLoadIndicator(int containerSizeDependent, int timeoutMs, bool processEvents)
        {
            if (!m_enabledLoadIndicator) { return -1; }
            if (m_showingLoadIndicator) { return -1; }
            if (this->hasDockWidgetArea())
            {
                if (!this->isVisibleWidget()) { return -1; }
            }

            if (containerSizeDependent >= 0)
            {
                // really with indicator?
                if (containerSizeDependent < ResizeSubsetThreshold) { return -1; }
            }
            m_showingLoadIndicator = true;
            emit this->loadIndicatorVisibilityChanged(m_showingLoadIndicator);

            if (!m_loadIndicator)
            {
                m_loadIndicator = new CLoadIndicator(64, 64, this);
            }
            this->centerLoadIndicator();
            return m_loadIndicator->startAnimation(timeoutMs > 0 ? timeoutMs : m_loadIndicatorTimeoutMsDefault, processEvents);
        }

        int CViewBaseNonTemplate::showLoadIndicatorWithTimeout(int timeoutMs, bool processEvents)
        {
            return this->showLoadIndicator(-1, timeoutMs, processEvents);
        }

        void CViewBaseNonTemplate::centerLoadIndicator()
        {
            if (!m_loadIndicator) { return; }
            const QPoint middle = this->viewport()->geometry().center();
            m_loadIndicator->centerLoadIndicator(middle);
        }

        void CViewBaseNonTemplate::hideLoadIndicator(int loadingId)
        {
            if (!m_showingLoadIndicator) { return; }
            m_showingLoadIndicator = false;
            emit this->loadIndicatorVisibilityChanged(m_showingLoadIndicator);
            if (!m_loadIndicator) { return; }
            m_loadIndicator->stopAnimation(loadingId);
        }

        bool CViewBaseNonTemplate::isResizeConditionMet(int containerSize) const
        {
            if (m_resizeMode == ResizingAlways) { return true; }
            if (m_resizeMode == PresizeSubset)  { return false; }
            if (m_resizeMode == ResizingOff)    { return false; }
            if (m_resizeMode == ResizingOnce)   { return m_resizeCount < 1; }
            if (m_resizeMode == ResizingAuto)
            {
                if (reachedResizeThreshold(containerSize)) { return false; }
                if (m_resizeAutoNthTime < 2)  { return true; }
                return (m_resizeCount % m_resizeAutoNthTime) == 0;
            }
            return false;
        }

        void CViewBaseNonTemplate::fullResizeToContents()
        {
            // resize to maximum magic trick from:
            // http://stackoverflow.com/q/3433664/356726
            this->setVisible(false);
            const QRect vpOriginal = this->viewport()->geometry();
            if (m_forceColumnsToMaxSize)
            {
                // vpNew.setWidth(std::numeric_limits<qint32>::max()); // largest finite value
                const QRect screenGeometry = QApplication::desktop()->screenGeometry();
                QRect vpNew = vpOriginal;
                vpNew.setWidth(screenGeometry.width());
                this->viewport()->setGeometry(vpNew);
            }

            this->resizeColumnsToContents(); // columns

            // useless if mode is Interactive
            if (m_rowResizeMode == Content)
            {
                this->resizeRowsToContents(); // rows
            }
            m_resizeCount++;

            // re-stretch
            if (m_forceStretchLastColumnWhenResized) { this->horizontalHeader()->setStretchLastSection(true); }
            if (m_forceColumnsToMaxSize) { this->viewport()->setGeometry(vpOriginal); }

            // if I store the original visibility and then
            // set it back here, the whole view disappears
            this->setVisible(true);
        }

        void CViewBaseNonTemplate::customMenuRequested(const QPoint &pos)
        {
            QMenu menu;
            CMenuActions menuActions;
            this->customMenu(menuActions);
            if (menuActions.isEmpty()) { return; }
            menuActions.toQMenu(menu, true);

            // Nested dock widget menu
            const CDockWidgetInfoArea *dockWidget = this->getDockWidgetInfoArea();
            if (dockWidget)
            {
                if (!menu.isEmpty()) { menu.addSeparator(); }
                const QString mm = QString("Dock widget '%1'").arg(dockWidget->windowTitleOrBackup());
                QMenu *dockWidgetSubMenu = menu.addMenu(CIcons::text16(), mm);
                dockWidget->addToContextMenu(dockWidgetSubMenu);
            }

            const QPoint globalPos = this->mapToGlobal(pos);
            menu.exec(globalPos);
        }

        void CViewBaseNonTemplate::toggleResizeMode(bool checked)
        {
            m_resizeMode = checked ? ResizingAuto : ResizingOff;
        }

        void CViewBaseNonTemplate::toggleAutoDisplay()
        {
            const QAction *a = qobject_cast<const QAction *>(QObject::sender());
            if (!a) { return; }
            Q_ASSERT_X(a->isCheckable(), Q_FUNC_INFO, "object not checkable");
            m_displayAutomatically = a->isChecked();
        }

        void CViewBaseNonTemplate::setSingleSelection()
        {
            this->setSelectionMode(SingleSelection);
        }

        void CViewBaseNonTemplate::setExtendedSelection()
        {
            if (this->allowsMultipleSelectedRows())
            {
                this->setSelectionMode(ExtendedSelection);
            }
        }

        void CViewBaseNonTemplate::setMultiSelection()
        {
            if (this->allowsMultipleSelectedRows())
            {
                this->setSelectionMode(MultiSelection);
            }
        }

        void CViewBaseNonTemplate::removeSelectedRowsChecked()
        {
            if (!m_enableDeleteSelectedRows) { return; }
            this->removeSelectedRows();
        }

        void CViewBaseNonTemplate::updatedIndicator()
        {
            this->update();
        }

        void CViewBaseNonTemplate::dragEnterEvent(QDragEnterEvent *event)
        {
            if (!event || !this->acceptDrop(event->mimeData())) { return; }
            this->setBackgroundRole(QPalette::Highlight);
            event->acceptProposedAction();
        }

        void CViewBaseNonTemplate::dragMoveEvent(QDragMoveEvent *event)
        {
            if (!event || !this->acceptDrop(event->mimeData())) { return; }
            event->acceptProposedAction();
        }

        void CViewBaseNonTemplate::dragLeaveEvent(QDragLeaveEvent *event)
        {
            if (!event) { return; }
            event->accept();
        }

        void CViewBaseNonTemplate::dropEvent(QDropEvent *event)
        {
            if (!event) { return; }
            QTableView::dropEvent(event);
        }

        int CViewBaseNonTemplate::getPresizeRandomElementsSize(int containerSize) const
        {
            containerSize = containerSize >= 0 ? containerSize : this->rowCount();
            const int presizeRandomElements = containerSize > 1000 ? containerSize / 100 : containerSize / 40;
            return presizeRandomElements;
        }
    } // namespace
} // namespace