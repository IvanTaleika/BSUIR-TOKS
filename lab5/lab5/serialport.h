#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QtSerialPort/QSerialPort>
#include <QPair>
#include <QThread>
#pragma pack(1)


class SerialPort : public QSerialPort {
  Q_OBJECT

 public:

  SerialPort(QObject* parent = nullptr);
  bool getIsPowered() const;
  void setIsPowered(bool value);

  bool getIsDeleteMarker() const;
  void setIsDeleteMarker(bool value);

 public slots:
  void sendData(QByteArray data, int destination, int corrupted = 0);
  void sendMarker();
 signals:
  void dataSent(QByteArray data);
  void dataRead(QByteArray data);
  void eventMessage(QString message);
  void sendToNext(QByteArray data);

 private:

  struct AccessControl {
    int priority;
    bool isFrame;
  };

  struct FrameHeader {
    bool recognized;
    bool copied;
    int destinationAddress;
    int sourceAddress;
  };

  static const QByteArray FIRST_BYTE;
  static const QByteArray ESCAPE_BYTE;
  static const QByteArray FIRST_REPLACEMENT;
  static const QByteArray ESCAPE_REPLACEMENT;
  static constexpr int BITS_IN_BYTE = 8;
  static const QString CRC8;
  bool isPowered;
  bool isDeleteMarker;
  QList <QPair<int, QByteArray>> sendingMessages;
  int priority;
  QByteArray cachedFrame;

  int getId();
  void writeFrame();
  void addCrc(QByteArray& array);
  void code(QByteArray& array);
  void decode(QByteArray& array);
  QString toBitString(QByteArray& array);
  int calcCrc(QString bitString);
  bool validateCrc(QByteArray& array);
  void readFrame(QByteArray& data);
  void deleteFrame(QByteArray& data);
 private slots:
  void readPackage();
};

#endif // SERIALPORT_H
