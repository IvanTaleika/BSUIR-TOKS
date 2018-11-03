#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow) {
  ui->setupUi(this);
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


void MainWindow::readData() {
  try {
    QByteArray data = serial->readPackage();
    ui->outputWindow->append("<resived>: " + data);
  } catch (SerialPortException e) {
    ui->outputWindow->append(e.getMessage());
  }
}

void MainWindow::handleError(QSerialPort::SerialPortError error) {
  if (error != QSerialPort::NoError) {
    QMessageBox::warning(this, tr("Critical Error #%1").arg(error), serial->errorString());
    closeSerialPort();
  }
}

void MainWindow::sendData() {
  QString message = ui->userText->toPlainText();
  serial->writePackage(message.toUtf8());
  ui->outputWindow->append("<sended>: " + message);

  ui->userText->clear();
}

void MainWindow::initActionsConnections() {
  connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
  connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
  connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(ui->actionConfigure, &QAction::triggered, settings, &SettingsDialog::show);
  connect(ui->actionClear, &QAction::triggered, ui->outputWindow, &QTextBrowser::clear);
}

void MainWindow::initPortConnections() {
  connect(serial, &SerialPort::errorOccurred, this, &MainWindow::handleError);
  connect(serial, &SerialPort::readyRead, this, &MainWindow::readData);
  connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::sendData);
}
void MainWindow::showStatusMessage(const QString& message) {
  status->setText(message);
}

