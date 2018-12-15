#include "serialport.h"
const QByteArray SerialPort::FIRST_BYTE  = QByteArrayLiteral("\x7e");
const QByteArray SerialPort::ESCAPE_BYTE = QByteArrayLiteral("\x7d");
const QByteArray SerialPort::FIRST_REPLACEMENT = QByteArrayLiteral("\x7d\x5e");
const QByteArray SerialPort::ESCAPE_REPLACEMENT = QByteArrayLiteral("\x7d\x5d");
const QString SerialPort::CRC8 = "100011011";


SerialPort::SerialPort(QObject* parent) : QSerialPort(parent) {
  connect(this, &SerialPort::readyRead, this, &SerialPort::readPackage);
  priority = 1;
  isPowered = true;
  isDeleteMarker = false;
}

void SerialPort::readPackage() {
  if (isPowered) {
    auto data = QSerialPort::readAll();
    if (isDeleteMarker) {
      emit eventMessage("Marker was deleted");
      return;
    }
    decode(data);
    auto ac = (AccessControl*) data.data();
    if (ac->isFrame) {
      auto header = (FrameHeader*)(data.data() + sizeof(AccessControl));
      if (header->destinationAddress == getId()) {
        readFrame(data);
      }
      if (header->sourceAddress == getId()) {
        deleteFrame(data);
      }
    }
    if (!cachedFrame.isEmpty()) {
      emit sendToNext(cachedFrame);
      cachedFrame.clear();
    } else if (!sendingMessages.isEmpty()) {
      if (ac->isFrame) {
        if (priority > ac->priority) {
          cachedFrame.append(data);
          writeFrame();
          priority = 1;
        } else {
          priority = ac->priority + 1;
          code(data);
          emit sendToNext(data);
        }
      } else {
        writeFrame();
        priority = 1;
      }
    } else {
      code(data);
      emit sendToNext(data);
    }
  } else {
    auto data = QSerialPort::readAll();
    emit sendToNext(data);
  }
}

void SerialPort::readFrame(QByteArray& data) {
  auto header = (FrameHeader*)(data.data() + sizeof(AccessControl));
  header->recognized = true;
  auto frame = data.right(data.size() - sizeof(AccessControl) - sizeof(FrameHeader));
  if (validateCrc(frame)) {
    frame.remove(frame.size() - 1, 1);
    header->copied = true;
    emit dataRead(tr("COM%1 - ").arg(header->sourceAddress).toUtf8() + frame);
  } else {
    emit eventMessage(tr("Received corrupted data from COM%1").arg(header->sourceAddress));
  }
}

void SerialPort::deleteFrame(QByteArray& data) {
  auto ac = (AccessControl*) data.data();
  ac->isFrame = false;
  ac->priority = 0;
  auto frame = data.right(data.size() - sizeof(AccessControl) - sizeof(FrameHeader));
  data.remove(sizeof(AccessControl) + sizeof(FrameHeader), frame.size());
  frame.remove(frame.size() - 1, 1);
  auto headerBytes = data.right(sizeof(FrameHeader));
  data.remove(sizeof(AccessControl), sizeof(FrameHeader));
  auto header = (FrameHeader*)headerBytes.data();
  data.remove(0, sizeof(FrameHeader));
  if (!header->recognized) {
    emit eventMessage(tr("Error sending \"%1\" to COM%2").arg(QString(frame)).arg(header->destinationAddress));
  } else {
    emit dataSent(tr("To COM%1 - ").arg(header->destinationAddress).toUtf8() + frame);
  }
}

void SerialPort::sendData(QByteArray data, int destination, int corrupted) {
  addCrc(data);
  if (corrupted) {
    data.replace(data.size() - 1, 1, QByteArray(1, data.at(data.size() - 1) + 1));
  }
  sendingMessages.append({destination, data});
}

void SerialPort::sendMarker() {
  AccessControl ac = {0, 0};
  QByteArray data((char*) &ac, sizeof(AccessControl));
  code(data);
  emit sendToNext(data);
}

bool SerialPort::getIsDeleteMarker() const {
  return isDeleteMarker;
}

void SerialPort::setIsDeleteMarker(bool value) {
  isDeleteMarker = value;
}

bool SerialPort::getIsPowered() const {
  return isPowered;
}

void SerialPort::setIsPowered(bool value) {
  isPowered = value;
}

int SerialPort::getId() {
  return portName().remove(0, 3).toInt();
}

void SerialPort::writeFrame() {
  auto pair = sendingMessages.takeFirst();
  auto data = pair.second;
  FrameHeader header = {0, 0, pair.first, getId()};
  data.prepend((char*) &header, sizeof(FrameHeader));
  AccessControl ac = {priority, true};
  data.prepend((char*) &ac, sizeof(AccessControl));
  code(data);
  emit sendToNext(data);
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

void SerialPort::decode(QByteArray& array) {
  if (array.at(0) == FIRST_BYTE[0]) {
    array.remove(0, 1);
  } else {
    emit eventMessage(tr("Marker is corrupted"));
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

bool SerialPort::validateCrc(QByteArray& array) {
  return calcCrc(toBitString(array)) == 0;
}
