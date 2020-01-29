from __future__ import print_function
import time
from RF24 import *
import RPi.GPIO as GPIO
from numpy import random

import dumper

PIN_CE=22
HOMEPACKAGE_TRANSCEIVER_PHYSICALCHANNEL=97 # value must be within 0-125 ~ DEFAULT: 76

TRANSCEIVER_DEVICETYPE_SMARTSOCKET=0
TRANSCEIVER_DEVICETYPE_IPCAMERA=1
TRANSCEIVER_DEVICETYPE_HUB=2
TRANSCEIVER_DEVICETYPE_ANYDEVICE=3
TRANSCEIVER_DEVICETYPE_REPEATER=3

TRANSCEIVER_NRFTM_BITLOC_COMMAND=0
TRANSCEIVER_NRFTM_BITLOC_RDT=4
TRANSCEIVER_NRFTM_BITLOC_TDT=6

TRANSCEIVER_NRFTM_BITCOUNT_COMMAND=4
TRANSCEIVER_NRFTM_BITCOUNT_RDT=2
TRANSCEIVER_NRFTM_BITCOUNT_TDT=2

TRANSCEIVER_REQUEST_NOREQUEST = 0
TRANSCEIVER_REQUEST_GETPUBLICCHANNEL = 1
TRANSCEIVER_REQUEST_GETPOWERCONSUMPTION = 2
TRANSCEIVER_REQUEST_GETAVERAGECURRENT = 3
TRANSCEIVER_REQUEST_GETRELAYSTATE = 4
TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION = 5
TRANSCEIVER_REQUEST_GETWIFICREDENTIALS = 6
TRANSCEIVER_REQUEST_GETWIFILOSTPACKET = 7
TRANSCEIVER_REQUEST_GETIPADDRESS = 8
TRANSCEIVER_REQUEST_PINGPONG = 64
TRANSCEIVER_REQUEST_RESTART = 128

TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST=0
TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO=1
TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION=2
TRANSCEIVER_COMMAND_SMARTSOCKET_AVERAGECURRENT=3
TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE=4
TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION=5
TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS=6
TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS=7
TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG=13
TRANSCEIVER_COMMAND_DEBUG_BYTEMESSAGE=14
TRANSCEIVER_COMMAND_DEBUG_CHARMESSAGE=15

TRANSCEIVER_BYTELOC_NRFTM=0
TRANSCEIVER_BYTELOC_RECEIVERSK=1
TRANSCEIVER_BYTELOC_TRANSMITTERSK=6
TRANSCEIVER_BYTELOC_TRASHBIN=11
TRANSCEIVER_BYTELOC_PACKETINFO=13
TRANSCEIVER_BYTELOC_INFORMATION=14

TRANSCEIVER_BYTECOUNT_NRFTM=1
TRANSCEIVER_BYTECOUNT_RECEIVERSK=5
TRANSCEIVER_BYTECOUNT_TRANSMITTERSK=5
TRANSCEIVER_BYTECOUNT_TRASHBIN=2
TRANSCEIVER_BYTECOUNT_PACKETINFO=1
TRANSCEIVER_BYTECOUNT_INFORMATION=18
TRANSCEIVER_BYTECOUNT_WORDINFO=17

TRANSCEIVER_MAXPACKETSCOUNT=31 # 20 packets is the safest for arduinos

TRANSCEIVER_HUB_FETCHINGINTERVAL=5000 # every 5 seconds he Hub fetches data

GPIO.setmode(GPIO.BCM)       # set the gpio mode

  # set the pipe address. this address shoeld be entered on the receiver alo
__public_channel__ = bytearray([102,32,92,192,97])

radio = RF24(PIN_CE,0)
radio.begin()
radio.setChannel(HOMEPACKAGE_TRANSCEIVER_PHYSICALCHANNEL) # set the channel as 76 hex
radio.setDataRate(RF24_250KBPS) # set radio data rate
radio.setPALevel(RF24_PA_MAX) # set PA level

radio.openReadingPipe(0, __public_channel__)
radio.printDetails() # print basic detals of radio

class messagedefinition:
    def __init__(infoslice):
        infoslice.RDT = -1
        infoslice.TDT = -1
        infoslice.Command = -1
        infoslice.ReceiverSK = bytearray(5)
        infoslice.TransmitterSK = bytearray(5)
        infoslice.Trashbin = bytearray(2)
        infoslice.PacketInfo = 0
        infoslice.Information = bytearray(18)

def decryptpacket(packet):
    message=messagedefinition()
    message.RDT = bytereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT)
    message.TDT = bytereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT)
    message.Command = bytereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND)
    message.ReceiverSK = bytearray(packet[TRANSCEIVER_BYTELOC_RECEIVERSK:TRANSCEIVER_BYTELOC_RECEIVERSK+TRANSCEIVER_BYTECOUNT_RECEIVERSK])
    message.TransmitterSK = bytearray(packet[TRANSCEIVER_BYTELOC_TRANSMITTERSK:TRANSCEIVER_BYTELOC_TRANSMITTERSK+TRANSCEIVER_BYTECOUNT_TRANSMITTERSK])
    message.Trashbin = bytearray(packet[TRANSCEIVER_BYTELOC_TRASHBIN:TRANSCEIVER_BYTELOC_TRASHBIN+TRANSCEIVER_BYTECOUNT_TRASHBIN])
    message.PacketInfo = packet[TRANSCEIVER_BYTELOC_PACKETINFO]
    message.Information = bytearray(packet[TRANSCEIVER_BYTELOC_INFORMATION:TRANSCEIVER_BYTELOC_INFORMATION+TRANSCEIVER_BYTECOUNT_INFORMATION])
    return message

def encryptmessage(message):
    packet=bytearray(32)
    packet[TRANSCEIVER_BYTELOC_NRFTM]=message.Command+(message.RDT<<TRANSCEIVER_NRFTM_BITLOC_RDT)+(message.TDT<<TRANSCEIVER_NRFTM_BITLOC_TDT)
    packet[TRANSCEIVER_BYTELOC_RECEIVERSK:TRANSCEIVER_BYTELOC_RECEIVERSK+TRANSCEIVER_BYTECOUNT_RECEIVERSK]=message.ReceiverSK
    packet[TRANSCEIVER_BYTELOC_TRANSMITTERSK:TRANSCEIVER_BYTELOC_TRANSMITTERSK+TRANSCEIVER_BYTECOUNT_TRANSMITTERSK]=message.TransmitterSK
    packet[TRANSCEIVER_BYTELOC_PACKETINFO]=message.PacketInfo
    packet[TRANSCEIVER_BYTELOC_INFORMATION:TRANSCEIVER_BYTELOC_INFORMATION+TRANSCEIVER_BYTECOUNT_INFORMATION]=message.Information
    constructtrashbin(packet)
    return packet

def transmitpacket(packet,commandswitch=64,requestswitch=0,timeout=1):
    try:
        radio.stopListening()
        decryptedmessage=decryptpacket(packet)
        message=messagedefinition()
        from_channel=__public_channel__[:]
        target_channel=__private_channels__[__private_channels_switch__][:]
        if commandswitch==TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST:
            message.Information[0]=requestswitch
            if requestswitch==TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION or requestswitch==TRANSCEIVER_REQUEST_GETRELAYSTATE:
                message.Information[1]=__relay_state__[__private_channels_switch__][0]
                message.Information[2]=__relay_state__[__private_channels_switch__][1]
                message.Information[3]=1
                message.Information[TRANSCEIVER_BYTECOUNT_INFORMATION-(TRANSCEIVER_BYTECOUNT_RECEIVERSK+TRANSCEIVER_BYTECOUNT_TRANSMITTERSK):TRANSCEIVER_BYTECOUNT_INFORMATION]=packet[TRANSCEIVER_BYTELOC_RECEIVERSK:TRANSCEIVER_BYTELOC_TRANSMITTERSK+TRANSCEIVER_BYTECOUNT_TRANSMITTERSK]
        elif commandswitch==TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO:
            if decryptedmessage.ReceiverSK==decryptedmessage.TransmitterSK:
                message.Information[0:len(__public_channel__)]=__public_channel__[:]
                target_channel=bytearray(decryptedmessage.TransmitterSK)
                from_channel=target_channel[:]
        elif commandswitch==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
                message.Information[0]=0
                message.Information[1]=__relay_state__[__private_channels_switch__][0]
                message.Information[2]=__relay_state__[__private_channels_switch__][1]
        message.RDT=TRANSCEIVER_DEVICETYPE_SMARTSOCKET
        message.TDT=TRANSCEIVER_DEVICETYPE_HUB
        message.Command=commandswitch
        message.TransmitterSK=from_channel[:]
        message.ReceiverSK=target_channel[:]
        packet=encryptmessage(message)
        timersaver=time.time()
        transmitted=False
        while not transmitted:
            radio.openWritingPipe(target_channel)
            transmitted=radio.write(packet)
            if transmitted or time.time()-timersaver>timeout:
                break
            radio.openWritingPipe(__public_channel__)
            bytewritebits(packet[TRANSCEIVER_BYTELOC_NRFTM],TRANSCEIVER_DEVICETYPE_REPEATER,TRANSCEIVER_NRFTM_BITLOC_TDT,TRANSCEIVER_NRFTM_BITCOUNT_TDT)
            transmitted=radio.write(packet)
            if transmitted:
                #print("message for device (",list(target_channel),") was broadcasted to public channel (",list(__public_channel__),')')
                break
            bytewritebits(packet[TRANSCEIVER_BYTELOC_NRFTM],TRANSCEIVER_DEVICETYPE_HUB,TRANSCEIVER_NRFTM_BITLOC_TDT,TRANSCEIVER_NRFTM_BITCOUNT_TDT)
            radio.setDataRate(RF24_1MBPS if radio.getDataRate()==RF24_250KBPS else RF24_250KBPS)
            #print("Failed to transmit message to device (",list(target_channel),')')
        radio.setDataRate(RF24_250KBPS)
        radio.openReadingPipe(0,__public_channel__)
        radio.openReadingPipe(1, __private_channels__[__private_channels_switch__])
        radio.startListening()
        #print("here listening to:",list(__private_channels__[__private_channels_switch__]))
        return transmitted
    except:
        print("FATAL ERROR OCCURRED AT TRANSMITPACKET!!!")

def processRX(commandswitch=255):
    received = False
    timesaver = time.time()

    while not received and time.time() - timesaver <= 2.25:
        while radio.available():
            try:
                __packet__=radio.read(32)
                ##print(list(__packet__))
                message=decryptpacket(__packet__)
                ##print("RDT:",message.RDT)
                ##print("RDT:",message.TDT)
                ##print("Commandswitch:",commandswitch)
                ##print("Command:",message.Command)
                ##print("ReceiverSK:",list(message.ReceiverSK))
                ##print("TransmitterSK:",list(message.TransmitterSK))
                ##print("Trashbin:",list(message.Trashbin))
                ##print("PacketInfo:",message.PacketInfo)
                ##print("Information:",list(message.Information))
                if not (message.RDT == TRANSCEIVER_DEVICETYPE_HUB \
                and (message.TDT == TRANSCEIVER_DEVICETYPE_SMARTSOCKET or message.TDT == TRANSCEIVER_DEVICETYPE_REPEATER) \
                and (message.ReceiverSK==__public_channel__ \
                or (message.Command == TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST and message.ReceiverSK==__private_channels__[__private_channels_switch__])) \
                and message.TransmitterSK in __private_channels__):
                    continue
                if wasintrashbin(message.Trashbin):
                    continue
                if not received and (commandswitch == 255 or message.Command == commandswitch):
                    received = True
                if message.Command==TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST:
                    if __packet__[TRANSCEIVER_BYTELOC_INFORMATION] == TRANSCEIVER_REQUEST_GETPUBLICCHANNEL \
                    and message.ReceiverSK==message.TransmitterSK:
                        transmitpacket(__packet__,TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO)
                    else: continue
                elif message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION \
                or message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_AVERAGECURRENT \
                or message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION \
                or message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
                    if message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION or message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
                        reading_relay=[bitRead(__packet__[TRANSCEIVER_BYTELOC_INFORMATION],0),bitRead(__packet__[TRANSCEIVER_BYTELOC_INFORMATION],1)]
                        for index in range(0,len(__private_channels__)):
                            if __private_channels__[index]==message.TransmitterSK:
                                __relay_state__[index]=reading_relay
                                print("relay state=",__relay_state__[index])
                                print("changing relay of ",list(__private_channels__[index]))
                                break
                                
                        print("Relay 1 State: ","LOW" if reading_relay[0]==0 else "HIGH")
                        print("Relay 2 State: ","LOW" if reading_relay[1]==0 else "HIGH")
                        hasrelay=bitRead(__packet__[TRANSCEIVER_BYTELOC_INFORMATION],7)
                    WhData=[0]*2
                    mAData=[0]*2
                    for index in range(2):
                        if message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION or message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION:
                            ## power consumption fetching
                            whole = 0
                            decimal = 0
                            for i in range(2):
                                whole += message.Information[2 + i + (8*index)] << (8 * i)
                            decimal=message.Information[4 + (8*index)]+(message.Information[5 + (8*index)] << 8)
                            WhData[index]=whole+(decimal/10000)
                            ##
                            print("Relay ",index+1," Power Consumption(Wh): ",WhData[index])
                        if message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION or message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_AVERAGECURRENT:
                            ## average current fetching
                            whole = 0
                            decimal = 0
                            for i in range(2):
                                whole+=message.Information[6 + i + (8*index)] << (8 * i)
                            decimal=message.Information[8 + (8*index)]+(message.Information[9 + (8*index)] << 8)
                            mAData[index]=whole+(decimal/10000)
                            ##
                            print("Relay ",index+1," Average Current(mA): ",mAData[index])
                    if (message.Command==TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION):
                        alldata.append({'serial':list(message.TransmitterSK),'data':WhData,'relay':bool(hasrelay)})
                        print("dumped data")
                        print(alldata)
                timesaver=time.time()
            except:
                print("FATAL ERROR OCCURRED AT PROCESSRX!!!")
        if commandswitch==255:
            break
    return received

def constructtrashbin(packet):
    publictrashbin=[0]*2
    while True:
        publictrashbin[0] = random.randint(0,255)
        publictrashbin[1] = random.randint(0,255)
        if not wasintrashbin(publictrashbin):
            break
    packet[TRANSCEIVER_BYTELOC_TRASHBIN:TRANSCEIVER_BYTELOC_TRASHBIN+TRANSCEIVER_BYTECOUNT_TRASHBIN]=publictrashbin
    return packet

publictrashbin_instances=10
__trashbin__=[[0]*2]*publictrashbin_instances
__trashbin_switch__=0

def wasintrashbin(passedtrashbin):
    global __trashbin_switch__,__trashbin__
    for index in range(publictrashbin_instances):
        if __trashbin__[index]==passedtrashbin: # return true if they are the same
            return True # was at the trashbin
    if __trashbin_switch__>=publictrashbin_instances-1:
        __trashbin_switch__=0
    else:
        __trashbin_switch__+=1
    __trashbin__[__trashbin_switch__]=passedtrashbin[:]
    return False

def bytewritebits(data,writer,bitfrom,bitcount):
    result=0
    for i in range(bitfrom,bitfrom+bitcount):
        bitWrite(data,i,bitRead(writer,i))

def bytereadbits(data,bitfrom,bitcount):
    result=0
    for i in range( bitfrom,bitfrom+bitcount ):
        result+=bitRead(data,i)*(2**(i-bitfrom))
    return result

def bitRead(data,whichbit=0):
    return (data & (1 << whichbit)) >> whichbit

def bitWrite(data,whichbit,bitstate):
    if bitstate>0:
        data = data | (1<<whichbit)
    else:
        data = data & (0xFFFF-(1<<whichbit))

handler=dumper.FileHandler()
getrelayandpowertimeout = time.time()      #start the time for checking delivery time
__packet__=bytearray(32)
__relay_state_old__=[0,0]
__private_channels_switch__=0
radio.startListening()

while True:
    try:
        try:
            database = handler.get()['appliance']
        except Exception as e:
            continue
        print(handler.get())
        __private_channels__=[]
        __relay_state__=[]
        for __loop_index__ in range(0,len(database)):
            __private_channels__.append(bytearray(database[__loop_index__]['serial']))
            __relay_state__.append(database[__loop_index__]['status'])
        if (len(__private_channels__)<=0):
            continue
        if time.time()-getrelayandpowertimeout>5:
            alldata=[] # data that will be sent to the server as empty
            channelswitchsaver=__private_channels_switch__
            for __loop_index__ in range(len(__private_channels__)):
                __private_channels_switch__=__loop_index__
                wastransmitted=transmitpacket(__packet__,TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST,TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION)
                if wastransmitted:
                    looptimeout=time.time()
                    wasreceived=False
                    while time.time()-looptimeout<=1.5 and not wasreceived:
                        wasreceived=processRX(TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION)
                else:
                    print("unresonsive device",list(__private_channels__[__private_channels_switch__]))
            __private_channels_switch__=channelswitchsaver
            getrelayandpowertimeout=time.time()
            handler.dump(alldata) # send this data to the server
        elif __relay_state_old__!=__relay_state__:
            channelswitchsaver=__private_channels_switch__
            for __loop_index__ in range(len(__private_channels__)):
                __private_channels_switch__=__loop_index__
                wastransmitted=transmitpacket(__packet__,TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE)
            __private_channels_switch__=channelswitchsaver
        else:
            if __private_channels_switch__<len(__private_channels__)-1:
                __private_channels_switch__+=1
            else:
                __private_channels_switch__=0
    
            #transmitpacket(__packet__,TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST,TRANSCEIVER_REQUEST_GETAVERAGECURRENT)
            processRX()
        __relay_state_old__=[[0]*2]*len(__private_channels__)
        __relay_state_old__=__relay_state__[:]
    except:
        print("FATAL!!! THERE WAS AN ERROR AT THE WHILE LOOP BLOCK!")


