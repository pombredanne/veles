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

#pragma once

#include <QObject>
#include <QIcon>
#include <QAbstractItemModel>

#include "data/nodeid.h"
#include "node.h"
#include "nodetree.h"

namespace veles {
namespace client {

typedef std::shared_ptr<proto::MsgpackMsg> msg_ptr;

/*****************************************************************************/
/* NodeTreeModelBase */
/*****************************************************************************/

class NodeTreeModelBase : public QAbstractItemModel {
  Q_OBJECT

 public:
  NodeTreeModelBase(NodeTree* node_tree, data::NodeID root,
      QObject* parent = nullptr);
  virtual ~NodeTreeModelBase();

  virtual bool hasChildren(const QModelIndex &parent = QModelIndex())
      const override;
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex &index) const override;
  virtual int rowCount(const QModelIndex &parent = QModelIndex())
      const override;

  QModelIndex indexFromId(data::NodeID id) const;
  data::NodeID idFromIndex(const QModelIndex &index) const;

 public slots:
  virtual void startNodeDataModificationSlot(QString id);
  virtual void endNodeDataModificationSlot(QString id);
  virtual void startChildrenModificationSlot(QString id);
  virtual void endChildrenModificationSlot(QString id);

  NodeTree* nodeTree() {return node_tree_;}

 protected:
  Node* parentNodeFromIndex(const QModelIndex &index) const;
  Node* nodeFromIndex(const QModelIndex &index) const;

  NodeTree* node_tree_;
  data::NodeID root_;
};

/*****************************************************************************/
/* NodeTreeModel */
/*****************************************************************************/

/*
 * TODO
 *
 * 1. Obsluzyc pobieranie i wysylanie BinData.
 * 2. Powiadamiac subskrybentow o zmianie danych.
 * 3. Obsluzyc removeRow.
 * 4. Pobierac poddrzewa wezlow tylko wtedy, kiedy jest to potrzebne.
 * 5. Wyswietlac ChunkDataItems w widokach.
 */

class NodeTreeModel : public NodeTreeModelBase {
  Q_OBJECT

 public:
  NodeTreeModel(NodeTree* node_tree, data::NodeID root,
      QObject* parent = nullptr);
  virtual ~NodeTreeModel();

  virtual int columnCount(const QModelIndex &parent = QModelIndex())
      const override;
  virtual QVariant data(const QModelIndex &index,
      int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
      int role = Qt::DisplayRole) const override;

  void addChunk(QString name, QString type, QString comment,
      int64_t start, int64_t end, const QModelIndex& index);
  void parse(data::NodeID root, QString parser = "", qint64 offset = 0,
      const QModelIndex& parent = QModelIndex());
  std::shared_ptr<data::BinData> binData(data::NodeID id);
  QModelIndex indexFromPos(int64_t pos,
      const QModelIndex &parent = QModelIndex());
  bool isRemovable(const QModelIndex &index = QModelIndex());

  static const Qt::ItemDataRole ROLE_BEGIN = Qt::UserRole;
  static const Qt::ItemDataRole ROLE_END = (Qt::ItemDataRole)(Qt::UserRole + 1);

  static const int COLUMN_INDEX_MAIN = 0;
  static const int COLUMN_INDEX_VALUE = 1;
  static const int COLUMN_INDEX_COMMENT = 2;
  static const int COLUMN_INDEX_POS = 3;

 private:
  QIcon icon(QModelIndex index) const;
  QColor color(int color_index) const;
  QVariant positionColumnData(Node* node, int role) const;
  QVariant valueColumnData(Node* node, int role) const;
};

/*****************************************************************************/
/* TopLevelResourcesModel */
/*****************************************************************************/

class TopLevelResourcesModel : public NodeTreeModelBase {
  Q_OBJECT

 public:
  TopLevelResourcesModel(NodeTree* node_tree, data::NodeID root,
      QObject* parent = nullptr);
  virtual ~TopLevelResourcesModel();

  virtual int columnCount(const QModelIndex &parent = QModelIndex())
      const override;
  virtual QVariant data(const QModelIndex &index,
      int role = Qt::DisplayRole) const override;
  virtual bool hasChildren(const QModelIndex &parent = QModelIndex())
      const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
      int role = Qt::DisplayRole) const override;
  virtual int rowCount(const QModelIndex &parent = QModelIndex())
      const override;
};

} // namespace client
} // namespace veles
