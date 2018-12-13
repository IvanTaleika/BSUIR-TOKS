#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QtSerialPort/QSerialPort>
#include <QTime>
#include <QThread>
#include <QtMath>
#include <QMap>
#include <QTimer>
#include <QSet>
#include <tuple>
#include <QSignalMapper>
#include <algorithm>


class SerialPort : public QSerialPort {
  Q_OBJECT

 public:
  static constexpr int CORRUPTED = 1;
  static constexpr int PACKAGE_LOSS = 2;
  static constexpr int PACKAGE_SHUFFLE = 4;
  SerialPort(QObject* parent);
  void writePackage(QByteArray array, int flags = 0);
 public slots:
  void readPackage();
 signals:
  void dataSent(QByteArray data);
  void dataRead(QByteArray data);
  void eventMessage(QString message);

 private:
  struct Header {
    char sn;
    char an;
    bool ack;

    bool operator<(const Header& h) const {
      return sn < h.sn;
    }
    bool operator==(const Header& h) const {
      return sn == h.sn;
    }
  };
  QTimer receiveTimer;

  int waitingIndex = 0;
  static constexpr int MAX_TRIES = 5;
  static constexpr int SLOT_TIME = 50;
  static constexpr int WAIT_TIME = 2000;
  static const QByteArray FIRST_BYTE;
  static const QByteArray ESCAPE_BYTE;
  static const QByteArray FIRST_REPLACEMENT;
  static const QByteArray ESCAPE_REPLACEMENT;
  static constexpr int BITS_IN_BYTE = 8;
  static const QString CRC8;
  QByteArray currentMessage;
  QMap<Header, QByteArray> waitingMessages;
  QList<Header> receivedMessages;
  QMap<int, std::tuple<int, QTimer*, QByteArray>> sentMessages;
  QSignalMapper mapper;

  void receiveRequest(QByteArray& message, Header& header);
  void receiveAcknowledge(Header& header);
  void writeAcknowledge(Header& header);

  void addCrc(QByteArray& array);
  void code(QByteArray& array);
  void decode(QByteArray& array);
  QString toBitString(QByteArray& array);
  int calcCrc(QString bitString);
  bool validateCrc(QByteArray& array);
 private slots:
  void clearReceivingData();
  void timerFired(int i);
};

#endif // SERIALPORT_H
