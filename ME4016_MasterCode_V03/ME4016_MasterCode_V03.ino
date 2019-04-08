/* MAIN CODE FOR ARDUINOS
  STILL NEEDS: FLOW SENSOR FUNCTION, LOAD CELL READING AND TRANSMITTING, LOAD CELL ZEROING
*/

#include "HX711.h"
#include <EEPROM.h>
#include <config.h>
#include <HX711_ADC.h>

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
      Serial.println("1");
      int voltageReading[loadCellAmount];
      for(int i = 0; i < loadCellAmount;i++){
        //loadCellReading[i] = scale[i].read_average(5);//scale[i].get_units();
        //voltageReading[i] = digitalRead(loadCellPins[i]); //DELETE?
        //loadCellReading[i] = voltageReading[i]*loadCellCali[i]+loadCellOffset[i]; //DELETE?
        Serial.println(loadCellReading[i],3);
      }
    }
};

/*The FlowSensor class takes care of measuring the flow from the pump to the emitters*/
class FlowSensor {
  int pinInput; //pin will be read
  float totalNeededVolume; //total volume is needed
  float readVolume; //what volume has been read
  uint16_t pulses;
  uint8_t lastFlowPinState;
  uint32_t lastFlowRateTimer;
  float flowRate;
  public:
  FlowSensor(int readPin){
    pinInput = readPin; //this will be the pin that is read
    totalNeededVolume = 0; //place holder. Initially we aren't reading anything
    pinMode(pinInput, INPUT); //pin will read
    pulses = 0;
    lastFlowRateTimer = 0;
    flowRate = 0.0;
  }
  
  void setNeededVolume(float inputVolume){ 
    totalNeededVolume = inputVolume; //sets how much fluid we plan on reading
  }
  float getNeededVolume(){ 
    return totalNeededVolume; //sets how much fluid we plan on reading
  }
  void setReadVolume(float inputReadVolume){
    readVolume = inputReadVolume;
  }
  float getReadVolume(){
    return readVolume; //returns how much volume has been read volume 
  }

  void setPin(int inputPin){ 
    pinInput = inputPin; //sets the reading pin
  }
  int getPin(){
    return pinInput; //returns pin to be read
  }
  
  void setPulses(uint16_t pulseSet){ //sets the current pulses read
    pulses = pulseSet;
  }
  uint16_t getPulses(){ //gets the pulses read
    return pulses;
  }
  
  void setFlowRate(uint16_t flowSet){ //sets the current pulses read
    flowRate = flowSet;
  }
  float getFlowRate(){ //gets the flow rate
    return flowRate;
  }
  
  void setLastFlowPinState(uint8_t stateSet){ //sets the last flow pin state
    lastFlowPinState = stateSet;
  }
  uint8_t getLastFlowPinState(){ //gets the last flow pin state
    return lastFlowPinState;
  }
  
  void setLastFlowRateTimer(uint32_t timerSet){ //sets the last flow rate timer
    lastFlowRateTimer = timerSet;
  }
  uint32_t getLastFlowRateTimer(){ //sets the last flow rate timer
    return lastFlowRateTimer;
  }
  
  void useInterrupt(boolean v) { //uses the interrupt
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
  void readingVolume(){
    //reads the volume
    useInterrupt(true); //allows interrupt
    long startTime = millis();
    long endTime =millis()+40000;
    while(getNeededVolume() >= getReadVolume() && millis()<= endTime){
      Serial.print("Freq: "); Serial.println(getFlowRate());
      Serial.print("Pulses: "); Serial.println(getPulses(), DEC);
      
      // if a plastic sensor use the following calculation
      // Sensor Frequency (Hz) = 7.5 * Q (Liters/min)
      // Liters = Q * time elapsed (seconds) / 60 (seconds/minute)
      // Liters = (Frequency (Pulses/second) / 7.5) * time elapsed (seconds) / 60
      // Liters = Pulses / (7.5 * 60)
      float liters = getPulses();
      liters /= 7.5;
      liters /= 60.0;
      setReadVolume(liters);
      Serial.print(liters); Serial.println(" Liters");
      Serial.println(millis()-startTime);
      delay(50); //CONSIDER REPLACING WITH MILLIS() APPROACH
      
    }
    useInterrupt(false); //UN-allows interrupt
    setPulses(0);
    setReadVolume(0);
  }
};



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

FlowSensor flow(15);// sets up the flow sensor


//LoadCells loadCells(loadCellPinInputs, offsetCellPinInputs, caliCellPinInputs, loadCellClockInput); //sets up the load cell function

/* PIN INTERRUPT FOR FLOW SENSOR*/
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(flow.getPin());
  
  if (x == flow.getLastFlowPinState()) {
    flow.setLastFlowRateTimer(flow.getLastFlowRateTimer()+1);
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    flow.setPulses(flow.getPulses()+1);
  }
  flow.setLastFlowPinState(x);
  flow.setFlowRate(1000.0);
  flow.setFlowRate(flow.getFlowRate() / flow.getLastFlowRateTimer());  // in hertz
  flow.setLastFlowRateTimer(0);
}


void setup() 
{
  Serial.begin(9600); //begins usb serial connection
  //for(int i = 0;i<=53;i++){
  //  pinMode(i,OUTPUT);
  //  digitalWrite(i,LOW);
  //}
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
      float volumeToWater =  Serial.parseFloat(); //sets how much total water is needed; INDIVIDUAL WATER MUST BE SET IN PYTHON
      Serial.println("Subzone Watered"); //confirms subzones watered
      flow.setNeededVolume(volumeToWater); //sets the volume;
      waterSubzone(subzoneToWater); //watering

    }
    else if (go == 'z'){ //zeros all of the load cells
      Serial.println("Load Cells Zeroed"); //confirms load cell zeroing
      //loadCells.zeroLoadCells();
    }
    else if (go == 'r'){ //read loads and returns array of readings
      //loadCells.readLoadCells();
      Serial.println("Load Cells Read"); //confirms load cells read
    }
    else{ //anything else
      Serial.println("Error");
    }
  }
}


int readHumid(){
  return 0; //humidity placeholder
}
/*returns the temperature in _UNITS_*/
int readTemp(){
  return 0; //temp placeholder
}
/*Waters an individual Subzone*/
void waterSubzone(int subzoneToWater){
  switch (subzoneToWater) {
    case 1:
      //Watering the first subzone
      //turns on valve A2, emitter A, and pump
      tankValveA.on();
      emitterValveA.on();
      subzonePump.on();
      //reads the volume from the flow meter
      //flow.readingVolume(flow.getVolume()); FIX
      delay(20000);
      subzonePump.off();

      /*
      
      //Flushes the Subzone
      tankValveA.on();
      tankValveB.off();
      subzonePump.on();
      //flow.readingVolume(500); //adjustable flush volume
      delay(5000);
      //SHUT OFF
      subzonePump.off();
      tankValveA.off();

      */
      tankValveA.off();
      emitterValveA.off();
      break;
    case 2:
      //Watering the first subzone
      //turns on valve A2, emitter A, and pump
      tankValveB.on();
      emitterValveB.on();
      subzonePump.on();
      //reads the volume from the flow meter
      //flow.readingVolume(flow.getVolume()); FIX
      delay(20000);
      subzonePump.off();

      /*
      
      //Flushes the Subzone
      tankValveA.on();
      tankValveB.off();
      subzonePump.on();
      //flow.readingVolume(500); //adjustable flush volume
      delay(5000);
      //SHUT OFF
      subzonePump.off();
      tankValveA.off();

      */
      tankValveB.off();
      emitterValveB.off();
      break;
    case 3:
      //Watering the first subzone
      //turns on valve A2, emitter A, and pump
      tankValveC.on();
      emitterValveC.on();
      subzonePump.on();
      //reads the volume from the flow meter
      //flow.readingVolume(flow.getVolume()); FIX
      delay(20000);
      subzonePump.off();

      /*
      
      //Flushes the Subzone
      tankValveA.on();
      tankValveB.off();
      subzonePump.on();
      //flow.readingVolume(500); //adjustable flush volume
      delay(5000);
      //SHUT OFF
      subzonePump.off();
      tankValveA.off();

      */
      tankValveC.off();
      emitterValveC.off();
      break;
    case 4:
      //Watering the first subzone
      //turns on valve A2, emitter A, and pump
      tankValveD.on();
      emitterValveD.on();
      subzonePump.on();
      //reads the volume from the flow meter
      flow.readingVolume(); 
      //delay(20000);
      subzonePump.off();

      /*
      
      //Flushes the Subzone
      tankValveA.on();
      tankValveB.off();
      pump.on();
      //flow.readingVolume(500); //adjustable flush volume
      delay(5000);
      //SHUT OFF
      subzonePump.off();
      tankValveA.off();

      */
      tankValveD.off();
      emitterValveD.off();
      break;
  }
}

void flushSubzone(Valve tank, Valve emitter, Valve pump){
  emitter.on();
  tank.on();
  pump.on();
  flow.readingVolume();
  pump.off();
  tank.off();
  emitter.off();
}
