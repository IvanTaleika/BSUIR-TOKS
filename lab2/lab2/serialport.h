#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QtSerialPort/QSerialPort>

class SerialPort : public QSerialPort {
 public:
  SerialPort(QObject* parent);
  QByteArray readPackage();
  qint64 writePackage(QByteArray array);
 private:
  const QByteArray FIRST_BYTE = QByteArrayLiteral("\x7e");
  const QByteArray ESCAPE_BYTE = QByteArrayLiteral("\x7d");
  const QByteArray FIRST_REPLACEMENT = QByteArrayLiteral("\x7d\x5e");
  const QByteArray ESCAPE_REPLACEMENT = QByteArrayLiteral("\x7d\x5d");

};

#endif // SERIALPORT_H
