/* MAIN CODE FOR ARDUINOS
  STILL NEEDS: FLOW SENSOR FUNCTION, LOAD CELL READING AND TRANSMITTING, LOAD CELL ZEROING
*/

#include "HX711.h"
#include <EEPROM.h>
#include <config.h>
#include <HX711_ADC.h>
#include <SparkFun_RHT03.h>

/* The Valve class encorporates all of the valves on the manifold, can also work for pump*/
class Valve{
  int pinOutput; //which pin will be toggled
  boolean stat; //is the valve on?
  char valveChar; //is it a tank or emitter valve
  int subzone; //which subzone
  public:
  Valve(int pin, int inputSubzone, char inputChar)  {
    pinOutput = pin; //sets the pin
    valveChar = inputChar; //sets the specific valve
    stat = false; //it starts off
    subzone = inputSubzone;
    pinMode(pinOutput, OUTPUT); //that pin outputs
    digitalWrite(pinOutput,LOW);
  }
  void on(){ 
    digitalWrite(pinOutput,HIGH); //turns the valve on
    stat = true;
  }
  void off(){
    digitalWrite(pinOutput,LOW); //turns the valve off
    stat = false;
  }
  boolean getStat(){ //gets the status of valve
    return stat;
  }
  int getSubzone(){ //returns subzone
    return subzone;
  }
}; 

/*The LoadCells class zeros, reads, prints, all of the data pertaining to all of the load cells*/
class LoadCells{
    int loadCellAmount; //how many load cells are we reading
    float loadCellReading[]; //what are the readings
    byte loadCellPins[]; //what pins are we reading from
    byte loadCellClock; //what is the clock pin
    int loadCellOffset[]; //what is the voltage offset
    int loadCellCali[]; //what is voltage calibrated gain
    HX711 scale[]; //initializes the load cell scale using the HX711 library
    public:
    LoadCells(byte pinsForLoadCell[],int offsetForLoadCell[], int caliForLoadCell[],byte clockForLoadCell){
      loadCellAmount = sizeof(pinsForLoadCell); //sets load cell amount
      loadCellPins[loadCellAmount]; //sets size for pin array
      loadCellReading[loadCellAmount]; //sets size for reading array
      loadCellOffset[loadCellAmount]; //sets size for offset array
      loadCellCali[loadCellAmount]; //sets size for calibration array
      scale[loadCellAmount]; //sets size for scale array
      loadCellClock = clockForLoadCell; //sets clock pin
      pinMode(loadCellClock, OUTPUT);
      digitalWrite(loadCellClock,LOW);
      
      for(int i = 0; i<loadCellAmount; i++){ 
        loadCellPins[i] = pinsForLoadCell[i]; // sets each individual pin
        loadCellOffset[i] = offsetForLoadCell[i]; // sets each individual offset
        loadCellCali[i] = caliForLoadCell[i]; // sets each individual calibration
        //pinMode(loadCellPins[i], INPUT); //POSSIBLY REMOVE?
        scale[i].begin(loadCellPins[i],loadCellClock); //initializes each scale
        scale[i].set_scale(loadCellCali[i]); //sets the scale calibration
        scale[i].set_offset(loadCellOffset[i]); //sets the scale offset
      }
    }
    void setLoadCellPins(byte pinsForLoadCell[]){ //sets each individual load cell pin and amount
      loadCellAmount = sizeof(pinsForLoadCell);
      for(int i = 0; i<loadCellAmount; i++){
        loadCellPins[i] = pinsForLoadCell[i];
        pinMode(loadCellPins[i], INPUT);
        digitalWrite(loadCellPins[i],LOW);
      }
    }
    void zeroLoadCells(){ //TO BE DONE!
      for(int i = 0; i < loadCellAmount;i++){
        scale[i].tare(); //should work?
      }
    }
    void readLoadCells(){ // Reads each individual load cell and prints
      //int voltageReading[loadCellAmount];
      for(int i = 0; i < loadCellAmount;i++){
        loadCellReading[i] = scale[i].get_units(10); //gets the average for 10 readings
        Serial.println(loadCellReading[i],3);
      }
    }
};

#define FLOWSENSORPIN 15

// count how many pulses!
volatile uint16_t pulses = 0;
// track the state of the pulse pin
volatile uint8_t lastflowpinstate;
// you can try to keep time of how long it is between pulses
volatile uint32_t lastflowratetimer = 0;
// and use that to calculate a flow rate
volatile float flowrate;
// the volume to total volume to water in mL
volatile int volumeToWater;
// Interrupt is called once a millisecond, looks for any pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN);
  
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    pulses++;
  }
  lastflowpinstate = x;
  if(lastflowratetimer == 0){
    flowrate = 0;
    return;
  }
  flowrate = 1000.0;
  flowrate /= lastflowratetimer;  // in hertz 
  lastflowratetimer = 0;
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}


//read pins, offset, calibration factors, and clock pin for all the load cells
//HARD CODED TEMPORARILY
static byte loadCellClockInput = 16;
static byte loadCellPinInputs[] = {26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
static int offsetCellPinInputs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int caliCellPinInputs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


/*all the valves, flow sensors, pumps, and load cells initialize*/
Valve subzonePump(21,0,'p'); //THIS IS A PUMP, NOT A VALVE, I'M JUST USING THE VALVE CLASS FOR SIMPLICITY
Valve tankValveA(22,0,'t'); //Tank A1 valve; Water Tank
Valve tankValveB(23,1,'t'); //Tank A2 valve;
Valve tankValveC(24,2,'t'); //Tank A3 valve;
Valve tankValveD(25,3,'t'); //Tank A4 valve;
 
Valve emitterValveA(17,1,'e'); //Emitter A; Subzone 1
Valve emitterValveB(18,2,'e'); //Emitter B; Subzone 2
Valve emitterValveC(19,3,'e'); //Emitter C; Subzone 3
Valve emitterValveD(20,4,'e'); //Emitter D; Subzone 4

const int RHT03_DATA_PIN = 4; // RHT03 data pin
RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

LoadCells loadCells(loadCellPinInputs, offsetCellPinInputs, caliCellPinInputs, loadCellClockInput); //sets up the load cell function

void setup() 
{
  Serial.begin(9600); //begins usb serial connection
  pinMode(FLOWSENSORPIN, INPUT);
  digitalWrite(FLOWSENSORPIN, HIGH);
  lastflowpinstate = digitalRead(FLOWSENSORPIN);
  rht.begin(RHT03_DATA_PIN);
}

void loop(){
  readData();
  
}

/*reads the data taken from the usb serial connection*/
void readData() 
{
   if (Serial.available() > 0){ //Checks if there is serial available 
    char go = Serial.read(); //Reads command
    if (go == 'w') //waters subzone; format: waterSubzone;1;10 <- waters subzone 1 for 10 milliliters
    {
      int subzoneToWater = Serial.parseInt(); //sets subzone to water
      volumeToWater =  Serial.parseInt(); //sets how much total water is needed in mL; INDIVIDUAL WATER MUST BE SET IN PYTHON
      //flow.setNeededVolume(volumeToWater); //sets the volume;
      switch (subzoneToWater) {
        case 1:
          waterSubzone(tankValveA, emitterValveA, subzonePump); //watering subzone 1
          break;
        case 2:
          waterSubzone(tankValveB, emitterValveB, subzonePump); //watering subzone 2
          break;
        case 3:
          waterSubzone(tankValveC, emitterValveC, subzonePump); //watering subzone 3
          break;
        case 4:
          waterSubzone(tankValveD, emitterValveD, subzonePump); //watering subzone 4
          break;
      }
      
      Serial.println('g'); //confirms subzones watered
    }
    else if (go == 'z'){ //zeros all of the load cells
      Serial.println("Load Cells Zeroed"); //confirms load cell zeroing
      loadCells.zeroLoadCells();
    }
    else if (go == 'r'){ //read loads and returns array of readings
      loadCells.readLoadCells();
      Serial.println("Load Cells Read"); //confirms load cells read
    }
    else if (go == 't'){ //read loads and returns array of readings
      //loadCells.readLoadCells();
      Serial.println(readHumid());
      Serial.println(readTemp());
      Serial.println("Temperature Read"); //confirms load cells read
    }
    else{ //anything else
      Serial.println("Error");
    }
  }
}

/* reads the volume */
void readingVolume(int inputVolume){
  long startTime = millis();
  float A = 32.195;
  float B = -706.12;
  long endTime = inputVolume*A + B;
  pulses = 0;
  float C = 0.2324;
  float D = 14.306 + 20 ;
  uint16_t endPulses = inputVolume*C + D;
  while(millis() < endTime && pulses < endPulses){
    
  }
  pulses = 0;
  lastflowpinstate = LOW;
  flowrate = 0;
  lastflowratetimer = 0;
  startTime = 0;
  
}

/*Waters an individual Subzone*/
void waterSubzone(Valve tank, Valve emitter, Valve pump){
  emitter.on();
  tank.on();
  pump.on();
  readingVolume(volumeToWater);
  pump.off();
  tank.off();
  emitter.off();
}
/*Reads the relative humidity*/
float readHumid(){
  // If successful, the update() function will return 1.
  // If update fails, it will return a value <0
  long startTime = millis()+5000;
  while(rht.update()< 1)
  {
    if(millis()>startTime){
      return NULL;
    }
    delay(RHT_READ_INTERVAL_MS);
  }
  float latestHumidity = rht.humidity();
  return latestHumidity;
}
/*Reads the temperature in C*/
float readTemp(){
  // If successful, the update() function will return 1.
  // If update fails, it will return a value <0
  long startTime = millis()+5000;
  while(rht.update()< 1)
  {
    if(millis()>startTime){
      return NULL;
    }
    delay(RHT_READ_INTERVAL_MS);
  }
  float latestTempC = rht.tempC();
  return latestTempC;
}
