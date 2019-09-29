#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SdFat.h>
#include <SDS011.h>

// RTC paramts
RTC_DS1307 RTC;
//bool SET_TIME = false;

// CO params
int INPUT_PIN   = A0;
int CONTROL_PIN = 9;

// SD params (D10 to D13)
const uint8_t chipSelect = SS;
SdFat sd;
SdFile file;

// PM params
SDS011 sds;
int TX_PM = 2;
int RX_PM = 3;

void setup() {
    Serial.begin(9600);
    Wire.begin();
    RTC.begin();
    
    // RTC
    if (! RTC.isrunning()) {
      Serial.println("RTC is NOT running!");
      // This will reflect the time that your sketch was compiled
      RTC.adjust(DateTime(__DATE__, __TIME__));
    }
    //else if (SET_TIME == true) {
    //RTC.adjust(DateTime(__DATE__, __TIME__));
    //}

    // PM sensor
    sds.begin(RX_PM, TX_PM);
    delay(1000);

    // CO
    pinMode(CONTROL_PIN, OUTPUT);

    // SD
    Serial.print("Initializing SD card...");
    // see if the card is present and can be initialized:
    if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
      Serial.println("Card failed, or not present");
      sd.initErrorHalt();
      // don't do anything more:
    }
    Serial.println("card initialized.");


}

void write_to_sd(float ppm, float pm25, float pm10) {

  Serial.println("Writing data to SD card...");
  
  // opening file
  if (!file.open("data.csv", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening test.txt for write failed");
  }
  
  // writing: time stamp, co ppm, pm 2.5, pm 10
  // ... timestamp
  DateTime now = RTC.now();
  file.print(String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
  file.print(";");

  // ... pm 2.5 and 10
  file.print(String(ppm) + ";" + String(pm25) + ";" + String(pm10));
  file.println();
  
  // closing file
  file.close();

}

void print_time() {

    DateTime now = RTC.now();
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  
}

# Comment this function Out if not using CO sensor
float get_co_reading() {

    // 5V for 1 minute
    Serial.println("Warming up CO");
    analogWrite(CONTROL_PIN, 255);  
    delay(60000);              

    // 1.4V for 1.5 minute
    Serial.println("Cooling down CO");
    analogWrite(CONTROL_PIN, 72);    
    delay(90000);              

    
    float sensorValue = analogRead(INPUT_PIN);  
    Serial.println(sensorValue);


    float sensor_volt = sensorValue / 1024 * 5.0;
    float RS_air = (5.0-sensor_volt) / sensor_volt;
    Serial.print("Rs air :");
    Serial.println(RS_air);
    Serial.print("sensor volt : ");
    Serial.println(sensor_volt);
    
    // following is the regression equation calculated from the datasheet of CO, by collecting points on log scale
    float ppm = 86.99529 * pow(RS_air/0.36, -1.590791); 
    Serial.print("ppm :");
    Serial.println(ppm);
    delay(1000);

    return ppm;
  
}

void loop() {

    print_time();
    
    float ppm = get_co_reading();
    //float ppm = 0;

    float p10, p25;
    int error;
    error = sds.read(&p25, &p10);
    if (!error) {
      Serial.println("P2.5: " + String(p25));
      Serial.println("P10:  " + String(p10));
    }
    else {
      Serial.println("Error: could not get a reading from PM sensor!");
      p25 = 0.0;
      p10 = 0.0;
    }

    write_to_sd(ppm, p25, p10);
    delay(1000);
 
}
