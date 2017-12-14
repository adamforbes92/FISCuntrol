//Include FIS Writer, TimeLib and KWP
#include "VW2002FISWriter.h"
#include "KWP.h"
#include "Wire.h"
#include "GetBootMessage.h"
#include "GetButtonClick.h"
#include "StalkRotate.h"

// KWP.  RX = Pin 2, TX = Pin 3
#define pinKLineRX 2
#define pinKLineTX 3
KWP kwp(pinKLineRX, pinKLineTX);

/*
  #define ADR_Engine 0x01 // Engine
  #define ADR_Gears  0x02 // Auto Trans
  #define ADR_ABS_Brakes 0x03
  #define ADR_Airbag 0x15
  #define ADR_Dashboard 0x17 // Instruments
  #define ADR_Immobilizer 0x25
  #define ADR_Central_locking 0x35
  #define ADR_Navigation 0x37

*/
//Define MAX Retries
#define NENGINEGROUPS 11
#define NDASHBOARDGROUPS 1
#define NABSGROUPS 1
#define NAIRBAGGROUPS 1

#define NMODULES 4

//define engine groups
//**********************************0, 1,  2, 3,   4,   5,  6)
//Block 3, 5 don't work?
int engineGroups[NENGINEGROUPS] = {2, 3, 11, 14, 20, 31, 32, 114, 115, 118};
int dashboardGroups[NDASHBOARDGROUPS] = { 2 };
int absGroups[NDASHBOARDGROUPS] = { 1 };
int airbagGroups[NDASHBOARDGROUPS] = { 1 };

//currentSensor ALWAYS has to be 0 (if displaying all 4!)
//Change currentGroup for the group you'd like above!^
int intCurrentModule = 0;
int currentGroup = 0;
int currentSensor = 0;
int nSensors = 0;
int nGroupsCustom = 1;
int nGroupsCustomCount = 1;
int maxSensors = 4;

//define engine as ECU, with ADR_Engine (address)
KWP_MODULE engine = {"ECU", ADR_Engine, engineGroups, NENGINEGROUPS};
KWP_MODULE dashboard = {"CLUSTER", ADR_Dashboard, dashboardGroups, NDASHBOARDGROUPS};
KWP_MODULE _abs = {"ABS", ADR_ABS_Brakes, absGroups, NABSGROUPS};
KWP_MODULE airbag = {"AIRBAG", ADR_Airbag, airbagGroups, NAIRBAGGROUPS};

//define modules
KWP_MODULE *modules[NMODULES] = { &engine, &dashboard, &_abs, &airbag};

//define current module.  Adjust for others (0, 1, 2...)
//0: ecu
//1: cluster
KWP_MODULE *currentModule = modules[0];

int StalkPushUp = 4;
int StalkPushDown = 5;
int StalkPushReset = 6;

//Define ignition live sense as Pin 7
int IgnitionMonitorPin = 7;

// FIS
#define FIS_CLK 13  // - Arduino 13 - PB5
#define FIS_DATA 11 // - Arduino 11 - PB3
#define FIS_ENA 8 // - Arduino 8 - PB0

VW2002FISWriter fisWriter(FIS_CLK, FIS_DATA, FIS_ENA);
GetBootMessage getBootMessage;
StalkRotate stalkRotate;
GetButtonClick StalkPushUpButton(StalkPushUp, LOW, CLICKBTN_PULLUP);
GetButtonClick StalkPushDownButton(StalkPushDown, LOW, CLICKBTN_PULLUP);
GetButtonClick StalkPushResetButton(StalkPushReset, LOW, CLICKBTN_PULLUP);

bool IgnitionState = false;         // variable for reading the ignition pin status
bool IgnitionStateRunOnce = false;
bool removeOnce = true;
bool successfulConn = false;
bool successfulConnNoDrop = false;
bool fisDisable = false;
bool fisBeenToggled = false;
bool isCustom = false;

int CurrentHour = 0;
String GreetingMessage1 = "";
String GreetingMessage2 = "";
String definedUnitsPressure = "mbar";
String fisLine1; String fisLine2; String fisLine3; String fisLine4; String fisLine5; String fisLine6; String fisLine7; String fisLine8;
String fisTopLine1; String fisTopLine2;

int maxAttemptsModule = 3;
int maxAttemptsCountModule = 1;
int refreshTime = 200;
long lastRefreshTime;

int count = 0;

void setup()
{
  //Initialise basic varaibles
  Serial.begin(9600);
  pinMode(IgnitionMonitorPin, INPUT);

  IgnitionStateRunOnce = false;
  maxAttemptsCountModule = 1;
  intCurrentModule = 0;
  currentGroup = 0;
  removeOnce = true;
  successfulConn = false;
  successfulConnNoDrop = false;
  fisBeenToggled = false;
  kwp.disconnect();
  fisWriter.remove_graphic();

  long now = (long)millis();
  long lastRefreshTime = now;

  //define button stuff
  StalkPushUpButton.debounceTime   = 20;   // Debounce timer in ms
  StalkPushUpButton.multiclickTime = 1000;  // Time limit for multi clicks
  StalkPushUpButton.longClickTime  = 3500; // time until "held-down clicks" register

  StalkPushDownButton.debounceTime   = 20;   // Debounce timer in ms
  StalkPushDownButton.multiclickTime = 1000;  // Time limit for multi clicks
  StalkPushDownButton.longClickTime  = 3500; // time until "held-down clicks" register

  StalkPushResetButton.debounceTime   = 20;   // Debounce timer in ms
  StalkPushResetButton.multiclickTime = 1000;  // Time limit for multi clicks
  StalkPushResetButton.longClickTime  = 3500; // time until "held-down clicks" register
}

void loop()
{
  //KWP_MODULE engine = {"ECU", ADR_Engine, engineGroups, NENGINEGROUPS};
  //Check to see the current state of the digital pins (monitor voltage for ign, stalk press)
  IgnitionState = digitalRead(IgnitionMonitorPin);

  //refresh the current "now" (for refresh rate!)
  long now = (long)millis();

  //update the Reset button (see if it's been clicked more than 2 times (there 3+)
  //if it has, toggle fisDisable to turn off/on the screen
  StalkPushResetButton.Update();
  if (StalkPushResetButton.clicks > 2)
  {
    //flip-flop disDisable
    fisDisable = !fisDisable;

    //set disBeenToggled to true to stop displaying the "good morning" on turn-on (only this session)
    fisBeenToggled = true;
  }

  //if the screen isn't disabled, carry on
  if (!fisDisable)
  {
    //see if the down button has been pressed.  If it's been held, it will return -1
    StalkPushDownButton.Update();

    if (StalkPushDownButton.clicks == -1)
    {
      //if the down button has been held, prepare to swap groups!
      //use return; to get the loop to refresh (pointers need refreshing!)
      currentGroup = 0;
      successfulConn = false;
      successfulConnNoDrop = false;
      maxAttemptsCountModule = 1;

      //if the next module causes a rollover, start again
      if ((intCurrentModule + 1) > (NMODULES - 1))
      {
        currentModule = modules[0];
        kwp.disconnect();
        fisWriter.remove_graphic();
        return;
      }
      else
      {
        //rollover isn't a thing, so go to the next module.  Needs a "return" to allow reset of module
        intCurrentModule++;
        currentModule = modules[intCurrentModule];
        kwp.disconnect();
        fisWriter.remove_graphic();
        return;
      }
    }

    //if the previous module causes a rollover, start again
    if (StalkPushDownButton.clicks > 0)
    {
      if ((currentGroup + 1) > (NENGINEGROUPS - 1))
      {
        currentGroup = 0;
        fisWriter.init_graphic();
        delay(80);
      }
      else
      {
        //rollover isn't a thing, so go to the next module.  Needs a "return" to allow reset of module
        currentGroup++;
        fisWriter.init_graphic();
        delay(80);
      }
    }

    //check to see if "up" is held.  If it is, toggle "isCustom"
    StalkPushUpButton.Update();
    if (StalkPushUpButton.clicks == -1)
    {
      isCustom = !isCustom;
      return;
    }

    if (StalkPushUpButton.clicks > 0)
    {
      if ((currentGroup - 1) < 0)
      {
        currentGroup = NENGINEGROUPS - 1;
        fisWriter.init_graphic();
        delay(80);
      }
      else
      {
        currentGroup--;
        fisWriter.init_graphic();
        delay(80);
      }
    }

    //check the ign goes dead, prep the variables for the next run
    if (IgnitionState == LOW)
    {
      IgnitionStateRunOnce = false;
      removeOnce = false;
      successfulConn = false;
      successfulConnNoDrop = false;
      fisBeenToggled = false;
      maxAttemptsCountModule = 1;
      kwp.disconnect();
      fisWriter.remove_graphic();
    }

    //If the ignition is currently "on" then work out the message
    if (IgnitionState && !IgnitionStateRunOnce && !fisBeenToggled)
    {
      getBootMessage.returnBootMsg();
      getBootMessage.displayBootMsg();

      IgnitionStateRunOnce = true;
    }

    //if KWP isn't connected, ign is live and the welcome message has just been displayed
    if (!kwp.isConnected())
    {
      if ((maxAttemptsCountModule > maxAttemptsModule) && IgnitionStateRunOnce)
      {
        fisWriter.remove_graphic();
      }

      if ((maxAttemptsCountModule <= maxAttemptsModule) && IgnitionStateRunOnce)
      {
        //Reconnect quietly if already connected (don't show that you're reconnecting!)
        if (!successfulConnNoDrop)
        {
          fisWriter.FIS_init();
          delay(100);
          fisWriter.init_graphic();
          delay(300);

          fisWriter.write_text_full(0, 24, "CONNECTING TO:");
          fisWriter.write_text_full(0, 32, currentModule->name);
          fisWriter.write_text_full(0, 48, "ATTEMPT " + String(maxAttemptsCountModule) + "/" + String(maxAttemptsModule));
        }

        if (kwp.connect(currentModule->addr, 10400))
        {
          maxAttemptsCountModule = 1;
          successfulConn = 1;
        }
        else
        {
          maxAttemptsCountModule++;
        }
      }
    }
    else
    {
      //If the first connection was successful, don't show reconnects!
      if (!successfulConnNoDrop)
      {
        fisWriter.init_graphic();
        delay(5);

        fisWriter.write_text_full(0, 24, "CONNECTED TO:");
        fisWriter.write_text_full(0, 32, currentModule->name);
        delay(500);
        fisWriter.init_graphic();
        delay(1);
        successfulConnNoDrop = true;
      }

      //only refresh every "refreshTime" ms (stock is 500ms)
      if ((now - lastRefreshTime) > refreshTime)
      {
        //grab data for the currentGroup.
        SENSOR resultBlock[maxSensors];
        nSensors = kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);

        //if the data is in a stock group (!isCustom), then fill out as normal
        if (isCustom)
        {
          fisLine3 = resultBlock[0].value + " " + resultBlock[0].units;
          fisLine4 = resultBlock[1].value + " " + resultBlock[1].units;
          fisLine5 = resultBlock[2].value + " " + resultBlock[2].units;
          fisLine6 = resultBlock[3].value + " " + resultBlock[3].units;
          lastRefreshTime = now;
          nGroupsCustom = 0;

          //stop "slow" fetching of data.  Remember that it refreshes variables every 500ms, so the first "grab" grabs each block every 500ms.  SLOW!
          //this makes sure that ALL the data is captured before displaying it.  Each "length" must have a value, so /8 > 1 will catch all
          if ((fisLine3.length() > 1 or fisLine3.length() == 0) && (fisLine4.length() > 1 or fisLine4.length() == 0) && (fisLine5.length() > 1 or fisLine5.length() == 0)
              && (fisLine6.length() > 1 or fisLine6.length() == 0))
          {
            //only write lines 3, 4, 5, 6 (middle of screen) for neatness
            fisWriter.write_text_full(0, 8, "BLOCK " + String(engineGroups[currentGroup]));
            fisWriter.write_text_full(0, 16, resultBlock[currentSensor].desc);

            fisWriter.write_text_full(0, 40, fisLine3);
            fisWriter.write_text_full(0, 48, fisLine4);
            fisWriter.write_text_full(0, 56, fisLine5);
            fisWriter.write_text_full(0, 64, fisLine6);
          }
        }
        else
        {
          //if isCustom, then same idea, only use ALL the lines
          if ((fisLine1.length() > 1 or fisLine1.length() == 0) && (fisLine2.length() > 1 or fisLine2.length() == 0) && (fisLine3.length() > 1 or fisLine3.length() == 0)
              && (fisLine4.length() > 1 or fisLine4.length() == 0) && (fisLine5.length() > 1 or fisLine5.length() == 0) && (fisLine6.length() > 1 or fisLine6.length() == 0)
              && (fisLine7.length() > 1 or fisLine7.length() == 0) && (fisLine8.length() > 1 or fisLine8.length() == 0))
          {
            fisWriter.write_text_full(0, 8, "CUSTOM BLOCK 1");
            fisWriter.write_text_full(0, 16, "AIT KNOCK FUEL");

            fisWriter.write_text_full(0, 24, fisLine1);
            fisWriter.write_text_full(0, 32, fisLine2);
            fisWriter.write_text_full(0, 40, fisLine3);
            fisWriter.write_text_full(0, 48, fisLine4);
            fisWriter.write_text_full(0, 56, fisLine5);
            fisWriter.write_text_full(0, 64, fisLine6);
            fisWriter.write_text_full(0, 72, fisLine7);
            fisWriter.write_text_full(0, 80, fisLine8);
          }

          //first run will be case 0 (since nGroupsCustom = 1).  Prepare for the NEXT run (which will be group 2 (AIT/timing)).
          switch (nGroupsCustom - 1)
          {
            //                                   0  1   2   3   4   5   6   7     8    9
            //int engineGroups[NENGINEGROUPS] = {2, 3, 11, 14, 20, 31, 32, 114, 115, 118, 999};
            case 0:
              nGroupsCustom++;
              currentGroup = 2;
              lastRefreshTime = now;
              return;
            case 1:
              //this is the data from the ABOVE group (group 2).  Prepare for the NEXT group
              fisLine2 = resultBlock[3].value + " " + resultBlock[2].value + " " + resultBlock[2].units;
              nGroupsCustom++;
              currentGroup = 4;
              lastRefreshTime = now;
              return;
            case 2:
              //this is the data from the ABOVE group (group 4).  Prepare for the NEXT group
              fisLine3 = resultBlock[0].value + " " + resultBlock[1].value + " " + resultBlock[1].units;
              fisLine4 = resultBlock[2].value + " " + resultBlock[3].value + " " + resultBlock[3].units;
              nGroupsCustom++;
              currentGroup = 5;
              lastRefreshTime = now;
              return;
            case 3:
              //this is the data from the ABOVE group (group 5).  Prepare for the NEXT group
              fisLine7 = resultBlock[0].value + " L. REQ";
              fisLine8 = resultBlock[1].value + " L. ACT";

              nGroupsCustom = 1;
              currentGroup = 2;
              lastRefreshTime = now;
              return;
          }
        }
      }
    }
  }
  else
  {
    removeOnce = false;
    successfulConn = false;
    successfulConnNoDrop = false;
    maxAttemptsCountModule = 1;
    kwp.disconnect();
    fisWriter.remove_graphic();
  }
}
