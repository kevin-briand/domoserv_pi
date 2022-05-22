import math
from luma.core.interface.serial import i2c
from luma.core.render import canvas
from luma.oled.device import sh1106
from threading import Timer
import time

serial = i2c(port=1, address=0x3C)
device = sh1106(serial, rotate=0)

temp = ""
hum = ""
version = ""
screen = 0
z1 = 0
z2 = 0
scanZ1 = 0
scanZ2 = 0
lastScreen = -1
updTemp = 0
switchState = 0
run = 1

def updateData():
    global temp, hum, screen, version, z1, z2, scanZ1, scanZ2
    f = open("/home/pi/domoserv_pi/build/temp.txt","r")
    for line in f:
        if line.find("temp=") != -1:
            temp = line.split("=")[1]
        elif line.find("hum=") != -1:
            hum = line.split("=")[1]
            hum = int(math.ceil(float(hum.split("\n")[0])))
        elif line.find("screen=") != -1:
            screen = int(line.split("=")[1])
        elif line.find("version=") != -1:
            version = line.split("=")[1]
        elif line.find("z1=") != -1:
            z1 = line.split("=")[1]
        elif line.find("z2=") != -1:
            z2 = line.split("=")[1]
        elif line.find("scanZ1=") != -1:
            scanZ1 = int(line.split("=")[1])
        elif line.find("scanZ2=") != -1:
            scanZ2 = int(line.split("=")[1])
        elif line.find("Stop") != -1:
            run = 0
    f.close()

def updateTemp():
    global updTemp
    updTemp = 1

#Screen Size
#X = 128
#Y = 64
def drawNumber(number,x,y):
    if number == 0:
        return [ (0+x,0+y), (10+x,0+y), (10+x,20+y), (0+x,20+y), (0+x,0+y) ]
    elif number == 1:
        return [ (10+x,0+y), (10+x,20+y) ]
    elif number == 2:
        return [ (0+x,0+y), (10+x,0+y), (10+x,10+y), (0+x,10+y), (0+x,20+y), (10+x,20+y) ]
    elif number == 3:
        return [ (0+x,0+y), (10+x,0+y), (10+x,10+y), (0+x,10+y), (10+x,10+y), (10+x,20+y), (0+x,20+y) ]
    elif number == 4:
        return [ (0+x,0+y), (0+x,10+y), (10+x,10+y), (10+x,0+y), (10+x,20+y) ]
    elif number == 5:
        return [ (10+x,0+y), (0+x,0+y), (0+x,10+y), (10+x,10+y), (10+x,20+y), (0+x,20+y) ]
    elif number == 6:
        return [ (10+x,0+y), (0+x,0+y), (0+x,20+y), (10+x,20+y), (10+x,10+y), (0+x,10+y) ]
    elif number == 7:
        return [ (0+x,0+y), (10+x,0+y), (10+x,20+y) ]
    elif number == 8:
        return [ (0+x,0+y), (10+x,0+y), (10+x,20+y), (0+x,20+y), (0+x,0+y), (0+x,10+y), (10+x,10+y) ]
    elif number == 9:
        return [ (10+x,10+y), (0+x,10+y), (0+x,0+y), (10+x,0+y), (10+x,20+y), (0+x,20+y) ]
    else:
        return [ (0,0) ]

def imgState(state,x,y,draw):
    if state == 0:
        draw.line([(1+x,6+y), (3+x,6+y)], fill="white")
        draw.line([(9+x,6+y), (11+x,6+y)], fill="white")
        draw.line([(6+x,1+y), (6+x,3+y)], fill="white")
        draw.line([(6+x,9+y), (6+x,11+y)], fill="white")
        draw.ellipse([(3+x,3+y), (9+x,9+y)], outline="white")

        draw.line([(1+x,1+y), (3+x,3+y)], fill="white")
        draw.line([(9+x,9+y), (11+x,11+y)], fill="white")
        draw.line([(9+x,3+y), (11+x,1+y)], fill="white")
        draw.line([(3+x,9+y), (1+x,11+y)], fill="white")
    elif state == 1:
        draw.arc([(2+x,2+y), (9+x, 9+y)], start=75, end=285, fill="white")
        draw.arc([(3+x,3+y), (8+x, 8+y)], start=90, end=270, fill="white")
    elif state == 2:
        draw.line([(5+x,0+y), (5+x,10+y)], fill="white")
        draw.line([(0+x,5+y), (10+x,5+y)], fill="white")
        draw.line([(0+x,0+y), (10+x,10+y)], fill="white")
        draw.line([(0+x,10+y), (10+x,0+y)], fill="white")
        draw.line([(3+x,0+y), (3+x,2+y), (7+x,0+y)], fill="white")
        draw.line([(3+x,8+y), (5+x,10+y), (7+x,8+y)], fill="white")
        draw.line([(2+x,3+y), (0+x,5+y), (2+x,7+y)], fill="white")
        draw.line([(8+x,3+y), (10+x,5+y), (8+x,7+y)], fill="white")
        draw.line([(2+x,0+y), (2+x,2+y), (0+x,2+y)], fill="white")
        #draw.line([(8+x,10+y), (10+x,10+y), (10+x,8+y)], fill="white")
        #draw.line([(0+x,8+y), (2+x,10+y), (2+x,10+y)], fill="white")
        #draw.line([(8+x,0+y), (10+x,2+y), (10+x,2+y)], fill="white")
    return draw

def updateScreen():
    global screen, lastScreen, updTemp, temp, hum, version, z1, z2, scanZ1, scanZ2, switchState

    if screen != lastScreen or (screen == 0 and updTemp == 1):
        if screen == 0:
            one = ""
            two = ""
            three = ""
            virgule = False
            t = str(temp)
            for var in t:
                if var == ".":
                    virgule = True
                elif one == "" and virgule == False:
                    one = var
                elif two == "" and virgule == False:
                    two = var
                elif three == "" and virgule == True:
                    three = var
            if two == "":
                two = one 

            with canvas(device) as draw:
                
                #temperature
                if one != "":
                    draw.line(drawNumber(int(one),31,12), fill="white")
                    draw.line(drawNumber(int(one),32,13), fill="white")
                draw.line(drawNumber(int(two),46,12), fill="white")
                draw.line(drawNumber(int(two),47,13), fill="white")
                draw.line([(60,32), (61,32), (61,33), (60,33)], fill="white")
                draw.line(drawNumber(int(three),65,12), fill="white")
                draw.line(drawNumber(int(three),66,13), fill="white")
                draw.text((85,12), "°C", fill="white")
                
                #humidity
                draw.text((45,35), str(hum), fill="white")
                draw.text((60,35), "%", fill="white")
                
                #state
                draw.text((10,50), "Z1 ", fill="white")
                if int(scanZ1) == 0 or (int(scanZ1) == 1 and switchState == 1):
                    draw = imgState(int(z1),30,50,draw)
                draw.text((60,50), "Z2 ", fill="white")
                if int(scanZ2) == 0 or (int(scanZ2) == 1 and switchState == 1):
                    draw = imgState(int(z2),80,50,draw)
                if switchState == 1:
                    switchState = 0
                else:
                    switchState = 1

                #starting timer
                if int(scanZ1) == 1 or int(scanZ2) == 1:
                    timerScreen = Timer(interval=1, function=updateTemp)
                else:
                    timerScreen = Timer(interval=30, function=updateTemp)
                timerScreen.start()
                updTemp = 0
        elif screen == 1:
            with canvas(device) as draw:
                draw.line([(5,15),(8,15),(6,13)],fill="white")
                draw.line([(8,15),(6,17)],fill="white")
                draw.text((10,0), "Selection Ordre :", fill="white")
                draw.text((10, 10), "Zone 1 : ", fill="white")
                imgState(int(z1),70,10,draw)
                draw.text((10, 20), "Zone 2 : ", fill="white")
                imgState(int(z2),70,20,draw)
        elif screen == 2:
            with canvas(device) as draw:
                draw.line([(5,25),(8,25),(6,23)],fill="white")
                draw.line([(8,25),(6,27)],fill="white")
                draw.text((10,0), "Selection Ordre :", fill="white")
                draw.text((10, 10), "Zone 1 : ", fill="white")
                imgState(int(z1),70,10,draw)
                draw.text((10, 20), "Zone 2 : ", fill="white")
                imgState(int(z2),70,20,draw)
        elif screen == 3:    
            with canvas(device) as draw:
                draw.text((10, 5), "Domoserv_pi", fill="white")
                draw.text((10, 20), "Version : " + version, fill="white")
        lastScreen = screen        


while run == 1:
    updateData()
    updateScreen()
    time.sleep(0.5)