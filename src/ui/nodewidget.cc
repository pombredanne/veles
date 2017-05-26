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

#include "dbif/info.h"
#include "dbif/types.h"
#include "dbif/universe.h"

#include "ui/hexeditwidget.h"
#include "ui/nodetreewidget.h"
#include "ui/nodewidget.h"
#include "ui/veles_mainwindow.h"

#include "util/settings/hexedit.h"
#include "util/icons.h"

#include "visualization/panel.h"

namespace veles {
namespace ui {

/*****************************************************************************/
/* Public methods */
/*****************************************************************************/

NodeWidget::NodeWidget(MainWindowWithDetachableDockWidgets *main_window,
    data::NodeID node,
    QSharedPointer<client::NodeTreeModel> node_tree_model,
    QSharedPointer<QItemSelectionModel>& selection_model)
    : View("Hex editor", ":/images/show_hex_edit.png"),
      main_window_(main_window), minimap_(nullptr),
      minimap_dock_(nullptr), node_(node),
      node_tree_model_(node_tree_model),
      selection_model_(selection_model),
      sampler_(nullptr) {
  hex_edit_widget_ = new HexEditWidget(main_window,
      node, node_tree_model, selection_model);
  addAction(hex_edit_widget_->findAction());
  addAction(hex_edit_widget_->findNextAction());
  addAction(hex_edit_widget_->showVisualizationAction());
  addAction(hex_edit_widget_->showHexEditAction());
  addAction(hex_edit_widget_->showNodeTreeAction());
  setCentralWidget(hex_edit_widget_);

  node_tree_dock_ = new QDockWidget;
  new DockWidgetVisibilityGuard(node_tree_dock_);
  node_tree_dock_->setWindowTitle("Node tree");
  node_tree_widget_ = new NodeTreeWidget(main_window,
      node, node_tree_model, selection_model);
  node_tree_dock_->setWidget(node_tree_widget_);
  node_tree_dock_->setContextMenuPolicy(Qt::PreventContextMenu);
  node_tree_dock_->setAllowedAreas(
      Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setDockNestingEnabled(true);
  addDockWidget(Qt::LeftDockWidgetArea, node_tree_dock_);

#if 0 // We do not use minimap for NodeWidget yet,
  minimap_dock_ = new QDockWidget;
  new DockWidgetVisibilityGuard(minimap_dock_);
  minimap_dock_->setWindowTitle("Minimap");
  minimap_ = new visualization::MinimapPanel(this);

  if(data_model_->binData().size() > 0) {
    loadBinDataToMinimap();
  } else {
    sampler_data_ = QByteArray("");
    sampler_ = new util::UniformSampler(sampler_data_);
    sampler_->setSampleSize(4096 * 1024);
    minimap_->setSampler(sampler_);
  }

  minimap_dock_->setWidget(minimap_);
  minimap_dock_->setContextMenuPolicy(Qt::PreventContextMenu);
  minimap_dock_->setAllowedAreas(
      Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  addDockWidget(Qt::LeftDockWidgetArea, minimap_dock_);
  MainWindowWithDetachableDockWidgets::splitDockWidget2(this, node_tree_dock_,
      minimap_dock_, Qt::Horizontal);
#endif

  connect(hex_edit_widget_, &HexEditWidget::showNodeTree,
      node_tree_dock_, &QDockWidget::setVisible);
  connect(node_tree_dock_, &QDockWidget::visibilityChanged,
        hex_edit_widget_, &HexEditWidget::nodeTreeVisibilityChanged);

#if 0 // We do not use minimap for NodeWidget yet,
  connect(hex_edit_widget_, &HexEditWidget::showMinimap,
        minimap_dock_, &QDockWidget::setVisible);
  connect(data_model_.data(), &FileBlobModel::newBinData,
      this, &NodeWidget::loadBinDataToMinimap);
  connect(minimap_dock_, &QDockWidget::visibilityChanged,
      hex_edit_widget_, &HexEditWidget::minimapVisibilityChanged);
#endif
}

NodeWidget::~NodeWidget() {
  delete sampler_;
}

void NodeWidget::loadBinDataToMinimap() {
  delete sampler_;

  sampler_data_ = QByteArray((const char *)node_tree_model_->binData(node_)->
      rawData(), static_cast<int>(node_tree_model_->binData(node_)->octets()));
  sampler_ = new util::UniformSampler(sampler_data_);
  sampler_->setSampleSize(4096 * 1024);
  minimap_->setSampler(sampler_);
}

}  // namespace ui
}  // namespace veles
