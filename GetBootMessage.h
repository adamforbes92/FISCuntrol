#include "Arduino.h"

#ifndef GetBootMessage_h
#define GetBootMessage_h

extern int CurrentHour;
extern String GreetingMessage1;
extern String GreetingMessage2;
extern class VW2002FISWriter fisWriter;

class GetBootMessage
{
  public:
    void returnBootMsg();
    void displayBootMsg();
    void displayBootImage();
  private:

};
#endif
