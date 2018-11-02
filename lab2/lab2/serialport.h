#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QtSerialPort/QSerialPort>

class SerialPort : public QSerialPort {
 public:
  SerialPort(QObject* parent);
  QByteArray readPackage();
  qint64 writePackage(QByteArray array);
 private:
  static const QByteArray FIRST_BYTE;
  static const QByteArray ESCAPE_BYTE;
  static const QByteArray FIRST_REPLACEMENT;
  static const QByteArray ESCAPE_REPLACEMENT;

};

#endif // SERIALPORT_H
