from Tkinter import *
import serial
import datetime
import time, string
ser = serial.Serial()
root = Tk()
T = Text(root, height=2, width=30)
T.pack()
ser.port = "/dev/ttyUSB0" # may be called something different
T.insert(END,"/dev/ttyUSB0")
ser.baudrate = 19200 # may be different
ser.timeout=1
ser.open()
d=datetime.datetime.now()
d1=str(d)
f=open(d1,'w')
f.write("Time =                               ")
f.write("Oxygen=  \n")
while(1):
  if ser.isOpen():
    ser.write("MSR1\r")
    time.sleep(0.5)
    ser.write("TMP1\r")
    time.sleep(0.5)
    ser.write("RMR1 3 0 13\r")
    time.sleep(0.5)
    s=ser.readline()
    d=datetime.datetime.now()
    oxygen = string.split(s);
    oxy_percent=float(oxygen[9])/1000.0
    print(float(oxygen[9])/1000.0)
    print(d)
    d1=str(d)
    zb=str(oxy_percent)
    f.write(d1)
    f.write("         ")
    f.write(zb)
    f.write("\n")

