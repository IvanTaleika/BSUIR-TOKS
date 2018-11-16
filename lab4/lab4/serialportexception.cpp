#include "serialportexception.h"

SerialPortException::SerialPortException(QString message) {
  this->message = message;
}

const QString SerialPortException::getMessage() const {
  return message;
}
