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

#include <functional>
#include <cstring>
#include <list>

#include <QFont>

#include "client/models.h"
#include "client/node.h"
#include "client/nodetree.h"
#include "client/networkclient.h"
#include "util/settings/theme.h"

namespace veles {
namespace client {

QString zeroPaddedHexNumber(uint64_t number) {
  auto num = QString::number(number, 16);
  while (num.length() < 4) {
    num = "0" + num;
  }
  return num;
}

/*****************************************************************************/
/* NodeTreeModelBase */
/*****************************************************************************/

NodeTreeModelBase::NodeTreeModelBase(NodeTree* node_tree, data::NodeID root,
    QObject* parent)
    : QAbstractItemModel(parent), node_tree_(node_tree), root_(root) {
  connect(node_tree, &NodeTree::startNodeDataModification,
      this, &NodeTreeModelBase::startNodeDataModificationSlot,
      Qt::UniqueConnection);
  connect(node_tree, &NodeTree::endNodeDataModification,
      this, &NodeTreeModelBase::endNodeDataModificationSlot,
      Qt::UniqueConnection);
  connect(node_tree, &NodeTree::startChildrenModification,
      this, &NodeTreeModelBase::startChildrenModificationSlot,
      Qt::UniqueConnection);
  connect(node_tree, &NodeTree::endChildrenModification,
      this, &NodeTreeModelBase::endChildrenModificationSlot,
      Qt::UniqueConnection);
}

NodeTreeModelBase::~NodeTreeModelBase() {
}

bool NodeTreeModelBase::hasChildren(const QModelIndex &parent) const {
  if (parent.isValid()) {
    Node* node = nodeFromIndex(parent);
    if (node && !node->childrenVect().empty()) {
      return true;
    }
  }

  return false;
}

QModelIndex NodeTreeModelBase::index(int row, int column,
    const QModelIndex &parent) const {
  Node* node = nodeFromIndex(parent);

  if (node) {
    if (node->childrenVect().size() > (unsigned)row) {
      return createIndex(row, column, node->childrenVect()[row]);
    }
  }
  return QModelIndex();
}

QModelIndex NodeTreeModelBase::parent(const QModelIndex &index) const {
  if (index.isValid()) {
    Node* node = parentNodeFromIndex(index);
    if (node) {
      return createIndex(node->index(), 0, node);
    }
  }

  return QModelIndex();
}

int NodeTreeModelBase::rowCount(const QModelIndex &parent) const {
  Node* node = nodeFromIndex(parent);

  if (node) {
    return node->childrenVect().size();
  }

  return 0;
}

QModelIndex NodeTreeModelBase::indexFromId(data::NodeID id) const {
  Node* node = node_tree_->node(id);
  if (node) {
    return createIndex(node->index(), 0, node_tree_->node(id));
  } else {
    return QModelIndex();
  }
}

data::NodeID NodeTreeModelBase::idFromIndex(const QModelIndex &index) const {
  Node* node = nodeFromIndex(index);
  if (node) {
    return node->id();
  } else {
    return *data::NodeID::getNilId();
  }
}

void NodeTreeModelBase::startNodeDataModificationSlot(QString id) {
  beginResetModel();
}

void NodeTreeModelBase::endNodeDataModificationSlot(QString id) {
  endResetModel();
}

void NodeTreeModelBase::startChildrenModificationSlot(QString id) {
  beginResetModel();
}

void NodeTreeModelBase::endChildrenModificationSlot(QString id) {
  endResetModel();
}

Node* NodeTreeModelBase::parentNodeFromIndex(const QModelIndex &index) const {
  Node* node = nodeFromIndex(index);
  return node ? node->parent() : nullptr;
}

Node* NodeTreeModelBase::nodeFromIndex(const QModelIndex &index) const {
  if (index.isValid()) {
    return reinterpret_cast<Node*>(index.internalPointer());
  } else {
    return nullptr;
  }
}

/*****************************************************************************/
/* NodeTreeModel */
/*****************************************************************************/

NodeTreeModel::NodeTreeModel(NodeTree* node_tree, data::NodeID root,
    QObject* parent)
    : NodeTreeModelBase(node_tree, root, parent) {
}

NodeTreeModel::~NodeTreeModel() {
}

int NodeTreeModel::columnCount(const QModelIndex &parent) const {
  return 4;
}

QVariant NodeTreeModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::SizeHintRole) {
    return QSize(50, 20);
  }

  auto node = nodeFromIndex(index);

  if (role == Qt::DecorationRole) {
    if (index.isValid() && index.column() == COLUMN_INDEX_MAIN) {
      if (node != nullptr && !icon(index).isNull()) {
        return icon(index);
      } else {
        return color(index.row());
      }
    }
  }

  if (node == nullptr) return QVariant();
  QString name("[no name]");
  QString comment("");
  node->getQStringAttr("name", name);
  node->getQStringAttr("comment", comment);

  if (index.column() == COLUMN_INDEX_COMMENT &&
      (role == Qt::DisplayRole || role == Qt::EditRole)) {
    return comment;
  }

  if (index.column() == COLUMN_INDEX_POS) {
    return positionColumnData(node, role);
  }

  if (index.column() == COLUMN_INDEX_VALUE) {
    return valueColumnData(node, role);
  }

  if (index.column() != COLUMN_INDEX_MAIN) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    return name;
  }
  return QVariant();
}

QVariant NodeTreeModel::headerData(int section, Qt::Orientation orientation,
    int role) const {
  if (orientation != Qt::Orientation::Horizontal) {
    return QVariant();
  }

  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (section == COLUMN_INDEX_MAIN) {
    return "Name";
  }

  if (section == COLUMN_INDEX_VALUE) {
    return "Value";
  }

  if (section == COLUMN_INDEX_COMMENT) {
    return "Comment";
  }

  if (section == COLUMN_INDEX_POS) {
    return "Position";
  }

  return QVariant();
}

void NodeTreeModel::addChunk(QString name, QString type, QString comment,
    int64_t start, int64_t end, const QModelIndex& index) {
  Node* parent_node = nodeFromIndex(index);
  if (parent_node) {
    node_tree_->addChunk(parent_node->id(), name, type, comment, start, end);
  }
}

void NodeTreeModel::parse(data::NodeID root, QString parser, qint64 offset,
    const QModelIndex& parent) {
  // TODO
  //dbif::ObjectHandle parent_chunk;
  //if (parent.isValid()) {
  //  parent_chunk = itemFromIndex(parent)->objectHandle();
  //}
  //fileBlob_->asyncRunMethod<dbif::BlobParseRequest>(this, parser, offset,
  //                                                  parent_chunk);
}

std::shared_ptr<data::BinData> NodeTreeModel::binData(data::NodeID id) {
  Node* node = node_tree_->node(id);
  if (node) {
    auto bin_data = node->binData("data");
    if (bin_data) {
      return bin_data;
    }
  }

  // FIXME
  return std::make_shared<data::BinData>(8, 11, (const uint8_t*)"ala ma kota");
}

QModelIndex NodeTreeModel::indexFromPos(int64_t pos,
    const QModelIndex &parent) {
  auto node = nodeFromIndex(parent);

  if (node == nullptr) {
    return QModelIndex();
  }

  for (unsigned child_index = 0;
      child_index < node->childrenVect().size();
      child_index++) {
    auto child = node->childrenVect()[child_index];
    int64_t begin = child->start();
    int64_t end = child->end();
    if (pos >= begin && pos < end) {
      return indexFromId(child->id());
    }
  }

  return QModelIndex();
}

bool NodeTreeModel::isRemovable(const QModelIndex &index) {
  return true;
}

QIcon NodeTreeModel::icon(QModelIndex index) const {
  return QIcon();
}

QColor NodeTreeModel::color(int colorIndex) const {
  return util::settings::theme::chunkBackground(colorIndex);
}

QVariant NodeTreeModel::positionColumnData(Node* node, int role) const {
  if (role == Qt::DisplayRole) {
    return zeroPaddedHexNumber(node->start()) + ":"
        + zeroPaddedHexNumber(node->end());
  } else if (role == Qt::FontRole) {
#ifdef Q_OS_WIN32
    return QFont("Courier", 10);
#else
    return QFont("Monospace", 10);
#endif
  } else if (role == ROLE_BEGIN) {
    return QString::number(node->start());
  } else if (role == ROLE_END) {
    return QString::number(node->end());
  }
  return QVariant();
}

QVariant NodeTreeModel::valueColumnData(Node* node, int role) const {
  if (role == Qt::DisplayRole) {
    // TODO chunk data item's value
    return QString("");
  }

  return QVariant();
}

/*****************************************************************************/
/* TopLevelResourcesModel */
/*****************************************************************************/

TopLevelResourcesModel::TopLevelResourcesModel(NodeTree* node_tree,
    data::NodeID root, QObject* parent)
    : NodeTreeModelBase(node_tree, root, parent) {
}

TopLevelResourcesModel::~TopLevelResourcesModel() {
}

int TopLevelResourcesModel::columnCount(const QModelIndex &parent) const {
  return 2;
}

QVariant TopLevelResourcesModel::data(const QModelIndex& index, int role) const {
  auto node = nodeFromIndex(index);

  if (node == nullptr || role != Qt::DisplayRole) {
    return QVariant();
  }

  if (index.column() == 0) {
    QString path("[no path available]");
    node->getQStringAttr("path", path);
    return QVariant(path);
  } else if (index.column() == 1) {
    return QVariant(node->id().toHexString());
  }

  return QVariant();
}

bool TopLevelResourcesModel::hasChildren(const QModelIndex &parent) const {
  if (parent.isValid()) {
    Node* node = nodeFromIndex(parent);
    if (node
        && node->id() == *data::NodeID::getRootNodeId()
        && !node->childrenVect().empty()) {
      return true;
    }
  }

  return false;
}

QVariant TopLevelResourcesModel::headerData(int section,
    Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Orientation::Horizontal
      || role != Qt::DisplayRole) {
    return QVariant();
  }

  if (section == 0) {
    return QString("Path");
  } else if (section == 1) {
    return QString("ID");
  }

  return QVariant();
}

int TopLevelResourcesModel::rowCount(const QModelIndex &parent) const {
  Node* node = nodeFromIndex(parent);

  if (node && node->id() == *data::NodeID::getRootNodeId()) {
    return node->childrenVect().size();
  }

  return 0;
}

} // namespace client
} // namespace veles
