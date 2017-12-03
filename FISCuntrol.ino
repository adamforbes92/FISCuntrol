//Include FIS Writer, TimeLib and KWP
#include "VW2002FISWriter.h"
#include "KWP.h"
#include "Wire.h"
#include "GetBootMessage.h"

//Define MAX Retries
#define NENGINEGROUPS 8
#define NDASHBOARDGROUPS 1

#define NMODULES 2

// KWP.  RX = Pin 2, TX = Pin 3
#define pinKLineRX 2
#define pinKLineTX 3
KWP kwp(pinKLineRX, pinKLineTX);

const int StalkPushUp = 4;
const int StalkPushDown = 5;
const int StalkPushReset = 6;

//Define ignition live sense as Pin 7
const int IgnitionMonitorPin = 7;

// FIS
#define FIS_CLK 13  // - Arduino 13 - PB5
#define FIS_DATA 11 // - Arduino 11 - PB3
#define FIS_ENA 8 // - Arduino 8 - PB0
VW2002FISWriter fisWriter( FIS_CLK, FIS_DATA, FIS_ENA );
GetBootMessage getBootMessage;
GetBootMessage displayBootMessage;

int IgnitionState = 0;         // variable for reading the ignition pin status
int IgnitionStateRunOnce = 0;

int StalkPushUpState = 0;         // variable for reading the pushbutton status
int StalkPushDownState = 0;         // variable for reading the pushbutton status
int StalkPushResetState = 0;         // variable for reading the pushbutton status

int CurrentHour = 0;
String GreetingMessage1 = "";
String GreetingMessage2 = "";

//define engine groups
//**********************************0, 1,  2, 3,   4,   5,  6)
//Block 3, 5 don't work?
int engineGroups[NENGINEGROUPS] = { 2, 3, 20, 31, 32, 118, 115, 15 };
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

int removeOnce = 1;
int count = 0;
int successfulConn = 0;
int successfulConnNoDrop = 0;

void setup() {
  //Serial.begin(9600);
  Serial.begin(9600);
  pinMode(IgnitionMonitorPin, INPUT);
  pinMode(StalkPushUp, INPUT);
  pinMode(StalkPushDown, INPUT);
  pinMode(StalkPushReset, INPUT);

  IgnitionStateRunOnce = 0;
  maxAttemptsCountModule = 1;
  removeOnce = 1;
  successfulConn = 0;
  successfulConnNoDrop = 0;
  kwp.disconnect();
  fisWriter.remove_graphic();
}

void loop() {
  //Failsafe incase time can't be calculated
  GreetingMessage1 = "WELCOME";
  GreetingMessage2 = "ADAM!";

  //KWP_MODULE engine = {"ECU", ADR_Engine, engineGroups, NENGINEGROUPS};
  //Check to see the current state of the digital pins (monitor voltage for ign, stalk press)
  IgnitionState = digitalRead(IgnitionMonitorPin);

  StalkPushUpState = digitalRead(StalkPushUp);
  StalkPushDownState = digitalRead(StalkPushDown);
  StalkPushResetState = digitalRead(StalkPushReset);

  //check the ign goes dead, prep the variables for the next run
  if (IgnitionState == 0)
  {
    Serial.println("Disconnecting, cleaning up...");
    IgnitionStateRunOnce = 0;
    maxAttemptsCountModule = 1;
    removeOnce = 0;
    successfulConn = 0;
    successfulConnNoDrop = 0;
    kwp.disconnect();
    fisWriter.remove_graphic();
  }

  //If the ignition is currently "on" then work out the message
  if (IgnitionState == 1 && IgnitionStateRunOnce == 0)
  {
    getBootMessage.returnBootMsg();
    //getBootMessage.displayBootMsg();

    Serial.println("Display welcome message");
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

    IgnitionStateRunOnce = 1;
  }

  //if KWP isn't connected, ign is live and the welcome message has just been displayed
  //if ((!kwp.isConnected()) && (IgnitionState == 1) && (IgnitionStateRunOnce == 1))
  if (!kwp.isConnected())
  {
    //Serial.println("Not connected!");
    if ((maxAttemptsCountModule > maxAttemptsModule) && IgnitionStateRunOnce == 1)
    {
      fisWriter.remove_graphic();
    }

    if ((maxAttemptsCountModule <= maxAttemptsModule) && IgnitionStateRunOnce == 1)
    {
      //Reconnect quietly if already connected (don't show that you're reconnecting!)
      if (successfulConnNoDrop != 1)
      {
        Serial.println("count++");
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
    Serial.println("Success" + String(successfulConn));

    //If the first connection was successful, don't show reconnects!
    if (successfulConnNoDrop != 1)
    {
      fisWriter.init_graphic();
      delay(5);

      fisWriter.write_text_full(0, 24, "CONNECTED TO:");
      fisWriter.write_text_full(0, 32, currentModule->name);
      delay(500);
      fisWriter.init_graphic();
      delay(1);
      successfulConnNoDrop = 1;
    }

    SENSOR resultBlock[maxSensors];
    nSensors = kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);

    //int KWP::readBlock(uint8_t addr, int group, int maxSensorsPerBlock, SENSOR resGroupSensor[]) {
    if (resultBlock[currentSensor].value != "")
    {
      //fisWriter.init_graphic();
      //delay(1);

      fisWriter.write_text_full(0, 8, "BLOCK " + String(engineGroups[currentGroup]));
      fisWriter.write_text_full(0, 16, resultBlock[currentSensor].desc);

      fisWriter.write_text_full(0, 40, resultBlock[0].value + " " + resultBlock[0].units);
      fisWriter.write_text_full(0, 48, resultBlock[1].value + " " + resultBlock[1].units);
      fisWriter.write_text_full(0, 56, resultBlock[2].value + " " + resultBlock[2].units);
      fisWriter.write_text_full(0, 64, resultBlock[3].value + " " + resultBlock[3].units);

      //fisWriter.sendMsg("line1 text", "line2 text", 1);

      //LCD.showText(resultBlock[currentSensor].desc, resultBlock[currentSensor].value + " " + resultBlock[currentSensor].units);

    }
  }
}
