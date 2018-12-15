#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow) {
  qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
  qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
  qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
  ui->setupUi(this);
  settings = new SettingsDialog(this);
  ui->actionDisconnect->setEnabled(false);
  status = new QLabel;
  ui->statusBar->addWidget(status);
  ui->sendButton->setEnabled(false);
  initActionsConnections();
  readPort = new SerialPort();
  writePort = new QSerialPort();
  readPort->moveToThread(&serialThread);
  connect(&serialThread, &QThread::finished, readPort, &QObject::deleteLater);
  initPortConnections();
  serialThread.start();
}

MainWindow::~MainWindow() {
  serialThread.quit();
  serialThread.wait();
  delete settings;
  delete ui;
}

void MainWindow::openSerialPort() {
  SettingsDialog::Settings p = settings->settings();
  openPort(readPort, p.name1, p);
  emit openReadSerial(QIODevice::ReadWrite);
  openPort(writePort, p.name, p);
  emit openWriteSerial(QIODevice::ReadWrite);
}

void MainWindow::openPort(QSerialPort* port, QString name, SettingsDialog::Settings p) {
  port->setPortName(name);
  port->setBaudRate(p.baudRate);
  port->setDataBits(p.dataBits);
  port->setParity(p.parity);
  port->setStopBits(p.stopBits);
  port->setFlowControl(p.flowControl);
  ui->actionConnect->setEnabled(false);
  ui->actionDisconnect->setEnabled(true);
  ui->actionConfigure->setEnabled(false);
  showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                    .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                    .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
  ui->sendButton->setEnabled(true);
}



void MainWindow::closeSerialPort() {
  if (readPort->isOpen()) {
    emit closeSerial();
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

void MainWindow::showMessage(QString message) {
  ui->outputWindow->append(message);
}

void MainWindow::handleError(QSerialPort::SerialPortError error) {
  if (error != QSerialPort::NoError) {
    QMessageBox::warning(this, tr("Critical Error #%1").arg(error), readPort->errorString());
    closeSerialPort();
  }
}

void MainWindow::sendData() {
  if (!ui->userText->toPlainText().isEmpty()) {
    emit sendingData(ui->userText->toPlainText().toUtf8(), ui->destinationLineEdit->text().toInt(),
                     ui->corruptResultBox->isChecked());
    ui->userText->clear();
  }
}

void MainWindow::witeToNext(QByteArray data) {
  writePort->write(data);
}

void MainWindow::initActionsConnections() {
  connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
  connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
  connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(ui->actionConfigure, &QAction::triggered, settings, &SettingsDialog::show);
  connect(ui->actionClear, &QAction::triggered, ui->outputWindow, &QTextBrowser::clear);
  connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::sendData);
}


void MainWindow::initPortConnections() {
  connect(readPort, &SerialPort::errorOccurred, this, &MainWindow::handleError);
  connect(readPort, &SerialPort::dataRead, this, &MainWindow::dataRead);
  connect(readPort, &SerialPort::dataSent, this, &MainWindow::dataSent);
  connect(readPort, &SerialPort::eventMessage, this, &MainWindow::showMessage);
  connect(this, &MainWindow::sendingData, readPort, &SerialPort::sendData);
  connect(ui->sendMarkerButton, &QPushButton::clicked, readPort, &SerialPort::sendMarker);
  connect(this, &MainWindow::openReadSerial, readPort, &SerialPort::open);
  connect(this, &MainWindow::openWriteSerial, writePort, &SerialPort::open);
  connect(this, &MainWindow::closeSerial, readPort, &SerialPort::close);
  connect(ui->powerCheckbox, &QCheckBox::stateChanged, readPort, &SerialPort::setIsPowered);
  connect(ui->deleteMarkerCheckbox, &QCheckBox::stateChanged, readPort, &SerialPort::setIsDeleteMarker);
  connect(readPort, &SerialPort::sendToNext, this, &MainWindow::witeToNext);
}


void MainWindow::showStatusMessage(const QString& message) {
  status->setText(message);
}

