/* MAIN CODE FOR ARDUINOS
  STILL NEEDS: FLOW SENSOR FUNCTION, LOAD CELL READING AND TRANSMITTING, LOAD CELL ZEROING
*/
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
    pinMode(pinOutput, OUTPUT); //that pin outpus
  }
  void on(){ 
    digitalWrite(pinOutput,HIGH); //turns the valve on
    stat = true;
  }
  void off(){
    digitalWrite(pinOutput,LOW); //turns the valve off
    stat = false;
  }
  boolean getStat(){
    return stat;
  }
  int getSubzone(){
    return subzone;
  }
}; 

/*The LoadCells class zeros, reads, prints, all of the data pertaining to all of the load cells*/
class LoadCells{
    int loadCellAmount; //
    float loadCellReading[];
    int loadCellPins[];
    int loadCellOffset[];
    int loadCellCali[];
    public:
    LoadCells(int pinsForLoadCells[],int offsetForLoadCell[], int caliForLoadCell[]){
      loadCellAmount = sizeof(pinsForLoadCells);
      loadCellPins[loadCellAmount];
      loadCellReading[loadCellAmount];
      loadCellOffset[loadCellAmount];      
      loadCellCali[loadCellAmount];
      for(int i = 0; i<loadCellAmount; i++){
        loadCellPins[i] = pinsForLoadCells[i];
        pinMode(loadCellPins[i], INPUT);
      }
      for(int i = 0; i<loadCellAmount; i++){
        loadCellOffset[i] = offsetForLoadCell[i];
      }
      for(int i = 0; i<loadCellAmount; i++){
        loadCellCali[i] = caliForLoadCell[i];
      }
    }
    void setLoadCellPins(int pinsForLoadCells[]){
      for(int i = 0; i<loadCellAmount; i++){
        loadCellPins[i] = pinsForLoadCells[i];
        pinMode(loadCellPins[i], INPUT);
      }
    }
    void zeroLoadCells(){
      
    }
    void readLoadCells(){
      int voltageReading[loadCellAmount];
      for(int i = 0; i < loadCellAmount;i++){
        voltageReading[i] = digitalRead(loadCellPins[i]);
        loadCellReading[i] = voltageReading[i]*loadCellCali[i]+loadCellOffset[i];
        Serial.println(loadCellReading[i],3);
      }
    }
};

/*The FlowSensor class takes care of measuring the flow from the pump to the emitters*/
class FlowSensor {
  int pinInput; //pin will be read
  int totalNeededVolume; //total volume is needed
  int readVolume; //what volume has been read
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
  
  void setNeededVolume(int inputVolume){ 
    totalNeededVolume = inputVolume; //sets how much fluid we plan on reading
  }
  int getVolume(){
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
  void readingVolume(int neededVolume){
    //reads the volume
    useInterrupt(true);
    while(neededVolume <= readVolume){
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
    
      Serial.print(liters); Serial.println(" Liters");
      delay(100);
    }
    useInterrupt(false);
  }
};



//pins for all the load cells
static int loadCellPinInputs[] = {15, 16 ,17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42};
static int offsetCellPinInputs[] = {15, 16 ,17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42};
static int caliCellPinInputs[] = {15, 16 ,17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42};



/*all the valves, flow sensors, pumps, and load cells initialize*/
Valve pump(52,0,'p'); //THIS IS A PUMP, NOT A VALVE, I'M JUST USING THE VALVE CLASS FOR SIMPLICITY
Valve waterValveA1(51,0,'t'); //Tank A1 valve; Water Tank
Valve tankValveA2(50,1,'t'); //Tank A2 valve;
Valve tankValveA3(49,2,'t'); //Tank A3 valve;
Valve tankValveA4(48,3,'t'); //Tank A4 valve;

Valve emitterValveA(47,1,'e'); //Emitter A; Subzone 1
Valve emitterValveB(46,2,'e'); //Emitter B; Subzone 2
Valve emitterValveC(45,3,'e'); //Emitter C; Subzone 3
Valve emitterValveD(44,4,'e'); //Emitter D; Subzone 4

FlowSensor flow(43);// sets up the flow sensor

LoadCells loadCells(loadCellPinInputs, offsetCellPinInputs, caliCellPinInputs);

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
  
}

void loop(){
  if(false){ //to be changed
    //other items
  }
  else{ //read serial data from PYTHON
    readData();
  }
}

/*reads the data taken from the usb serial connection*/
void readData() 
{
   if (Serial.available() > 0){ //Checks if there is serial available 
    String go = Serial.readStringUntil(';'); //Reads until ';'
    if (go.equals("waterSubzone")) //waters subzone; format: waterSubzone;1;10 <- waters subzone 1 for 10 milliliters
    {
      int subzoneToWater = Serial.parseInt(); //sets subzone to water
      int volumeToWater =  Serial.parseInt(); //sets how much total water is needed; INDIVIDUAL WATER MUST BE SET IN PYTHON
      Serial.println("Subzone Watered"); //confirms subzones watered
    }
    else if (go.equals("zeroLoadCells")){ //zeros all of the load cells
      Serial.println("Load Cells Zeroed"); //confirms load cell zeroing
      loadCells.zeroLoadCells();
    }
    else if (go.equals("readLoadCells")){ //read loads and returns array of readings
      loadCells.readLoadCells();
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
      tankValveA2.on();
      emitterValveA.on();
      pump.on();
      //reads the volume from the flow meter
      flow.readingVolume(flow.getVolume());
      pump.off();
      //Flushes the Subzone
      waterValveA1.on();
      tankValveA2.off();
      pump.on();
      flow.readingVolume(500); //adjustable flush volume
      //SHUT OFF
      pump.off();
      waterValveA1.off();
      emitterValveA.off();
      break;
    case 2:
      //Watering the second subzone
      //turns on valve A3, emitter B, and pump
      tankValveA3.on();
      emitterValveB.on();
      pump.on();
      //reads the volume from the flow meter
      flow.readingVolume(flow.getVolume());
      pump.off();
      //Flushes the Subzone
      waterValveA1.on();
      tankValveA3.off();
      pump.on();
      flow.readingVolume(500); //adjustable flush volume
      //SHUT OFF
      pump.off();
      waterValveA1.off();
      emitterValveB.off();
      break;
    case 3:
      //Watering the third subzone
      //turns on valve A4, emitter C, and pump
      tankValveA4.on();
      emitterValveC.on();
      pump.on();
      //reads the volume from the flow meter
      flow.readingVolume(flow.getVolume());
      pump.off();
      //Flushes the Subzone
      waterValveA1.on();
      tankValveA4.off();
      pump.on();
      flow.readingVolume(500); //adjustable flush volume
      //SHUT OFF
      pump.off();
      waterValveA1.off();
      emitterValveC.off();
      break;
    case 4:
      //Watering the fourth subzone
      //turns on valve A4, emitter D, and pump
      tankValveA4.on();
      emitterValveD.on();
      pump.on();
      //reads the volume from the flow meter
      flow.readingVolume(flow.getVolume());
      pump.off();
      //Flushes the Subzone
      waterValveA1.on();
      tankValveA4.off();
      pump.on();
      flow.readingVolume(500); //adjustable flush volume
      //SHUT OFF
      pump.off();
      waterValveA1.off();
      emitterValveD.off();
      break;
  }
}
  
