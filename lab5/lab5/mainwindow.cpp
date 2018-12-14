#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow) {
  qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
  qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
  ui->setupUi(this);
  settings = new SettingsDialog(this);
  ui->actionDisconnect->setEnabled(false);
  status = new QLabel;
  ui->statusBar->addWidget(status);
  ui->sendButton->setEnabled(false);
  initActionsConnections();
  serial = new SerialPort();
  serial->moveToThread(&serialThread);
  connect(&serialThread, &QThread::finished, serial, &QObject::deleteLater);
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
  serial->setPortName(p.name);
  serial->setBaudRate(p.baudRate);
  serial->setDataBits(p.dataBits);
  serial->setParity(p.parity);
  serial->setStopBits(p.stopBits);
  serial->setFlowControl(p.flowControl);
  ui->actionConnect->setEnabled(false);
  ui->actionDisconnect->setEnabled(true);
  ui->actionConfigure->setEnabled(false);
  showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                    .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                    .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
  ui->sendButton->setEnabled(true);
  emit openSerial(QIODevice::ReadWrite);
}

void MainWindow::closeSerialPort() {
  if (serial->isOpen()) {
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
    QMessageBox::warning(this, tr("Critical Error #%1").arg(error), serial->errorString());
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

void MainWindow::initActionsConnections() {
  connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
  connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
  connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(ui->actionConfigure, &QAction::triggered, settings, &SettingsDialog::show);
  connect(ui->actionClear, &QAction::triggered, ui->outputWindow, &QTextBrowser::clear);
  connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::sendData);
}


void MainWindow::initPortConnections() {
  connect(serial, &SerialPort::errorOccurred, this, &MainWindow::handleError);
  connect(serial, &SerialPort::dataRead, this, &MainWindow::dataRead);
  connect(serial, &SerialPort::dataSent, this, &MainWindow::dataSent);
  connect(serial, &SerialPort::eventMessage, this, &MainWindow::showMessage);
  connect(this, &MainWindow::sendingData, serial, &SerialPort::sendData);
  connect(ui->sendMarkerButton, &QPushButton::clicked, serial, &SerialPort::sendMarker);
  connect(this, &MainWindow::openSerial, serial, &SerialPort::open);
  connect(this, &MainWindow::closeSerial, serial, &SerialPort::close);
  connect(ui->powerCheckbox, &QCheckBox::stateChanged, serial, &SerialPort::setIsPowered);
  connect(ui->deleteMarkerCheckbox, &QCheckBox::stateChanged, serial, &SerialPort::setIsDeleteMarker);
}
void MainWindow::showStatusMessage(const QString& message) {
  status->setText(message);
}

