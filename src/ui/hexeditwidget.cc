/*
 * Copyright 2017 CodiLime
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <QAction>
#include <QColorDialog>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "dbif/info.h"
#include "dbif/types.h"
#include "dbif/universe.h"

#include "ui/hexeditwidget.h"
#include "ui/nodewidget.h"
#include "ui/veles_mainwindow.h"

#include "util/settings/hexedit.h"
#include "util/settings/shortcuts.h"
#include "util/settings/theme.h"
#include "util/icons.h"

#include "visualization/panel.h"

namespace veles {
namespace ui {

using util::settings::shortcuts::ShortcutsModel;

/*****************************************************************************/
/* Public methods */
/*****************************************************************************/

HexEditWidget::HexEditWidget(
    MainWindowWithDetachableDockWidgets *main_window,
    data::NodeID node,
    QSharedPointer<client::NodeTreeModel> node_tree_model,
    QSharedPointer<QItemSelectionModel>& selection_model)
    : View("Hex editor", ":/images/show_hex_edit.png"),
    main_window_(main_window),
    node_(node),
    node_tree_model_(node_tree_model),
    selection_model_(selection_model) {
  hex_edit_ = new HexEdit(node, node_tree_model, selection_model, this);
  setCentralWidget(hex_edit_);
  connect(hex_edit_, &HexEdit::selectionChanged,
      this, &HexEditWidget::selectionChanged);

  search_dialog_ = new SearchDialog(hex_edit_, this);

  createActions();
  createToolBars();

  setupDataModelHandlers();

  reapplySettings();
  client::Node* root = node_tree_model->nodeTree()->node(node_);
  if (root) {
    QString name("_");
    root->getQStringAttr("name", name);
    setWindowTitle(name);
  }

  connect(&parsers_menu_, &QMenu::triggered, this, &HexEditWidget::parse);
  setParserIds(dynamic_cast<VelesMainWindow*>(
      MainWindowWithDetachableDockWidgets::getFirstMainWindow())
      ->parsersList());
  selectionChanged(0, 0);
}

void HexEditWidget::reapplySettings() {
  hex_edit_->setBytesPerRow(util::settings::hexedit::columnsNumber(),
      util::settings::hexedit::resizeColumnsToWindowWidth());
}

void HexEditWidget::setParserIds(QStringList ids) {
  parsers_ids_ = ids;
  initParsersMenu();
  hex_edit_->setParserIds(ids);
}

QString HexEditWidget::addressAsText(qint64 addr) {
  return QString::number(addr, 16).rightJustified(
      sizeof(qint64) * 2, '0');
}

/*****************************************************************************/
/* Private UI methods */
/*****************************************************************************/

void HexEditWidget::createActions() {
//  Currently not implemented
//  upload_act_ = new QAction(QIcon(":/images/upload-32.ico"), tr("&Upload"),
//      this);
//  upload_act_->setShortcuts(QKeySequence::Save);
//  upload_act_->setStatusTip(tr("Upload changed to database"));
//  connect(upload_act_, SIGNAL(triggered()), this, SLOT(uploadChanges()));
//  upload_act_->setEnabled(false);

//  save_as_act_ = new QAction(QIcon(":/images/save.png"), tr("Save &As..."),
//      this);
//  save_as_act_->setShortcuts(QKeySequence::SaveAs);
//  save_as_act_->setStatusTip(tr("Save the document under a new name"));
//  connect(save_as_act_, SIGNAL(triggered()), this, SLOT(saveAs()));
//  save_as_act_->setEnabled(false);

//  save_readable_ = new QAction(tr("Save &Readable..."), this);
//  save_readable_->setStatusTip(tr("Save document in readable form"));
  // connect(save_readable_, SIGNAL(triggered()), this,
  // SLOT(saveToReadableFile()));

//  undo_act_ = new QAction(QIcon(":/images/undo.png"), tr("&Undo"), this);
//  undo_act_->setShortcuts(QKeySequence::Undo);
//  undo_act_->setEnabled(false);

//  redo_act_ = new QAction(QIcon(":/images/redo.png"), tr("&Redo"), this);
//  redo_act_->setShortcuts(QKeySequence::Redo);
//  redo_act_->setEnabled(false);

  find_act_ = ShortcutsModel::getShortcutsModel()->createQAction(
        util::settings::shortcuts::HEX_FIND,
        this, QIcon(":/images/find.png"), Qt::WidgetWithChildrenShortcut);
  find_act_->setStatusTip(tr("Show the dialog for finding and replacing"));
  connect(find_act_, SIGNAL(triggered()), this, SLOT(showSearchDialog()));

  find_next_act_ = ShortcutsModel::getShortcutsModel()->createQAction(
        util::settings::shortcuts::HEX_FIND_NEXT,
        this, QIcon(":/images/find.png"), Qt::WidgetWithChildrenShortcut);
  find_next_act_->setStatusTip(
      tr("Find next occurrence of the searched pattern"));
  find_next_act_->setEnabled(false);
  connect(find_next_act_, SIGNAL(triggered()), this, SLOT(findNext()));
  connect(search_dialog_, SIGNAL(enableFindNext(bool)),
      this, SLOT(enableFindNext(bool)));

  QColor icon_color = palette().color(QPalette::WindowText);
  visualization_act_ = ShortcutsModel::getShortcutsModel()->createQAction(
        util::settings::shortcuts::OPEN_VISUALIZATION, this,
        util::getColoredIcon(":/images/trigram_icon.png", icon_color),
        Qt::WidgetWithChildrenShortcut);
  visualization_act_->setToolTip(tr("Visualization"));

  visualization_act_->setEnabled(
      node_tree_model_->binData(node_)->size() > 0);

  connect(visualization_act_, SIGNAL(triggered()), this,
          SLOT(showVisualization()));

  show_node_tree_act_ = ShortcutsModel::getShortcutsModel()->createQAction(
        util::settings::shortcuts::SHOW_NODE_TREE,
        this, QIcon(":/images/show_node_tree.png"), Qt::WidgetWithChildrenShortcut);
  show_node_tree_act_->setToolTip(tr("Node tree"));
  show_node_tree_act_->setEnabled(true);
  show_node_tree_act_->setCheckable(true);
  show_node_tree_act_->setChecked(true);
  connect(show_node_tree_act_, SIGNAL(toggled(bool)),
      this, SIGNAL(showNodeTree(bool)));

//  Currently not implemented
//  show_minimap_act_ = ShortcutsModel::ShortcutsModel::getShortcutsModel()->createQAction(
//        util::settings::shortcuts::SHOW_MINIMAP,
//        this, QIcon(":/images/show_minimap.png"), Qt::WidgetWithChildrenShortcut);
//  show_minimap_act_->setToolTip(tr("Minimap"));
//  show_minimap_act_->setEnabled(true);
//  show_minimap_act_->setCheckable(true);
//  show_minimap_act_->setChecked(false);
//  connect(show_minimap_act_, SIGNAL(toggled(bool)), this,
//      SIGNAL(showMinimap(bool)));

  show_hex_edit_act_ = ShortcutsModel::getShortcutsModel()->createQAction(
        util::settings::shortcuts::OPEN_HEX,
        this, QIcon(":/images/show_hex_edit.png"), Qt::WidgetWithChildrenShortcut);
  show_hex_edit_act_->setToolTip(tr("Hex editor"));
  show_hex_edit_act_->setEnabled(true);
  connect(show_hex_edit_act_, SIGNAL(triggered()),
      this, SLOT(showHexEditor()));
}

void HexEditWidget::createToolBars() {
  tools_tool_bar_ = new QToolBar(tr("Tools"));
  addAction(show_node_tree_act_);
  tools_tool_bar_->addAction(show_node_tree_act_);
  //Disabled until minimap does anything of value in hex-view
  //addAction(show_minimap_act_);
  //tools_tool_bar_->addAction(show_minimap_act_);

  auto parser_tool_button = new QToolButton();
  parser_tool_button->setMenu(&parsers_menu_);
  parser_tool_button->setPopupMode(QToolButton::InstantPopup);
  parser_tool_button->setIcon(QIcon(":/images/parse.png"));
  parser_tool_button->setText(tr("&Parse"));
  parser_tool_button->setToolTip(tr("Parser"));
  parser_tool_button->setAutoRaise(true);
  auto widget_action = new QWidgetAction(tools_tool_bar_);
  widget_action->setDefaultWidget(parser_tool_button);
  tools_tool_bar_->addAction(widget_action);

  addAction(visualization_act_);
  addAction(show_hex_edit_act_);
  tools_tool_bar_->addAction(visualization_act_);
  tools_tool_bar_->addAction(show_hex_edit_act_);
  tools_tool_bar_->setContextMenuPolicy(Qt::PreventContextMenu);
  addToolBar(tools_tool_bar_);

  //Not implemented yet.
  //file_tool_bar_ = new QToolBar(tr("File"));
  //file_tool_bar_->addAction(upload_act_);
  //file_tool_bar_->addAction(save_as_act_);
  //addToolBar(file_tool_bar_);

  edit_tool_bar_ = new QToolBar(tr("Edit"));

  //Not implemented yet.
  //edit_tool_bar_->addAction(undo_act_);
  //edit_tool_bar_->addAction(redo_act_);

  edit_tool_bar_->addAction(find_act_);
  edit_tool_bar_->addAction(find_next_act_);
  edit_tool_bar_->setContextMenuPolicy(Qt::PreventContextMenu);
  addToolBar(edit_tool_bar_);

  createSelectionInfo();
}

void HexEditWidget::initParsersMenu() {
  parsers_menu_.clear();
  parsers_menu_.addAction("auto");
  parsers_menu_.addSeparator();
  for (auto id : parsers_ids_) {
    parsers_menu_.addAction(id);
  }
}

void HexEditWidget::createSelectionInfo() {
  QWidgetAction* widget_action = new QWidgetAction(this);
  QWidget* selection_panel = new QWidget;
  QHBoxLayout* layout = new QHBoxLayout;
  selection_panel->setLayout(layout);
  layout->addStretch(1);
  selection_label_ = new QLabel;
  selection_label_->setFont(util::settings::theme::font());
  selection_label_->setText("");
  selection_label_->setTextInteractionFlags(
      Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
  layout->addWidget(selection_label_);
  widget_action->setDefaultWidget(selection_panel);
  QToolBar* selection_toolbar = new QToolBar;
  selection_toolbar->addAction(widget_action);
  selection_toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
  addToolBar(selection_toolbar);
}

void HexEditWidget::addChunk(QString name, QString type, QString comment,
                          uint64_t start, uint64_t end,
                          const QModelIndex &index) {
  node_tree_model_->addChunk(name, type, comment, start, end, index);
}

void HexEditWidget::setupDataModelHandlers() {
  // FIXME
  // connect(data_model_.data(), &FileBlobModel::newBinData,
  //    this, &HexEditWidget::newBinData);
}

/*****************************************************************************/
/* Other private methods */
/*****************************************************************************/

bool HexEditWidget::saveFile(const QString &file_name) {
  QString tmp_file_name = file_name + ".~tmp";

  QFile file(tmp_file_name);
  file.open(QIODevice::WriteOnly);
  auto bin_data = node_tree_model_->binData(node_);
  bool ok = false;
  if (bin_data) {
    ok = file.write(QByteArray((const char *)bin_data->rawData(),
        static_cast<int>(bin_data->size()))) != -1;
  }
  if (QFile::exists(file_name)) ok = QFile::remove(file_name);

  if (ok) {
    ok = file.copy(file_name);
    if (ok) ok = QFile::remove(tmp_file_name);
  }
  file.close();

  if (!ok) {
    QMessageBox::warning(this, tr("HexEdit"),
        tr("Cannot write file %1.").arg(file_name));
    return false;
  }

  return true;
}

/*****************************************************************************/
/* Private Slots */
/*****************************************************************************/

void HexEditWidget::parse(QAction *action) {
  if (action->text() == "auto") {
    node_tree_model_->parse(node_);
  } else {
    node_tree_model_->parse(node_, action->text());
  }
}

void HexEditWidget::findNext() { search_dialog_->findNext(); }

void HexEditWidget::showSearchDialog() { search_dialog_->show(); }

void HexEditWidget::uploadChanges() {
}

bool HexEditWidget::saveAs() {
  QString file_name = QFileDialog::getSaveFileName(
      this, tr("Save As"), cur_file_);
  if (file_name.isEmpty()) return false;

  return saveFile(file_name);
}

void HexEditWidget::showVisualization() {
  auto *panel = new visualization::VisualizationPanel(main_window_,
      node_, node_tree_model_);
  panel->setData(QByteArray((const char *)node_tree_model_->binData(node_)
      ->rawData(), static_cast<int>(node_tree_model_->binData(node_)
      ->size())));
  panel->setWindowTitle(cur_file_path_);
  panel->setAttribute(Qt::WA_DeleteOnClose);

  QString name("_");
  client::Node* root = node_tree_model_->nodeTree()->node(node_);
  if (root) {
      root->getQStringAttr("name", name);
  }
  auto dock_widget = main_window_->addTab(panel, name);
  connect(dock_widget, &DockWidget::visibilityChanged,
      panel, &visualization::VisualizationPanel::visibilityChanged);
}

void HexEditWidget::showHexEditor() {
  QSharedPointer<QItemSelectionModel> new_selection_model(
        new QItemSelectionModel(node_tree_model_.data()));
  NodeWidget *node_edit = new NodeWidget(main_window_, node_,
      node_tree_model_, new_selection_model);
  QString name("_");
  client::Node* root = node_tree_model_->nodeTree()->node(node_);
  if (root) {
      root->getQStringAttr("name", name);
  }
  auto sibling = DockWidget::getParentDockWidget(this);
  auto dock_widget = main_window_->addTab(node_edit,
      name, sibling);
  if (sibling == nullptr) {
    main_window_->addDockWidget(Qt::RightDockWidgetArea, dock_widget);
  }
}

void HexEditWidget::newBinData() {
  visualization_act_->setEnabled(node_tree_model_->binData(node_)
      ->size() > 0);
}

void HexEditWidget::enableFindNext(bool enable) {
  find_next_act_->setEnabled(enable);
}

void HexEditWidget::selectionChanged(qint64 start_addr,
    qint64 selection_size) {
  selection_label_->setText(
      QString("%1:%2 (%3 bytes)")
      .arg(addressAsText(start_addr))
      .arg(addressAsText(start_addr + selection_size))
      .arg(QString::number(selection_size, 10).rightJustified(8, '0')));
}

void HexEditWidget::nodeTreeVisibilityChanged(bool visibility) {
  show_node_tree_act_->setChecked(visibility);
}

void HexEditWidget::minimapVisibilityChanged(bool visibility) {
  show_minimap_act_->setChecked(visibility);
}

}  // namespace ui
}  // namespace veles
