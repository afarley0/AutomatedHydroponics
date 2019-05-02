volatile int pinStatus[54] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //status of the pin: HIGH or LOW
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for(int i = 0;i<=53;i++){ //sets up the pins and turns them off
    pinMode(i,OUTPUT);
    digitalWrite(i,LOW);
  }
  digitalWrite(10, HIGH);
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(14, HIGH);
  digitalWrite(15, HIGH);
  digitalWrite(16, HIGH);
   
  Serial.print('.');
}
void loop() {
   if (Serial.available() > 0){ //Checks if there is serial available 
    int pin = Serial.parseInt(); //reads the pin input
    if(pinStatus[pin] == 0){ //toggles status
      pinStatus[pin] = 1;
    }
    else{
      pinStatus[pin] = 0;
    }
    digitalWrite(pin,pinStatus[pin]); //sets the pin to the current status
    Serial.print("pin ");
    Serial.print(pin);
    Serial.print(" = ");
    Serial.println(pinStatus[pin]);
    
    
  }
}


//
//
//
//#include <EEPROM.h>
//int loadCellAmount = 28;
//int eepromAddressCal = 2; //address where calibration and offset values will be stored
//int eepromAddressOffset = 6;
//
//void setup() {
//  Serial.begin(9600); delay(10);
//  bool startCal = false;
//  while(!startCal){
//    if(Serial.available()>0){
//      startCal = (Serial.read()=='s') ? true : false;
//      Serial.println("go");
//    }
//  }
//  for(int i=0;i<loadCellAmount;i++){
//    float cali = EEPROM.read(eepromAddressCal);
//    Serial.print("Load Cell ");
//    Serial.print(i);
//    Serial.print("Cali =  ");
//    Serial.println(cali,5);
//    eepromAddressCal = eepromAddressCal+10;
//  }
//}
//void loop() {
//  //Serial.println("Load Cells Calibrated");
//  delay(3000);
//}
