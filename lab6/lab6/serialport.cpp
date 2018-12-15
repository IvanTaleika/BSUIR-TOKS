#include "serialport.h"
const QByteArray SerialPort::FIRST_BYTE  = QByteArrayLiteral("\x7e");
const QByteArray SerialPort::ESCAPE_BYTE = QByteArrayLiteral("\x7d");
const QByteArray SerialPort::FIRST_REPLACEMENT = QByteArrayLiteral("\x7d\x5e");
const QByteArray SerialPort::ESCAPE_REPLACEMENT = QByteArrayLiteral("\x7d\x5d");
const QString SerialPort::CRC8 = "100011011";


SerialPort::SerialPort(QObject* parent) : QSerialPort(parent) {
  qsrand(QTime::currentTime().msec());
  connect(&mapper, SIGNAL(mapped(int)), this, SLOT(timerFired(int)));
  connect(&receiveTimer, SIGNAL(timeout()), this, SLOT(clearReceivingData()));
}

void SerialPort::readPackage() {
  QByteArray allData = QSerialPort::readAll();
  for (auto data : allData.split(FIRST_BYTE.at(0))) {
    if (data.isEmpty()) {
      continue;
    }
    decode(data);
    if (validateCrc(data)) {
      data.remove(data.size() - 1, 1); //remove CRC
      Header header = {data.at(0), data.at(1), data.at(2)};
      data.remove(0, sizeof(Header));
      if (header.ack == false) {
        emit eventMessage(tr("Received: %1 - %2").arg((int) header.sn).arg(data.data()));
        header.ack = true;
        writeAcknowledge(header);
        if (!receivedMessages.contains(header)) {
          receiveTimer.start(MAX_TRIES * WAIT_TIME);
          receiveRequest(data, header);
        }
      } else {
        emit eventMessage(tr("receiveAcknowledge - %1").arg((int) header.sn));
        receiveAcknowledge(header);
      }
    } else {
      emit eventMessage("Read data is corrupted.");
    }
  }
}

void SerialPort::receiveRequest(QByteArray& message, SerialPort::Header& header) {
  receivedMessages.append(header);
  if (header.sn == waitingIndex) {
    waitingIndex = header.an;
    currentMessage.append(message);
    for (auto i = waitingMessages.begin(); i != waitingMessages.end()
         && i.key().sn == waitingIndex; i = waitingMessages.erase(i)) {
      waitingIndex = i.key().an;
      currentMessage.append(i.value());
    }
    if (waitingIndex == 0) {
      emit dataRead(currentMessage);
      clearReceivingData();
    }
  } else {
    waitingMessages.insert(header, message);
  }
}



void SerialPort::receiveAcknowledge(SerialPort::Header& header) {
  if (sentMessages.contains(header.sn)) {
    auto timer = std::get<1>(sentMessages.take((int) header.sn));
    timer->stop();
    delete timer;
  }
}

void SerialPort::writeAcknowledge(SerialPort::Header& header) {
  QByteArray message((char*) &header, sizeof(Header));
  addCrc(message);
  code(message);
  QSerialPort::write(message);
  QSerialPort::flush();
}

void SerialPort::clearReceivingData() {
  receiveTimer.stop();
  currentMessage.clear();
  waitingMessages.clear();
  receivedMessages.clear();
  waitingIndex = 0;
}

void SerialPort::writePackage(QByteArray array, int flags) {
  QList<QByteArray> messages = array.split(' ');
  QList<std::tuple<Header, QTimer*, QByteArray>> messageTuples;
  Header header = {0, 0, 0};
  for (char i = 0; i < messages.size(); i++) {
    if (messages[i].isEmpty()) {
      continue;
    }
    if (i != messages.size() - 1) {
      messages[i].append(' ');
      header.an = i + 1;
    } else {
      header.an = 0;
    }
    header.sn = i;
    messages[i].prepend((char*) &header, sizeof(Header));
    addCrc(messages[i]);
    code(messages[i]);
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), &mapper, SLOT(map()));
    mapper.setMapping(timer, i);
    messageTuples.append(std::make_tuple(header, timer, messages[i]));
  }
  if (flags & PACKAGE_SHUFFLE) {
    std::random_shuffle(messageTuples.begin(), messageTuples.end());
  }
  if (flags & CORRUPTED) {
    auto nCorruption = qrand() % messageTuples.size();
    for (int i = 0; i < nCorruption; i++) {
      auto pos = qrand() % messageTuples.size();
      auto arr = std::get<2>(messageTuples.at(pos));
      arr.replace(arr.size() - 1, 1, QByteArray(1, arr.at(arr.size() - 1) + 1));
      std::get<2>(messageTuples[pos]) = arr;
    }
  }
  for (auto tuple : messageTuples) {
    auto sn = std::get<0>(tuple).sn;
    auto message = std::get<2>(tuple);
    auto timer = std::get<1>(tuple);
    sentMessages.insert(sn, std::make_tuple(0, timer, message));
    if ((flags & PACKAGE_LOSS) && qrand() % 2) {
      emit eventMessage(tr("Sending: %1").arg((int) sn));
      QSerialPort::write(message);
      SerialPort::flush();
    }
    timer->start(WAIT_TIME);
  }

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

void SerialPort::timerFired(int i) {
  auto tuple = &sentMessages.find(i).value();
  auto nTries = std::get<0>(*tuple);
  if (nTries < MAX_TRIES) {
    nTries++;
    std::get<0>(*tuple) = nTries;
    QSerialPort::write(std::get<2>(*tuple));
    emit eventMessage(tr("Timer fired %1 times for \"%2\"").arg(nTries).arg(i));
  } else {
    auto timer = std::get<1>(*tuple);
    timer->stop();
    delete timer;
    sentMessages.remove(i);
    emit eventMessage(tr("Data sent timeout for \"%1\"").arg(i));
  }
}
