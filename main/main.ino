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
unsigned long receiveEnd = millis();

RCSwitch rcSwitch = RCSwitch();
ESPiLight piLight(-1); // use -1 to disable transmitter

static inline void safeDelayMicroseconds(unsigned long duration)
{

  // if delay > 10 milliseconds, use yield() to avoid wdt reset
  unsigned long start = micros();
  while ((micros() - start) < duration)
  {
    yield();
  }
}

void pilightCallback(const String &protocol, const String &message, int status,
                     size_t repeats, const String &deviceID)
{
  if (status == VALID)
  {
    Log.trace(F("Creating RF PiLight buffer" CR));
    Log.notice(F("value: %s" CR), (char *)message.c_str());
    Log.notice(F("protocol: %s" CR), (char *)protocol.c_str());
    Log.notice(F("length: %s" CR), (char *)deviceID.c_str());
    Log.notice(F("Freq: %F" CR), frequencies[mark_freq]);
    Log.notice(F("RSSI: %d" CR), ELECHOUSE_cc1101.getRssi());
    Log.notice(F("Time: %d" CR), receiveEnd - millis());
  }
}

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial);
  ELECHOUSE_cc1101.Init();
  // ELECHOUSE_cc1101.setRxBW(58);
  ELECHOUSE_cc1101.SetRx(frequencies[freq]);
  pinMode(RF_EMITTER_GPIO, OUTPUT);
  rcSwitch.disableTransmit();
  piLight.setCallback(pilightCallback);

  Serial.println();
  Serial.println("Frequency Scanner Ready");
}

void loop()
{

  if (!receiveMode)
  {
    ELECHOUSE_cc1101.SetRx(frequencies[freq]);
    // Serial.print(ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE));
    // Serial.print("-");
    safeDelayMicroseconds(1300);
    rssi = ELECHOUSE_cc1101.getRssi();
    // Serial.print(ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE));
    // Serial.print(",");
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
          ELECHOUSE_cc1101.SetRx(frequencies[mark_freq]);
          rcSwitch.enableReceive(RF_RECEIVER_GPIO);
          receiveMode = true;
          receiveEnd = millis() + 800; // Max signal length
          break;
        case 2:
          Log.notice(F("PiLight Enabled: %d" CR), RF_RECEIVER_GPIO);
          // ELECHOUSE_cc1101.SetRx(frequencies[mark_freq]);
          piLight.enableReceiver();
          piLight.initReceiver(RF_RECEIVER_GPIO);
          receiveMode = true;
          receiveEnd = millis() + 800; // Max signal length
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
    if (millis() > receiveEnd)
    {
      receiveEnd = millis();
      receiveMode = false;
      Log.notice(F("Receive ended." CR));
      rcSwitch.disableReceive();
      piLight.initReceiver(-1);
      piLight.disableReceiver();
    }
  }

  piLight.loop();

  if (rcSwitch.available())
  {
    Log.trace(F("RF Task running on core :%d" CR), xPortGetCoreID());
    Log.notice(F("value: %u" CR), (unsigned long)rcSwitch.getReceivedValue());
    Log.notice(F("protocol: %d" CR), (int)rcSwitch.getReceivedProtocol());
    Log.notice(F("length: %d" CR), (int)rcSwitch.getReceivedBitlength());
    Log.notice(F("delay: %d" CR), (int)rcSwitch.getReceivedDelay());
    Log.notice(F("Freq: %F" CR), frequencies[mark_freq]);
    Log.notice(F("RSSI: %d" CR), ELECHOUSE_cc1101.getRssi());
      Log.notice(F("Time: %d" CR), receiveEnd - millis());

    rcSwitch.resetAvailable();
    rcSwitch.disableReceive();
    receiveMode = false;
  }
}
