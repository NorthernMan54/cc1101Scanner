#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h> // library for controling Radio frequency switch
#include <ESPiLight.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_TRACE
#endif

#include <ArduinoLog.h>

float frequencies[] = {303.79, 315.026, 433.92};

int start_freq = 0;
int stop_freq = 2;

int freq = start_freq;
int compare_freq;
int mark_freq;
int rssi;
int mark_rssi = -100;

bool receiveMode = false;
unsigned long receiveStart = millis();

RCSwitch mySwitch = RCSwitch();
ESPiLight rf(-1); // use -1 to disable transmitter

#define SIGNAL_SIZE_UL_ULL uint64_t

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial);
  ELECHOUSE_cc1101.Init();
  // ELECHOUSE_cc1101.setRxBW(58);
  ELECHOUSE_cc1101.SetRx(frequencies[freq]);
  pinMode(RF_EMITTER_GPIO, OUTPUT); 
  mySwitch.disableTransmit();
  Serial.println("Frequency Scanner Ready");
}

void loop()
{
  if (!receiveMode)
  {
    ELECHOUSE_cc1101.SetRx(frequencies[freq]);
    delay(2);
    rssi = ELECHOUSE_cc1101.getRssi();
    // Serial.print(ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE));
    // Serial.print(",");

    // Serial.print( rssi);
    // Serial.print( ",");

    if (rssi > -200)
    {
      if (rssi > mark_rssi)
      {
        // Serial.print(".");
        mark_rssi = rssi;
        mark_freq = freq;
      }
    }

    freq += 1;

    if (freq > stop_freq)
    {
      // Serial.println();
      freq = start_freq;

      if (mark_rssi > -70)
      {

        Serial.println();
        Serial.print("Freq: ");
        Serial.println(frequencies[mark_freq]);
        Serial.print("Rssi: ");
        Serial.println(mark_rssi);
        mark_rssi = -200;

        switch (mark_freq)
        {
        case 0: // RC Switch
        case 1: // RC Switch
          Log.notice(F("RC Switch Enabled: %d" CR), RF_RECEIVER_GPIO);
          mySwitch.enableReceive(RF_RECEIVER_GPIO);
          receiveMode = true;
          receiveStart = millis() + 1600; // Max signal length
          break;
        case 2:
          Log.notice(F("PiLight Enabled: %d" CR), RF_RECEIVER_GPIO);
          receiveMode = true;
          receiveStart = millis() + 800;  // Max signal length
          break;
        default:
          Log.notice(F("Unhandled Frequency: %d %f" CR), mark_freq, frequencies[mark_freq]);
          break;
        }
      }
    }
  }
  else
  {
    if (millis() > receiveStart )
    {
      receiveStart = millis();
      receiveMode = false;
      Log.notice(F("Receive ended." CR));
      mySwitch.disableReceive();
    }
  }

  if (mySwitch.available())
  {
    Log.trace(F("RF Task running on core :%d" CR), xPortGetCoreID());
    SIGNAL_SIZE_UL_ULL MQTTvalue = mySwitch.getReceivedValue();
    Log.notice(F("value :%s" CR), (SIGNAL_SIZE_UL_ULL)MQTTvalue);
    Log.notice(F("protocol :%d" CR), (int)mySwitch.getReceivedProtocol());
    Log.notice(F("length :%d" CR), (int)mySwitch.getReceivedBitlength());
    Log.notice(F("delay :%d" CR), (int)mySwitch.getReceivedDelay());
    Log.notice(F("Freq :%d" CR), frequencies[mark_freq]);
    mySwitch.resetAvailable();
    mySwitch.disableReceive();
    receiveMode = false;
  }
}