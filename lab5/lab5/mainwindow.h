#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settingsdialog.h"
#include "serialport.h"

#include <QMainWindow>
#include <QLabel>
#include <QDebug>
#include <QMessageBox>
#include <QShortcut>
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

 private slots:
  void openSerialPort();
  void openPort(QSerialPort* port, QString name, SettingsDialog::Settings p);
  void closeSerialPort();
  void dataRead(QByteArray data);
  void dataSent(QByteArray data);
  void showMessage(QString message);
  void handleError(QSerialPort::SerialPortError error);
  void sendData();
  void witeToNext(QByteArray data);

 signals:
  void sendingData(QByteArray data, int destinationAddress, int flags);
  void openReadSerial(QIODevice::OpenMode mode);
  void openWriteSerial(QIODevice::OpenMode mode);
  void closeSerial();

 private:
  void initPortConnections();
  void initActionsConnections();
  int readFlags();


  void showStatusMessage(const QString& message);

  Ui::MainWindow* ui;
  QLabel* status;
  SettingsDialog* settings;
  SerialPort* readPort;
  QSerialPort* writePort;
  QThread serialThread;
};
#endif // MAINWINDOW_H
