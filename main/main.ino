#include <ELECHOUSE_CC1101_SRC_DRV.h>

float start_freq = 303;
float stop_freq = 305;

float freq = start_freq;
long compare_freq;
float mark_freq;
int rssi;
int mark_rssi = -100;

void setup()
{
  Serial.begin(115200);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setRxBW(58);
  ELECHOUSE_cc1101.SetRx(freq);
  Serial.println("Frequency Scanner Ready");
}

void loop()
{
  ELECHOUSE_cc1101.setMHZ(freq);
  rssi = ELECHOUSE_cc1101.getRssi();

  // Serial.print( rssi);
  // Serial.print( ",");

  if (rssi > -200)
  {
    if (rssi > mark_rssi)
    {
      mark_rssi = rssi;
      mark_freq = freq;
    }
  }

  freq += 0.01;

  if (freq > stop_freq)
  {
    Serial.print(".");
    freq = start_freq;

    if (mark_rssi > -75)
    {

      long fr = mark_freq * 100;

      if (fr == compare_freq)
      {
        Serial.println();
        Serial.print("Freq: ");
        Serial.println(mark_freq);
        Serial.print("Rssi: ");
        Serial.println(mark_rssi);
        mark_rssi = -100;
        compare_freq = 0;
        mark_freq = 0;
      }
      else
      {
        compare_freq = mark_freq * 100;
        freq = mark_freq - 0.10;
        mark_freq = 0;
        mark_rssi = -100;
      }
    }
  }
}