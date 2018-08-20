/* Special dates:
    Birthday
    Christmas
    Friday
    Normal (Good morning, good evening...)
*/
#include "GetBootMessage.h"
#include "VW2002FISWriter.h"
#include "Wire.h"
#include "RTClib.h"
#include "VWbitmaps.h"

RTC_DS1307 rtc_time;    //  Real Time Clock DS1307 for actual date/time functions

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String CurrentMonth = "";
String CurrentDay = "";
String CurrentMinute = "";
String CombinedDayMonth = "";
byte currentHour = 0;

void GetBootMessage::returnBootMessage()
{
  //Get the current hour.  Minute not required.
  //Failsafe incase time can't be calculated
  GreetingMessage3 = "WELCOME";
  GreetingMessage4 = "ADAM!";

  rtc_time.begin();
  //rtc_time.adjust(DateTime(F(__DATE__), F(__TIME__)));

  DateTime now_time = rtc_time.now();

  CurrentMonth = String(now_time.month(), DEC);
  CurrentDay = String(now_time.day(), DEC);
  CurrentMinute = String(now_time.minute(), DEC);

  Serial.println(CurrentMinute + String(now_time.second()));

  currentHour = now_time.hour(), DEC;
  CombinedDayMonth = CurrentDay + CurrentMonth;

  switch (CombinedDayMonth.toInt()) {
    //C drops leading zero.  Compare for special dates.  DayMonth.  So Jan 01 is 11, Jan 02 is 12
    //Birthday
    case 2512:
      GreetingMessage3 = "MERRY";
      GreetingMessage4 = "CHRISTMAS!";

      GreetingMessage6 = ":)";
      break;

    case 11:
      GreetingMessage3 = "HAPPY NEW";
      GreetingMessage4 = "YEAR ADAM!";

      GreetingMessage6 = ":)";
      break;

    case 145:
      GreetingMessage3 = "HAPPY BIRTHDAY";
      GreetingMessage4 = "ADAM!";

      GreetingMessage6 = ":)";
      break;

    case 205:
      GreetingMessage3 = "ENJOY THE";
      GreetingMessage4 = "WEDDING!";

      GreetingMessage6 = ":)";
      break;

    //Today (for debug!)
    case 255:
      GreetingMessage3 = "ENJOY";
      GreetingMessage4 = "VOLKSFLING!";

      GreetingMessage6 = ":)";
      break;

    //Today (for debug!)
    case 227:
      GreetingMessage3 = "HAPPY";
      GreetingMessage4 = "ANNIVERSARY!";

      GreetingMessage6 = ":)";
      break;

    //Today (for debug!)
    case 318:
      GreetingMessage3 = "ENJOY";
      GreetingMessage4 = "EDITION 38!";

      GreetingMessage6 = ":)";
      break;

    //Default (if not special day!)
    default:
      if (currentHour >= 0 && currentHour < 12)
      {
        GreetingMessage3 = "GOOD MORNING";
        GreetingMessage4 = "ADAM!";
      }

      if (currentHour >= 12 && currentHour < 18)
      {
        GreetingMessage3 = "GOOD AFTERNOON";
        GreetingMessage4 = "ADAM!";
      }

      if (currentHour >= 18 && currentHour < 24)
      {
        GreetingMessage3 = "GOOD EVENING";
        GreetingMessage4 = "ADAM!";
      }
      break;
  }
}

void GetBootMessage::displayBootMessage()
{
  //Init the display and clear the screen
  fisWriter.FIS_init();
  delay(200);
  fisWriter.init_graphic();

  //Display the greeting.  40/48 is the height.
  fisWriter.write_text_full(0, 24, GreetingMessage1);
  fisWriter.write_text_full(0, 32, GreetingMessage2); delay(5);
  fisWriter.write_text_full(0, 40, GreetingMessage3); delay(5);
  fisWriter.write_text_full(0, 48, GreetingMessage4); delay(5);
  fisWriter.write_text_full(0, 56, GreetingMessage5); delay(5);
  fisWriter.write_text_full(0, 64, GreetingMessage6); delay(5);
  fisWriter.write_text_full(0, 72, GreetingMessage7); delay(5);
  fisWriter.write_text_full(0, 80, GreetingMessage8); delay(5);
  delay(3500);
}

void GetBootMessage::displayBootImage()
{
  //Init the display and clear the screen
  fisWriter.FIS_init();
  delay(200);
  fisWriter.init_graphic();
  //delay(5);

  //Display the greeting.  40/48 is the height.
  fisWriter.GraphicFromArray(0, 0, 64, 65, Q, 1);
  fisWriter.GraphicFromArray(0, 70, 64, 16, QBSW, 1);
  delay(3500);
}
