#include <ELECHOUSE_CC1101_SRC_DRV.h>

float frequencies [] = {303.79, 315.026, 433.92 };

int start_freq = 0;
int stop_freq = 2;

int freq = start_freq;
int compare_freq;
int mark_freq;
int rssi;
int mark_rssi = -100;

void setup()
{
  Serial.begin(115200);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setRxBW(58);
  ELECHOUSE_cc1101.SetRx(frequencies[freq]);
  Serial.println("Frequency Scanner Ready");
}

void loop()
{
  ELECHOUSE_cc1101.SetRx(frequencies[freq]);
  delay(2);
  // Serial.print(ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE));
  // Serial.print(",");
  rssi = ELECHOUSE_cc1101.getRssi();

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
        mark_rssi=-200;
  
    }
  }
}