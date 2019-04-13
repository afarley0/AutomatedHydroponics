#include <HX711.h>
#include <config.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

int eepromAddressCal = 1; //address where calibration and offset values will be stored
int eepromAddressOffset = 6;
const int LOADCELL_DOUT_PIN = 31; //HX711 constructor
const int LOADCELL_SCK_PIN = 14;

//HX711 constructor (dout pin, sck pin):
HX711_ADC LoadCell(LOADCELL_DOUT_PIN,LOADCELL_SCK_PIN);
HX711 scale;

long t;

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("It is assumed that the mcu was started with no load applied to the load cell.");
  Serial.println("Now, place your known mass on the loadcell,");
  Serial.println("then send the weight of this mass (i.e. 100.0) from serial monitor.");
  float mass = 0;
  boolean f = 0;
  while (f == 0) {
    LoadCell.update();
    if (Serial.available() > 0) {
      mass = Serial.parseFloat();
      if (mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(mass);
        f = 1;
      }
      else {
        Serial.println("Invalid value");
      }
    }
  }
  delay(1000);
  float c = LoadCell.getData() / mass;
  float offset = 0;
  
  LoadCell.setCalFactor(c);
  Serial.print("Calculated calibration value is: ");
  Serial.print(c);
  Serial.println(", use this in your project sketch");
  f = 0;
  Serial.print("Save this value to EEPROM address ");
  Serial.print(eepromAddressCal);
  Serial.println("? y/n");
  while (f == 0) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
        EEPROM.put(eepromAddressCal, c);
        Serial.print("Calibration value ");
        Serial.print(c);
        Serial.println(" saved to EEPROM address: ");
        //Serial.println(eepromAddressCal);
        //Serial.println(EEPROM.get(eepromAddressCal, c));
        //Serial.println(scale.get_offset());
        float offset = scale.get_offset();
        
        EEPROM.put(eepromAddressOffset,offset);
        //Serial.println(EEPROM.get(eepromAddressCal, c));
        //EEPROM.get(eepromAddressOffset,offset);
        Serial.println("Tare Offset value ");
        //Serial.print(offset);
        Serial.println(EEPROM.get(eepromAddressOffset, offset));
        Serial.print(" saved to EEPROM address: ");
        Serial.println(eepromAddressOffset);
        f = 1;  
        //Serial.println(EEPROM.get(eepromAddressCal, c));
        //Serial.println(EEPROM.get(eepromAddressOffset, offset));
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        f = 1;
      }
    }
  }
  Serial.println("End calibration");
  Serial.println("For manual edit, send 'c' from serial monitor");
  Serial.println("***");
}

void changeSavedCalFactor() {
  float c = LoadCell.getCalFactor();
  boolean f = 0;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(c);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  while (f == 0) {
    if (Serial.available() > 0) {
      c = Serial.parseFloat();
      if (c != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(c);
        LoadCell.setCalFactor(c);
        f = 1;
      }
      else {
        Serial.println("Invalid value, exit");
        return;
      }
    }
  }
  f = 0;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(eepromAddressCal);
  Serial.println("? y/n");
  while (f == 0) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
        #if defined(ESP8266)
        EEPROM.begin(512);
        #endif
        EEPROM.put(eepromAddressCal, c);
        #if defined(ESP8266)
        EEPROM.commit();
        #endif
        EEPROM.get(eepromAddressCal, c);
        Serial.print("Value ");
        Serial.print(c);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(eepromAddressCal);
        f = 1;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        f = 1;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}

void setup() {
  Serial.begin(9600); delay(10);
  Serial.println();
  Serial.println("Starting...");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  
  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  //delay(1000);
  Serial.println(scale.read());      // print a raw reading from the ADC
  //delay(1000);
  scale.set_offset(scale.read_average(20));
  //delay(1000);
  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));   // print the average of 20 readings from the 
  //delay(1000);
  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)
  //delay(1000);
  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
            // by the SCALE parameter (not set yet)
  //scale.set_scale();

  scale.tare();
  
  LoadCell.begin();
  long stabilisingtime = 2000; // tare precision can be improved by adding a few seconds of stabilising time
  LoadCell.start(stabilisingtime);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Tare timeout, check MCU>HX711 wiring and pin designations");
  }
  else {
    LoadCell.setCalFactor(1.0); // user set calibration value (float)
    Serial.println("Startup + tare is complete");
  }
  while (!LoadCell.update());
  calibrate();
}
void loop() {
  //update() should be called at least as often as HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS
  //longer delay in sketch will reduce effective sample rate (be carefull with delay() in the loop)
  LoadCell.update();
  //get smoothed value from the data set
  if (millis() > t + 250) {
    float i = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(i);
    t = millis();
  }

  //receive from serial terminal
  if (Serial.available() > 0) {
    float i;
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
    else if (inByte == 'c') changeSavedCalFactor();
  }

  //check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}








