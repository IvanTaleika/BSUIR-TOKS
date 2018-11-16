#ifndef SERIALPORTEXCEPTION_H
#define SERIALPORTEXCEPTION_H
#include <exception>
#include <QString>

class SerialPortException : public std::exception {
 public:
  SerialPortException(QString message);
  virtual const QString getMessage() const;
 private:
  QString message;

};

#endif // SERIALPORTEXCEPTION_H
