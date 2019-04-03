import serial
import time

ser1 = serial.Serial('COM3', baudrate = 9600, timeout = 1)
ser1.close()
ser2 = serial.Serial('COM4', baudrate = 9600, timeout = 1)
ser2.close()
time.sleep(3)
numPoints = 5
dataList = [0]*numPoints



def getValues1():

	ser1.open()
	ser1.write(b'g')
	arduinoData1 = ser1.readline().decode().split('\r\n')
	time.sleep(1)
	ser1.close()
	
	return arduinoData1


def getValues2():

	ser2.open()
	ser2.write(b'g')
	arduinoData2 = ser2.readline().decode().split('\r\n')
	time.sleep(1)
	ser2.close()
	
	return arduinoData2



portInput = input('Which Arduino do you want to collect data from? 1 or 2? ')

if portInput == '1':
	userInput = input('Get data points?')

	if userInput == 'y':
		for i in range(0,numPoints):
			data = getValues1()
			dataList[i] = data
       
		print(dataList)
		ser1.close()

	

if portInput == '2':
	userInput = input('Get data points? ')
	
	if userInput == 'y':
		for i in range(0,numPoints):
			data = getValues2()
			dataList[i] = data
       
		print(dataList)
		ser2.close()



