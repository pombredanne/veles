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

#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QLabel>
#include <QPixmap>
#include <QHBoxLayout>
#include <QScrollArea>

#include "client/networkclient.h"
#include "client/models.h"
#include "ui/connectiondialog.h"
#include "ui_connectionnotificationwidget.h"
#include "network/msgpackwrapper.h"

namespace veles {
namespace ui {

/*****************************************************************************/
/* ConnectionManager */
/*****************************************************************************/

class ConnectionManager : public QObject {
  Q_OBJECT

 public:
  ConnectionManager(QWidget* parent = nullptr);
  virtual ~ConnectionManager();

  client::NetworkClient* networkClient();
  QAction* showConnectionDialogAction();
  QAction* disconnectAction();
  QAction* killLocallyCreatedServerAction();
  QSharedPointer<client::NodeTreeModel> nodeTreeModel();
  QSharedPointer<client::TopLevelResourcesModel> resourcesModel();

 signals:
  void connectionStatusChanged(
      client::NetworkClient::ConnectionStatus connection_status);
  void connectionsChanged(
      std::shared_ptr<std::vector<std::shared_ptr<proto::Connection>>>
      connections);

 public slots:
  void locallyCreatedServerStarted();
  void locallyCreatedServerFinished(int exit_code,
      QProcess::ExitStatus exit_status);
  void connectionDialogAccepted();
  void startClient();
  void startLocalServer();
  void killLocalServer();
  void disconnect();
  void serverProcessReadyRead();
  void serverProcessErrorOccurred(QProcess::ProcessError error);
  void updateConnectionStatus(
      client::NetworkClient::ConnectionStatus connection_status);
  void raiseConnectionDialog();
  void sendListConnectionsMessage();
  void messageReceived(client::msg_ptr message);

 private:
  QAction* show_connection_dialog_action_;
  QAction* disconnect_action_;
  QAction* kill_locally_created_server_action_;
  QProcess* server_process_;
  ConnectionDialog* connection_dialog_;
  bool is_local_server_;
  client::NetworkClient* network_client_;
  QTextStream* network_client_output_;
  QSharedPointer<client::NodeTreeModel> node_tree_model_;
  QSharedPointer<client::TopLevelResourcesModel> resources_model_;
};

/*****************************************************************************/
/* ConnectionNotificationWidget */
/*****************************************************************************/

class ConnectionNotificationWidget : public QWidget {
  Q_OBJECT

 public:
  ConnectionNotificationWidget(QWidget* parent = nullptr);
  virtual ~ConnectionNotificationWidget();

 public slots:
  void updateConnectionStatus(
      client::NetworkClient::ConnectionStatus connection_status);

 protected:
  void timerEvent(QTimerEvent* event);

 private:
  client::NetworkClient::ConnectionStatus connection_status_;

  int frame_;
  int last_status_change_;

  QPixmap icon_connected_;
  QPixmap icon_not_connected_;
  QPixmap icon_alarm_;

  Ui::ConnectionNotificationWidget* ui_;
};

/*****************************************************************************/
/* ConnectionsWidget */
/*****************************************************************************/

class ConnectionsWidget : public QWidget {
  Q_OBJECT

 public:
  ConnectionsWidget(QWidget* parent = nullptr);
  virtual ~ConnectionsWidget();

 public slots:
  void updateConnectionStatus(
      client::NetworkClient::ConnectionStatus connection_status);
  void updateConnections(
      std::shared_ptr<std::vector<std::shared_ptr<proto::Connection>>>
      connections);

 private:
  void clear();

  QPixmap users_icon_;
  QLabel* users_icon_label_;
  QHBoxLayout* layout_;
  QHBoxLayout* scroll_area_layout_;
  std::list<QLabel*> user_labels_;
  QString label_stylesheet_;
};

}  // namespace ui
}  // namespace veles
