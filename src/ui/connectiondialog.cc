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
#include "ui/connectiondialog.h"
#include "ui_connectiondialog.h"
#include "util/settings/connection_client.h"

#include <climits>
#include <cstdint>
#include <ctime>
#include <limits>
#include <random>

#include <QtGlobal>
#include <QSettings>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace veles {
namespace ui {

ConnectionDialog::ConnectionDialog(QWidget* parent)
    : QDialog(parent),
      ui_(new Ui::ConnectionDialog) {
  ui_->setupUi(this);

  QRegularExpressionValidator* validator
      = new QRegularExpressionValidator(QRegularExpression(
      "^[0-9A-F]{0,128}$", QRegularExpression::CaseInsensitiveOption), this);
  ui_->key_line_edit->setValidator(validator);

  connect(ui_->ok_button, &QPushButton::clicked, this, &QDialog::accept);
  connect(ui_->cancel_button, &QPushButton::clicked, this, &QDialog::reject);
  connect(ui_->server_localhost_button, &QPushButton::clicked,
      this, &ConnectionDialog::serverLocalhost);
  connect(ui_->client_localhost_button, &QPushButton::clicked,
        this, &ConnectionDialog::clientLocalhost);
  connect(ui_->random_key_button, &QPushButton::clicked,
      this, &ConnectionDialog::randomKey);
  connect(ui_->new_server_radio_button, &QPushButton::toggled,
      this, &ConnectionDialog::newServerToggled);

  db_file_dialog_ = new QFileDialog(this);
  db_file_dialog_->setAcceptMode(QFileDialog::AcceptOpen);
  db_file_dialog_->setFileMode(QFileDialog::AnyFile);
  db_file_dialog_->setNameFilters({"All files (*.*)"});

  connect(ui_->select_database_button, &QPushButton::clicked,
        db_file_dialog_, &QFileDialog::show);
  connect(db_file_dialog_, &QFileDialog::fileSelected,
      this, &ConnectionDialog::databaseFileSelected);

  server_file_dialog_ = new QFileDialog(this);
  server_file_dialog_->setAcceptMode(QFileDialog::AcceptOpen);
  server_file_dialog_->setFileMode(QFileDialog::ExistingFile);
  server_file_dialog_->setNameFilters({"Python scripts (*.py)"});

  connect(ui_->select_server_executable_button, &QPushButton::clicked,
      server_file_dialog_, &QFileDialog::show);
  connect(server_file_dialog_, &QFileDialog::fileSelected,
      this, &ConnectionDialog::serverFileSelected);
  connect(ui_->load_defaults_button, &QPushButton::clicked,
      this, &ConnectionDialog::loadDefaultValues);
  connect(this, &QDialog::accepted, this, &ConnectionDialog::dialogAccepted);
  connect(ui_->profile, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConnectionDialog::profileChanged);
  connect(ui_->profile, &QComboBox::editTextChanged, this, &ConnectionDialog::profileNameEdited);
  connect(ui_->remove_profile_button, &QPushButton::clicked, this, &ConnectionDialog::profileRemoved);
  connect(ui_->new_profile_button, &QPushButton::clicked, this, &ConnectionDialog::newProfile);
  connect(ui_->default_profile, &QPushButton::clicked, this, &ConnectionDialog::defaultProfile);

  newServerToggled(ui_->new_server_radio_button->isChecked());
  loadProfiles();
}

ConnectionDialog::~ConnectionDialog() {
  db_file_dialog_->deleteLater();
  delete ui_;
}

bool ConnectionDialog::runANewServer() {
  return ui_->new_server_radio_button->isChecked();
}

QString ConnectionDialog::serverHost() {
  return ui_->server_host_line_edit->text();
}

int ConnectionDialog::serverPort() {
  return ui_->port_spin_box->value();
}

QString ConnectionDialog::clientInterface() {
  return ui_->client_interface_line_edit->text();
}

QString ConnectionDialog::authenticationKey() {
  return ui_->key_line_edit->text();
}

QString ConnectionDialog::clientName() {
  return ui_->client_name_line_edit->text();
}

QString ConnectionDialog::databaseFile() {
  return ui_->database_line_edit->text();
}

QString ConnectionDialog::serverScript() {
  return ui_->server_executable_line_edit->text();
}

QString localhost("127.0.0.1");

void ConnectionDialog::serverLocalhost() {
  ui_->server_host_line_edit->setText(localhost);
}

void ConnectionDialog::clientLocalhost() {
  ui_->client_interface_line_edit->setText(localhost);
}

void ConnectionDialog::randomKey() {
  // TODO: This is cryptographically-secure on all modern OS-es, but this isn't
  // explicitely guaranteed by the standard. We should fix it someday.
  std::random_device rd;
  std::uniform_int_distribution<uint32_t> uniform;
  auto gen_key_part = [&rd, &uniform](){
    return QString("%1").arg(uniform(rd), 8 /* width */, 16 /* base */,
                             QChar('0'));
  };
  ui_->key_line_edit->setText(gen_key_part() + gen_key_part() + gen_key_part()
                              + gen_key_part());
}

void ConnectionDialog::newServerToggled(bool toggled) {
  ui_->database_label->setEnabled(toggled);
  ui_->database_line_edit->setEnabled(toggled);
  ui_->select_database_button->setEnabled(toggled);

  ui_->server_executable_label->setEnabled(toggled);
  ui_->server_executable_line_edit->setEnabled(toggled);
  ui_->select_server_executable_button->setEnabled(toggled);

  ui_->random_key_button->setEnabled(toggled);
}

void ConnectionDialog::databaseFileSelected(const QString& file_name) {
  ui_->database_line_edit->setText(file_name);
}

void ConnectionDialog::serverFileSelected(const QString& file_name) {
  ui_->server_executable_line_edit->setText(file_name);
}

void ConnectionDialog::loadDefaultValues() {
  ui_->new_server_radio_button->setChecked(
      util::settings::connection::runServerDefault());
  ui_->server_host_line_edit->setText(
      util::settings::connection::serverHostDefault());
  ui_->port_spin_box->setValue(
      util::settings::connection::serverPortDefault());
  ui_->key_line_edit->setText(
      util::settings::connection::connectionKeyDefault());
  ui_->client_interface_line_edit->setText(
      util::settings::connection::clientInterfaceDefault());
  ui_->client_name_line_edit->setText(
      util::settings::connection::clientNameDefault());
  ui_->database_line_edit->setText(
      util::settings::connection::databaseNameDefault());
  ui_->server_executable_line_edit->setText(
      util::settings::connection::serverScriptDefault());
}

void ConnectionDialog::loadProfiles() {
  QString current_profile = util::settings::connection::currentProfile();
  QStringList profiles = util::settings::connection::profileList();

  ui_->profile->clear();
  ui_->profile->addItems(profiles);
  ui_->profile->setEditable(!profiles.contains(current_profile));
  ui_->profile->setCurrentText(current_profile);
  loadSettings();
}

void ConnectionDialog::loadSettings() {
  (util::settings::connection::runServer() ? ui_->new_server_radio_button
      : ui_->existing_server_radio_button)->setChecked(true);
  ui_->server_host_line_edit->setText(
      util::settings::connection::serverHost());
  ui_->port_spin_box->setValue(util::settings::connection::serverPort());
  ui_->key_line_edit->setText(util::settings::connection::connectionKey());
  ui_->client_interface_line_edit->setText(
      util::settings::connection::clientInterface());
  QString client_name = util::settings::connection::clientName();
  ui_->client_name_line_edit->setText(client_name);
  ui_->database_line_edit->setText(
      util::settings::connection::databaseName());
  ui_->server_executable_line_edit->setText(
      util::settings::connection::serverScript());
}

void ConnectionDialog::saveSettings() {
  util::settings::connection::setCurrentProfile(ui_->profile->currentText());
  util::settings::connection::setRunServer(
      ui_->new_server_radio_button->isChecked());
  util::settings::connection::setServerHost(
      ui_->server_host_line_edit->text());
  util::settings::connection::setServerPort(
      ui_->port_spin_box->value());
  util::settings::connection::setClientInterface(
      ui_->client_interface_line_edit->text());
  util::settings::connection::setClientName(
      ui_->client_name_line_edit->text());
  util::settings::connection::setConnectionKey(
      ui_->save_key_check_box->isChecked() ?
      ui_->key_line_edit->text() : "");
  util::settings::connection::setDatabaseName(
      ui_->database_line_edit->text());
  util::settings::connection::setServerScript(
      ui_->server_executable_line_edit->text());

  loadProfiles();
}

void ConnectionDialog::dialogAccepted() {
  saveSettings();
}

void ConnectionDialog::profileChanged(int index) {
  ui_->profile->setEditable(false);
  util::settings::connection::setCurrentProfile(ui_->profile->currentText());
  loadSettings();
}

void ConnectionDialog::profileRemoved() {
  util::settings::connection::removeProfile(ui_->profile->currentText());
  loadProfiles();
}

void ConnectionDialog::newProfile() {
  QString name = util::settings::connection::uniqueProfileName("profile");
  util::settings::connection::setCurrentProfile(name);
  loadProfiles();
  loadDefaultValues();
}

void ConnectionDialog::defaultProfile() {
  util::settings::connection::setDefaultProfile(ui_->profile->currentText());
}

void ConnectionDialog::profileNameEdited(QString name) {
  ui_->profile->setEditText(util::settings::connection::uniqueProfileName(name));
}

void ConnectionDialog::showEvent(QShowEvent* event) {
  loadSettings();
  ui_->server_host_line_edit->setFocus(Qt::OtherFocusReason);
}

}  // namespace ui
}  // namespace veles
