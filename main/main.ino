 /* vim: set filetype=cpp: */

//date: 2016-12-8
//#define     CC1101_DEBUG 1

// include the library code:
//#include <MemoryFree.h>
#include <LiquidCrystal.h>
#include <sunhu_cc1101.h>

#define MIN_DBM -120
#define STEP_DBM 10
#define RSSI_DELAY 600 //rssi delay in micro second
#define NUM_OF_SUB_BANDS 7
#define CAL_INT 20 //cal every 10 channels(every 1MHz)
//variables used to calculate rssi
uint8_t rssi_dec;
int16_t rssi_dBm;
uint8_t rssi_offset[NUM_OF_SUB_BANDS] ={74,74,74,74,74,74,74};

#define CHAN_SPA 0.05		//channel spacing
/* setting to use 100khz channel spacing****************************************
//freq.band      Range                       Channel
//2            438.199738-463.693512       0-255
//1             412.599640-438.093414       0-255
//0             386.999939-412.493713       0-255
float base_freq[NUM_OF_SUB_BANDS]={387,412.6,438.2};
                                            //FREQ2,FREQ1,FREQ0
uint8_t freqSettings[NUM_OF_SUB_BANDS][3]={  {0x0E,0xE2,0x76},   //band0
                                            {0x0F,0xDE,0x85},   //band1
                                            {0x10,0xDA,0x95}};  //band2
//no change in TEST0 WHERE (>430.5MHz) one should change from TEST0=0x0B to 0x09
uint16_t limitTest0Reg[NUM_OF_SUB_BANDS]={256,180,0}; 
********************************************************************************/

/* setting to use 50khz channel spacing*****************************************
//freq.band      Range                       Channel
//2             437.799835-450.546722       0-255
//1             424.999786-437.746674       0-255
//0             412.199738-424.946625       0-255
float base_freq[NUM_OF_SUB_BANDS]={412.2,425,437.8};
                                            //FREQ2,FREQ1,FREQ0
uint8_t freqSettings[NUM_OF_SUB_BANDS][3]={  {0x0F,0xDA,0x95},   //band0
                                            {0x10,0x58,0x9D},   //band1
                                            {0x10,0xD6,0xA5}};  //band2
//no change in TEST0 WHERE (>430.5MHz) one should change from TEST0=0x0B to 0x09
uint16_t limitTest0Reg[NUM_OF_SUB_BANDS]={256,111,0}; 
/ setting to use 50khz channel spacing****************************************/

/* setting to use 50khz channel spacing whole band*****************************************/
//freq.band      Range                       Channel
//0             386.999939-399.746826       0-255
//1 			399.796814-412.543701		0-255
//2				412.593689-425.340576		0-255
//3				425.390564-438.137451		0-255
//4 			438.187439-450.934326		0-255
//5 			450.994232-463.741119		0-255
//6 			463.791107-463.991058		0-4

float base_freq[NUM_OF_SUB_BANDS]={387,399.8,412.6,425.4,438.2,451,463.8};
                                            //FREQ2,FREQ1,FREQ0
uint8_t freqSettings[NUM_OF_SUB_BANDS][3]={  {0x0E,0xE2,0x76},   //band0
											{0x0F,0x60,0x76},
                                            {0x0F,0xDE,0x76},   //band1
                                            {0x10,0x5C,0x76},
											{0x10,0xDA,0x76},
											{0x11,0x58,0x8F},
											{0x11,0xD6,0x8F}};  //band2
//no change in TEST0 WHERE (>430.5MHz) one should change from TEST0=0x0B to 0x09
uint16_t limitTest0Reg[NUM_OF_SUB_BANDS]={256,256,256,103,0,0,0}; 
/* setting to use 50khz channel spacing whole band*****************************************/

//int16_t rssiData[NUM_OF_SUB_BANDS][256];
int16_t rssiTable[256];
uint16_t channelNumber[256];
uint8_t carrierSenseCounter = 0;//counter used to keep track on how many CS has been asserted

//stop channel in each subband
uint8_t lastChannel[NUM_OF_SUB_BANDS] ={255,255,255,255,255,255,4};
//initialized to a value lower than the rssi threshold/ higher than channel number
int16_t highRSSI[NUM_OF_SUB_BANDS] ={MIN_DBM,MIN_DBM,MIN_DBM,MIN_DBM,MIN_DBM,MIN_DBM,MIN_DBM};
uint16_t selectedChannel[NUM_OF_SUB_BANDS] ={300,300,300,300,300,300,300} ;

int8_t activeBand ; //store subband that contains strongest signal
uint16_t activeChannel; //

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8,7,5,4,3,2); //(RS,E,D4,D5,D6,D7)
sunhu_cc1101 cc1101(10,19,18); //(ss_pin, GDO0, GDO2)

#define LED 13
void setup() {
  // initialize the serial communications:
  Serial.begin(115200);

  //cc1101
  cc1101.Init();
  /* setting to use 50khz channel spacing**************************************/
  cc1101.SpiWriteReg(CC1101_MDMCFG4,  0xCD);	// RX filter bandwidth 58.035714(0xFD) 100k(0xCD) 200k(0x8D)
  cc1101.SpiWriteReg(CC1101_MDMCFG3,  0x3B);	//datarate config 250kBaud  for the purpose of fast rssi measurement
  cc1101.SpiWriteReg(CC1101_MDMCFG1,  0x20);	//FEC preamble etc. last 2 bits for channel spacing
  cc1101.SpiWriteReg(CC1101_MDMCFG0,  0xF8);	//50khz channel spacing
  /* setting to use 50khz channel spacing**************************************/

  // set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  lcd.noCursor();
  lcd.noBlink();
  lcd.write("initiated");

}

void loop() {

 //reset highRSSI and activeChannel
 for(uint8_t i=0; i <=NUM_OF_SUB_BANDS;i++){
    highRSSI[i]=MIN_DBM;
 }
 activeChannel=300;
 //scan
 scanFreq();

 //LCD display
 //clear first line
 lcd.setCursor(0,0);
 for(uint8_t i=0;i<16;i++){
    lcd.write(" ");
 }
 if (activeChannel<256){
  float freq;
  freq=base_freq[activeBand]+CHAN_SPA*activeChannel;
  char tempBuf[5];
  dtostrf(freq,5,2,tempBuf);
  lcd.setCursor(0,0);
  lcd.write(tempBuf);
  lcd.write("M:");
  dtostrf(highRSSI[activeBand],5,0,tempBuf);
  lcd.write(tempBuf);
  lcd.write("dBm");

  // Serial.print(freq,4);
  // Serial.println("MHz: ");
  // Serial.print(highRSSI[activeBand]);
  // Serial.println("dBm" );

  //start jamming /* jamming will affect coming freq sweeping a lot!!!!! */
  // uint16_t jamChannel;
  // if (activeChannel+5<=lastChannel[activeBand]) {
  //     jamChannel=activeChannel+5;
  // } else {
  //     jamChannel=activeChannel-5;
  // }
  // jamming(activeBand,jamChannel,2000);
  // flashLED(1);

 }
 else {
  lcd.setCursor(0,0);
  lcd.write("0 carrier sensed");

  // Serial.println("no carrier sensed");
 }
 uint8_t syncWord[6]={128,128,128,128,128,128};
 Serial.write(syncWord,6);

}
//send jamming
void jamming(uint8_t band, uint16_t channel, uint16_t miniSec){
    cc1101.SetFreq(freqSettings[band][0],freqSettings[band][1],freqSettings[band][2]);
    cc1101.SetChannel(channel);
    digitalWrite(19,0);
    cc1101.SetTransmit();
    delay(miniSec);

    cc1101.SpiStrobe(CC1101_SIDLE);
}

//bardisplay function
#define BAR_CNT 40
void barDisp(int16_t temp[], int16_t minimum, int16_t step){
  //create and display charmaps
  int curX=0;
  for(uint8_t i=0;i<BAR_CNT;i=i+5){
    byte charmap[8]={B0,B0,B0,B0,B0,B0,B0,B0};
    for(int8_t j=7;j>=0;j--){
      for(uint8_t m=0;m<5;m++){
        if(temp[i+m]>minimum){
          charmap[j]+=1;
          charmap[j]=charmap[j]<<1;
          temp[i+m]-=step;
        }
        else{
          charmap[j]+=0;
          charmap[j]=charmap[j]<<1;
        }
      }
    }
    //display charmap
    lcd.createChar(curX,charmap);
    lcd.setCursor(curX+4,1);
    lcd.write((byte)curX);
    curX++;
    //delay(10);
  }
}

void scanFreq(void){
  uint8_t calCounter; //to deterin when to calibrate
  uint8_t subBand;
  uint16_t channel;
  uint16_t i;

  float freq;

  cc1101.SpiWriteReg(CC1101_MCSM0,0x08);//disalbe FS_AUTOCAL
  cc1101.SpiWriteReg(CC1101_AGCCTRL2,0x43|0x0C);//MAX_DVGA_GAIN to 11 for fast rssi
  cc1101.SpiWriteReg(CC1101_AGCCTRL0,0xB0);//max AGC WAIT_TIME; 0 filter_length
  cc1101.SetMod(GFSK); //set to GFSK for fast rssi measurement | +8 is dcfilter off
  // 1) loop through all sub bands
  for(subBand = 0; subBand<NUM_OF_SUB_BANDS;subBand++){
      //1.1) set subBands freq by FREQ2, FREQ1, FREQ0
      cc1101.SetFreq(freqSettings[subBand][0],freqSettings[subBand][1],freqSettings[subBand][2]);
      //1.2) set TEST0--maybe!
      //1.3) reset calibration counter
      calCounter=0;
      // 1.4) loop throuhg all channels
      for (channel=0;channel<=lastChannel[subBand];channel++){
        uint8_t pktStatus;
        //1.4.1) set channel register
        cc1101.SetChannel(channel);
        //1.4.2)  set TEST0
		if(channel==limitTest0Reg[subBand]){
			//set test0 to 0x09
			cc1101.SpiWriteReg(CC1101_TEST0,    0x09); 
			//set FSCAL2 to 0x2A to force VCO HIGH
			cc1101.SpiWriteReg(CC1101_FSCAL2,	0x2A);
			//clear calCounter to invoke mannual calibration
			calCounter=0;
		}
        //1.4.3) calibrate every 1MHz
        if(calCounter++==0){
            //perform a manual calibration by issuing SCAL command
            cc1101.SpiStrobe(CC1101_SCAL);
        }
        //1.4.4) reset calCounter when 1MHz reached
        if(calCounter==CAL_INT){
            calCounter=0;
        }
        // 1.4.5-6 enter rx mode
        cc1101.SetReceive();
        //1.4.7 wait for RSSI to be valid: less than 1.5ms
        delayMicroseconds(RSSI_DELAY);
        // 1.4.8) read PKTSTATUS register while the radio is in RX state
        pktStatus=cc1101.SpiReadStatus(CC1101_PKTSTATUS);
        // 1.4.9) enter IDLE state by issuing a SIDLE command
        cc1101.SpiStrobe(CC1101_SIDLE);
        // 1.4.10) check if CS is assearted
        // //read rssi value and converto to dBm form
        rssi_dec=(uint8_t)cc1101.SpiReadStatus(CC1101_RSSI);
        rssi_dBm=calRSSI(rssi_dec,rssi_offset[subBand]);
        //rssiData[subBand][channel]=rssi_dBm;
        if (pktStatus & 0x40){ //CS assearted
          //store rssi value and corresponding channel number
          rssiTable[carrierSenseCounter]=rssi_dBm;
          channelNumber[carrierSenseCounter]=channel;
          carrierSenseCounter++;
        }

        Serial.write(rssi_dec);//for external data processing
        #ifdef CC1101_DEBUG
          Serial.print("rssi_dBm:");
          Serial.println(rssi_dBm);
        #endif

      }//end channel lop
      //1.5)before moving to next sub band, scan through rssiTable to find highest rssi value
      for(i=0; i< carrierSenseCounter;i++){
        if(rssiTable[i]>highRSSI[subBand]){
            highRSSI[subBand]=rssiTable[i];
            selectedChannel[subBand]=channelNumber[i];
        }
      }
      // Serial.print("subBand:------------------>");
      // Serial.println(subBand);
      // Serial.print("selectedChannel:");
      // Serial.println(selectedChannel[subBand]);
      // Serial.print("highRSSI:");
      // Serial.println(highRSSI[subBand]);

      //1.6) reset carrierSenseCounter
      carrierSenseCounter=0;
    }// end band loop

    //2) when all sub bands has been scanned , find best subband and channel
    int16_t tempRssi=MIN_DBM;
    for(subBand=0; subBand<NUM_OF_SUB_BANDS;subBand++){
        if(highRSSI[subBand]>tempRssi){
            tempRssi=highRSSI[subBand];
            activeChannel=selectedChannel[subBand];
            activeBand=subBand;
        }
    }
    // Serial.print("activeBand:**********>");
    // Serial.println(activeBand);
    // Serial.print("activeChannel:");
    // Serial.println(activeChannel);

    cc1101.SpiWriteReg(CC1101_MCSM0,0x18);//enable FS_AUTOCAL
    cc1101.SpiWriteReg(CC1101_AGCCTRL2,0x43);//back to recommended config
    cc1101.SpiWriteReg(CC1101_AGCCTRL0,0x91);//back to recommended config
  }

  int16_t calRSSI(uint8_t rssi_dec,uint8_t rssiOffset){
    int16_t rssi;
    if (rssi_dec>=128)
    rssi=(int16_t)((int16_t)( rssi_dec - 256) / 2) - rssiOffset;
    else
    rssi = (rssi_dec / 2) - rssiOffset;
    return(rssi);
  }

  void flashLED(uint8_t cnt){
    cc1101.SpiEnd();
    for(uint8_t i=0;i<cnt;i++){
      digitalWrite(LED,HIGH);
      delay(500);
      digitalWrite(LED,LOW);
      delay(500);
    }
    cc1101.SpiInit();
  }
