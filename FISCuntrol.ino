//Include FIS Writer, TimeLib and KWP
#include "VW2002FISWriter.h"
#include "KWP.h"
#include "Wire.h"
#include "GetBootMessage.h"
#include "GetButtonClick.h"

//Define MAX Retries
#define NENGINEGROUPS 5
#define NDASHBOARDGROUPS 1

#define NMODULES 2

// KWP.  RX = Pin 2, TX = Pin 3
#define pinKLineRX 2
#define pinKLineTX 3
KWP kwp(pinKLineRX, pinKLineTX);

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

int CurrentHour = 0;
String GreetingMessage1 = "";
String GreetingMessage2 = "";

//define engine groups
//**********************************0, 1,  2, 3,   4,   5,  6)
//Block 3, 5 don't work?
int engineGroups[NENGINEGROUPS] = {2, 3, 31, 32, 115};
int dashboardGroups[NDASHBOARDGROUPS] = { 2 };

//currentSensor ALWAYS has to be 0 (if displaying all 4!)
//Change currentGroup for the group you'd like above!^
int currentGroup = 0;
int currentSensor = 0;
int nSensors = 0;
int maxSensors = 4;

//define engine as ECU, with ADR_Engine (address)
KWP_MODULE engine = {"ECU", ADR_Engine, engineGroups, NENGINEGROUPS};
KWP_MODULE dashboard = {"CLUSTER", ADR_Dashboard, dashboardGroups, NDASHBOARDGROUPS};

//define modules
KWP_MODULE *modules[NMODULES] = { &engine, &dashboard };

//define current module.  Adjust for others (0, 1, 2...)
//0: ecu
//1: cluster
KWP_MODULE *currentModule = modules[0];
//KWP_MODULE *currentModule = modules[1];

int maxAttemptsModule = 3;
int maxAttemptsCountModule = 1;
int refreshTime = 500;
long lastRefreshTime;

int count = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(IgnitionMonitorPin, INPUT);

  IgnitionStateRunOnce = false;
  maxAttemptsCountModule = 1;
  currentGroup = 0;
  removeOnce = true;
  successfulConn = false;
  successfulConnNoDrop = false;
  fisBeenToggled = false;
  kwp.disconnect();
  fisWriter.remove_graphic();

  long now = (long)millis();
  long lastRefreshTime = now;

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

  long now = (long)millis();
  StalkPushResetButton.Update();
  //Serial.println(millis());
  if (StalkPushResetButton.clicks > 2)
  {
    Serial.println("Toggle screen!");
    fisDisable = !fisDisable;
    fisBeenToggled = true;
  }

  if (!fisDisable)
  {
    StalkPushDownButton.Update();  

    if (StalkPushDownButton.clicks > 0)
    {
      Serial.println("currentgroupdown" + String(currentGroup));
      if ((currentGroup + 1) > (NENGINEGROUPS - 1))
      {
        currentGroup = 0;
        fisWriter.init_graphic();
        delay(80);
      }
      else
      {
        currentGroup++;
        fisWriter.init_graphic();
        delay(80);
      }
    }

    StalkPushUpButton.Update();
    if (StalkPushUpButton.clicks > 0)
    {
      Serial.println("currentgroupup" + String(currentGroup));
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
      //Serial.println("Disconnecting, cleaning up...");
      IgnitionStateRunOnce = false;
      removeOnce = false;
      successfulConn = false;
      successfulConnNoDrop = false;
      maxAttemptsCountModule = 1;
      currentGroup = 0;
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
    //if ((!kwp.isConnected()) && (IgnitionState == 1) && (IgnitionStateRunOnce == 1))
    if (!kwp.isConnected())
    {
      //Serial.println("Not connected!");
      if ((maxAttemptsCountModule > maxAttemptsModule) && IgnitionStateRunOnce)
      {
        fisWriter.remove_graphic();
      }

      if ((maxAttemptsCountModule <= maxAttemptsModule) && IgnitionStateRunOnce)
      {
        //Reconnect quietly if already connected (don't show that you're reconnecting!)
        if (!successfulConnNoDrop)
        {
          //Serial.println("count++");
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
      //Serial.println("Success " + String(successfulConn));

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

      if ((now - lastRefreshTime) > refreshTime)
      {
        SENSOR resultBlock[maxSensors];
        nSensors = kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);

        if (resultBlock[currentSensor].value != "")
        {
          fisWriter.write_text_full(0, 8, "BLOCK " + String(engineGroups[currentGroup]));
          fisWriter.write_text_full(0, 16, resultBlock[currentSensor].desc);

          fisWriter.write_text_full(0, 40, resultBlock[0].value + " " + resultBlock[0].units);
          fisWriter.write_text_full(0, 48, resultBlock[1].value + " " + resultBlock[1].units);
          fisWriter.write_text_full(0, 56, resultBlock[2].value + " " + resultBlock[2].units);
          fisWriter.write_text_full(0, 64, resultBlock[3].value + " " + resultBlock[3].units);
        }
        lastRefreshTime = now;
      }
    }
  }
  else
  {
    //Serial.println("Disconnecting, cleaning up...");
    //IgnitionStateRunOnce = false;
    removeOnce = false;
    successfulConn = false;
    successfulConnNoDrop = false;
    maxAttemptsCountModule = 1;
    currentGroup = 0;
    kwp.disconnect();
    fisWriter.remove_graphic();
  }
}
