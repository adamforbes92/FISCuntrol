/*    ClickButton

  Arduino library that decodes multiple clicks on one button.
  Also copes with long clicks and click-and-hold.

  Usage: ClickButton buttonObject(pin [LOW/HIGH, [CLICKBTN_PULLUP]]);

  where LOW/HIGH denotes active LOW or HIGH button (default is LOW)
  CLICKBTN_PULLUP is only possible with active low buttons.

  Returned click counts:
   A positive number denotes the number of (short) clicks after a released button
   A negative number denotes the number of "long" clicks

  NOTE!
  This is the OPPOSITE/negative of click codes from the last pre-2013 versions!
  (this seemed more logical and simpler, so I finally changed it)
*/

#include "GetButtonClick.h"

//StalkPushUpButton.debounceTime   = 100;   // Debounce timer in ms
//StalkPushUpButton.multiclickTime = 1500;  // Time limit for multi clicks
//StalkPushUpButton.longClickTime  = 5000; // time until "held-down clicks" register

GetButtonClick::GetButtonClick(uint8_t buttonPin)
{
  _pin           = buttonPin;
  _activeHigh    = LOW;           // Assume active-low button
  _btnState      = !_activeHigh;  // initial button state in active-high logic
  _lastState     = _btnState;
  _clickCount    = 0;
  clicks         = 0;
  depressed      = false;
  _lastBounceTime = 0;
  debounceTime   = 20;            // Debounce timer in ms
  multiclickTime = 1000;           // Time limit for multi clicks
  longClickTime  = 3500;          // time until long clicks register
  pinMode(_pin, INPUT);
}


GetButtonClick::GetButtonClick(uint8_t buttonPin, boolean activeType)
{
  _pin           = buttonPin;
  _activeHigh    = activeType;
  _btnState      = !_activeHigh;  // initial button state in active-high logic
  _lastState     = _btnState;
  _clickCount    = 0;
  clicks         = 0;
  depressed      = 0;
  _lastBounceTime = 0;
  debounceTime   = 20;            // Debounce timer in ms
  multiclickTime = 1000;           // Time limit for multi clicks
  longClickTime  = 3500;          // time until long clicks register
  pinMode(_pin, INPUT);
}

GetButtonClick::GetButtonClick(uint8_t buttonPin, boolean activeType, boolean internalPullup)
{
  _pin           = buttonPin;
  _activeHigh    = activeType;
  _btnState      = !_activeHigh;  // initial button state in active-high logic
  _lastState     = _btnState;
  _clickCount    = 0;
  clicks         = 0;
  depressed      = 0;
  _lastBounceTime = 0;
  debounceTime   = 20;            // Debounce timer in ms
  multiclickTime = 1000;           // Time limit for multi clicks
  longClickTime  = 3500;          // time until long clicks register
  pinMode(_pin, INPUT);
  // Turn on internal pullup resistor if applicable
  if (_activeHigh == LOW && internalPullup == CLICKBTN_PULLUP) digitalWrite(_pin, HIGH);
}

void GetButtonClick::Update()
{
  long now = (long)millis();      // get current time
  _btnState = digitalRead(_pin);  // current appearant button state

  // Make the button logic active-high in code
  if (!_activeHigh) _btnState = !_btnState;

  // If the switch changed, due to noise or a button press, reset the debounce timer
  if (_btnState != _lastState) _lastBounceTime = now;


  // debounce the button (Check if a stable, changed state has occured)
  if (now - _lastBounceTime > debounceTime && _btnState != depressed)
  {
    depressed = _btnState;
    if (depressed) _clickCount++;
  }

  // If the button released state is stable, report nr of clicks and start new cycle
  if (!depressed && (now - _lastBounceTime) > multiclickTime)
  {
    // positive count for released buttons
    clicks = _clickCount;
    _clickCount = 0;
  }

  // Check for "long click"
  if (depressed && (now - _lastBounceTime > longClickTime))
  {
    // negative count for long clicks
    clicks = 0 - _clickCount;
    _clickCount = 0;
  }

  _lastState = _btnState;
}

