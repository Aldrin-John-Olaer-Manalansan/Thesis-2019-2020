import json
import os
from datetime import date, datetime

import time
import array






#print(DATA['appliance'][0]['consumption']['log20200112'])
#data['appliance'][0]['consumption']['log20200112']['data'][len(data['appliance'][0]['consumption']['log20200112']['data'])] = 69
#print(DATA['appliance'][0]['consumption']['log20200112'])
#save(data)

class FileHandler():
    def __init__(self,push='bridge/userdata.json',pull='bridge/queue.json'):
        dirname = os.path.dirname(__file__)
        self.input=os.path.join(dirname, pull)
        self.output=os.path.join(dirname, push)
        self.timestamp = 0
        self.timeout = 0
        self.expected = []
        self.fetch()
        '''
        Note:
            The JSON can be access via the object.
            e.g:
            userdata = FileHandler()
            userdata.object

            Disregard this string. This will not be compiled
        '''
#--------------Separator for Readability--------------#

    #Basic JSON operations
    def save(self):
        #SAVE OBJECT TO OUTPUT
        with open(self.output, 'w') as outfile:
            json.dump(self.object, outfile)
    def fetch(self):
        #READ THE OUTPUT FILE FOR EDITING
        with open(self.output) as string:
            self.object = json.load(string)
            self.object['timestamp']=self.timestamp
        self.serial = []
        for appliance in self.object['appliance']:
            self.serial.append(appliance['serial'])

    #READ INPUT FILE TO DECODE
    def read(self):
        #open file
        with open(self.input) as string:
            #check for command
            cache = json.load(string)
            #compare old command to avoid extra processes
            if len(cache)==0:
                print('No requests')
                return
            this=cache[0]
            print(this)
            if not 'timestamp' in this:
                print('No time stamp')
                return
            if this['timestamp']==self.timestamp:
                print('old request')
                print(len(self.expected))
                self.timeout+=1
                if self.timeout > 10:
                    self.object['timestamp'] = this['timestamp']
                    self.object['notification']='none'
                    if len(self.expected)>0:
                        self.expected=[]
                        self.object['notification'] = 'Socket not found!'
                    print('Request timed out' + str(len(self.expected)))
                    self.save()
                return

            #save last command
            self.timestamp = this['timestamp']
            self.timeout = 0
            #verify request
            result = self.do(this)
            print('Result: '+ str(result) + '\n')
            return result
            #Do some automatic task or return an object
        return 'err'


#--------------Separator for Readability--------------#
    #Handlers
    def do(self, queue):
        if not 'request' in queue and not 'serial' in queue and 'data' in queue:
            print('invalid request')
        #temporary
        if queue['request'] == 'toggle':
            self.toggle(queue['serial'],queue['socket'],queue['data'])

        # do ifs for all automatic methods
        if queue['request'] == 'register':
            #add is not automatic it would still depends if the transmitter finds the socket
            print('add command on queue file. See parameters below:')
            print(queue)
            self.expected.append(queue['serial'])
            print('Queue' + str(self.expected))

        if queue['request'] == 'remove':
            #remove is not automatic, the
            print('remove command on queue file. See parameters below:')
            print(queue)
            self.remove(queue['serial'],queue['data'])

        if queue['request'] == 'rename':
            print('remove command on queue file. Bypassing the program... Automatic execution')
            self.rename(queue['serial'], queue['socket'],queue['data'])

        if queue['request'] == 'automation':
            if not 'time' in queue['data'] and not 'enable' in queue['data']:
                print('invalid automation request')
            else:
                print('Automation command on queue file. Bypassing the program.. Automatic execution')
                self.automation(queue['serial'], queue['socket'], queue['data'])


#--------------Separator for Readability--------------#
    #appliance user
    def addSocket(self, serial='', relay=True):
        print('Adding Socket')
        #check for eligibility
        self.fetch()
        #object
        if relay==True:
            type='Switch'
        else:
            type='Monitor'

        for appliance in self.object['appliance']:
            if appliance['serial']==serial:
                print('Socket Already Exist')
                return
        for i in range(1,3):
            token = {
                'name': 'Socket ' + str(i),
                'type':type,
                'status':False,
                'serial':serial,
                'socket':i,
                'automation_enabled':False,
                'automation':['06:00 PM','06:00 AM'],
                'consumption':{}
            }
            self.object['appliance'].append(token)
        self.save()

    def remove(self, serial='', mode='appliance'):
        self.fetch()
        toRemove=[]; index=0
        for appliance in self.object[mode]:
            if appliance['serial']==serial:
                toRemove.append(index)
            index+=1
        for i in reversed(toRemove) :
            del self.object['appliance'][i]
        self.save()
        return

    def rename(self, serial='', socket=1, data=''):
        self.fetch()
        newName = data['name']
        type = data['device']
        print('renaming appliance....')
        for device in self.object[type]:
            if device['serial']==serial and device['socket']==socket:
                device['name']=str(newName)
                print('rename successful')
                break;
        self.save()
        print('appliance not found')

#--------------Separator for Readability--------------#
    #appliance functionalities
    def toggle(self, serial='',socket=1, data=0):
        self.fetch()
        for appliance in self.object['appliance']:
            if appliance['serial']==serial and appliance['socket']==socket:
                appliance['status']=bool(data)
                print('Appliance has been toggled')
        self.save()
        return


    def log(self, serial='', socket=1, data=0):
        self.fetch()
        #today timestamp --- log20200111 => jan 11, 2020
        stamp = date.today().strftime("log%Y%m%d")
        print("log ="+ stamp)
        for appliance in self.object['appliance']:
            t = int(time.strftime('%H'))
            #scan
            if appliance['serial']==serial and appliance['socket']==socket:
                array = [0]*24
                temp = [0]
                if stamp in appliance['consumption']:
                    if 'data' in appliance['consumption'][stamp]:
                        temp = appliance['consumption'][stamp]['data']
                    else:
                        print('no data: creating new data')
                else:
                    print('no log:' + str(appliance['consumption']))
                for i in range(len(temp)):
                    array[i] += round(temp[i]*100)/100
                array[t]+=data
                appliance['consumption'][stamp]={'data':array[0:t+1]}

                #self.save()

            #create socket instance
            #update data


#--------------Separator for Readability--------------#
    # Advanced Funtions (TIME FUNCTIONS)
    def time(self, serial='', socket=1):
        self.fetch()
        for appliance in self.object['appliance']:
            if appliance['serial']==serial and appliance['socket']==socket and appliance['automation_enabled']==True:
                print(appliance['automation'])
                return appliance['automation']
            return ['','']

    def automation(self, serial, socket, data ):
        self.fetch()
        print(data)
        #enable, on, off
        for appliance in self.object['appliance']:
            if appliance['serial']==serial and appliance['socket']==socket:
                 appliance['automation_enabled']=data['enable']
                 appliance['automation']=data['time']


    def data(self):
        self.fetch()
        data = []
        serial = []
        status = []
        for appliance in self.object['appliance']:
            if appliance['socket']==1:
                serial.append(appliance['serial'])
                status.append([appliance['status']])

        for appliance in self.object['appliance']:
            if appliance['socket']==2:
                index = serial.index(appliance['serial'])
                status[index].append(appliance['status'])
        for i in range(len(serial)):
            data.append({'serial':serial[i], 'status':status[i]})
        print(data)
        return data
#--------------Separator for Readability--------------#
    # User Function
    def get(self):
        self.fetch()
        self.read()
        data = self.data()
        for i in self.expected:
            data.append({'serial':i, 'status':[False,False]})
        return data

    def dump(self, data):
        #beta
        #get data array
        #[ 'serial':[123,123,123,123,123], 'data':[10.3,22.22], 'relay':True]
        for sample in data:

            if not sample['serial'] in self.serial:
                self.addSocket(sample['serial'],sample['relay'])
                self.expected.remove(sample['serial'])
                print('New Socket')
            self.log(sample['serial'], 1, sample['data'][0])
            self.log(sample['serial'], 2, sample['data'][1])
        self.save()
        print(self.object['appliance'])
        return





UserData = FileHandler()



#UserData.rename('oldes', 2, 'asd')

# Debug tool
active = True




print("""







--------------------- Read Me ---------------------
This is a CLI program. Type the below functions to try it out.
Running the nodejs server and the app is recommended to see real time data.


Methods:
----------
log(serial, socket, data)       --- Adds data to appliance
remove(serial)                  --- Remove socket
addSocket(serial,socket,type)   --- Add socket
e.g.

log(serial='@#4as',socket=2, data=30)
remove(serial='@#4as')
addSocket('asdfg', socket=2)

defaults:
serial: None (Always require) (type:str)
socket: 1   (type:int)
data= 0     (type:int)

newName: none ( Always require) (type:str)
type = 'appliance'  (type:str)




IMPORTANT Methods

data() -- returns the state(on/off) of every appliance as array
log(serial, socket, data)
read() -- returns a python dictionary containing what has to be done



NOTE:
this py file must have relative paths in order to work

/project-folder/
    -- dumper.py
    -- bridge/
        -- userdata.json
        -- queue.json

----------


""")
for i in UserData.object['appliance']:
    print(i['name'] + ': \nSerial:' + str(i['serial']) + '\nSocket:'+str(i['socket']) + '\n')

print('Program Start')




UserData.dump([
{'serial':[126, 199, 90, 177, 112], 'data':[123,123], 'relay':True},

])




while(active):
    #sample = 'asd'+input('press enter to read')
    #print(sample)
    time.sleep(1)
    try:
        UserData.get()
    except Exception as e:
        active=False
        raise


            #check if today array exist
