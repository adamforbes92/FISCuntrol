#include "Arduino.h"

#ifndef GetBootMessage_h
#define GetBootMessage_h

extern String GreetingMessage1;
extern String GreetingMessage2;
extern String GreetingMessage3;
extern String GreetingMessage4;
extern String GreetingMessage5;
extern String GreetingMessage6;
extern String GreetingMessage7;
extern String GreetingMessage8;

extern class VW2002FISWriter fisWriter;
extern class RTC_DS1307 rtc_time;
extern class RTC_Millis rtc;

class GetBootMessage
{
  public:
    void returnBootMessage();
    void displayBootMessage();
    void displayBootImage();
  private:

};
#endif

