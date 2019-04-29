volatile int pinStatus[54] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //status of the pin: HIGH or LOW

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for(int i = 0;i<=53;i++){ //sets up the pins and turns them off
    pinMode(i,OUTPUT);
    digitalWrite(i,LOW);
  }
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
