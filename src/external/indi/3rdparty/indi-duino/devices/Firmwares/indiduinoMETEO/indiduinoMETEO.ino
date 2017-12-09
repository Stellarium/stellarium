/* INDIDUINOMETEO FIRMWARE.
NACHO MAS 2013. http://indiduino.wordpress.com
Magnus W. Eriksen 2017 https://github.com/magnue

Several modifications over indiduinoTemplate:
.- Include  "i2cmaster.h",  "Adafruit_BMP085.h", "Adafruit_MLX90614.h" and  "dht.h" libraries
   to read the sensors.
.- Add custimization of pinnumbers, and flags for frezzing and daylight.
.- Allow user to disable any sensor(s) and its (their) code when compiling.
.- Several additional functions to read the sensor and calculate flags,
   cloudcover and dew point.
.- Extend event loop fail checks to awoid reading values from unreadable sensors (IR, P and DHT).
.- Overwrite firmata TOTAL_ANALOG_PINS and TOTAL_PINS to make room for
   aditional (>6) analog calculate inputs.
.- Use pullup resitor on inputs for signal flags.

IMPORTANT: Customize following values to match your setup
*/

//Comment out if your setup don't include some sensor.
#define USE_DHT_SENSOR          //USE DHT HUMITITY SENSOR. Comment if not.
#define USE_IR_SENSOR           //USE MELEXIS IR SENSOR. Comment if not.
#define USE_P_SENSOR            //USE BMP085 PRESSURE SENSOR. Comment if not.
#define USE_IRRADIANCE_SENSOR   //USE IRRADIANCE SENSOR (solar cell). Comment if not.

//Not everyone consider zero celsius as frezzing and other drivers will react to frezzing as an alert.
//define temperature limit for issuing alert.
//Default 0
#define FREZZING 0

//All sensors (Thr=DHT22,Tir=MELEXIS and Tp=BMP[085,180]) include a ambient temperature
//Chosse  that sensor, only one, is going to use for main Ambient Temperature:
//#define T_MAIN_Thr
//#define T_MAIN_Tir
#define T_MAIN_Tp

#ifdef USE_DHT_SENSOR
  // what pin we're connected DHT22 to
  #define DHTPIN 3
#endif //USE_DHT_SENSOR

#ifdef USE_IRRADIANCE_SENSOR
  //A multitude of solar cells can be used as IRRADIANCE sensor.
  //Set MINIMUM_DAYLIGHT to the IRRADIANCE output at start of dusk.
  #define MINIMUM_DAYLIGHT 100

   // what analog pin we connected IRRADCIANCE to
  #define IR_RADIANCE_PIN 0
#endif //USE_IRRADIANCE_SENSOR

#ifdef USE_IR_SENSOR
  //Cloudy sky is warmer that clear sky. Thus sky temperature meassure by IR sensor
  //is a good indicator to estimate cloud cover. However IR really meassure the
  //temperatura of all the air column above increassing with ambient temperature.
  //So it is important include some correction factor:
  //From AAG Cloudwatcher formula. Need to improve futher.
  //http://www.aagware.eu/aag/cloudwatcher700/WebHelp/index.htm#page=Operational%20Aspects/23-TemperatureFactor-.htm
  //Sky temp correction factor. Tsky=Tsky_meassure - Tcorrection
  //Formula Tcorrection = (K1 / 100) * (Thr - K2 / 10) + (K3 / 100) * pow((exp (K4 / 1000* Thr)) , (K5 / 100));
  #define  K1 33.
  #define  K2 0.
  #define  K3 4.
  #define  K4 100.
  #define  K5 100.

  //Clear sky corrected temperature (temp below means 0% clouds)
  #define CLOUD_TEMP_CLEAR  -8
  //Totally cover sky corrected temperature (temp above means 100% clouds)
  #define CLOUD_TEMP_OVERCAST  0
  //Activation treshold for cloudFlag (%)
  #define CLOUD_FLAG_PERCENT  30
#endif //USE_IR_SENSOR



/*END OFF CUSTOMITATION. YOU SHOULT NOT NEED TO CHANGE ANYTHING BELOW */


/*
 * Firmata is a generic protocol for communicating with microcontrollers
 * from software on a host computer. It is intended to work with
 * any host computer software package.
 *
 * To download a host software package, please clink on the following link
 * to open the download page in your default browser.
 *
 * http://firmata.org/wiki/Download
 */

/*
  Copyright (C) 2012 Nacho Mas. By default Standard firmata write analog 
  input value to PWM pin directly and send the ADC readings to analog output
  without modification. By this modification you can adapt this behaviour.

  two functions are added to the original StandardFirmata.:

  mapAndSendAnalog(int pin):

	Change the value returned by readAnalog before send through
	firmata protocol. By this you can adapt the 0-1024 stadard ADC range
	to more apropiate range (i.e phisical range of a sensor). Also you 
	can do some logic or event sent a variable value instead of 
	readAnalog. 

  mapAndWriteAnalog(int pin,int value):

	Change the value recived through firmata protocol before write to
	PWM output. Alternative can be used to change internal variable value instead
	of setting PWM output.

  (TODO: same for digital input/output)

  NOTE: This only a template and by default do nothing! You have to addapt 
	to your real needs before.					  
 
  Copyright (C) 2006-2008 Hans-Christoph Steiner.  All rights reserved.
  Copyright (C) 2010-2011 Paul Stoffregen.  All rights reserved.
  Copyright (C) 2009 Shigeru Kobayashi.  All rights reserved.
  Copyright (C) 2009-2011 Jeff Hoefs.  All rights reserved.
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
 
  See file LICENSE.txt for further informations on licensing terms.

  formatted using the GNU C formatting and indenting
*/

/* 
 * TODO: use Program Control to load stored profiles from EEPROM
 */

#include <Servo.h>
#include "Wire.h"
#include <Firmata.h>


#ifdef USE_IR_SENSOR
  #include "Adafruit_MLX90614.h"
  Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#endif //USE_IR_SENSOR

#ifdef USE_DHT_SENSOR
  #include "dht.h"
  dht DHT;
#endif //USE_DHT_SENSOR

#ifdef USE_P_SENSOR
  #include "Adafruit_BMP085.h"
  Adafruit_BMP085 bmp;
#endif //USE_P_SENSOR


float P,HR,IR,T,Tp,Thr,Tir,Dew,Light,Clouds,skyT;
int cloudy,dewing,frezzing;
bool irSuccess,bmpSuccess;

#define TOTAL_ANALOG_PINS       11
#define TOTAL_PINS              25

void setupMeteoStation(){
#ifdef USE_IR_SENSOR
        if (!(irSuccess=mlx.begin())) {
            //set IR sensor fail flag
            digitalWrite(PIN_TO_DIGITAL(7), HIGH);
            IR=0;
            Tir=0;
        } else digitalWrite(PIN_TO_DIGITAL(7), LOW); //Make sure IR sensor fail flag is off on success

#endif //USE_IR_SENSOR

#ifdef USE_P_SENSOR
        if (!(bmpSuccess=bmp.begin())) {
            //set P sensor fail flag
            digitalWrite(PIN_TO_DIGITAL(9), HIGH);
            Tp=0;
            P=0;
        } else digitalWrite(PIN_TO_DIGITAL(9), LOW); //Make sure P sensor fail flag is off on success

#endif //USE_P_SENSOR

#ifndef USE_IRRADIANCE_SENSOR
    Light=0;
#endif //USE_IRRADIANCE_SENSOR

}

/*==============================================================================
 * METEOSTATION FUNCTIONS
 *============================================================================*/
void runMeteoStation() {
  
#ifdef USE_IR_SENSOR
    if (irSuccess) {
        Tir=mlx.readAmbientTempC();
        IR=mlx.readObjectTempC();
    } else if (irSuccess=mlx.begin()) {
        // Retry mlx.begin(), and clear IR sensor fail flag
        digitalWrite(PIN_TO_DIGITAL(7), LOW);
    }
    
    Clouds=cloudIndex();
    skyT=skyTemp();
    if (Clouds>CLOUD_FLAG_PERCENT) {
        cloudy=1;
    } else {
        cloudy=0;
    }
#else
    //set IR sensor fail flag
    digitalWrite(PIN_TO_DIGITAL(7), HIGH);
#endif //USE_IR_SENSOR

#ifdef USE_P_SENSOR
    if (bmpSuccess) {
        Tp=bmp.readTemperature();
        P=bmp.readPressure();
    } else if (bmpSuccess=bmp.begin()) {
        // Retry bmp.begin(), and clear P sensor fail flag
        digitalWrite(PIN_TO_DIGITAL(9), LOW);
    }
#else
    //set P sensor fail flag
    digitalWrite(PIN_TO_DIGITAL(9), HIGH);
#endif //USE_P_SENSOR

#ifdef USE_DHT_SENSOR
     int chk=DHT.read22(DHTPIN);
     if (chk==DHTLIB_OK) {
         //OK.clear HR sensor fail flag
         digitalWrite(PIN_TO_DIGITAL(8), LOW);
         HR=DHT.humidity;
         Thr=DHT.temperature;
     } else {
         //set HR sensor fail flag
         digitalWrite(PIN_TO_DIGITAL(8), HIGH);
     }

    Dew=dewPoint(Thr,HR);
    if (Thr<=Dew+2) {
        dewing=1;
    } else {
        dewing=0;
    }
#else
    //set HR sensor fail flag
    digitalWrite(PIN_TO_DIGITAL(8), HIGH);
#endif //USE_DHT_SENSOR

#ifdef USE_IRRADIANCE_SENSOR
    Light=analogRead(IR_RADIANCE_PIN);
#endif //USE_IRRADIANCE_SENSOR

#if defined T_MAIN_Thr
    T=Thr;
#elif defined T_MAIN_Tir
    T=Tir;
#elif defined T_MAIN_Tp
    T=Tp;
#endif  //T_MAIN

    if (T<FREZZING) {
      frezzing=1;
    } else {
      frezzing=0;
    }
}

void checkMeteo() {

    if (cloudy==1) {
       digitalWrite(PIN_TO_DIGITAL(3), HIGH); // enable internal pull-ups
    } else {
       digitalWrite(PIN_TO_DIGITAL(3), LOW); // disable internal pull-ups
    }
  
    if (dewing==1) {
       digitalWrite(PIN_TO_DIGITAL(4), HIGH); // enable internal pull-ups
    } else {
       digitalWrite(PIN_TO_DIGITAL(4), LOW); // disable internal pull-ups
    }

    if (frezzing == 1) {
       digitalWrite(PIN_TO_DIGITAL(5), HIGH); // enable internal pull-ups
    } else {
       digitalWrite(PIN_TO_DIGITAL(5), LOW); // disable internal pull-ups
    }
  
    if (Light>MINIMUM_DAYLIGHT) {
       digitalWrite(PIN_TO_DIGITAL(6), HIGH); // enable internal pull-ups
    } else {
       digitalWrite(PIN_TO_DIGITAL(6), LOW); // disable internal pull-ups
    }

}

#ifdef USE_DHT_SENSOR
// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm
double dewPoint(double celsius, double humidity)
{
        double A0= 373.15/(273.15 + celsius);
        double SUM = -7.90298 * (A0-1);
        SUM += 5.02808 * log10(A0);
        SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1);
        SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1);
        SUM += log10(1013.246);
        double VP = pow(10, SUM-3) * humidity;
        double T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558-T);
}

// delta max = 0.6544 wrt dewPoint()
// 5x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
        double a = 17.271;
        double b = 237.7;
        double temp = (a * celsius) / (b + celsius) + log(humidity/100);
        double Td = (b * temp) / (a - temp);
        return Td;
}
#endif //USE_DHT_SENSOR

#ifdef USE_IR_SENSOR
//From AAG Cloudwatcher formula. Need to improve futher.
//http://www.aagware.eu/aag/cloudwatcher700/WebHelp/index.htm#page=Operational%20Aspects/23-TemperatureFactor-.htm
//https://azug.minpet.unibas.ch/wikiobsvermes/index.php/AAG_cloud_sensor#Snow_on_the_sky_temperature_sensor
double skyTemp() {
  //Constant defined above
  double Td = (K1 / 100.) * (T - K2 / 10) + (K3 / 100.) * pow((exp (K4 / 1000.* T)) , (K5 / 100.));
  double Tsky=IR-Td;
  return Tsky;
}


double cloudIndex() {
   double Tcloudy= CLOUD_TEMP_OVERCAST,Tclear=CLOUD_TEMP_CLEAR;
   double Tsky=skyTemp();
   double Index;
   if (Tsky<Tclear) Tsky=Tclear;
   if (Tsky>Tcloudy) Tsky=Tcloudy;
   Index=(Tsky-Tclear)*100/(Tcloudy-Tclear);
   return Index;
}
#endif //USE_IR_SENSOR

/* Nacho Mas.
	Change the value returned by readAnalog before send through
	firmata protocol. By this you can adapt the 0-1024 stadard ADC range
	to more apropiate range (i.e phisical range of a sensor). Also you 
	can do some logic or event sent a variable value instead of
	readAnalog.
*/
int mapAndSendAnalog(int pin) {

//some scalation are use. Don't change without changing also skeleton file
  int value=0;
  int result=0;
 
  switch(pin) {
      //PIN 14->A0, 24->A10
       case 0:
               result=(IR+273)*20;
               break;
       case 1:
               result=(Tir+273)*20;
               break;
       case 2:
               result=(P/10);
               break;
       case 3:
               result=(Tp+273)*20;
               break;
       case 4:
               result=HR*100;
               break;
       case 5:
               result=(Thr+273)*20;
               break;
       case 6:
               result=(Dew+273)*20;
               break;
       case 7:
               result=Light;
               break;
       case 8:
               result=Clouds;
               break;
       case 9:
               result=(skyT+273)*20;
               break;
       case 10:
               result=(T+273)*20;
               break;


       default:
             result=value;
             break;
  }
  Firmata.sendAnalog(pin,result);
}

/* Nacho Mas.
	Change the value recived through firmata protocol before write to
	PWM output. Alternative can be used to change internal variable value instead
	of setting PWM output.
*/
int mapAndWriteAnalog(int pin,int value) {
  int pwmPin=PIN_TO_PWM(pin);
  int result=0;
  switch(pwmPin) {
       case 5:
       case 6:
       case 9:               
//             result=map(value, 0, 100, 0, 255);
//             break;             
       default:
             result=value;
             break;
  }
  analogWrite(pwmPin, result);
}  

/*==============================================================================
 * STANDAR FIRMATA FUNCTIONS
 *============================================================================*/

void disableI2CPins();
void enableI2CPins();
void reportAnalogCallback(byte analogPin, int value);

// move the following defines to Firmata.h?
#define I2C_WRITE B00000000
#define I2C_READ B00001000
#define I2C_READ_CONTINUOUSLY B00010000
#define I2C_STOP_READING B00011000
#define I2C_READ_WRITE_MODE_MASK B00011000
#define I2C_10BIT_ADDRESS_MODE_MASK B00100000

#define MAX_QUERIES 1
#define MINIMUM_SAMPLING_INTERVAL 1000 //

#define REGISTER_NOT_SPECIFIED -1

/*==============================================================================
 * GLOBAL VARIABLES
 *============================================================================*/

/* analog inputs */
int analogInputsToReport = 0; // bitwise array to store pin reporting

/* digital input ports */
byte reportPINs[TOTAL_PORTS];       // 1 = report this port, 0 = silence
byte previousPINs[TOTAL_PORTS];     // previous 8 bits sent

/* pins configuration */
byte pinConfig[TOTAL_PINS];         // configuration of every pin
byte portConfigInputs[TOTAL_PORTS]; // each bit: 1 = pin in INPUT, 0 = anything else
int pinState[TOTAL_PINS];           // any value that has been written

/* timer variables */
unsigned long currentMillis;        // store the current value from millis()
unsigned long previousMillis=0;       // for comparison with currentMillis
int samplingInterval = 19;          // how often to run the main loop (in ms)

/* i2c data */
struct i2c_device_info {
  byte addr;
  byte reg;
  byte bytes;
};

/* for i2c read continuous more */
i2c_device_info query[MAX_QUERIES];

byte i2cRxData[32];
boolean isI2CEnabled = false;
signed char queryIndex = -1;
unsigned int i2cReadDelayTime = 0;  // default delay time between i2c read request and Wire.requestFrom()

Servo servos[MAX_SERVOS];

void readAndReportData(byte address, int theRegister, byte numBytes) {
  // allow I2C requests that don't require a register read
  // for example, some devices using an interrupt pin to signify new data available
  // do not always require the register read so upon interrupt you call Wire.requestFrom()  
  if (theRegister != REGISTER_NOT_SPECIFIED) {
    Wire.beginTransmission(address);
    #if ARDUINO >= 100
    Wire.write((byte)theRegister);
    #else
    Wire.send((byte)theRegister);
    #endif
    Wire.endTransmission();
    delayMicroseconds(i2cReadDelayTime);  // delay is necessary for some devices such as WiiNunchuck
  } else {
    theRegister = 0;  // fill the register with a dummy value
  }

  Wire.requestFrom(address, numBytes);  // all bytes are returned in requestFrom

  // check to be sure correct number of bytes were returned by slave
  if(numBytes == Wire.available()) {
    i2cRxData[0] = address;
    i2cRxData[1] = theRegister;
    for (int i = 0; i < numBytes; i++) {
      #if ARDUINO >= 100
      i2cRxData[2 + i] = Wire.read();
      #else
      i2cRxData[2 + i] = Wire.receive();
      #endif
    }
  }
  else {
    if(numBytes > Wire.available()) {
      Firmata.sendString("I2C Read Error: Too many bytes received");
    } else {
      Firmata.sendString("I2C Read Error: Too few bytes received"); 
    }
  }

  // send slave address, register and received bytes
  Firmata.sendSysex(SYSEX_I2C_REPLY, numBytes + 2, i2cRxData);
}

void outputPort(byte portNumber, byte portValue, byte forceSend)
{
  // pins not configured as INPUT are cleared to zeros
  portValue = portValue & portConfigInputs[portNumber];
  // only send if the value is different than previously sent
  if(forceSend || previousPINs[portNumber] != portValue) {
    Firmata.sendDigitalPort(portNumber, portValue);
    previousPINs[portNumber] = portValue;
  }
}

/* -----------------------------------------------------------------------------
 * check all the active digital inputs for change of state, then add any events
 * to the Serial output queue using Serial.print() */
void checkDigitalInputs(void)
{
  /* Using non-looping code allows constants to be given to readPort().
   * The compiler will apply substantial optimizations if the inputs
   * to readPort() are compile-time constants. */
 //Nacho Mas. 
 //TODO: Cach deafult behaviour 
  boolean send_always=false;
  if (TOTAL_PORTS > 0 && reportPINs[0]) outputPort(0, readPort(0, portConfigInputs[0]), send_always);
  if (TOTAL_PORTS > 1 && reportPINs[1]) outputPort(1, readPort(1, portConfigInputs[1]), send_always);
  if (TOTAL_PORTS > 2 && reportPINs[2]) outputPort(2, readPort(2, portConfigInputs[2]), send_always);
  if (TOTAL_PORTS > 3 && reportPINs[3]) outputPort(3, readPort(3, portConfigInputs[3]), send_always);
  if (TOTAL_PORTS > 4 && reportPINs[4]) outputPort(4, readPort(4, portConfigInputs[4]), send_always);
  if (TOTAL_PORTS > 5 && reportPINs[5]) outputPort(5, readPort(5, portConfigInputs[5]), send_always);
  if (TOTAL_PORTS > 6 && reportPINs[6]) outputPort(6, readPort(6, portConfigInputs[6]), send_always);
  if (TOTAL_PORTS > 7 && reportPINs[7]) outputPort(7, readPort(7, portConfigInputs[7]), send_always);
  if (TOTAL_PORTS > 8 && reportPINs[8]) outputPort(8, readPort(8, portConfigInputs[8]), send_always);
  if (TOTAL_PORTS > 9 && reportPINs[9]) outputPort(9, readPort(9, portConfigInputs[9]), send_always);
  if (TOTAL_PORTS > 10 && reportPINs[10]) outputPort(10, readPort(10, portConfigInputs[10]), send_always);
  if (TOTAL_PORTS > 11 && reportPINs[11]) outputPort(11, readPort(11, portConfigInputs[11]), send_always);
  if (TOTAL_PORTS > 12 && reportPINs[12]) outputPort(12, readPort(12, portConfigInputs[12]), send_always);
  if (TOTAL_PORTS > 13 && reportPINs[13]) outputPort(13, readPort(13, portConfigInputs[13]), send_always);
  if (TOTAL_PORTS > 14 && reportPINs[14]) outputPort(14, readPort(14, portConfigInputs[14]), send_always);
  if (TOTAL_PORTS > 15 && reportPINs[15]) outputPort(15, readPort(15, portConfigInputs[15]), send_always);
}

// -----------------------------------------------------------------------------
/* sets the pin mode to the correct state and sets the relevant bits in the
 * two bit-arrays that track Digital I/O and PWM status
 */
void setPinModeCallback(byte pin, int mode)
{
  if (pinConfig[pin] == I2C && isI2CEnabled && mode != I2C) {
    // disable i2c so pins can be used for other functions
    // the following if statements should reconfigure the pins properly
    disableI2CPins();
  }
  if (IS_PIN_SERVO(pin) && mode != SERVO && servos[PIN_TO_SERVO(pin)].attached()) {
    servos[PIN_TO_SERVO(pin)].detach();
  }
  if (IS_PIN_ANALOG(pin)) {
    reportAnalogCallback(PIN_TO_ANALOG(pin), mode == ANALOG ? 1 : 0); // turn on/off reporting
  }
  if (IS_PIN_DIGITAL(pin)) {
    if (mode == INPUT) {
      portConfigInputs[pin/8] |= (1 << (pin & 7));
    } else {
      portConfigInputs[pin/8] &= ~(1 << (pin & 7));
    }
  }
  pinState[pin] = 0;
  switch(mode) {
  case ANALOG:
    if (IS_PIN_ANALOG(pin)) {
      if (IS_PIN_DIGITAL(pin)) {
        pinMode(PIN_TO_DIGITAL(pin), INPUT); // disable output driver
        digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable internal pull-ups
      }
      pinConfig[pin] = ANALOG;
    }
    break;
  case INPUT:
    if (IS_PIN_DIGITAL(pin)) {
      pinMode(PIN_TO_DIGITAL(pin), INPUT); // disable output driver
      digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable internal pull-ups
      pinConfig[pin] = INPUT;
    }
    break;
  case OUTPUT:
    if (IS_PIN_DIGITAL(pin)) {
      digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable PWM
      pinMode(PIN_TO_DIGITAL(pin), OUTPUT);
      pinConfig[pin] = OUTPUT;
    }
    break;
  case PWM:
    if (IS_PIN_PWM(pin)) {
      pinMode(PIN_TO_PWM(pin), OUTPUT);
      analogWrite(PIN_TO_PWM(pin), 0);
      pinConfig[pin] = PWM;
    }
    break;
  case SERVO:
    if (IS_PIN_SERVO(pin)) {
      pinConfig[pin] = SERVO;
      if (!servos[PIN_TO_SERVO(pin)].attached()) {
          servos[PIN_TO_SERVO(pin)].attach(PIN_TO_DIGITAL(pin));
      }
    }
    break;
  case I2C:
    if (IS_PIN_I2C(pin)) {
      // mark the pin as i2c
      // the user must call I2C_CONFIG to enable I2C for a device
      pinConfig[pin] = I2C;
    }
    break;
  default:
    Firmata.sendString("Unknown pin mode"); // TODO: put error msgs in EEPROM
  }
  // TODO: save status to EEPROM here, if changed
}

void analogWriteCallback(byte pin, int value)
{
  if (pin < TOTAL_PINS) {
    switch(pinConfig[pin]) {
    case SERVO:
      if (IS_PIN_SERVO(pin))
        servos[PIN_TO_SERVO(pin)].write(value);
        pinState[pin] = value;
      break;
    case PWM:
      if (IS_PIN_PWM(pin))
        //Nacho Mas. Write analog and do something before set PWM. 
        mapAndWriteAnalog(pin,value);
        pinState[pin] = value;
      break;
    }
  }
}

void digitalWriteCallback(byte port, int value)
{
  byte pin, lastPin, mask=1, pinWriteMask=0;

  if (port < TOTAL_PORTS) {
    // create a mask of the pins on this port that are writable.
    lastPin = port*8+8;
    if (lastPin > TOTAL_PINS) lastPin = TOTAL_PINS;
    for (pin=port*8; pin < lastPin; pin++) {
      // do not disturb non-digital pins (eg, Rx & Tx)
      if (IS_PIN_DIGITAL(pin)) {
        // only write to OUTPUT and INPUT (enables pullup)
        // do not touch pins in PWM, ANALOG, SERVO or other modes
        if (pinConfig[pin] == OUTPUT || pinConfig[pin] == INPUT) {
          pinWriteMask |= mask;
          pinState[pin] = ((byte)value & mask) ? 1 : 0;
        }
      }
      mask = mask << 1;
    }
    //Nacho Mas. TODO: sustitute this call by
    //mapAndWriteDigital(port, (byte)value, pinWriteMask)
    writePort(port, (byte)value, pinWriteMask);
  }
}


// -----------------------------------------------------------------------------
/* sets bits in a bit array (int) to toggle the reporting of the analogIns
 */
//void FirmataClass::setAnalogPinReporting(byte pin, byte state) {
//}
void reportAnalogCallback(byte analogPin, int value)
{
  if (analogPin < TOTAL_ANALOG_PINS) {
    if(value == 0) {
      analogInputsToReport = analogInputsToReport &~ (1 << analogPin);
    } else {
      analogInputsToReport = analogInputsToReport | (1 << analogPin);
    }
  }
  // TODO: save status to EEPROM here, if changed
}

void reportDigitalCallback(byte port, int value)
{
  if (port < TOTAL_PORTS) {
    reportPINs[port] = (byte)value;
  }
  // do not disable analog reporting on these 8 pins, to allow some
  // pins used for digital, others analog.  Instead, allow both types
  // of reporting to be enabled, but check if the pin is configured
  // as analog when sampling the analog inputs.  Likewise, while
  // scanning digital pins, portConfigInputs will mask off values from any
  // pins configured as analog
}

/*==============================================================================
 * SYSEX-BASED commands
 *============================================================================*/

void sysexCallback(byte command, byte argc, byte *argv)
{
  byte mode;
  byte slaveAddress;
  byte slaveRegister;
  byte data;
  unsigned int delayTime; 
  
  switch(command) {
  case I2C_REQUEST:
    mode = argv[1] & I2C_READ_WRITE_MODE_MASK;
    if (argv[1] & I2C_10BIT_ADDRESS_MODE_MASK) {
      Firmata.sendString("10-bit addressing mode is not yet supported");
      return;
    }
    else {
      slaveAddress = argv[0];
    }

    switch(mode) {
    case I2C_WRITE:
      Wire.beginTransmission(slaveAddress);
      for (byte i = 2; i < argc; i += 2) {
        data = argv[i] + (argv[i + 1] << 7);
        #if ARDUINO >= 100
        Wire.write(data);
        #else
        Wire.send(data);
        #endif
      }
      Wire.endTransmission();
      delayMicroseconds(70);
      break;
    case I2C_READ:
      if (argc == 6) {
        // a slave register is specified
        slaveRegister = argv[2] + (argv[3] << 7);
        data = argv[4] + (argv[5] << 7);  // bytes to read
        readAndReportData(slaveAddress, (int)slaveRegister, data);
      }
      else {
        // a slave register is NOT specified
        data = argv[2] + (argv[3] << 7);  // bytes to read
        readAndReportData(slaveAddress, (int)REGISTER_NOT_SPECIFIED, data);
      }
      break;
    case I2C_READ_CONTINUOUSLY:
      if ((queryIndex + 1) >= MAX_QUERIES) {
        // too many queries, just ignore
        Firmata.sendString("too many queries");
        break;
      }
      queryIndex++;
      query[queryIndex].addr = slaveAddress;
      query[queryIndex].reg = argv[2] + (argv[3] << 7);
      query[queryIndex].bytes = argv[4] + (argv[5] << 7);
      break;
    case I2C_STOP_READING:
	  byte queryIndexToSkip;      
      // if read continuous mode is enabled for only 1 i2c device, disable
      // read continuous reporting for that device
      if (queryIndex <= 0) {
        queryIndex = -1;        
      } else {
        // if read continuous mode is enabled for multiple devices,
        // determine which device to stop reading and remove it's data from
        // the array, shifiting other array data to fill the space
        for (byte i = 0; i < queryIndex + 1; i++) {
          if (query[i].addr = slaveAddress) {
            queryIndexToSkip = i;
            break;
          }
        }
        
        for (byte i = queryIndexToSkip; i<queryIndex + 1; i++) {
          if (i < MAX_QUERIES) {
            query[i].addr = query[i+1].addr;
            query[i].reg = query[i+1].addr;
            query[i].bytes = query[i+1].bytes; 
          }
        }
        queryIndex--;
      }
      break;
    default:
      break;
    }
    break;
  case I2C_CONFIG:
    delayTime = (argv[0] + (argv[1] << 7));

    if(delayTime > 0) {
      i2cReadDelayTime = delayTime;
    }

    if (!isI2CEnabled) {
      enableI2CPins();
    }
    
    break;
  case SERVO_CONFIG:
    if(argc > 4) {
      // these vars are here for clarity, they'll optimized away by the compiler
      byte pin = argv[0];
      int minPulse = argv[1] + (argv[2] << 7);
      int maxPulse = argv[3] + (argv[4] << 7);

      if (IS_PIN_SERVO(pin)) {
        if (servos[PIN_TO_SERVO(pin)].attached())
          servos[PIN_TO_SERVO(pin)].detach();
        servos[PIN_TO_SERVO(pin)].attach(PIN_TO_DIGITAL(pin), minPulse, maxPulse);
        setPinModeCallback(pin, SERVO);
      }
    }
    break;
  case SAMPLING_INTERVAL:
    if (argc > 1) {
      samplingInterval = argv[0] + (argv[1] << 7);
      if (samplingInterval < MINIMUM_SAMPLING_INTERVAL) {
        samplingInterval = MINIMUM_SAMPLING_INTERVAL;
      }      
    } else {
      //Firmata.sendString("Not enough data");
    }
    break;
  case EXTENDED_ANALOG:
    if (argc > 1) {
      int val = argv[1];
      if (argc > 2) val |= (argv[2] << 7);
      if (argc > 3) val |= (argv[3] << 14);
      analogWriteCallback(argv[0], val);
    }
    break;
  case CAPABILITY_QUERY:
    Serial.write(START_SYSEX);
    Serial.write(CAPABILITY_RESPONSE);
    for (byte pin=0; pin < TOTAL_PINS; pin++) {
      if (IS_PIN_DIGITAL(pin)) {
        Serial.write((byte)INPUT);
        Serial.write(1);
        Serial.write((byte)OUTPUT);
        Serial.write(1);
      }
      if (IS_PIN_ANALOG(pin)) {
        Serial.write(ANALOG);
        Serial.write(10);
      }
      if (IS_PIN_PWM(pin)) {
        Serial.write(PWM);
        Serial.write(8);
      }
      if (IS_PIN_SERVO(pin)) {
        Serial.write(SERVO);
        Serial.write(14);
      }
      if (IS_PIN_I2C(pin)) {
        Serial.write(I2C);
        Serial.write(1);  // to do: determine appropriate value 
      }
      Serial.write(127);
    }
    Serial.write(END_SYSEX);
    break;
  case PIN_STATE_QUERY:
    if (argc > 0) {
      byte pin=argv[0];
      Serial.write(START_SYSEX);
      Serial.write(PIN_STATE_RESPONSE);
      Serial.write(pin);
      if (pin < TOTAL_PINS) {
        Serial.write((byte)pinConfig[pin]);
	Serial.write((byte)pinState[pin] & 0x7F);
	if (pinState[pin] & 0xFF80) Serial.write((byte)(pinState[pin] >> 7) & 0x7F);
	if (pinState[pin] & 0xC000) Serial.write((byte)(pinState[pin] >> 14) & 0x7F);
      }
      Serial.write(END_SYSEX);
    }
    break;
  case ANALOG_MAPPING_QUERY:
    Serial.write(START_SYSEX);
    Serial.write(ANALOG_MAPPING_RESPONSE);
    for (byte pin=0; pin < TOTAL_PINS; pin++) {
      Serial.write(IS_PIN_ANALOG(pin) ? PIN_TO_ANALOG(pin) : 127);
    }
    Serial.write(END_SYSEX);
    break;
  }
}

void enableI2CPins()
{
  byte i;
  // is there a faster way to do this? would probaby require importing 
  // Arduino.h to get SCL and SDA pins
  for (i=0; i < TOTAL_PINS; i++) {
    if(IS_PIN_I2C(i)) {
      // mark pins as i2c so they are ignore in non i2c data requests
      setPinModeCallback(i, I2C);
    } 
  }
   
  isI2CEnabled = true; 
  
  // is there enough time before the first I2C request to call this here?
  Wire.begin();
}

/* disable the i2c pins so they can be used for other functions */
void disableI2CPins() {
    isI2CEnabled = false;
    // disable read continuous mode for all devices
    queryIndex = -1;
    // uncomment the following if or when the end() method is added to Wire library
    // Wire.end();
}

/*==============================================================================
 * SETUP()
 *============================================================================*/

void systemResetCallback()
{
  // initialize a defalt state
  // TODO: option to load config from EEPROM instead of default
  if (isI2CEnabled) {
  	disableI2CPins();
  }
  for (byte i=0; i < TOTAL_PORTS; i++) {
    reportPINs[i] = false;      // by default, reporting off
    portConfigInputs[i] = 0;	// until activated
    previousPINs[i] = 0;
  }
  // pins with analog capability default to analog input
  // otherwise, pins default to digital output
  for (byte i=0; i < TOTAL_PINS; i++) {
    if (IS_PIN_ANALOG(i)) {
      // turns off pullup, configures everything
      setPinModeCallback(i, ANALOG);
    } else {
      // sets the output to 0, configures portConfigInputs
      setPinModeCallback(i, OUTPUT);
    }
  }
  // by default, do not report any analog inputs
  analogInputsToReport = 0;

  /* send digital inputs to set the initial state on the host computer,
   * since once in the loop(), this firmware will only send on change */
  /*
  TODO: this can never execute, since no pins default to digital input
        but it will be needed when/if we support EEPROM stored config
  for (byte i=0; i < TOTAL_PORTS; i++) {
    outputPort(i, readPort(i, portConfigInputs[i]), true);
  }
  */
  setupMeteoStation();
}

void setup() 
{
  Firmata.setFirmwareVersion(FIRMATA_MAJOR_VERSION, FIRMATA_MINOR_VERSION);

  Firmata.attach(ANALOG_MESSAGE, analogWriteCallback);
  Firmata.attach(DIGITAL_MESSAGE, digitalWriteCallback);
  Firmata.attach(REPORT_ANALOG, reportAnalogCallback);
  Firmata.attach(REPORT_DIGITAL, reportDigitalCallback);
  Firmata.attach(SET_PIN_MODE, setPinModeCallback);
  Firmata.attach(START_SYSEX, sysexCallback);
  Firmata.attach(SYSTEM_RESET, systemResetCallback);

  Firmata.begin(57600);
  systemResetCallback();  // reset to default config
}

/*==============================================================================
 * LOOP()
 *============================================================================*/
void loop() 
{
  byte pin, analogPin;

  /* DIGITALREAD - as fast as possible, check for changes and output them to the
   * FTDI buffer using Serial.print()  */
  //Nacho Mas. TODO: sustitute this call by
  //mapAndSendDigital()
  //checkDigitalInputs();  

  /* SERIALREAD - processing incoming messagse as soon as possible, while still
   * checking digital inputs.  */
  while(Firmata.available())
    Firmata.processInput();

  /* SEND FTDI WRITE BUFFER - make sure that the FTDI buffer doesn't go over
   * 60 bytes. use a timer to sending an event character every 4 ms to
   * trigger the buffer to dump. */

  currentMillis = millis();
  if (currentMillis - previousMillis > samplingInterval) {  
    previousMillis += samplingInterval;
    checkMeteo();
    runMeteoStation();
    checkDigitalInputs();
    /* ANALOGREAD - do all analogReads() at the configured sampling interval */
    for(pin=0; pin<TOTAL_PINS; pin++) {
      if (IS_PIN_ANALOG(pin) && pinConfig[pin] == ANALOG) {
        analogPin = PIN_TO_ANALOG(pin);
        if (analogInputsToReport & (1 << analogPin)) {
          //Nacho Mas. Read analog and do something before send. Then send it
          mapAndSendAnalog(analogPin);
        }
      }
    }
    // report i2c data for all device with read continuous mode enabled
    if (queryIndex > -1) {
      for (byte i = 0; i < queryIndex + 1; i++) {
        readAndReportData(query[i].addr, query[i].reg, query[i].bytes);
      }
    }
  }
}
