//Include FIS Writer, TimeLib and KWP
#include "VW2002FISWriter.h"
#include "KWP.h"
#include "GetBootMessage.h"
#include "GetButtonClick.h"

// KWP.  RX = Pin 2, TX = Pin 3
#define pinKLineRX 12
#define pinKLineTX 13
KWP kwp(pinKLineRX, pinKLineTX);

#define stalkPushUp 24             // input stalk UP
#define stalkPushDown 23           // input stalk DOWN
#define stalkPushReset 22          // input stalk RESET

#define stalkPushUpMonitor 24             // input stalk UP (to monitor when FIS Disabled)
#define stalkPushDownMonitor 23           // input stalk DOWN (to monitor when FIS Disabled)
#define stalkPushResetMonitor 22          // input stalk RESET (to monitor when FIS Disabled)

#define stalkPushUpReturn 30       // if FIS disable - use this to match stalk UP
#define stalkPushDownReturn 31    // if FIS disable - use this to match stalk DOWN
#define stalkPushResetReturn 32
// if F`IS disable - use this to match stalk RESET

//Define ignition live sense as Pin 7
#define ignitionMonitorPin 10

// FIS
//#define FIS_CLK 13  // - Arduino 13 - PB5
//#define FIS_DATA 11 // - Arduino 11 - PB3
//#define FIS_ENA 8 // - Arduino 8 - PB0

#define FIS_CLK 52  // - Arduino 13 - PB5
#define FIS_DATA 51 // - Arduino 11 - PB3
#define FIS_ENA 53   // - Arduino 8 - PB0

//Define MAX Retries2
#define NENGINEGROUPS 20
#define NDASHBOARDGROUPS 1
#define NABSGROUPS 1
#define NAIRBAGGROUPS 1

#define NMODULES 2

//define engine groups
//**********************************0, 1,2, 3, 4, 5, 6,   7,  8,  9,  10, 11, 12, 13, 14, 15, 16,  17,  18,  19)
//Block 3, 5 don't work?
int engineGroups[NENGINEGROUPS] = {2, 3, 4, 5, 6, 10, 11, 14, 15, 16, 20, 31, 32, 90, 91, 92, 113, 114, 115, 118};   //defined blocks to read
int dashboardGroups[NDASHBOARDGROUPS] = {2};                    //dashboard groups to read
//int absGroups[NDASHBOARDGROUPS] = { 1 };                          //abs groups to read
//int airbagGroups[NDASHBOARDGROUPS] = { 1 };                       //airbag groups to read

//define engine as ECU, with ADR_Engine (address)
KWP_MODULE engine = {"ECU", ADR_Engine, engineGroups, NENGINEGROUPS};
KWP_MODULE dashboard = {"CLUSTER", ADR_Dashboard, dashboardGroups, NDASHBOARDGROUPS};
//KWP_MODULE _abs = {"ABS", ADR_ABS_Brakes, absGroups, NABSGROUPS};
//KWP_MODULE airbag = {"AIRBAG", ADR_Airbag, airbagGroups, NAIRBAGGROUPS};

//define modules
KWP_MODULE *modules[NMODULES] = { &engine, &dashboard };// &_abs, &airbag};
KWP_MODULE *currentModule = modules[0];

VW2002FISWriter fisWriter(FIS_CLK, FIS_DATA, FIS_ENA);
GetBootMessage getBootMessage;
GetButtonClick stalkPushUpButton(stalkPushUp, LOW, CLICKBTN_PULLUP);
GetButtonClick stalkPushDownButton(stalkPushDown, LOW, CLICKBTN_PULLUP);
GetButtonClick stalkPushResetButton(stalkPushReset, LOW, CLICKBTN_PULLUP);

bool ignitionState = false;         // variable for reading the ignition pin status
bool ignitionStateRunOnce = false;  // variable for reading the first run loop
bool removeOnce = true;             // variable for removing the screen only once
bool successfulConn = false;
bool successfulConnNoDrop = false;
bool fisDisable = false;            // is the FIS turned off?
bool fisBeenToggled = false;        // been toggled on/off?  Don't display welcome if!
bool isCustom = false;              // is the data block custom?
bool fetchedData = false;           // stop flickering when waiting on data

String GreetingMessage1 = ""; String GreetingMessage2 = ""; String GreetingMessage3 = ""; String GreetingMessage4 = "";
String GreetingMessage5 = ""; String GreetingMessage6 = "";  String GreetingMessage7 = ""; String GreetingMessage8 = "";
String definedUnitsPressure = "mbar";
String fisLine1; String fisLine2; String fisLine3; String fisLine4; String fisLine5; String fisLine6; String fisLine7; String fisLine8;

//currentSensor ALWAYS has to be 0 (if displaying all 4!)
//Change currentGroup for the group you'd like above!^
byte intCurrentModule = 0;
byte currentGroup = 0;
byte currentSensor = 0;
byte nSensors = 0;
byte nGroupsCustom = 1;
byte nGroupsCustomCount = 1;
byte maxSensors = 4;
byte maxAttemptsModule = 3;
byte maxAttemptsCountModule = 1;
byte lastGroup = 0;                  // remember the last group when toggling from custom
byte lastRefreshTime;               // capture the last time the data was refreshed
int refreshTime = 50;              // time for refresh in ms.  100 works.  250 works.  50 works.  1?
byte delayTime = 5;
int startTime = 10000;            // start up time = 1 minute

void setup()
{
  //Initialise basic varaibles
  Serial.begin(115200);
  pinMode(ignitionMonitorPin, INPUT);
  pinMode(stalkPushUpReturn, OUTPUT); pinMode(stalkPushDownReturn, OUTPUT); pinMode(stalkPushResetReturn, OUTPUT);

  //Cleanup, even remove last drawn object if not already/disconnect from module too.
  ignitionStateRunOnce = false;
  maxAttemptsCountModule = 1;
  intCurrentModule = 0;
  currentGroup = 0;
  lastGroup = 0;
  removeOnce = true;
  successfulConn = false;
  successfulConnNoDrop = false;
  fisBeenToggled = false;
  fetchedData = false;
  kwp.disconnect();
  fisWriter.remove_graphic();

  long current_millis = (long)millis();
  long lastRefreshTime = current_millis;

  //define button stuff
  stalkPushUpButton.debounceTime   = 20;   // Debounce timer in ms
  stalkPushUpButton.multiclickTime = 350;  // Time limit for multi clicks
  stalkPushUpButton.longClickTime  = 2500; // time until "held-down clicks" register

  stalkPushDownButton.debounceTime   = 20;   // Debounce timer in ms
  stalkPushDownButton.multiclickTime = 350;  // Time limit for multi clicks
  stalkPushDownButton.longClickTime  = 2500; // time until "held-down clicks" register

  stalkPushResetButton.debounceTime   = 20;   // Debounce timer in ms
  stalkPushResetButton.multiclickTime = 350;  // Time limit for multi clicks
  stalkPushResetButton.longClickTime  = 2500; // time until "held-down clicks" register
}

void loop()
{
  //Check to see the current state of the digital pins (monitor voltage for ign, stalk press)
  ignitionState = digitalRead(ignitionMonitorPin);
  stalkPushDownButton.Update();
  stalkPushUpButton.Update();
  stalkPushResetButton.Update();

  //refresh the current "current_millis" (for refresh rate!)
  long current_millis = (long)millis();

  //update the Reset button (see if it's been clicked more than 2 times (there 3+)
  //if it has, toggle fisDisable to turn off/on the screen
  if (stalkPushResetButton.clicks == -1)
  {
    fisDisable = !fisDisable;   //flip-flop disDisable

    //set disBeenToggled to true to stop displaying the "good morning" on turn-on (only this session)
    fisBeenToggled = false;
    removeOnce = false;
    successfulConn = false;
    successfulConnNoDrop = false;
    fetchedData = false;
    maxAttemptsCountModule = 1;
    kwp.disconnect();
    fisWriter.remove_graphic();
    return;
  }

  //check the ign goes dead, prep the variables for the next run
  if (ignitionState == LOW)
  {
    ignitionStateRunOnce = false;
    removeOnce = false;
    successfulConn = false;
    successfulConnNoDrop = false;
    fisBeenToggled = false;
    fetchedData = false;
    maxAttemptsCountModule = 1;
    kwp.disconnect();
    fisWriter.remove_graphic();
  }

  //If the ignition is currently "on" then work out the message
  if (ignitionState == HIGH && !ignitionStateRunOnce) //&& !fisBeenToggled)
  {
    Serial.println("Return & Display Boot Message");
    getBootMessage.returnBootMessage();         //find out the boot message
    //getBootMessage.displayBootImage();        //display the boot message
    getBootMessage.displayBootMessage();    //TODO: build graph support!

    ignitionStateRunOnce = true;            //set it's been ran, to stop redisplay of welcome message until ign. off.
  }

  //if the screen isn't disabled, carry on
  if (!fisDisable && ignitionState == HIGH)
  {
    if (stalkPushDownButton.clicks == -1)   //see if the down button has been pressed.  If it's been held, it will return -1
    {
      //if the down button has been held, prepare to swap groups!
      //use return; to get the loop to refresh (pointers need refreshing!)
      currentGroup = 0;
      successfulConn = false;
      successfulConnNoDrop = false;
      maxAttemptsCountModule = 1;
      fisLine1 = ""; fisLine2 = ""; fisLine3 = ""; fisLine4 = ""; fisLine5 = ""; fisLine6 = ""; fisLine7 = ""; fisLine8 = ""; //empty the lines from previous
      fetchedData = false;

      if ((intCurrentModule + 1) > (NMODULES - 1))        //if the next module causes a rollover, start again
      {
        currentModule = modules[0];                       //reset the currentModule to 0
        intCurrentModule = 0;                             //reset the intCurrentModule to 0 too
        kwp.disconnect();                                 //disconnect kwp
        fisWriter.init_graphic();                         //clear the screen
        return;                                           //return (to bounce back to the start.  Issues otherwise).
      }
      else
      {
        //rollover isn't a thing, so go to the next module.  Needs a "return" to allow reset of module
        intCurrentModule++;
        currentModule = modules[intCurrentModule];
        kwp.disconnect();
        fisWriter.init_graphic();
        return;
      }
    }

    //if the previous module causes a rollover, start again
    if (stalkPushDownButton.clicks > 0)
    {
      if ((currentGroup + 1) > (NENGINEGROUPS - 1))
      {
        currentGroup = 0;
        fisLine1 = ""; fisLine2 = ""; fisLine3 = ""; fisLine4 = ""; fisLine5 = ""; fisLine6 = ""; fisLine7 = ""; fisLine8 = ""; //empty the lines from previous
        fetchedData = false;
        fisWriter.init_graphic();
        return;
      }
      else
      {
        //rollover isn't a thing, so go to the next module.  Needs a "return" to allow reset of module
        currentGroup++;
        fisLine1 = ""; fisLine2 = ""; fisLine3 = ""; fisLine4 = ""; fisLine5 = ""; fisLine6 = ""; fisLine7 = ""; fisLine8 = ""; //empty the lines from previous
        fetchedData = false;
        fisWriter.init_graphic();
        return;
      }
    }

    //check to see if "up" is held.  If it is, toggle "isCustom"
    if (stalkPushUpButton.clicks == -1)
    {
      isCustom = !isCustom; //toggle isCustom
      if (isCustom == true)
      {
        lastGroup = currentGroup;
      }
      else
      {
        currentGroup = lastGroup;
      }

      fisLine1 = ""; fisLine2 = ""; fisLine3 = ""; fisLine4 = ""; fisLine5 = ""; fisLine6 = ""; fisLine7 = ""; fisLine8 = ""; //empty the lines from previous
      nGroupsCustom = 1;
      fetchedData = false;
      fisWriter.init_graphic();
      return;
    }

    if (stalkPushUpButton.clicks > 0)
    {
      if ((currentGroup - 1) < 0)
      {
        currentGroup = NENGINEGROUPS - 1;
        fisLine1 = ""; fisLine2 = ""; fisLine3 = ""; fisLine4 = ""; fisLine5 = ""; fisLine6 = ""; fisLine7 = ""; fisLine8 = ""; //empty the lines from previous
        fetchedData = false;
        fisWriter.init_graphic();
        return;
      }
      else
      {
        currentGroup--;
        fisLine1 = ""; fisLine2 = ""; fisLine3 = ""; fisLine4 = ""; fisLine5 = ""; fisLine6 = ""; fisLine7 = ""; fisLine8 = ""; //empty the lines from previous
        fetchedData = false;
        fisWriter.init_graphic();
        return;
      }
    }

    //if KWP isn't connected, ign is live and the welcome message has just been displayed
    if (!kwp.isConnected())
    {
      if ((maxAttemptsCountModule > maxAttemptsModule) && ignitionStateRunOnce)
      {
        fisWriter.remove_graphic();
      }

      if ((maxAttemptsCountModule <= maxAttemptsModule) && ignitionStateRunOnce)
      {
        //Reconnect quietly if already connected (don't show that you're reconnecting!)
        if (!successfulConnNoDrop)
        {
          fisWriter.FIS_init(); delay(delayTime);
          fisWriter.init_graphic(); delay(delayTime);

          fisWriter.write_text_full(0, 24, "CONNECTING TO:"); delay(delayTime);
          fisWriter.write_text_full(0, 32, currentModule->name); delay(delayTime);
          fisWriter.write_text_full(0, 48, "ATTEMPT " + String(maxAttemptsCountModule) + "/" + String(maxAttemptsModule)); delay(delayTime);
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
        fisWriter.write_text_full(0, 24, "CONNECTED TO:"); delay(5);
        fisWriter.write_text_full(0, 32, currentModule->name); delay(5);
        delay(500);
        successfulConnNoDrop = true;
      }

      //only refresh every "refreshTime" ms (stock is 500ms)
      if ((current_millis - lastRefreshTime) > refreshTime)
      {
        //grab data for the currentGroup.
        SENSOR resultBlock[maxSensors];
        nSensors = kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);

        //if the data is in a stock group (!isCustom), then fill out as normal
        if (!isCustom)
        {
          //stop "slow" fetching of data.  Remember that it refreshes variables every 500ms, so the first "grab" grabs each block every 500ms.  SLOW!
          //this makes sure that ALL the data is captured before displaying it.  Each "length" must have a value, so /8 > 1 will catch all
          if ((fisLine3.length() > 1) && (fisLine4.length() > 1) && (fisLine5.length() > 1) && (fisLine6.length() > 1) && fetchedData == false)
          {
            fetchedData = true;
            fisWriter.init_graphic();
          }

          if ((fisLine3.length() > 1) && (fisLine4.length() > 1) && (fisLine5.length() > 1) && (fisLine6.length() > 1) && fetchedData == true)
          {
            fisWriter.write_text_full(0, 8, "BLOCK " + String(engineGroups[currentGroup]));
            fisWriter.write_text_full(0, 16, resultBlock[currentSensor].desc);
            //only write lines 3, 4, 5, 6 (middle of screen) for neatness
          
            fisWriter.write_text_full(0, 40, fisLine3); delay(delayTime);
            fisWriter.write_text_full(0, 48, fisLine4); delay(delayTime);
            fisWriter.write_text_full(0, 56, fisLine5); delay(delayTime);
            fisWriter.write_text_full(0, 64, fisLine6); delay(delayTime);
          }
          else
          {
            fisWriter.write_text_full(0, 48, "FETCHING DATA"); delay(delayTime);
            fisWriter.write_text_full(0, 56, "PLEASE WAIT..."); delay(delayTime);
          }

          if (resultBlock[0].value != "") {
            fisLine3 = resultBlock[0].value + " " + resultBlock[0].units;
          } else {
            fisLine3 = "  " ;
          }

          if (resultBlock[1].value != "") {
            fisLine4 = resultBlock[1].value + " " + resultBlock[1].units;
          } else {
            fisLine4 = "  " ;
          }

          if (resultBlock[2].value != "") {
            fisLine5 = resultBlock[2].value + " " + resultBlock[2].units;
          } else {
            fisLine5 = "  " ;
          }

          if (resultBlock[3].value != "") {
            fisLine6 = resultBlock[3].value + " " + resultBlock[3].units;
          } else {
            fisLine6 = "  " ;
          }
          lastRefreshTime = current_millis;
          nGroupsCustom = 0;
        }
        else
        {
          //if isCustom, then same idea, only use ALL the lines
          //only populating Line 2, 3, 4, 7, 8, so wait until ALL lines are filled before displaying.  Looks cleaner
          if ((fisLine2.length() > 1) && (fisLine4.length() > 1) && (fisLine5.length() > 1) && (fisLine6.length() > 1) && (fisLine7.length() > 1) && (fisLine8.length() > 1) && fetchedData == false)
          {
            fetchedData = true;
            fisWriter.init_graphic();
          }

          if ((fisLine2.length() > 1) && (fisLine4.length() > 1) && (fisLine5.length() > 1) && (fisLine6.length() > 1) && (fisLine7.length() > 1) && (fisLine8.length() > 1) && fetchedData == true)
          {
            fisWriter.write_text_full(0, 8, "CUSTOM BLOCK"); delay(delayTime);
            fisWriter.write_text_full(0, 16, "AIT & KNOCK"); delay(delayTime);
            fisWriter.write_text_full(0, 32, fisLine2); delay(delayTime);
            fisWriter.write_text_full(0, 48, fisLine4); delay(delayTime);
            fisWriter.write_text_full(0, 56, fisLine5); delay(delayTime);
            fisWriter.write_text_full(0, 64, fisLine6); delay(delayTime);
            fisWriter.write_text_full(0, 72, fisLine7); delay(delayTime);
            fisWriter.write_text_full(0, 80, fisLine8); delay(delayTime);
          }
          else
          {
            fisWriter.write_text_full(0, 48, "FETCHING DATA"); delay(delayTime);
            fisWriter.write_text_full(0, 56, "PLEASE WAIT..."); delay(delayTime);
            //fisWriter.init_graphic();
          }

          //first run will be case 0 (since nGroupsCustom = 1).  Prepare for the NEXT run (which will be group 2 (AIT/timing)).
          switch (nGroupsCustom - 1)
          {
            //                                   0  1   2   3   4   5   6   7   8   9,  10, 11, 12, 13, 14, 15,  16,  17,  18
            //int engineGroups[NENGINEGROUPS] = {2, 3, 4, 5, 6, 10, 11, 14, 15, 16, 20, 31, 32, 90, 91, 92, 113, 114, 115, 118}; 
            case 0:
              nGroupsCustom++;
              currentGroup = 6;
              lastRefreshTime = current_millis;
              return;
            case 1:
              //this is the data from the ABOVE group (group 2).  Prepare for the NEXT group
              fisLine2 = "AIT: " + resultBlock[2].value + " " + resultBlock[2].units; //Group 11, Block 3, AIT
              fisLine4 = resultBlock[3].value + " " + resultBlock[3].units; //Group 11, Block 4, Timing Angle
              nGroupsCustom++;
              currentGroup = 10;
              lastRefreshTime = current_millis;
              return;
            case 2:
              //this is the data from the ABOVE group (group 4).  Prepare for the NEXT group
              fisLine5 = resultBlock[0].value + " " + resultBlock[0].units; //Group 20, Block 1, Timing Retard (Cyl 1)
              fisLine6 = resultBlock[1].value + " " + resultBlock[1].units; //Group 20, Block 1, Timing Retard (Cyl 2)

              fisLine7 = resultBlock[2].value + " " + resultBlock[2].units; //Group 20, Block 1, Timing Retard (Cyl 3)
              fisLine8 = resultBlock[3].value + " " + resultBlock[3].units; //Group 20, Block 1, Timing Retard (Cyl 4)

              nGroupsCustom = 2;
              currentGroup = 6;
              lastRefreshTime = current_millis;
              return;
          }
        }
      }
    }
  }
  else
  {
    //If FIS Disabled:  Minic the stalk buttons!
    removeOnce = false;
    successfulConn = false;
    successfulConnNoDrop = false;
    fisBeenToggled = false;
    fetchedData = false;
    maxAttemptsCountModule = 1;
    kwp.disconnect();
    fisWriter.remove_graphic();

    //Force HIGH on all the return pins
    Serial.println("Disabled.  Force pins HIGH");
    digitalWrite(stalkPushUpReturn, HIGH);
    digitalWrite(stalkPushDownReturn, HIGH);
    digitalWrite(stalkPushResetReturn, HIGH);

    //if the Pins are held LOW (therefore, false), hold the corresponding pin LOW, too.
    if (!digitalRead(stalkPushDownMonitor))
    {
      Serial.println("Disabled.  Force Down LOW");
      for (int i = 0; i <= 1500; i++) {
        if (digitalRead(stalkPushDownMonitor)) {
          i = 1700;
        }
        else {
          Serial.println("Down LOW");
          digitalWrite(stalkPushDownReturn, LOW);
        }
        delay(1);
      }
    }

    if (!digitalRead(stalkPushUpMonitor))
    {
      Serial.println("Disabled.  Force Up LOW");
      for (int i = 0; i <= 1500; i++) {
        if (digitalRead(stalkPushUpMonitor)) {
          i = 1700;
        }
        else {
          Serial.println("Up LOW");
          digitalWrite(stalkPushUpReturn, LOW);
        }
        delay(1);
      }
    }

    if (!digitalRead(stalkPushResetMonitor))
    {
      Serial.println("Disabled.  Force Reset LOW");
      //read the reset pin.  If it's held for more than 1700 ms, jump
      for (int i = 0; i <= 1500; i++) {
        if (digitalRead(stalkPushResetMonitor)) {
          i = 1700;
        }
        else {
          Serial.println("Reset LOW");
          digitalWrite(stalkPushResetReturn, LOW);
        }
        delay(1);
      }
    }
  }
}

