/*
  AD7124 Multiple sensor example 2

  Demonstrate another way to read multiple channels that can be used in either 
  single conversion or continuous conversion mode. like the other MultiSensor 
  example, this example configures the AD7124 for 3 differential and 2 single 
  ended channels and reads the internal temperature sensor and uses it for the 
  cold junction reference. When running in continuous conversion mode, you MUST 
  use the multi-channel form of the readVolts(..) or readRaw(..) methods when 
  reading more than one channel. You then scale your sensor readings from 
  voltage after taking the readings. This works well in single conversion mode 
  too, and can be considerably faster than using the single channel read methods.

  NOTE: Continuous conversion mode may not play nice with other devices on the 
  SPI bus, especially if running at high output rates. 

  NOTE 2: Perhaps counterintuitively, you can often get higher effective sample 
  rates with single conversion mode when reading multiple channels. You will
  also see less "jitter" in the timing when sampling at faster rates. The reason
  is that when reading multiple channels in continuous mode and performing other
  tasks (especially on slower hardware), there can be what amounts to framing 
  errors because the read happens in the middle of the group of channels. When 
  this occurs, the data gets thrown out and the read function waits for the next
  full set of readings to store in the buffer. In single conversion mode, we 
  manually start the conversion for each set of readings, so the framing is 
  always correct. 

  Uses the switched 2.5V excitation provided on the NHB AD7124 board, which must
  be turned on before reading the sensor. On NHB boards the on chip low side switch
  is tied to the enable pin of a 2.5V regulator to provide excitation voltage to
  sensors. This allows the regulator to be shut down between readings to save power
  when doing long term, low speed logging.

  
  The channels are configured as follows: 

  Channel 0   Differential    Load Cell (or other full bridge)
  Channel 1   Differential    Thermocouple
  Channel 2   Differential    Thermocouple   
  Channel 3   Single Ended    Potentiometer
  Channel 4   Single Ended    Potentiometer
  Channel 5   Internal        IC Temperature  
 

  For more on AD7124, see
  http://www.analog.com/media/en/technical-documentation/data-sheets/AD7124-4.pdf



  This file is part of the NHB_AD7124 library.

  MIT License - A copy of the full text should be included with the library

  Copyright (C) 2021  Jaimy Juliano

*/

#include <NHB_AD7124.h>


#define CH_COUNT 6 // 3 differential channels + 2 single ended + internal temperature sensor


// You can run this example in either single conversion or continuous mode.
// Uncomment the desired mode below
#define CONVMODE  AD7124_OpMode_SingleConv
//#define CONVMODE  AD7124_OpMode_Continuous



// The filter select bits determine the filtering and ouput data rate
// 1 = Minimum filter, Maximum sample rate
// 2047 = Maximum filter, Minumum sample rate
int filterSelBits = 4; 



const uint8_t csPin = 10; // Change if

Ad7124 adc(csPin, 4000000);


void setup() {

  //Initialize serial and wait for port to open:
  Serial.begin (115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println ("AD7124 Multiple sensor type example");

  // Initializes the AD7124 device
  adc.begin();

  // Configuring ADC for chosen conversion mode and full power 
  adc.setAdcControl (CONVMODE, AD7124_FullPower, true);

 
  // Set the "setup" configurations for different sensor types. There are 7 differnet "setups"
  // in the ADC that can be configured. There are 8 setups that can be configured. Each 
  // setup holds settings for the reference used, the gain setting, filter type, and rate

  adc.setup[0].setConfig(AD7124_Ref_ExtRef1, AD7124_Gain_128, true); // Load Cell:           External reference tied to excitation, Gain = 128, Bipolar = True
  adc.setup[1].setConfig(AD7124_Ref_Internal, AD7124_Gain_32, true); // Thermocouple:        Internal reference, Gain = 32, Bipolar = True  
  adc.setup[2].setConfig(AD7124_Ref_ExtRef1, AD7124_Gain_1, false);  // Ratiometric Voltage: External reference tied to excitation, Gain = 1, Bipolar = False
  adc.setup[3].setConfig(AD7124_Ref_Internal, AD7124_Gain_1, true);  // IC Temp sensor:      Internal reference, Gain = 1, Bipolar = True



  // Filter settings for each setup. In this example they are all the same so
  // this really could have just been done with a loop
  adc.setup[0].setFilter(AD7124_Filter_SINC3, filterSelBits);
  adc.setup[1].setFilter(AD7124_Filter_SINC3, filterSelBits);
  adc.setup[2].setFilter(AD7124_Filter_SINC3, filterSelBits);
  adc.setup[3].setFilter(AD7124_Filter_SINC3, filterSelBits);
  


  // Set channels, i.e. what setup they use and what pins they are measuring on
  
  adc.setChannel(0, 0, AD7124_Input_AIN0, AD7124_Input_AIN1, true); //Channel 0 - Load cell
  adc.setChannel(1, 1, AD7124_Input_AIN2, AD7124_Input_AIN3, true); //Channel 1 - Type K Thermocouple 1
  adc.setChannel(2, 1, AD7124_Input_AIN4, AD7124_Input_AIN5, true); //Channel 2 - Type K Thermocouple 2 
  adc.setChannel(3, 2, AD7124_Input_AIN6, AD7124_Input_AVSS, true); //Channel 3 - Single ended potentiometer 1  
  adc.setChannel(4, 2, AD7124_Input_AIN7, AD7124_Input_AVSS, true); //Channel 4 - Single ended potentiometer 2
  adc.setChannel(5, 3, AD7124_Input_TEMP, AD7124_Input_AVSS, true); //Channel 5 - ADC IC temperature


  // For thermocouples we have to set the voltage bias on the negative input pin 
  // for the channel, in this case it's AIN3 and AIN5
  adc.setVBias(AD7124_VBias_AIN3,true); 
  adc.setVBias(AD7124_VBias_AIN5,true);  


  // Turn on excitation voltage. If you were only taking readings periodically
  // you could turn this off between readings to save power.
  adc.setPWRSW(1);
}

// -----------------------------------------------------------------------------

void loop() {

  Ad7124_Readings readings[CH_COUNT];  
  long dt;

  // Take readings, and measure how long it takes. You can change the
  // filterSelectBits above and see how it affects the output rate and
  // the noise in your signals.
  // NOTE: On some architectures micros() is not very accurate 
  dt = micros();
  
  //We need to just read voltage from all channels first when using
  //continuous conversion mode, but it also works well for single
  //conversion mode when reading mutiple channels.
  //
  //NOTE: The on-chip temperature sensor will be returned
  //as raw adc counts because it is not a voltage measurement.
  adc.readVolts(readings,CH_COUNT); 

  dt = micros() - dt;
  
  // Now we can apply the appropriate scaling. 
  // The potentiometers will be left as voltage for this example
  readings[5].value = adc.scaleIcTemp(readings[5].value);
  readings[0].value = adc.scaleFB(readings[0].value, 2.5, 5.00);
  readings[1].value = adc.scaleTC(readings[1].value,readings[5].value, Type_K);
  readings[2].value = adc.scaleTC(readings[2].value,readings[5].value, Type_K);


  // You would probably do something more interesting with the readings here
  // but we'll just print them out for our example.

  for(int i = 0; i < CH_COUNT; i++){
    Serial.print("Ch");
    Serial.print(readings[i].ch);
    Serial.print(' ');
    Serial.print(readings[i].value, DEC); //Load Cell
    Serial.print('\t');
  }  

  Serial.print("dt=");
  Serial.print(dt);
  Serial.print("uS");
  Serial.println();

}