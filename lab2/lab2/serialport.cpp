#include "serialport.h"

const QByteArray SerialPort::FIRST_BYTE = QByteArrayLiteral("\x7e");
const QByteArray SerialPort::ESCAPE_BYTE = QByteArrayLiteral("\x7d");
const QByteArray SerialPort::FIRST_REPLACEMENT = QByteArrayLiteral("\x7d\x5e");
const QByteArray SerialPort::ESCAPE_REPLACEMENT = QByteArrayLiteral("\x7d\x5d");

SerialPort::SerialPort(QObject* parent) : QSerialPort(parent) {
}

QByteArray SerialPort::readPackage() {
  QByteArray data = QSerialPort::readAll();
  if (data.at(0) == FIRST_BYTE[0]) {
    data.remove(0, 1);
  } else {
    setErrorString("Invalide package");
    return QByteArray();
  }
  data.replace(ESCAPE_REPLACEMENT, ESCAPE_BYTE);
  data.replace(FIRST_REPLACEMENT, FIRST_BYTE);
  return data;
}

qint64 SerialPort::writePackage(QByteArray array) {
  array.replace(ESCAPE_BYTE, ESCAPE_REPLACEMENT);
  array.replace(FIRST_BYTE, FIRST_REPLACEMENT);
  array.insert(0, FIRST_BYTE);
  return QSerialPort::write(array);
}



