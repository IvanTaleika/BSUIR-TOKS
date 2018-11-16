#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow) {

  ui->setupUi(this);
  ui->collisionComboBox->setItemData(0, QVariant(0));
  ui->collisionComboBox->setItemData(1, QVariant(2));
  ui->collisionComboBox->setItemData(2, QVariant(4));
  serial = new SerialPort(this);
  settings = new SettingsDialog;
  ui->actionDisconnect->setEnabled(false);
  status = new QLabel;
  ui->statusBar->addWidget(status);
  ui->sendButton->setEnabled(false);
  initActionsConnections();
  initPortConnections();
}

MainWindow::~MainWindow() {
  delete settings;
  delete ui;
}

void MainWindow::openSerialPort() {
  SettingsDialog::Settings p = settings->settings();
  serial->setPortName(p.name);
  serial->setBaudRate(p.baudRate);
  serial->setDataBits(p.dataBits);
  serial->setParity(p.parity);
  serial->setStopBits(p.stopBits);
  serial->setFlowControl(p.flowControl);
  if (serial->open(QIODevice::ReadWrite)) {
    ui->actionConnect->setEnabled(false);
    ui->actionDisconnect->setEnabled(true);
    ui->actionConfigure->setEnabled(false);
    showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                      .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                      .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    ui->sendButton->setEnabled(true);
  } else {
    QMessageBox::critical(this, tr("Error"), serial->errorString());
    showStatusMessage(tr("Open error"));
  }
}

void MainWindow::closeSerialPort() {
  if (serial->isOpen()) {
    serial->close();
  }
  ui->actionConnect->setEnabled(true);
  ui->actionDisconnect->setEnabled(false);
  ui->actionConfigure->setEnabled(true);
  ui->userText->clear();
  ui->outputWindow->clear();
  showStatusMessage(tr("Disconnected"));
}

void MainWindow::dataRead(QByteArray data) {
  ui->outputWindow->append("<resived>: " + data);
}

void MainWindow::dataSent(QByteArray data) {
  ui->outputWindow->append("<sended>: " + data);
}

void MainWindow::showError(QString message) {
  ui->outputWindow->append(message);
}

void MainWindow::handleError(QSerialPort::SerialPortError error) {
  if (error != QSerialPort::NoError) {
    QMessageBox::warning(this, tr("Critical Error #%1").arg(error), serial->errorString());
    closeSerialPort();
  }
}

void MainWindow::sendData() {
  serial->writePackage(ui->userText->toPlainText().toUtf8(), readFlags());
  ui->userText->clear();
}

void MainWindow::initActionsConnections() {
  connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
  connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
  connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(ui->actionConfigure, &QAction::triggered, settings, &SettingsDialog::show);
  connect(ui->actionClear, &QAction::triggered, ui->outputWindow, &QTextBrowser::clear);
}

int MainWindow::readFlags() {
  return ui->corruptResultBox->isChecked() | ui->collisionComboBox->currentData().toInt();
}

void MainWindow::initPortConnections() {
  connect(serial, &SerialPort::errorOccurred, this, &MainWindow::handleError);
  connect(serial, &SerialPort::readyRead, serial, &SerialPort::readPackage);
  connect(serial, &SerialPort::dataRead, this, &MainWindow::dataRead);
  connect(serial, &SerialPort::dataSent, this, &MainWindow::dataSent);
  connect(serial, &SerialPort::sendError, this, &MainWindow::showError);
  connect(serial, &SerialPort::readError, this, &MainWindow::showError);
  connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::sendData);
}
void MainWindow::showStatusMessage(const QString& message) {
  status->setText(message);
}

