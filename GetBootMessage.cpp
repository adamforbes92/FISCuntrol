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
String CombinedDayMonth = "";

#define FIS_CLK 13  // - Arduino 13 - PB5
#define FIS_DATA 11 // - Arduino 11 - PB3
#define FIS_ENA 8 // - Arduino 8 - PB0

void GetBootMessage::returnBootMsg()
{
  //Get the current hour.  Minute not required.
  //Failsafe incase time can't be calculated
  GreetingMessage1 = "WELCOME";
  GreetingMessage2 = "ADAM!";

  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

  DateTime now = rtc.now();
  CurrentMonth = String(now.month());
  CurrentDay = String(now.day());
  CurrentHour = now.hour();
  CombinedDayMonth = CurrentDay + CurrentMonth;

  switch (CombinedDayMonth.toInt()) {
    //C drops leading zero.  Compare for special dates.  MonthDay.  So Jan 01 is 101, Jan 02 is 102
    //Happy new year!
    case 101:
      GreetingMessage1 = "HAPPY NEW YEAR";
      GreetingMessage2 = "ADAM!";
      break;

    //Christmas
    case 2512:
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
  //Serial.println("Display welcome message");
  //Init the display and clear the screen
  fisWriter.FIS_init();
  delay(200);
  fisWriter.init_graphic();
  delay(300);

  //fisWriter.write_graph("11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
  //Display the greeting.  40/48 is the height.
  fisWriter.write_text_full(0, 40, GreetingMessage1);
  fisWriter.write_text_full(0, 48, GreetingMessage2);
  delay(3000);
}

void GetBootMessage::displayBootImage()
{
  //Serial.println("Display welcome message");
  //Init the display and clear the screen
  fisWriter.FIS_init();
  delay(200);
  fisWriter.init_graphic();
  delay(300);

  fisWriter.write_graph("0F0F0F");
  delay(3000);
}
