/* Induino METEOSTATION test firmware.
Usefull to test if all the hardware work as
expected prior to go further
NACHO MAS 2013
Magnus W. Eriksen 2017

This firmware is independent of indiduino
drivers. Just load on your board and look 
at serial monitor.

Firmware should run, even if reading a sensor fails.

If you do se "XX read, start" without "XX read, done" in the serial monitor,
then comment out the definition of the sensor

i.e.
//#define USE_XX_SENSOR
*/

#define USE_DHT_SENSOR
#define USE_IR_SENSOR
#define USE_P_SENSOR
#define USE_IRRADIANCE_SENSOR
#define DHTPIN 3            // what pin DHT22 is connected to
#define IR_RADIANCE_PIN 0   // what analog pin IRRADIANCE is connecte to

#include "Wire.h"
#include "Adafruit_BMP085.h"
#include "Adafruit_MLX90614.h"
#include "dht.h"

dht DHT;
Adafruit_BMP085 bmp;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

float P,HR,IR,Tp,Thr,Tir,Light;
bool irSuccess, bmpSuccess;

void setupMeteoStation(){
    // Initialize sensor readouts.    
    IR = 0;
    Tir = 0;
    HR = 0;
    Thr = 0;
    Tp = 0;
    P = 0;
    Light = 0.0;

    #ifdef USE_IR_SENSOR
        irSuccess = mlx.begin();
    #endif
    #ifdef USE_P_SENSOR
        bmpSuccess = bmp.begin();
    #endif
    Serial.println("Setup done!");
}

void runMeteoStation() {

    // Read MLX90614
    #ifdef USE_IR_SENSOR
        Serial.println("IR read, start\t");

        if (!irSuccess) {
            Serial.println("Could not find a valid MLX90614 (IR) sensor, check wiring!\t");
            irSuccess = mlx.begin(); //Retry for next iteration.
            IR = 0;
            Tir = 0;
        } else {
            Tir = mlx.readAmbientTempC();
            IR = mlx.readObjectTempC();
        }

        Serial.println("IR read, done\t");
    #else
        Serial.println("IR sensor skipped, not defined\t");
    #endif

    // Read DHT22 AM2303
    #ifdef USE_DHT_SENSOR
        Serial.println("DHT read, start\t");

        int chk = DHT.read22(DHTPIN);
        switch (chk)
        {
            case DHTLIB_OK:
                Serial.print("Read OK\n");
                HR = DHT.humidity;
                Thr = DHT.temperature;
                break;
            case DHTLIB_ERROR_CHECKSUM:
                Serial.print("Checksum error\n");
                break;
            case DHTLIB_ERROR_TIMEOUT:
                Serial.print("Time out error\n");
                break;
            default:
                Serial.print("Unknown error\n");
                break;
        }
        if (chk != DHTLIB_OK)
        {
            Serial.println("Could not read DHT22 AM2303 (HR) sensor, check wiring!\t");
            HR = 0;
            Thr = 0;
        }

        Serial.println("DHT read, done\t");
    #else
        Serial.println("DHT sensor skipped, not defined\t");
    #endif

    // Read BMP085 / BMP180
    #ifdef USE_P_SENSOR
        Serial.println("BMP read, start\t");

        if (!bmpSuccess) {
            Serial.println("Could not find a valid BMP085 / BMP180 (P) sensor, check wiring!\t");
            bmpSuccess = bmp.begin(); //Rety for next iteration.
            P = 0;
            Tp = 0;
        } else {
            Tp = bmp.readTemperature();
            P = bmp.readPressure();
            Serial.println("BMP read, done\t");
        }

    #else
        Serial.println("BMP sensor skipped, not defined!\t");
    #endif

    // Read analog Light sensor
    #ifdef USE_IRRADIANCE_SENSOR
        Serial.println("IR Radiance read. No checks, use flashlight and check output value\t");
        Light = analogRead(IR_RADIANCE_PIN);
    #else
        Serial.println("IR Radiance sensor skipped, not defined!.\t");
    #endif
}

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

void setup(){
    Serial.begin(9600);
	Serial.println("Setup...");
	setupMeteoStation();
}

void loop(){
    Serial.println("LOOP BEGIN\t");
    runMeteoStation();
    Serial.println("RESULT\t");
    Serial.print("IR:");
    Serial.print(IR);
    Serial.print(", P:");
    Serial.print(P);
    Serial.print(", HR:");
    Serial.print(HR);
    Serial.print(", DEW:");
    Serial.print(dewPoint(Thr,HR));
    Serial.print(", IR Radiance:");
    Serial.print(Light);
    Serial.print(", Temp HR:");
    Serial.print(Thr);
    Serial.print(", Temp IR:");
    Serial.print(Tir);
    Serial.print(", Temp P:");
    Serial.println(Tp);
    Serial.println("\n");
    delay(1000); // wait a second before printing again
}
