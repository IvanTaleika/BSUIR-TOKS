#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QtSerialPort/QSerialPort>
#include <QTime>
#include "serialportexception.h"


class SerialPort : public QSerialPort {
 public:
  SerialPort(QObject* parent);
  QByteArray readPackage();
  qint64 writePackage(QByteArray array);
  qint64 writeCorruptedPackage(QByteArray array);
 private:
  static const QByteArray FIRST_BYTE;
  static const QByteArray ESCAPE_BYTE;
  static const QByteArray FIRST_REPLACEMENT;
  static const QByteArray ESCAPE_REPLACEMENT;
  static const int BITS_IN_BYTE;
  static const QString CRC8;


  void pack(QByteArray& array);
  void code(QByteArray& array);
  void decode(QByteArray& array);
  QString toBitString(QByteArray& array);
  int calcCrc(QString bitString);
  void validateCrc(QByteArray& array);

};

#endif // SERIALPORT_H
