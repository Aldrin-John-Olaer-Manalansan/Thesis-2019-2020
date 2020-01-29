import json
import os
from datetime import date, datetime
import array

import time
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
    def save(self, notification='none'):
        #SAVE OBJECT TO OUTPUT
        self.object['notification'] = notification
        self.object['timestamp'] = self.timestamp
        with open(self.output, 'w') as outfile:
            json.dump(self.object, outfile)

    def fetch(self):
        #READ THE OUTPUT FILE FOR EDITING
        try:
            with open(self.output) as string:
                self.object = json.loads(string.read())
                self.object['timestamp']=self.timestamp
            self.serial = []
            for appliance in self.object['appliance']:
                self.serial.append(appliance['serial'])
        except Exception as e:
            self.object = {"appliance":[], "camera":[],"timestamp":self.timestamp}

    #READ INPUT FILE TO DECODE
    def read(self):
        #open file
        cache=[]
        try:
            with open(self.input) as string:
                cache = json.loads(string.read())
        except Exception as e:
            print('json parse error for queue')
            cache = []
        ####print(len(cache))
            #compare old command to avoid extra processes
        if len(cache)==0:
            ####print('No requests')
            return
        this=cache[0]
        ####print(this)
        if not 'timestamp' in this:
            print('No time stamp')
            return
        if this['timestamp']==self.timestamp:
            ####print('old request')
            ####print(len(self.expected))
            self.timeout+=1
            if self.timeout > 10 and self.timeout < 15:
                self.object['timestamp'] = this['timestamp']
                if len(self.expected)>0:
                    self.expected=[]
                    print('Socket not found!')
                    self.save('Socket not found!')
                    return
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
        self.save()

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
        newName = data['name']
        type = data['device']

        if type == 'appliance':
            print('renaming appliance....')
            for device in self.object[type]:
                if device['serial']==serial and device['socket']==socket:
                    device['name']=str(newName)
                    print('rename successful')
                    break;
        elif type =='camera':
            print('renaming camera....')
            for device in self.object[type]:
                if device['serial']==serial:
                    device['name']=str(newName)
                    print('rename successful')
                    break;
        self.save()
        print('device not found. rename skipped....')

#--------------Separator for Readability--------------#
    #appliance functionalities
    def toggle(self, serial='',socket=1, data=0):
        print('attempt to toggle'+ str(socket)+':'+str(serial))
        for appliance in self.object['appliance']:
            if appliance['serial']==serial and appliance['socket']==socket:
                appliance['status']=bool(data)
                print('Appliance has been toggled')
                self.save()
                return
        print('Appliance has not been toggled...')
        return


    def log(self, serial='', socket=1, data=0):
        self.fetch()
        #today timestamp --- log20200111 => jan 11, 2020
        stamp = date.today().strftime("log%Y%m%d")
        ####print("log ="+ stamp)
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
                ####else:
                ####    print('no log:' + str(appliance['consumption']))
                for i in range(len(temp)):
                    array[i] += round(temp[i]*100)/100
                array[t]+=data
                appliance['consumption'][stamp]={'data':array[0:t+1]}

                #self.save()

            #create socket instance
            #update data


#--------------Separator for Readability--------------#
    # Advanced Funtions (TIME FUNCTIONS)
    def time(self):
        self.fetch()
        for appliance in self.object['appliance']:
            if appliance['automation_enabled']==True:
                stamp = time.strftime("%I:%M %p")
                window = time.strftime("%S")
                if stamp == appliance['automation'][1] and int(window)<=3:
                    appliance['status']=True
                    print('Auto toggle to on!')
                    self.save()
                if stamp == appliance['automation'][0] and int(window)<=3:
                    print('auto toggle to off!')
                    appliance['status']=False
                    self.save()
                return


    def automation(self, serial, socket, data ):
        self.fetch()
        print(data)
        #enable, on, off
        for appliance in self.object['appliance']:
            if appliance['serial']==serial and appliance['socket']==socket:
                 appliance['automation_enabled']=data['enable']
                 appliance['automation']=data['time']

    def data(self):
        data = []
        serial = []
        status = []
        for appliance in self.object['appliance']:
            if appliance['socket']==1:
                serial.append(appliance['serial'])
                status.append([int(appliance['status'])])

        for appliance in self.object['appliance']:
            if appliance['socket']==2:
                index = serial.index(appliance['serial'])
                status[index].append(int(appliance['status']))
        for i in range(len(serial)):
            data.append({'serial':serial[i], 'status':status[i]})

        return data
#--------------Separator for Readability--------------#
    # User Function
    def get(self):
        self.fetch()
        self.read()
        self.time()
        appliance = self.data()
        camera = []
        for i in self.expected:
            appliance.append({'serial':i, 'status':[False,False]})
        for cam in self.object['camera']:
            camera.append({'serial':cam['serial'], 'type':'cctv'})
        #print(data)
        ####print({"appliance":appliance, "camera":camera})
        return {"appliance":appliance, "camera":camera}

    def dump(self, data):
        #beta
        #get data array
        #[ 'serial':[123,123,123,123,123], 'data':[10.3,22.22], 'relay':True]
        if len(data)==0:
            return
        online = []
        for sample in data:
            if not sample['serial'] in self.serial:
                self.addSocket(sample['serial'],sample['relay'])
                self.expected.remove(sample['serial'])
                print('New Socket')
            online.append(sample['serial'])
            self.log(sample['serial'], 1, sample['data'][0])
            self.log(sample['serial'], 2, sample['data'][1])
        for appliance in self.object['appliance']:
            if appliance['serial'] in online:
                appliance['online'] = True
        self.save()
        #print(self.object['appliance'])
        return

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`
