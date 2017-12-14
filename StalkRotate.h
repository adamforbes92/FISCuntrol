/*
  Morse.h - Library for flashing Morse code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#include "Arduino.h"

#ifndef StalkRotate_h
#define StalkRotate_h

extern int CurrentHour;
extern String GreetingMessage1;
extern String GreetingMessage2;

class StalkRotate
{
  public:
    void StalkRotateUp();
    void StalkRotateDown();
    
    void StalkRotateUpHeld();
    void StalkRotateDownHeld();
  private:

};
#endif
