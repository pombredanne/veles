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
#include <memory>
#include <string>

#include <QHostAddress>

#include "proto/exceptions.h"
#include "network/msgpackwrapper.h"
#include "network/msgpackobject.h"

#include "client/node.h"
#include "client/nodetree.h"
#include "client/networkclient.h"

namespace veles {
namespace client {

/*****************************************************************************/
/* NetworkClient */
/*****************************************************************************/

QString NetworkClient::connStatusStr(ConnectionStatus status) {
  switch (status) {
  case ConnectionStatus::Connected:
    return QString("Connected");
    break;
  case ConnectionStatus::Connecting:
    return QString("Connecting");
    break;
  case ConnectionStatus::NotConnected:
    return QString("Not Connected");
    break;
  default:
    return QString("Unknown status");
    break;
  }
}

NetworkClient::NetworkClient(QObject* parent) : QObject(parent),
    client_socket_(nullptr),
    status_(ConnectionStatus::NotConnected),
    server_name_("127.0.0.1"), server_port_(3135),
    client_interface_name_("127.0.0.1"),
    protocol_version_(1), client_name_(""),
    client_version_("[unspecified version]"), client_description_(""),
    client_type_(""), authentication_key_(""), quit_on_close_(false),
    output_stream_(nullptr), qid_(0) {
  registerMessageHandlers();
  // FIXME
  //node_tree_ = QSharedPointer<NodeTree>::create(this, this);
  node_tree_ = new NodeTree(this, this);
}

NetworkClient::~NetworkClient() {
}

NetworkClient::ConnectionStatus NetworkClient::connectionStatus() {
  return status_;
}

void NetworkClient::connect(
    QString server_name,
    int server_port,
    QString client_interface_name,
    QString client_name,
    QString client_version,
    QString client_description,
    QString client_type,
    const QByteArray &authentication_key,
    bool quit_on_close) {
  server_name_ = server_name;
  server_port_ = server_port;
  client_interface_name_ = client_interface_name;
  client_name_ = client_name;
  client_version_ = client_version;
  client_description_ = client_description;
  client_type_ = client_type;
  quit_on_close_ = quit_on_close;

  authentication_key_ = authentication_key;
  const int target_size = 64;
  int key_size = authentication_key_.size();
  authentication_key_.resize(target_size);
  for(int i = key_size; i < target_size; ++i) {
    authentication_key_[i] = 0;
  }

  if (status_ != ConnectionStatus::Connected
      && status_ != ConnectionStatus::Connecting) {
    client_socket_ = new QTcpSocket(this);
    QObject::connect(client_socket_, &QAbstractSocket::connected,
        this, &NetworkClient::socketConnected, Qt::QueuedConnection);
    QObject::connect(client_socket_, &QAbstractSocket::disconnected,
        this, &NetworkClient::socketDisconnected, Qt::QueuedConnection);
    QObject::connect(client_socket_, &QIODevice::readyRead,
        this, &NetworkClient::newDataAvailable);
    QObject::connect(client_socket_,
        static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(
        &QAbstractSocket::error), this, &NetworkClient::socketError);

    if (output()) {
    *output() << "NetworkClient::connect" << endl
        << "    client interface: " << client_interface_name_ << endl
        << "    server host: " << server_name_ << endl
        << "    server port: " << server_port_ << endl;
    }

    if (1) {
    // TODO: Why is bind causing a problem here?
    // if (client_socket_->bind(QHostAddress(client_interface_name))) {
      if (output()) {
        *output() << "NetworkClient: bind successful." << endl;
      }
      client_socket_->connectToHost(server_name_, server_port_);
      setConnectionStatus(ConnectionStatus::Connecting);
    } else {
      if (output()) {
        *output() << "NetworkClient: bind failed." << endl;
      }
    }
  }
}

void NetworkClient::disconnect() {
  if (output()) {
    *output() << "NetworkClient: Disconnect." << endl;
  }

  setConnectionStatus(ConnectionStatus::NotConnected);

  if(client_socket_) {
    client_socket_->disconnectFromHost();
  }
}

//QSharedPointer<NodeTree> const& NetworkClient::nodeTree() {
NodeTree* NetworkClient::nodeTree() {
  // FIXME
  return node_tree_;
}

uint64_t NetworkClient::nextQid() {
  return ++qid_;
}

unsigned int NetworkClient::protocolVersion() {
  return protocol_version_;
}

QString NetworkClient::clientName() {
  return client_name_;
}

QString NetworkClient::clientVersion() {
  return client_version_;
}

QString NetworkClient::clientDescription() {
  return client_description_;
}

QString NetworkClient::clientType() {
  return client_type_;
}

QString NetworkClient::authenticationKey() {
  return authentication_key_;
}

QTextStream* NetworkClient::output() {
  return output_stream_;
}

void NetworkClient::setOutput(QTextStream* stream) {
  output_stream_ = stream;
}

void NetworkClient::sendMsgConnect() {
  std::shared_ptr<std::string> client_name_ptr(
      new std::string(client_name_.toStdString()));
  std::shared_ptr<std::string> client_version_ptr(
      new std::string(client_version_.toStdString()));
  std::shared_ptr<std::string> client_description_ptr(
      new std::string(client_description_.toStdString()));
  std::shared_ptr<std::string> client_type_ptr(
      new std::string(client_type_.toStdString()));

  msg_ptr msg(new proto::MsgConnect(
      1,
      pair_str(true, client_name_ptr),
      pair_str(true, client_version_ptr),
      pair_str(true, client_description_ptr),
      pair_str(true, client_type_ptr),
      quit_on_close_
      ));

  sendMessage(msg);
}

void NetworkClient::registerMessageHandlers() {
  message_handlers_["subscription_cancelled"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["get_reply"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["connected"]
      = &NetworkClient::handleConnectedMessage;
  message_handlers_["proto_error"]
      = &NetworkClient::handleProtoErrorMessage;
  message_handlers_["connections_reply"]
      = &NetworkClient::handleConnectionsMessage;
  message_handlers_["registry_reply"]
      = &NetworkClient::handleRegistryReplyMessage;
  message_handlers_["method_result"]
      = &NetworkClient::handleMthdResMessage;
  message_handlers_["method_error"]
      = &NetworkClient::handleMthdResMessage;
  message_handlers_["broadcast_result"]
      = &NetworkClient::handleMthdResMessage;
  message_handlers_["plugin_trigger_run"]
      = &NetworkClient::handlePluginTriggerRunMessage;
  message_handlers_["request_error"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["get_list_reply"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["get_data_reply"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["get_query_reply"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["query_error"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["request_ack"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["get_bindata_reply"]
      = &NetworkClient::handleNodeTreeRelatedMessage;
  message_handlers_["connection_error"]
      = &NetworkClient::handleConnErrorMessage;
  message_handlers_["plugin_method_run"]
      = &NetworkClient::handlePluginMethodRunMessage;
  message_handlers_["plugin_query_get"]
      = &NetworkClient::handlePluginQueryGetMessage;
  message_handlers_["plugin_broadcast_run"]
      = &NetworkClient::handleBroadcastRunMessage;
  message_handlers_["plugin_handler_unregistered"]
      = &NetworkClient::handlePluginHandlerUnregisteredMessage;
}

void NetworkClient::handleNodeTreeRelatedMessage(msg_ptr msg) {
}

void NetworkClient::handleConnectedMessage(msg_ptr msg) {
  if (connectionStatus() != ConnectionStatus::Connecting) {
    if (output()) {
      *output() << "NetworkClient: Very confusing... "
        "Received \"connected\" message while already connected." << endl;
    }
  } else {
    if (output()) {
        *output() << "NetworkClient: Received \"connected\" message." << endl;
    }

    setConnectionStatus(ConnectionStatus::Connected);

    // FIXME
    Node* root = node_tree_->node(*data::NodeID::getRootNodeId());
    if (root) {
      root->getList(true);
    }
  }
}

void NetworkClient::handleProtoErrorMessage(msg_ptr msg) {
  proto::MsgProtoError* mpe
      = dynamic_cast<proto::MsgProtoError*>(msg.get());
  if (mpe) {
    if (output()) {
      *output()
          << "Received protocol error message. Aborting connection..." << endl
          << "    code: " << mpe->err->code.c_str()
          << "  msg: " << mpe->err->msg.c_str() << endl;
    }

    disconnect();
  }
}

void NetworkClient::handleConnectionsMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }

  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::handleRegistryReplyMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }

  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::handleMthdResMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }
  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::handlePluginTriggerRunMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }

  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::handleConnErrorMessage(msg_ptr msg) {
  proto::MsgConnectionError* cem
      = dynamic_cast<proto::MsgConnectionError*>(msg.get());
  if (cem) {
    if (output()) {
      *output()
          << "Received connection error message. Aborting connection..."
          << endl << "    code: " << cem->err->code.c_str()
          << "  msg: " << cem->err->msg.c_str() << endl;
    }

    disconnect();
  }
}

void NetworkClient::handlePluginMethodRunMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }

  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::handlePluginQueryGetMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }

  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::handleBroadcastRunMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }

  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::handlePluginHandlerUnregisteredMessage(msg_ptr msg) {
  if (output()) {
    *output() << "NetworkClient: Received \"" << QString::fromStdString(msg->object_type)
        << "\" message." << endl;
  }

  // TODO - is this something that client should implement in a subclass?
}

void NetworkClient::sendMessage(msg_ptr msg) {
  if (client_socket_ && client_socket_->isValid()) {
    msgpack::sbuffer buf;
    msgpack::packer<msgpack::sbuffer> packer(buf);
    messages::MsgpackWrapper::dumpObject(packer, msg);
    client_socket_->write(buf.data(), buf.size());
  }
}

void NetworkClient::setConnectionStatus(ConnectionStatus connection_status) {
  if (status_ != connection_status) {
    status_ = connection_status;
    if (output()) {
      *output() << "NetworkClient: New connection status: "
          << connStatusStr(connection_status) << "." << endl;
    }
    emit connectionStatusChanged(status_);
  }
}

void NetworkClient::socketConnected() {
  if (output()) {
    *output() << "NetworkClient: TCP socket connected - sending an "
        "authentication key and \"connect\" message." << endl;
  }

  client_socket_->write(authentication_key_);
  sendMsgConnect();
  node_tree_->reset();
}

void NetworkClient::socketDisconnected() {
  setConnectionStatus(ConnectionStatus::NotConnected);
  if(output()) {
    *output() << "NetworkClient: TCP socket disconnected." << endl;
  }

  if (client_socket_) {
    client_socket_->deleteLater();
    client_socket_ = nullptr;
  }
}

void NetworkClient::newDataAvailable() {
  while (client_socket_) {
    msg_ptr msg = nullptr;
    try {
      msg = msgpack_wrapper_.loadMessage(client_socket_);
    } catch (proto::SchemaError& schema_error) {
      if (output()) {
        *output() << "NetworkClient: SchemaError - "
            << QString::fromStdString(schema_error.msg) << endl;
      }
    }

    if (msg) {
      auto handler_iter = message_handlers_.find(msg->object_type);
      if(handler_iter != message_handlers_.end()) {
        MessageHandler handler = handler_iter->second;
        (this->*handler)(msg);
      } else {
        if (output()) {
          *output() << "NetworkClient: Received message of not handled "
              "type: \"" << msg->object_type.c_str() << "\"." << endl;
        }
      }
      emit messageReceived(msg);
    } else {
      break;
    }
  }
}

void NetworkClient::socketError(QAbstractSocket::SocketError socketError) {
  setConnectionStatus(ConnectionStatus::NotConnected);
  if (output() && client_socket_) {
    *output() << "NetworkClient: Socket error - "
        << client_socket_->errorString() << endl;
  }
}

/*****************************************************************************/
/* RequestPromise */
/*****************************************************************************/

RequestPromise::RequestPromise(
    NetworkClient* network_client, uint64_t qid, QObject* parent)
    : QObject(parent), qid_(qid) {
  connect(network_client, &NetworkClient::messageReceived,
      this, &RequestPromise::messageReceived);
}

void RequestPromise::messageReceived(msg_ptr message) {
  if (message->object_type == "request_ack") {
    auto reply = std::dynamic_pointer_cast<proto::MsgRequestAck>(message);
    if(reply && reply->rid == qid_) {
      emit done(qid_);
    }
  } else if (message->object_type == "query_error") {
    auto reply = std::dynamic_pointer_cast<proto::MsgQueryError>(message);
    if(reply && reply->qid == qid_) {
      emit failed(qid_);
    }
  } else if (message->object_type == "get_reply") {
    auto reply = std::dynamic_pointer_cast<proto::MsgGetReply>(message);
    if(reply && reply->qid == qid_) {
      emit done(qid_);
    }
  } else if (message->object_type == "get_list_reply") {
    auto reply = std::dynamic_pointer_cast<proto::MsgGetListReply>(message);
    if(reply && reply->qid == qid_) {
      emit done(qid_);
    }
  } else if (message->object_type == "get_data_reply") {
    auto reply = std::dynamic_pointer_cast<proto::MsgGetDataReply>(message);
    if(reply && reply->qid == qid_) {
      emit done(qid_);
    }
  } else if (message->object_type == "get_bindata_reply") {
    auto reply = std::dynamic_pointer_cast<proto::MsgGetBinDataReply>(message);
    if(reply && reply->qid == qid_) {
      emit done(qid_);
    }
  }
}

} // client
} // veles
