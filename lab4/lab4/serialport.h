#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QtSerialPort/QSerialPort>
#include <QTime>
#include <QThread>
#include <QtMath>


class SerialPort : public QSerialPort {
  Q_OBJECT

 public:
  static constexpr int CORRUPTED = 1;
  static constexpr int COLLISION = 2;
  static constexpr int LAST_COLLISION = 4;
  SerialPort(QObject* parent);
  qint64 writePackage(QByteArray array, int flags = 0);
 public slots:
  void readPackage();
 signals:
  void dataSent(QByteArray data);
  void dataRead(QByteArray data);
  void sendError(QString message);
  void readError(QString message);

 private:
  struct SentData {
    QByteArray data;
    int collision;
    int nTries;
  } sentData;
  static constexpr int N_TRIES = 5;
  static constexpr int SLOT_TIME = 50;
  static const QByteArray FIRST_BYTE;
  static const QByteArray ESCAPE_BYTE;
  static const QByteArray FIRST_REPLACEMENT;
  static const QByteArray ESCAPE_REPLACEMENT;
  static constexpr int BITS_IN_BYTE = 8;
  static const QString CRC8;

  void validateSentData(QByteArray& array);
  void addCrc(QByteArray& array);
  void code(QByteArray& array);
  bool decode(QByteArray& array);
  QString toBitString(QByteArray& array);
  int calcCrc(QString bitString);
  bool validateCrc(QByteArray& array);
};

#endif // SERIALPORT_H
