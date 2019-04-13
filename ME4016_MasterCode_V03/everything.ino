//#include "DHT.h"
#include "HX711.h"  //You must have this library in your arduino library folder
#include <config.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

//HX711 constructor (dout pin, sck pin):
//HX711_ADC LoadCell(9, 8); //ARRAY NEEDED TO SWITCH DOUT PIN, MAYBE INITIALIZE LATER OR WITHOUT PIN DESIGNATION
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define CLK 14 //analog pin connected to clock on HX711 
//DHT dht(DHTPIN, DHTTYPE);
HX711 scale; 
 
int eepromAddressCal = 0; //addresses for calibration factor and offset in EEPROM
int eepromAddressOffset = 0; 
//int DOUT[] = {26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53}; //array of pins connected to DOUT on HX711
int DOUT[] = {30,31,32,33};//,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,51,52,53};
long t;

int serInLen = 25;  //GOBETWINO VARIABLES???
char serInString[25];
int logValue1=0;
int logValue2=0;
int logValue3=0;
int result;

float cal = 0; //calibration factor for scale
float offset = 0; //offset value for scale


void setup() {
  //pins for fluid control??? HIGHLY VARIABLE TO CHANGE
  //LABELING THESE FOR CLARITY WOULD HELP
  /*
  pinMode(22, OUTPUT); 
  pinMode(23, OUTPUT);
  pinMode(24, OUTPUT); 
  pinMode(25, OUTPUT); 
  pinMode(26, OUTPUT); 
  pinMode(27, OUTPUT); 
  pinMode(28, OUTPUT);
  pinMode(29, OUTPUT); 
  pinMode(30, OUTPUT); 
  pinMode(31, OUTPUT); 
  pinMode(32, OUTPUT); 
  pinMode(33, OUTPUT);
  pinMode(34, OUTPUT); 
  pinMode(35, OUTPUT); 
  pinMode(36, OUTPUT); 
  pinMode(37, OUTPUT); 
  pinMode(38, OUTPUT);
  pinMode(39, OUTPUT); 
  pinMode(40, OUTPUT); 
  pinMode(41, OUTPUT); 
  pinMode(42, OUTPUT); 
  pinMode(43, OUTPUT); 
  pinMode(45, OUTPUT); 
  pinMode(46, OUTPUT); 
  pinMode(47, OUTPUT); 
  pinMode(48, OUTPUT); 
  */
  
  Serial.begin(9600); delay(10);
  const int triggerPin = 13;
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, HIGH);
  // Setup serial comm. Initialize random function.

  //Serial.println();
  //Serial.println("Starting...");
  //scale.begin(DOUT,CLK);
  //scale.set_offset(EEPROM.get(eepromAddressOffset,offset));
  //Serial.println(EEPROM.get(eepromAddressCal,cal));
  //scale.set_scale(EEPROM.get(eepromAddressCal,cal));                      
  //Serial.println(EEPROM.get(eepromAddressCal,cal)); //lines for testing
  //scale.tare();                // reset the scale to 0
  //scale.set_offset(EEPROM.get(eepromAddressOffset,offset));
  //Serial.println(EEPROM.get(eepromAddressOffset,offset));

  //randomSeed(analogRead(0));
  //delay(5000); 
  // Use the CPTEST copy file command to make a copy of a new empty logfile
  Serial.println("#S|CPTEST|[]#");
  readSerialString(serInString,1000);
  //scale.set_scale();
  //scale.tare(); //Reset the scale to 0
  //Serial.println("DHTxx test!");
  //dht.begin();
  /*Serial.print("#S|LOGTEST|[");
  Serial.print("Timestamp");
  Serial.print(",");
  Serial.print("Humidity");
  Serial.print(",");
  Serial.print("Temperature(F)");
  Serial.print(",");
  Serial.print("Weight(kg)");
  Serial.println("]#"); */
  // There ought to be a check here for a non 0 return value indicating an error and some error handeling
}

void loop() {
 loadCellReading();
}

void loadCellReading() {
  int i = 1;
  int doutIndex = 0; //index for dout array for analog pins  
  float readings[] = {};
  int readingsSize = 0;
  //readings[0] = random(0,1000); //FOR TESTING PURPOSES
  //readings[1] = random(0,1000);

  //float h = dht.readHumidity();
  //float f = dht.readTemperature(true);

  for(i = 1; i <= 31; i = i + 10){ //CHANGE TO INDEX LIMIT UP TO NUMBER OF LOAD CELLS, 831 FOR 84 LOAD CELLS
    int eepromAddressCal = 1; //i; //eeprom addresses for calibration factor and offset
    int eepromAddressOffset = 6; //i+5;
    
    float cal = 0; //holds values for calibration and offset
    float offset = 0;
    //Serial.println(eepromAddressCal);
    
    scale.begin(DOUT[doutIndex],CLK);
    //Serial.println("test");
    //Serial.println("test");
    //scale.set_offset(EEPROM.get(eepromAddressOffset,offset));
    //Serial.println(EEPROM.get(eepromAddressCal,cal));
    scale.set_scale(EEPROM.get(eepromAddressCal,cal)); //obtains calibration and offset value corresponding to proper load cell                     
    //Serial.println(EEPROM.get(eepromAddressCal,cal)); 
    scale.set_offset(EEPROM.get(eepromAddressOffset,offset));
    //Serial.println(EEPROM.get(eepromAddressOffset,offset));
   

    //Serial.println(cal);
    
    readings[doutIndex] = scale.get_units(10);
    //readings[doutIndex+2] = random(0,1000);
    Serial.println(readings[doutIndex]);
    Serial.println(doutIndex);

    //logData(h,f,r); //calls function to log data to excel through gobetwino
                    //MIGHT SCRAP GOBETWINO
    doutIndex++; //move to next load cell by changing pin
    delay(1000); 
  }
    //Serial.println("test");
  //logData(readings[0],readings[1],readings[2]);
  //readingsSize = sizeof(readings);
  
  int j = 0;
  for (j = 0; j <= 1; j++) {
  float a = readings[j];
  //Serial.println(readings[j]);
  //Serial.println(a);
  }
}

void multiZero() {
  Serial.begin(9600); delay(10); //IS THIS NEEDED, PROBABLY NOT
  Serial.println();
  Serial.println("Starting...");
  
  //Start loop here for zeroing and switching load cells
  int i = 1;
  int doutIndex = 0; //index for dout array for analog pins
  for (i = 1; i <= 11; i = i + 10) //CHANGE TO INDEX LIMIT UP TO NUMBER OF LOAD CELLS, 831 FOR 84 LOAD CELLS
  {
    scale.begin(DOUT[doutIndex], CLK);
    int eepromAddressOffset = i+5; //address of offset values
    scale.tare(); //zero scale and save offset value
    float offset = scale.get_offset(); //obtains temporary offset value
    EEPROM.put(eepromAddressOffset,offset); //places offset value into eeprom address

    Serial.println("Tare Offset value "); //prints offset value in serial monitor for confirmation
    Serial.println(EEPROM.get(eepromAddressOffset, offset));
    Serial.print(" saved to EEPROM address: ");
    Serial.println(eepromAddressOffset);
    Serial.print("For load cell at array location: ");
    Serial.println(doutIndex);  
    doutIndex++; //index to next load cell
    //end loop for zeroing here
  }
  Serial.println("Load cell zeroing complete");
}

void multiCalibration() {

}

void fluidCycleAllEmitters() { //DOES THIS ACTUALLY COVER ALL TANKS??
                               //DELAY TIMING WILL NEED VARIABLES
                               
//emitter 1 code
  digitalWrite(22, HIGH),digitalWrite(31, HIGH),digitalWrite(40, HIGH);//Opens tank 1 valve
  digitalWrite(26, HIGH),digitalWrite(35, HIGH),digitalWrite(44, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(22, HIGH),digitalWrite(31, HIGH),digitalWrite(40, HIGH);// Closes tank 1 valve
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  delay(5000); //Runs pump for flushing
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(26, HIGH),digitalWrite(35, HIGH),digitalWrite(44, HIGH); //Closes emitter 1 valve

  //emitter 2 code
  digitalWrite(23, HIGH),digitalWrite(32, HIGH),digitalWrite(41, HIGH);//Opens tank 1 valve
  digitalWrite(27, HIGH),digitalWrite(36, HIGH),digitalWrite(45, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(23, HIGH),digitalWrite(32, HIGH),digitalWrite(41, HIGH);// Closes tank 1 valve
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  delay(5000); //Runs pump for flushing
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(27, HIGH),digitalWrite(36, HIGH),digitalWrite(45, HIGH); //Closes emitter 1 valve

    //emitter 3 code
  digitalWrite(24, HIGH),digitalWrite(33, HIGH),digitalWrite(42, HIGH);//Opens tank 1 valve
  digitalWrite(28, HIGH),digitalWrite(37, HIGH),digitalWrite(46, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(24, HIGH),digitalWrite(33, HIGH),digitalWrite(42, HIGH);// Closes tank 1 valve
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  delay(5000); //Runs pump for flushing
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(44, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(28, HIGH),digitalWrite(37, HIGH),digitalWrite(46, HIGH); //Closes emitter 1 valve

    //emitter 4 code
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  digitalWrite(29, HIGH),digitalWrite(38, HIGH),digitalWrite(47, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(44, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(29, HIGH),digitalWrite(38, HIGH),digitalWrite(47, HIGH); //Closes emitter 1 valve  
}

void fluidCycleEmitter1() {
  //emitter 1 code
  digitalWrite(22, HIGH),digitalWrite(31, HIGH),digitalWrite(40, HIGH);//Opens tank 1 valve
  digitalWrite(26, HIGH),digitalWrite(35, HIGH),digitalWrite(44, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(22, HIGH),digitalWrite(31, HIGH),digitalWrite(40, HIGH);// Closes tank 1 valve
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  delay(5000); //Runs pump for flushing
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(26, HIGH),digitalWrite(35, HIGH),digitalWrite(44, HIGH); //Closes emitter 1 valve
}

void fluidCycleEmitter2() {
  //emitter 2 code
  digitalWrite(23, HIGH),digitalWrite(32, HIGH),digitalWrite(41, HIGH);//Opens tank 1 valve
  digitalWrite(27, HIGH),digitalWrite(36, HIGH),digitalWrite(45, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(23, HIGH),digitalWrite(32, HIGH),digitalWrite(41, HIGH);// Closes tank 1 valve
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  delay(5000); //Runs pump for flushing
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(27, HIGH),digitalWrite(36, HIGH),digitalWrite(45, HIGH); //Closes emitter 1 valve
}

void fluidCycleEmitter3() {
  //emitter 3 code
  digitalWrite(24, HIGH),digitalWrite(33, HIGH),digitalWrite(42, HIGH);//Opens tank 1 valve
  digitalWrite(28, HIGH),digitalWrite(37, HIGH),digitalWrite(46, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(24, HIGH),digitalWrite(33, HIGH),digitalWrite(42, HIGH);// Closes tank 1 valve
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  delay(5000); //Runs pump for flushing
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(44, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(28, HIGH),digitalWrite(37, HIGH),digitalWrite(46, HIGH); //Closes emitter 1 valve
}

void fluidCycleEmitter4() { //WHY IS THIS SHORTER THAN THE OTHER 3
  //emitter 4 code
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(43, HIGH);//Opens tank 4/DI valve
  digitalWrite(29, HIGH),digitalWrite(38, HIGH),digitalWrite(47, HIGH);//Opens emitter 1 valve
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH);//turns on pump
  delay(5000);     // Runs pump to deliver nutrients for time 
  digitalWrite(25, HIGH),digitalWrite(34, HIGH),digitalWrite(44, HIGH); //Closes tank 4/DI valve
  delay(500); // Time for system to clear to reduce pump wind down effect
  digitalWrite(30, HIGH),digitalWrite(39, HIGH),digitalWrite(48, HIGH); //turns off pump
  digitalWrite(29, HIGH),digitalWrite(38, HIGH),digitalWrite(47, HIGH); //Closes emitter 1 valve  
}

void logData( float value1, float value2, float value3) 
{
   char buffer[5];
  
   //Serial.print("#S|LOGTEST|[");
   //Serial.print(itoa((value1), buffer, 10));
   //Serial.print(",");
   Serial.print(value1,3);
   Serial.print(",");
   //Serial.print(itoa((value2), buffer, 10));
   Serial.print(value2,3);
   Serial.print(",");
   //Serial.print(itoa((value3), buffer, 10));
   Serial.print(value3,3);
   Serial.println("]#");
   readSerialString(serInString,1000);
   // There ought to be a check here for a non 0 return value indicating an error and some error handeling
} 

//read a string from the serial and store it in an array
//you must supply the array variable - return if timeOut ms passes before the sting is read
void readSerialString (char *strArray,long timeOut) 
{
   long startTime=millis();
   int i;

   while (!Serial.available()) {
      if (millis()-startTime >= timeOut) {
         return;
      }
   }
   while (Serial.available() && i < serInLen) {
      strArray[i] = Serial.read();
      i++;
   }
}
