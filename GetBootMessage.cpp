/* Special dates:
    Birthday
    Christmas
    Friday
    Normal (Good morning, good evening...)
*/

#include "GetBootMessage.h"
#include "TimeLib.h"
#include "Wire.h"
#include "VW2002FISWriter.h"
#include "RTClib.h"

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
//#define Serial SerialUSB
#endif

RTC_Millis rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String CurrentMonth = "";
String CurrentDay = "";
String CombinedMonthDay = "";

#define FIS_CLK 13  // - Arduino 13 - PB5
#define FIS_DATA 11 // - Arduino 11 - PB3
#define FIS_ENA 8 // - Arduino 8 - PB0

void GetBootMessage::returnBootMsg()
{
  //Get the current hour.  Minute not required.

  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

  DateTime now = rtc.now();
  CurrentMonth = String(now.month());
  CurrentDay = String(now.day());
  CurrentHour = now.hour();
  CombinedMonthDay = CurrentDay + CurrentMonth;

  switch (CombinedMonthDay.toInt()) {
    //C drops leading zero.  Compare for special dates.  MonthDay.  So Jan 01 is 101, Jan 02 is 102
    //Happy new year!
    case 101:
      GreetingMessage1 = "HAPPY NEW YEAR";
      GreetingMessage2 = "ADAM!";
      break;

    //Christmas
    case 1225:
      GreetingMessage1 = "MERRY XMAS";
      GreetingMessage2 = "ADAM!";
      break;

    //Birthday
    case 1405:
      GreetingMessage1 = "HAPPY BIRTHDAY";
      GreetingMessage2 = "ADAM!";
      break;

    //Today (for debug!)
    case 312:
      GreetingMessage1 = "HAPPY BIRTHDAY";
      GreetingMessage2 = "ADAM!";
      break;

    //Default (if not special day!)
    default:
      if (CurrentHour > 00 && CurrentHour < 12)
      {
        GreetingMessage1 = "GOOD MORNING";
        GreetingMessage2 = "ADAM!";
      }

      if (CurrentHour >= 12 && CurrentHour < 18)
      {
        GreetingMessage1 = "GOOD AFTERNOON";
        GreetingMessage2 = "ADAM!";
      }

      if (CurrentHour >= 18 && CurrentHour < 24)
      {
        GreetingMessage1 = "GOOD EVENING";
        GreetingMessage2 = "ADAM!";
      }
      break;
  }
}

void GetBootMessage::displayBootMsg()
{

}
