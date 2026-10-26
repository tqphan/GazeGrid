#pragma once
// Stable event IDs shared by the action parser and injected test backends.
#ifndef _WIN32
#include <linux/input.h>
#else
constexpr int ABS_HAT0X=0x10;
constexpr int ABS_HAT0Y=0x11;
constexpr int ABS_RUDDER=0x07;
constexpr int ABS_RX=0x03;
constexpr int ABS_RY=0x04;
constexpr int ABS_RZ=0x05;
constexpr int ABS_THROTTLE=0x06;
constexpr int ABS_WHEEL=0x08;
constexpr int ABS_X=0x00;
constexpr int ABS_Y=0x01;
constexpr int ABS_Z=0x02;
constexpr int BTN_C=0x132;
constexpr int BTN_DPAD_DOWN=0x221;
constexpr int BTN_DPAD_LEFT=0x222;
constexpr int BTN_DPAD_RIGHT=0x223;
constexpr int BTN_DPAD_UP=0x220;
constexpr int BTN_EAST=0x131;
constexpr int BTN_MODE=0x13c;
constexpr int BTN_NORTH=0x133;
constexpr int BTN_SELECT=0x13a;
constexpr int BTN_SOUTH=0x130;
constexpr int BTN_START=0x13b;
constexpr int BTN_THUMBL=0x13d;
constexpr int BTN_THUMBR=0x13e;
constexpr int BTN_TL=0x136;
constexpr int BTN_TL2=0x138;
constexpr int BTN_TR=0x137;
constexpr int BTN_TR2=0x139;
constexpr int BTN_TRIGGER_HAPPY=0x2c0;
constexpr int BTN_WEST=0x134;
constexpr int BTN_Z=0x135;
constexpr int EV_ABS=0x03;
constexpr int EV_KEY=0x01;
constexpr int EV_SYN=0x00;
#endif
