import serial
import time
ser1 = serial.Serial('COM10', baudrate = 9600, timeout = 1) #opens serial for arduino 1
ser1.close()
#ser2 = serial.Serial('COM8', baudrate = 9600, timeout = 1) #opens serial for arduino 2
#ser2.close()
#ser3 = serial.Serial('COM9', baudrate = 9600, timeout = 1) #opens serial for arduino 3
#ser3.close()
time.sleep(1)

numPoints = 1

dataList = [0]*numPoints



def waterSubzone(serPort,subzone,volume):
    time.sleep(1)
    serPort.write(b'w')
    serPort.write(subzone)
    serPort.write(volume)
    time.sleep(1)
    confirm = serPort.readline()#.decode.split('\r\n')
    print(confirm)
    print('confirm')
    

def getValues():

    ser1.write(b'g')

    arduinoData = ser1.readline().decode().split('\r\n')
    print(arduinoData)
    #return arduinoData[0]
while(1):
    userInput = input('Action?')
    if userInput == 'w':
        ser1.open()
        waterSubzone(ser1,1,1)
        #for i in range(0,numPoints):
            #time.sleep(1)
            #data = getValues()
            #data = int(data)
            #dataList[i] = data
            #dataAvg = sum(dataList)/numPoints

            #print(data)

           #print(dataList)
        ser1.close()