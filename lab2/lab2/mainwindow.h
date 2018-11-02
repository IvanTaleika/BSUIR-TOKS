#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settingsdialog.h"
#include "serialport.h"

#include <QMainWindow>
#include <QLabel>
//#include <QtSerialPort/QSerialPort>
#include <QDebug>
#include <QMessageBox>
#include <QShortcut>


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
  void closeSerialPort();
  void readData();
  void handleError(QSerialPort::SerialPortError error);
  void sendData();

 private:
  void initPortConnections();
  void initActionsConnections();


  void showStatusMessage(const QString& message);

  Ui::MainWindow* ui;
  QLabel* status;
  SettingsDialog* settings;
  SerialPort* serial;
};

#endif // MAINWINDOW_H
