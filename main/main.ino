#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h> // library for controling Radio frequency switch
#include <ESPiLight.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_TRACE
#endif

#include <ArduinoLog.h>

float frequencies[] = {303.79, 315.026, 433.92};

byte FSCAL1[3];
byte FSCAL2[3];
byte FSCAL3[3];

int start_freq = 0;
int stop_freq = 2;

int freq = start_freq;
int compare_freq;
int rssi;
int mark_rssi = -100;

bool receiveMode = false;
unsigned long receiveEnd = millis();

RCSwitch rcSwitch = RCSwitch();
ESPiLight piLight(-1); // use -1 to disable transmitter

byte radioState = 0;

static inline void safeDelayMicroseconds(unsigned long duration)
{
  // if delay > 10 milliseconds, use yield() to avoid wdt reset
  unsigned long start = micros();
  // Log.notice(F("Radio State: %d" CR), radioState);
  while ((micros() - start) < duration)
  {
    if (radioState != ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE))
    {
      Log.notice(F("Time: %d" CR), micros() - start);
      radioState = ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE);
      Log.notice(F("Radio State: %d" CR), radioState);
    }

    yield();
  }
}

void waitForRadioReady()
{
  while (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) != 13)
  {
    yield();
  }
}

void pilightCallback(const String &protocol, const String &message, int status,
                     size_t repeats, const String &deviceID)
{
  if (status == VALID)
  {
    Log.notice(F("time: %d" CR), millis() / 1000);
    Log.notice(F("value: %s" CR), (char *)message.c_str());
    Log.notice(F("protocol: %s" CR), (char *)protocol.c_str());
    Log.notice(F("length: %s" CR), (char *)deviceID.c_str());
//     Log.notice(F("Freq: %F" CR), frequencies[freq]);
    Log.notice(F("RSSI: %d" CR), ELECHOUSE_cc1101.getRssi());
    Log.notice(F("Time: %d" CR), receiveEnd - millis());
  }
}

void calibrate()
{
  for (int freq = 0; freq <= 2; freq++)
  {
    ELECHOUSE_cc1101.SetRx(frequencies[freq]);
    waitForRadioReady();
    safeDelayMicroseconds(300);
    FSCAL1[freq] = ELECHOUSE_cc1101.SpiReadStatus(CC1101_FSCAL1);
    FSCAL2[freq] = ELECHOUSE_cc1101.SpiReadStatus(CC1101_FSCAL2);
    FSCAL3[freq] = ELECHOUSE_cc1101.SpiReadStatus(CC1101_FSCAL3);
    Log.notice(F("Calibrated: %F" CR), frequencies[freq]);
    Log.notice(F("FSCAL1: %d" CR), FSCAL1[freq]);
    Log.notice(F("FSCAL2: %d" CR), FSCAL2[freq]);
    Log.notice(F("FSCAL3: %d" CR), FSCAL3[freq]);
  }
}

void setMHZ(float mhz)
{
  byte freq2 = 0;
  byte freq1 = 0;
  byte freq0 = 0;

  float MHz = mhz;

  for (bool i = 0; i == 0;)
  {
    if (mhz >= 26)
    {
      mhz -= 26;
      freq2 += 1;
    }
    else if (mhz >= 0.1015625)
    {
      mhz -= 0.1015625;
      freq1 += 1;
    }
    else if (mhz >= 0.00039675)
    {
      mhz -= 0.00039675;
      freq0 += 1;
    }
    else
    {
      i = 1;
    }
  }
  if (freq0 > 255)
  {
    freq1 += 1;
    freq0 -= 256;
  }

  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ2, freq2);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ1, freq1);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ0, freq0);

  // Calibrate();
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

#ifdef HOPPING
  calibrate();
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_MCSM0, ELECHOUSE_cc1101.SpiReadReg(CC1101_MCSM0) & 0x0F);
#endif
}

void loop()
{

  if (!receiveMode)
  {
    unsigned long start = micros();
#ifdef HOPPING
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    setMHZ(frequencies[freq]);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCAL1, FSCAL1[freq]);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCAL2, FSCAL2[freq]);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCAL3, FSCAL3[freq]);
#else
    ELECHOUSE_cc1101.SetRx(frequencies[freq]);
#endif
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SRX);
    waitForRadioReady();
    safeDelayMicroseconds(200);
    rssi = ELECHOUSE_cc1101.getRssi();

    if (rssi > -70)
    {

      Serial.println();
      Log.notice(F("Freq: %F" CR), frequencies[freq]);
      Log.notice(F("RSSI: %d" CR), ELECHOUSE_cc1101.getRssi());

      switch (freq)
      {
      case 0: // RC Switch
      case 1: // RC Switch
        Log.notice(F("RC Switch Enabled: %d" CR), RF_RECEIVER_GPIO);
        rcSwitch.enableReceive(RF_RECEIVER_GPIO);
        receiveMode = true;
        receiveEnd = millis() + 300; // Max signal length
        break;
      case 2:
        Log.notice(F("PiLight Enabled: %d" CR), RF_RECEIVER_GPIO);
        piLight.enableReceiver();
        piLight.initReceiver(RF_RECEIVER_GPIO);
        receiveMode = true;
        receiveEnd = millis() + 300; // Max signal length
        break;
      default:
        Log.notice(F("Unhandled Frequency: %d %f" CR), freq, frequencies[freq]);
        break;
      }
    }
    else
    {
      freq += 1;

      if (freq > stop_freq)
      {
        // Serial.println();
        freq = start_freq;
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
    Log.notice(F("time: %d" CR), millis() / 1000);
    Log.notice(F("value: %u" CR), (unsigned long)rcSwitch.getReceivedValue());
    Log.notice(F("protocol: %d" CR), (int)rcSwitch.getReceivedProtocol());
    Log.notice(F("length: %d" CR), (int)rcSwitch.getReceivedBitlength());
    // Log.notice(F("delay: %d" CR), (int)rcSwitch.getReceivedDelay());
    // Log.notice(F("Freq: %F" CR), frequencies[freq]);
    Log.notice(F("RSSI: %d" CR), ELECHOUSE_cc1101.getRssi());
    Log.notice(F("Time: %d" CR), receiveEnd - millis());

    rcSwitch.resetAvailable();
    rcSwitch.disableReceive();
    receiveMode = false;
  }
}
