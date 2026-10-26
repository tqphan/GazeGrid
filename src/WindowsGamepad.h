#pragma once
#include <QString>
#include <QMap>
#include <functional>
#include <cstdint>

// Platform-independent Xbox 360 report translation; native transport is ViGEmBus.
class WindowsGamepad {
public:
 struct Report {
  uint16_t buttons=0;
  uint8_t leftTrigger=0,rightTrigger=0;
  int16_t leftX=0,leftY=0,rightX=0,rightY=0;
  bool operator==(const Report &)const=default;
 };
 struct Backend {
  std::function<bool(QString &)> open;
  std::function<bool(const Report &,QString &)> submit;
  std::function<void()> close;
 };
 static Backend nativeBackend();
 explicit WindowsGamepad(Backend backend=nativeBackend());
 ~WindowsGamepad();
 WindowsGamepad(const WindowsGamepad &)=delete;
 WindowsGamepad &operator=(const WindowsGamepad &)=delete;
 bool send(int type,int code,int value);
 void close();
 QString error()const{return message;}
private:
 Backend backend;
 QMap<int,int> buttons,axes;
 bool connected=false;
 QString message;
 Report report()const;
};
