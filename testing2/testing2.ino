void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //for(int i = 0;i<=53;i++){
  //  pinMode(i,OUTPUT);
  //}
}

void loop() {
  readData();
}

void readData() 
{
   if (Serial.available() > 0){ //Checks if there is serial available 
    char go = Serial.read();
    delay(500);
    //Serial.println(go);
    if (go == 'r') //waters subzone; format: waterSubzone;1;10 <- waters subzone 1 for 10 milliliters
    {
      for(int i = 0;i<10;i++){
        Serial.print(i);
        delay(100);
      }
      
      //int subzoneToWater = Serial.parseInt(); //sets subzone to water
      //float volumeToWater =  Serial.parseFloat(); //sets how much total water is needed; INDIVIDUAL WATER MUST BE SET IN PYTHON
      //Serial.println(subzoneToWater);
      //Serial.println(volumeToWater);
      //Serial.print('w'); //confirms subzones watered
      Serial.print(';');

    }
    else if(go == 'w'){
      int subzoneToWater = Serial.parseInt(); //sets subzone to water
      int volumeToWater =  Serial.parseInt(); //sets how much total water is needed in mL; INDIVIDUAL WATER MUST BE SET IN PYTHON
      //Serial.println(subzoneToWater);
      //Serial.println(volumeToWater);
      delay(1500);
      Serial.print('g');
    }
    else{ //anything else
      Serial.print('e');
    }
    
  }
}
