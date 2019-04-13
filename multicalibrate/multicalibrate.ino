#include <HX711.h> //Library A (scale.begin())
#include <config.h>
#include <HX711_ADC.h>//Library B
#include <EEPROM.h>

int eepromAddressCal = 1; //address where calibration and offset values will be stored
int eepromAddressOffset = 6;
const int LOADCELL_SCK_PIN = 16; //HX711 constructor
const int LOADCELL_DOUT_PIN[] = {18};
static int loadCellAmount = 1;
volatile float cali[1];
volatile float offset[1];
//HX711 constructor (dout pin, sck pin):
//HX711_ADC LoadCell(LOADCELL_DOUT_PIN,LOADCELL_SCK_PIN);
//HX711 scale;

long t;

//void calibrate() {
//  Serial.println("***");
//  Serial.println("Start calibration:");
//  for(int i=1;i<loadCellAmount;i++){
//    Serial.print("It is assumed that the mcu was started with no load applied to the load cell.");
//    Serial.println(i);
//    Serial.println("Now, place your known mass on loadcell ");
//    Serial.println("then send the weight of this mass (i.e. 100.0) from serial monitor.");
//    float mass = 0; //known weight we input
//    boolean f = 0; //waiting value
//    while (f == 0) {
//      LoadCell[i].update(); //
//      if (Serial.available() > 0) {
//        mass = Serial.parseFloat(); //reads the known wieght
//        if (mass != 0) {
//          Serial.print("Known mass is: ");
//          Serial.println(mass);
//          f = 1;
//        }
//        else {
//          Serial.println("Invalid value");
//        }
//      }
//    }
//    //delay(1000);
//    float c = LoadCell[i].getData() / mass; //calibration 
//    float offset = 0; //original offset
//    
//    LoadCell[i].setCalFactor(c); //sets the calibration
//    Serial.print("Calculated calibration value is: ");
//    Serial.print(c);
//    Serial.println(", use this in your project sketch");
////    f = 0;
////    Serial.print("Save this value to EEPROM address ");
////    Serial.print(eepromAddressCal);
////    Serial.println("? y/n");
////    while (f == 0) {
////      if (Serial.available() > 0) {
////        char inByte = Serial.read();
////        if (inByte == 'y') {
////          EEPROM.put(eepromAddressCal, c);
////          Serial.print("Calibration value ");
////          Serial.print(c);
////          Serial.println(" saved to EEPROM address: ");
////          //Serial.println(eepromAddressCal);
////          //Serial.println(EEPROM.get(eepromAddressCal, c));
////          //Serial.println(scale.get_offset());
////          float offset = scale.get_offset();
////          
////          EEPROM.put(eepromAddressOffset,offset);
////          //Serial.println(EEPROM.get(eepromAddressCal, c));
////          //EEPROM.get(eepromAddressOffset,offset);
////          Serial.println("Tare Offset value ");
////          //Serial.print(offset);
////          Serial.println(EEPROM.get(eepromAddressOffset, offset));
////          Serial.print(" saved to EEPROM address: ");
////          Serial.println(eepromAddressOffset);
////          f = 1;  
////          //Serial.println(EEPROM.get(eepromAddressCal, c));
////          //Serial.println(EEPROM.get(eepromAddressOffset, offset));
////        }
////        else if (inByte == 'n') {
////          Serial.println("Value not saved to EEPROM");
////          f = 1;
////        }
////      }
////    }
//  }
//  Serial.println("End calibration");
//  Serial.println("For manual edit, send 'c' from serial monitor");
//  Serial.println("***");
//}
HX711 scale[1];

void setup() {
  Serial.begin(9600); delay(10);
  bool startCal = false;
  while(!startCal){
    if(Serial.available()>0){
      startCal = (Serial.read()=='s') ? true : false;
      Serial.println("go");
    }
  }
  for(int i=0;i<loadCellAmount;i++){

    
    scale[i].begin(LOADCELL_DOUT_PIN[i], LOADCELL_SCK_PIN);
    Serial.println("Before setting up the scale:");
    Serial.print("read: \t\t");

    Serial.println(scale[i].read());      // print a raw reading from the ADC

    scale[i].set_offset(scale[i].read_average(20));

    Serial.print("read average: \t\t");
    Serial.println(scale[i].read_average(20));   // print the average of 20 readings from the 

    Serial.print("get value: \t\t");
    Serial.println(scale[i].get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)
    //delay(1000);
    Serial.print("get units: \t\t");
    Serial.println(scale[i].get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
              // by the SCALE parameter (not set yet)
    //scale.set_scale();
  
    scale[i].tare();
    HX711_ADC LoadCell(LOADCELL_DOUT_PIN[i],LOADCELL_SCK_PIN);
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

    Serial.println("Start calibration:");
    Serial.print("It is assumed that the mcu was started with no load applied to the load cell.");
    Serial.println(i);
    Serial.println("Now, place your known mass on loadcell ");
    Serial.println("then send the weight of this mass (i.e. 100.0) from serial monitor.");
    float mass = 0; //known weight we input
    boolean f = 0; //waiting value
    while (f == 0) {
      LoadCell.update(); //
      if (Serial.available() > 0) {
        mass = Serial.parseFloat(); //reads the known wieght
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
    //delay(1000);
    cali[i] = LoadCell.getData() / mass; //calibration 
    offset[i] = scale[i].get_offset();
    //float offset = 0; //original offset
    
    LoadCell.setCalFactor(cali[i]); //sets the calibration
    Serial.print("LOAD CELL: ");
    Serial.println(i);
    Serial.print("Calibration: ");
    Serial.println(cali[i]);
    Serial.print("Offset: ");
    Serial.println(offset[i]);
    Serial.println(' ');
    Serial.println(' ');

  }
}
void loop() {
  Serial.println("Load Cells Calibrated");
  delay(3000);
}
