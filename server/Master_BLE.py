from bluepy.btle import Scanner,Peripheral
import time
import subprocess
import re

IPCamera_SerialKey=["z8fqa2j6"] # registered ip camera serial keys

def getconnectednetworks():
    wificredentials=[]
    try:
        target_ssid=re.findall('(?<=ESSID:").*(?=")',str(subprocess.check_output(['sudo', 'iwgetid']),'utf-8'))
        wpa_supplicant = open("/etc/wpa_supplicant/wpa_supplicant.conf", "r").read()
        networkdata=re.findall('(?<=network={).*?(?=})',wpa_supplicant,re.DOTALL)
        for i in range(0,len(networkdata)):
            ssid=re.findall('(?<=ssid=").*?(?=")',networkdata[i])
            for index in range(0,len(target_ssid)):
                if ssid[0]==target_ssid[index]:
                    password=re.findall('(?<=psk=").*?(?=")',networkdata[i])
                    wificredentials.append({'ssid':ssid[0],'password':password[0] if len(password)>0 else ""})
    except Exception as e:
        print("AN ERROR HAPPENED",e)
    return wificredentials

BLE_IPCAMERA_Service_UUID     =   "eca9c05a-1250-41a6-9869-b455f8c96f80"
BLE_IPCAMERA_Characteristics_UUID="769c9614-8180-4722-9ce5-55a03b30fb5a"
BLE_Dot_KeyWord="nU)7S"
BLE_YouAreFree_KeyWord="oE9*3&#M0*a2vcA"
BLE_RequestCredentials_KeyWord="I75^SuSwIoNv&6y"
BLE_WrongCredential_KeyWord="iU8#9Dv_dieYb*3"
BLE_IPAddress_KeyWord="m*Ay@9X7&4Sp,u8"
BLE_SSID_KeyWord="Mq83Uc_3ZvSeLVe"
BLE_Password_KeyWord="p!i^p##0&eOsqm3"

networkcredentials=getconnectednetworks()
print("Fetched by BLE as Credentials:",networkcredentials)
if len(networkcredentials)>0:
  networkswitch=0
  scanner = Scanner()
  while True:
    allbledata=[]
    for i in range(0,len(IPCamera_SerialKey)):
      allbledata.append({'serial':IPCamera_SerialKey[i],'bledetected':False})
    devices=scanner.scan(2)
    for dev in devices:
        hasipcamservice=False
        for (adtype, desc, value) in dev.getScanData():
            macaddress=""
            if adtype==7 and value==BLE_IPCAMERA_Service_UUID:
              hasipcamservice=True
              serviceuuid=value
            elif adtype==9:
              macaddress=value
            if hasipcamservice and macaddress!="":
              break
        if hasipcamservice and macaddress!="":
          retrytimer=time.time()
          while(time.time()-retrytimer<=1):
            try:
              bleperipherical=Peripheral(macaddress)
              bleerror=0
              break
            except:
              bleerror=1
              time.sleep(0.25)
              continue
          if bleerror:
            continue
          retrytimer=time.time()
          while(time.time()-retrytimer<=1):
            try:
              service=bleperipherical.getServiceByUUID(BLE_IPCAMERA_Service_UUID)
              bleerror=0
              break
            except:
              bleerror=1
              time.sleep(0.25)
              continue
          if bleerror:
            continue
          uuid_service=service.uuid
          retrytimer=time.time()
          while(time.time()-retrytimer<=1):
            try:
              characteristic=service.getCharacteristics()[0]
              bleerror=0
              break
            except:
              bleerror=1
              time.sleep(0.25)
              continue
          if bleerror:
            continue
          try:
              if characteristic.supportsRead() and characteristic.uuid==BLE_IPCAMERA_Characteristics_UUID:
                BLE_message=str(characteristic.read(),'utf-8')
                
                for i in range(0,len(IPCamera_SerialKey)):
                  if BLE_message.startswith(IPCamera_SerialKey[i]) and BLE_message.endswith(IPCamera_SerialKey[i]):
                    allbledata[i]['bledetected']=True
                    BLE_Properties=characteristic.propertiesToString()
                    if BLE_message.find(BLE_RequestCredentials_KeyWord)!=-1 or BLE_message.find(BLE_WrongCredential_KeyWord)!=-1:
                      if BLE_message.find(BLE_WrongCredential_KeyWord)!=-1:
                        print("wrong credential said by ",IPCamera_SerialKey[i])
                        networkswitch=networkswitch+1 if networkswitch<len(networkswitch)-1 else 0
                      else:
                        print("credential was requested by ",IPCamera_SerialKey[i])
                      if BLE_Properties.find("WRITE")!=-1:
                        writer=IPCamera_SerialKey[i]+BLE_SSID_KeyWord+networkcredentials[networkswitch]['ssid']+BLE_Password_KeyWord+networkcredentials[networkswitch]['password']+IPCamera_SerialKey[i]
                        retrytimer=time.time()
                        while(time.time()-retrytimer<=1):
                            if bleperipherical.getState()!="conn":
                              try:
                                bleperipherical.connect(macaddress)
                              except:
                                continue
                            else:
                              try:
                                characteristic.write(writer.encode(),True)
                                break
                              except:
                                continue
                    break
              retrytimer=time.time()
              while time.time()-retrytimer<60:
                  try:
                    if bleperipherical.getState()=="conn":
                      try:
                        bleperipherical.disconnect()
                        break
                      except:
                        continue
                    else:
                      break
                  except:
                    break
          except:
            continue
    print(allbledata)
