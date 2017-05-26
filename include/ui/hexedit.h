/*
 * Copyright 2016 CodiLime
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

#include <QAbstractScrollArea>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMouseEvent>
#include <QStringList>

#include "ui/createchunkdialog.h"
#include "ui/fileblobmodel.h"
#include "ui/gotoaddressdialog.h"
#include "util/encoders/hex_encoder.h"
#include "util/encoders/text_encoder.h"
#include "client/models.h"

namespace veles {
namespace ui {

class HexEdit : public QAbstractScrollArea {
  Q_OBJECT
 public:
  HexEdit(
      data::NodeID node_id,
      QSharedPointer<client::NodeTreeModel> data_model,
      QSharedPointer<QItemSelectionModel> chunk_selection_model,
      QWidget *parent = 0);
  /** Mark bytes as selected and optionally scroll screen to make these
   *  bytes visible */
  void setSelection(qint64 start, qint64 size, bool set_visible = false);
  /** Sets how many bytes should be displayed in the single hex row or
   *  optionaly turn on automatic mode which will adjust bytes per row to
   *  window size */
  void setBytesPerRow(int bytesCount, bool automatic = false);
  /** Scroll screen to make byte visible */
  void scrollToByte(qint64 bytePos, bool doNothingIfVisable = false);
  void scrollRows(qint64 num_rows);
  QSharedPointer<client::NodeTreeModel> dataModel() { return data_model_;};
  data::NodeID nodeID() {return node_id_;}
  void setParserIds(QStringList ids);
  void processMoveEvent(QKeyEvent *event);
  void processSelectionChangeEvent(QKeyEvent *event);

public slots:
  void newBinData();
  void dataChanged();
  void modelSelectionChanged();

 protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  bool focusNextPrevChild(bool next) override;

 signals:
  void selectionChanged(qint64 start_addr, qint64 selection_size);

 private:
  data::NodeID node_id_;
  QSharedPointer<client::NodeTreeModel> data_model_;
  QSharedPointer<QItemSelectionModel> chunk_selection_model_;

  /** Total number of bytes in the blob */
  qint64 dataBytesCount_;
  /** Number of bytes displayed in singled hex edit row */
  qint64 bytesPerRow_;
  /** Indicates if bytes per row should be automatically adjusted to window width */
  bool autoBytesPerRow_;

  /** Byte offset of whole blob */
  qint64 startOffset_;
  /** Total number of rows in hex edit (counting last address only row) */
  qint64 rowsCount_;
  /** Number of rows displayed on the screen )calculated from window and font height) */
  qint64 rowsOnScreen_;
  /** Number of hex chars used to display one byte */
  qint64 byteCharsCount_;
  /** Number of pixels between two bytes in hex view (calculated from char width) */
  qint64 spaceAfterByte_;
  /** Width of single character in pixels */
  qint64 charWidht_;
  /** Height of single character in pixels */
  qint64 charHeight_;
  /** Number of pixels between two bytes (vertically) in hex view (calculated from char height)  */
  qint64 verticalByteBorderMargin_;
  /** Number of bytes (8 bit) used to represent offset addr */
  qint64 addressBytes_;
  /** Width in pixels of address area */
  qint64 addressWidth_;
  /** Width in pixles of hex area */
  qint64 hexAreaWidth_;
  /** Width in pixels of ascii area */
  qint64 asciiWidth_;
  /** Height in pixels of area separator */
  qint64 lineWidth_;

  /** Number of first row displayed on the screen */
  qint64 startRow_;
  /** Number of first pixel from left which should be displayed on the screen */
  qint64 startPosX_;

  enum class WindowArea {
    ADDRESS,
    HEX,
    ASCII,
    OUTSIDE,
  };

  /** Number of byte where selection starts (counting from beginning of blob) */
  qint64 current_position_;
  /** Number of bytes in selection */
  qint64 selection_size_;

  WindowArea current_area_;
  qint64 cursor_pos_in_byte_;
  bool cursor_visible_;

  CreateChunkDialog *createChunkDialog_;
  GoToAddressDialog *goToAddressDialog_;

  QAction *createChunkAction_;
  QAction *createChildChunkAction_;
  QAction *removeChunkAction_;
  QAction *goToAddressAction_;
  QAction *saveSelectionAction_;
  QStringList parsers_ids_;
  QMenu menu_;
  QMenu parsers_menu_;
  QTimer cursor_timer_;
  QScopedPointer<util::encoders::HexEncoder> hexEncoder_;
  QScopedPointer<util::encoders::TextEncoder> textEncoder_;

  void recalculateValues();
  void initParseMenu();
  void adjustBytesPerRowToWindowSize();
  QRect bytePosToRect(qint64 pos, bool ascii = false);
  qint64 pointToRowNum(QPoint pos);
  qint64 pointToColumnNum(QPoint pos);
  qint64 pointToBytePos(QPoint pos);
  void flipCursorVisibility();
  WindowArea pointToWindowArea(QPoint pos);
  QString addressAsText(qint64 pos);
  QString hexRepresentationFromBytePos(qint64 pos);
  QString asciiRepresentationFromBytePos(qint64 pos);

  qint64 byteValue(qint64 pos);
  QColor byteTextColorFromPos(qint64 pos);
  QColor byteBackroundColorFromPos(qint64 pos);

  qint64 selectionStart();
  qint64 selectionEnd();
  qint64 selectionSize();

  QModelIndex selectedChunk();

  void getRangeFromIndex(QModelIndex index, qint64 *begin, qint64 *size);
  void drawBorder(qint64 start, qint64 size, bool asciiArea = false,
                  bool doted = false);

  void setSelectedChunk(QModelIndex newSelectedChunk);
  bool isRangeVisible(qint64 start, qint64 size);
  bool isByteVisible(qint64 bytePos);
  void setSelectionEnd(qint64 bytePos);
  void saveSelectionToFile(QString path);
  void scrollToCurrentChunk();
  void parse(QAction *action);
  void resetCursor();

 private slots:
  void copyToClipboard(util::encoders::IEncoder *enc = nullptr);
};

}  // namespace ui
}  // namespace veles
