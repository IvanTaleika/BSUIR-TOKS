#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QtSerialPort/QSerialPort>

class SerialPort : public QSerialPort {
 public:
  SerialPort(QObject* parent);

};

#endif // SERIALPORT_H
