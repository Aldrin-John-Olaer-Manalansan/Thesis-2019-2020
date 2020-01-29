import json
import requests
import os
import re

class reftunnel:
	def __init__(reference):
		reference.host=[]
		reference.http=[]
		reference.https=[]
		reference.tcp=[]

def get_ngrok_url():
	url = "http://localhost:4040/api/tunnels/"
	res = requests.get(url)
	res_unicode = res.content.decode("utf-8")
	res_json = json.loads(res_unicode)
	ngrokinfo=reftunnel()
	for obj in res_json["tunnels"]:
		if obj['config']['addr'] not in ngrokinfo.host:
			ngrokinfo.host.append(obj['config']['addr'])
			ngrokinfo.tcp.append('')
			ngrokinfo.http.append('')
			ngrokinfo.https.append('')
		if obj['proto']=='tcp':
			ngrokinfo.tcp[ngrokinfo.host.index(obj['config']['addr'])]=obj['public_url']
		if obj['proto']=='http':
			ngrokinfo.http[ngrokinfo.host.index(obj['config']['addr'])]=obj['public_url']
		if obj['proto']=='https':
				ngrokinfo.https[ngrokinfo.host.index(obj['config']['addr'])]=obj['public_url']
	return ngrokinfo

os.system("sudo /home/pi/Documents/server/ngrok http localhost:6969 -bind-tls=true -inspect=false -region=jp --authtoken 1UYiqnDwjx3c37Tf8oHtudCVOKe_3zDnY8QPgS5tEFagJkdjQ & 2> /home/pi/Documents/server/exceptions/ngrok.log")
while True:
	try:
		urlinfo=re.findall('(?<=://).*(?=.jp.ngrok.io)',get_ngrok_url().https[0])[0]
		print(urlinfo)
		break
	except:
		continue
if not os.path.exists('/home/pi/Documents/server/bridge/'):
	os.system("mkdir /home/pi/Documents/server/bridge/")
os.system("echo -n '[]' >| /home/pi/Documents/server/bridge/queue.json")
os.system("echo -n '"+urlinfo+"' >| /home/pi/Documents/server/bridge/link.txt")
#the idiom >2 means dump the stderr at a log file for debugging
if not os.path.exists('/home/pi/Documents/server/exceptions/'):
	os.system("mkdir /home/pi/Documents/server/exceptions/")

os.system("sudo python3.7 /home/pi/Documents/server/Master_Transceiver.py & 2> /home/pi/Documents/server/exceptions/Master_Transceiver.log")
os.system("sudo python3.7 /home/pi/Documents/server/Master_BLE.py & 2> /home/pi/Documents/server/exceptions/Master_BLE.log")
while True:
	os.system("cd /home/pi/Documents/server;sudo node server.js 2> /home/pi/Documents/server/exceptions/server.log")
