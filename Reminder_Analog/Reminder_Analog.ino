/*

  Copyright <2018> <Andreas Spiess>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.


  Based on:

    Google Calendar Integration ESP8266
    Created by Daniel Willi, 2016

    Based on the WifiClientSecure example by
    Ivan Grokhotkov

*/

// from LarryD, Arduino forum
#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG    //Macros are usually in all capital letters.
#define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "HTTPSRedirect.h"
//#include <credentials.h>

Ticker blinker;

//Connection Settings
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort = 443;

unsigned long entryCalender, entryPrintStatus, entrySwitchPressed, heartBeatEntry, heartBeatLedEntry;
String url;

unsigned long intEntry;


#define UPDATETIME 20000

#ifdef CREDENTIALS
const char* ssid = mySSID;
const char* password = myPASSWORD;
const char *GScriptIdRead = GoogleScriptIdRead;
const char *GScriptIdWrite = GoogleScriptIdWrite;
#else
//Network credentials
const char* ssid = "........."; //replace with you ssid
const char* password = ".........."; //replace with your password
//Google Script ID
const char *GScriptIdRead = "............"; //replace with you gscript id for reading the calendar
const char *GScriptIdWrite = "..........."; //replace with you gscript id for writing the calendar
#endif

#define NBR_EVENTS 8
String  possibleEvents[NBR_EVENTS] = {"Cat", "Paper", "Cardboard", "Green",  "Laundry", "Fitness", "Meal", "XXX"};
//String  possibleEvents[NBR_EVENTS] = {"Cat", "Paper", "Green", "Cardboard"};
byte  LEDpins[NBR_EVENTS]    = {D0, D1, D2, D4, D5, D6, D7, D8};
bool switchPressed[NBR_EVENTS];
unsigned long switchOff = 0;
boolean beat = false;
int beatLED = 0;

enum taskStatus {
  none,
  due,
  done
};

taskStatus taskStatus[NBR_EVENTS];

String calendarData = "";
bool calenderUpToDate;

//Connect to wifi
void connectToWifi() {
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create TLS connection
  HTTPSRedirect client(httpsPort);

  Serial.print("Connecting to ");
  Serial.println(host);
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client.connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else  Serial.println("Connection failed. Retrying...");
    if (!flag) {
      Serial.print("Could not connect to server: ");
      Serial.println(host);
      Serial.println("Exiting...");
      ESP.reset();
    }
  }
  Serial.println("Connected to Google");
}

void printStatus() {
  for (int i = 0; i < NBR_EVENTS; i++) {
    Serial.print("Task ");
    Serial.print(i);
    Serial.print(" Status ");
    Serial.println(taskStatus[i]);
  }
  Serial.println("----------");
}

void getCalendar() {
  //  Serial.println("Start Request");
  HTTPSRedirect client(httpsPort);
  unsigned long getCalenderEntry = millis();
  while (!client.connected() && millis() < getCalenderEntry + 8000) {
    Serial.print(".");
    client.connect(host, httpsPort);
  }

  //Fetch Google Calendar events
  url = String("/macros/s/") + GScriptIdRead + "/exec";
  calendarData = client.getData(url, host, googleRedirHost);
  Serial.print("Calendar Data---> ");
  Serial.println(calendarData);
  calenderUpToDate = true;
  yield();
}

void createEvent(String title) {
  // Serial.println("Start Write Request");
  HTTPSRedirect client(httpsPort);
  if (!client.connected()) client.connect(host, httpsPort);
  // Create event on Google Calendar
  url = String("/macros/s/") + GScriptIdWrite + "/exec" + "?title=" + title;
  client.getData(url, host, googleRedirHost);
  //  Serial.println(url);
  Serial.println("Write Event created ");
  calenderUpToDate = false;
}

void manageStatus() {
  for (int i = 0; i < NBR_EVENTS; i++) {
    switch (taskStatus[i]) {
      case none:
        if (switchPressed[i]) {
          digitalWrite(LEDpins[i], HIGH);
          while (!calenderUpToDate) getCalendar();
          if (!eventHere(i)) createEvent(possibleEvents[i]);
          Serial.print(i);
          Serial.println(" 0-->1");
          //getCalendar();
          taskStatus[i] = due;
        } else {
          if (eventHere(i)) {
            digitalWrite(LEDpins[i], HIGH);
            Serial.print(i);
            Serial.println(" 0-->1");
            taskStatus[i] = due;
          }
        }
        break;
      case due:
        if (switchPressed[i]) {
          digitalWrite(LEDpins[i], LOW);
          Serial.print(i);
          Serial.println(" 1-->2");
          taskStatus[i] = done;
        }
        break;
      case done:
        if (calenderUpToDate && !eventHere(i)) {
          digitalWrite(LEDpins[i], LOW);
          Serial.print(i);
          Serial.println(" 2-->0");
          taskStatus[i] = none;
        }
        break;
      default:
        break;
    }
    switchPressed[i] = false;
  }
  yield();
}

bool eventHere(int task) {
  if (calendarData.indexOf(possibleEvents[task], 0) >= 0 ) {
    //    Serial.print("Task found ");
    //    Serial.println(task);
    return true;
  } else {
    //   Serial.print("Task not found ");
    //   Serial.println(task);
    return false;
  }
}

void handleInterrupt() {
  intEntry = millis();
  int reading = analogRead(A0);
  float hi = (reading / (1024.0 / NBR_EVENTS) - 0.5);

  if (hi >= 0) {
    if (switchOff > 10) {
      int button = (int)hi;
      switchPressed[button] = true;
    }
    switchOff = 0;
  }
  else switchOff++;  // no switch pressed
}

void setup() {
  Serial.begin(115200);
  blinker.attach_ms(50, handleInterrupt);
  //   attachInterrupt(digitalPinToInterrupt(switchPins[i]), handleInterrupt, FALLING);
  connectToWifi();
  for (int i = 0; i < NBR_EVENTS; i++) {
    pinMode(LEDpins[i], OUTPUT);
    taskStatus[i] = none;  // Reset all LEDs
    switchPressed[i] = false;
  }
  getCalendar();
  entryCalender = millis();
  pinMode(D0, OUTPUT);
}


void loop() {
  if (millis() > entryCalender + UPDATETIME) {
    getCalendar();
    entryCalender = millis();
  }
  manageStatus();
  if (millis() > entryPrintStatus + 5000) {
    printStatus();
    entryPrintStatus = millis();
  }

  if (millis() > heartBeatEntry + 30000) {
    beat = true;
    heartBeatEntry = millis();
  }
  heartBeat();
}

void heartBeat() {
  if (beat) {
    if ( millis() > heartBeatLedEntry + 100) {
      heartBeatLedEntry = millis();
      if (beatLED < NBR_EVENTS) {

        if (beatLED > 0) digitalWrite(LEDpins[beatLED - 1], LOW);
        digitalWrite(LEDpins[beatLED], HIGH);
        beatLED++;
      }
      else {
        for (int i = 0; i < NBR_EVENTS; i++) {
          if (taskStatus[i] == due) digitalWrite(LEDpins[i], HIGH);
          else digitalWrite(LEDpins[i], LOW);
        }
        beatLED = 0;
        beat = false;
      }
    }
  }
}



