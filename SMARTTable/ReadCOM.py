import sys
import serial

ser1= serial.Serial('COM3', baudrate = 9600, timeout = 1)
ser2 = serial.Serial('COM4', baudrate = 9600, timeout = 1)

while True:  # The program never ends... will be killed when master is over.
    # sys.stdin.readline()

	ser1.write('serial command here\n') #send command to serial port
	output1= ser1.readline().decode().split('\r\n') # read output
	
	ser2.write('serial command here\n') #send command to serial port
	output2= ser2.readline().decode().split('\r\n') # read output

	sys.stdout.write(output1) #write output to stdout
	sys.stdout.flush()
	
	sys.stdout.write(output2) #write output to stdout
	sys.stdout.flush()
	