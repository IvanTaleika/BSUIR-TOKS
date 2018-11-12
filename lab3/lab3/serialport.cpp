#include "serialport.h"
const QByteArray SerialPort::FIRST_BYTE = QByteArrayLiteral("\x7e");
const QByteArray SerialPort::ESCAPE_BYTE = QByteArrayLiteral("\x7d");
const QByteArray SerialPort::FIRST_REPLACEMENT = QByteArrayLiteral("\x7d\x5e");
const QByteArray SerialPort::ESCAPE_REPLACEMENT = QByteArrayLiteral("\x7d\x5d");
const int SerialPort::BITS_IN_BYTE = 8;
const QString SerialPort::CRC8 = "100011011";


SerialPort::SerialPort(QObject* parent) : QSerialPort(parent) {
  qsrand(QTime::currentTime().msec());
}

QByteArray SerialPort::readPackage() {
  QByteArray data = QSerialPort::readAll();
  decode(data);
  validateCrc(data);
  data.remove(data.size() - 1, 1);
  return data;
}

qint64 SerialPort::writePackage(QByteArray array) {
  pack(array);
  return QSerialPort::write(array);
}

qint64 SerialPort::writeCorruptedPackage(QByteArray array) {
  pack(array);
  auto pos = qrand() % array.size();
  char value = array.at(pos) + 1;
  array.replace(pos, 1, QByteArray(1, value));
  return QSerialPort::write(array);
}

void SerialPort::pack(QByteArray& array) {
  QString bitString = toBitString(array);
  bitString.append(QString("%1").arg(0, CRC8.size() - 1, 2, QChar('0')));
  array.append(calcCrc(bitString));
  code(array);
}

void SerialPort::code(QByteArray& array) {
  array.replace(ESCAPE_BYTE, ESCAPE_REPLACEMENT);
  array.replace(FIRST_BYTE, FIRST_REPLACEMENT);
  array.insert(0, FIRST_BYTE);
}

void SerialPort::decode(QByteArray& array) {
  if (array.at(0) == FIRST_BYTE[0]) {
    array.remove(0, 1);
  } else {
    throw SerialPortException("Decoding error occurs.");
  }
  array.replace(ESCAPE_REPLACEMENT, ESCAPE_BYTE);
  array.replace(FIRST_REPLACEMENT, FIRST_BYTE);
}

QString SerialPort::toBitString(QByteArray& array) {
  QString bitStr;
  for (int i = 0; i < array.size(); i++) {
    bitStr.append(QString("%1").arg(static_cast<unsigned char>(array.at(i)), BITS_IN_BYTE, 2, QChar('0')));
  }
  bitStr.remove(0, bitStr.indexOf('1'));
  return bitStr;
}


int SerialPort::calcCrc(QString bitString) {
  bool ok;
  int polinom = CRC8.toInt(&ok, 2);
  while (bitString.size() >= CRC8.size()) {
    int value = bitString.mid(0, CRC8.size()).toInt(&ok, 2);
    value ^= polinom;
    bitString.replace(0, CRC8.size(), QString("%1").arg(value, BITS_IN_BYTE, 2, QChar('0')));
    bitString.remove(0, bitString.indexOf('1'));
  }
  return bitString.toInt(&ok, 2);
}

void SerialPort::validateCrc(QByteArray& array) {
  if (calcCrc(toBitString(array)) != 0) {
    throw SerialPortException("Transmission error occurs.");
  }
}

