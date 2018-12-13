#include "serialport.h"
const QByteArray SerialPort::FIRST_BYTE  = QByteArrayLiteral("\x7e");
const QByteArray SerialPort::ESCAPE_BYTE = QByteArrayLiteral("\x7d");
const QByteArray SerialPort::FIRST_REPLACEMENT = QByteArrayLiteral("\x7d\x5e");
const QByteArray SerialPort::ESCAPE_REPLACEMENT = QByteArrayLiteral("\x7d\x5d");
const QString SerialPort::CRC8 = "100011011";


SerialPort::SerialPort(QObject* parent) : QSerialPort(parent), sentData() {
  qsrand(QTime::currentTime().msec());
}

void SerialPort::readPackage() {
  QByteArray data = QSerialPort::readAll();
  if (!sentData.data.isEmpty()) {
    return validateSentData(data);
  }
  QSerialPort::write(data);
  if (data.at(data.size() - 1) == 0) {
    data.remove(data.size() - 1, 1); //remove collision flag
    if (decode(data) && validateCrc(data)) {
      data.remove(data.size() - 1, 1); //remove CRC
      emit dataRead(data);
    } else {
      emit readError("Read data is corrupted.");
    }
  }
}

void SerialPort::validateSentData(QByteArray& array) {
  int collision = array.at(array.size() - 1);
  if (collision == COLLISION) {
    if (sentData.nTries < N_TRIES) {
      sentData.nTries++;
      unsigned long delay = qrand() % static_cast<unsigned long>(qPow(2, sentData.nTries));
      emit sendError(tr("Collision found. Trying to fix the error. Try %1, delay %2").arg(sentData.nTries).arg(delay));
      QThread::msleep(delay);
      if (delay % 2 == 0) {
        array.replace(array.size() - 1, 1, QByteArrayLiteral("\x0"));
      }
      QSerialPort::write(array);
      return;
    } else {
      emit sendError("Collision resended 5 times. Giving up.");
    }
  } else if (collision == LAST_COLLISION) {
    emit sendError("Last collision found. Message cannot be sended.");
  } else {
    emit dataSent(sentData.data);
  }
  sentData.data.clear();
  sentData.nTries = 0;
}

qint64 SerialPort::writePackage(QByteArray array, int flags) {
  sentData.data.append(array);
  addCrc(array);
  if (flags & CORRUPTED) {
    auto pos = qrand() % array.size();
    char value = array.at(pos) + 1;
    array.replace(pos, 1, QByteArray(1, value));
  }
  code(array);
  if (flags & COLLISION) {
    array.append(static_cast<char>(COLLISION));
    sentData.collision = COLLISION;
  } else if (flags & LAST_COLLISION) {
    array.append(static_cast<char>(LAST_COLLISION));
    sentData.collision = LAST_COLLISION;
  } else {
    array.append(static_cast<char>(0));
    sentData.collision = 0;
  }
  return QSerialPort::write(array);
}

void SerialPort::addCrc(QByteArray& array) {
  QString bitString = toBitString(array);
  bitString.append(QString("%1").arg(0, CRC8.size() - 1, 2, QChar('0')));
  array.append(calcCrc(bitString));
}

void SerialPort::code(QByteArray& array) {
  array.replace(ESCAPE_BYTE, ESCAPE_REPLACEMENT);
  array.replace(FIRST_BYTE, FIRST_REPLACEMENT);
  array.insert(0, FIRST_BYTE);
}

bool SerialPort::decode(QByteArray& array) {
  if (array.at(0) == FIRST_BYTE[0]) {
    array.remove(0, 1);
  } else {
    return false;
  }
  array.replace(ESCAPE_REPLACEMENT, ESCAPE_BYTE);
  array.replace(FIRST_REPLACEMENT, FIRST_BYTE);
  return true;
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

bool SerialPort::validateCrc(QByteArray& array) {
  return calcCrc(toBitString(array)) == 0;
}
