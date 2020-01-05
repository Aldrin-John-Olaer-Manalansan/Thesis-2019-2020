NRF Research Specification:
Maximum of 32bytes(32 characters) per packet can be retrieved at a time

[========]

[========]

### NRFTHM
###### - NRF Transmission Header Message
###### - Format : 1byte=8bits

  b7<<<b0
  
0bttrrcccc

|0b                   |tt       |rr      |cccc
| :--------------:|:-----:|:-----:|:-----------------:|
| Binary Format|T.D.T.|R.D.T.|Command Switch


------------



##### tt - transmitter device type(nag iindicate kung anong device ang nag transmit ng message nato , repeater and any device compatible)

|binary|decimal|Device Type  |Description
| :-----:|:--------:|:-------------:|:-------------|
| 0b00 |0           |Smart Socket|Sending the Power Consumption or Relay State.
| 0b01 |1           |IP Camera    |Sending its IP Address(NOT APPLIED).
| 0b10 |2           |Hub              |Sending Requests to its Slaves.
| 0b11 |3           |Repeater      |information is requested to be REPEATED/Rebroadcast(receiver device will act as repeater), ginagamit pag di abot ng master/R-Pi or ng slave/device yung destination device.


------------


##### rr - receiver device type(nag iindicate kung anong device ang dapat makareceive ng message nato)

|binary|decimal|Device Type  |Description
| :-----:|:--------:|:-------------:|:-------------|
| 0b00 |0           |Smart Socket|Receiving Commands,or feedback requests.
| 0b01 |1           |IP Camera    |Receiving Commands,or feedback requests(NOT APPLIED).
| 0b10 |2           |Hub              |Receving its Requests to its slaves.
| 0b11 |3           |ANY Device  |Broadcasting of HUB to its controlled slave devices


------------


##### cccc - command switches(16 possible commands)

|binary    |decimal |Description
| :--------:|:--------:|:-------------|
| 0b0000 |0           |information is a REQUEST(eg. esp32 requesting the wi-fi info,a device is requesting the "public channel info", r-pi is requesting a device power consumption or ac line state, etc...)|
| 0b0001 |1           |information is the "public channel"(used for registering all NRF in one channel/room)|
| 0b0010 |2           |information is the "smart socket's Power Consumption"(majored for current sensor)|
| 0b0011 |3           |information is the "smart socket's AC Line State"(majored for the Relay)|
|0b0100  |4|(NOT APPLIED)information is the "Wi-Fi" SSID and Password
|0b0101  |5|(NOT APPLIED)information is the "IP-Address" of the IP-Camera Device
|0b0110  |6|(RESERVED)
|0b0111  |7|(RESERVED)
|0b1000  |8|(RESERVED)
|0b1001  |9|(RESERVED)
|0b1010  |10|(RESERVED)
|0b1011  |11|(RESERVED)
|0b1100  |12|(RESERVED)
|0b1101  |13|PING PONG Communication - used indicate if the Slave(smart socket and IP Camera) Devices's NRF can reach the Master(Raspberry-Pi) Device's NRF
|0b1110  |14|byte message for debugging purposes
|0b1111  |15|char message for debugging purposes

[========]

[========]

### NRF Transmission Scheme:
###### - Format : 32bytes=256bits

data[0]---------------->--------------------data[31]

0BzRRRRRTTTTTAAPNMMMMMMMMMMMMMMMMM

|0B|z|RRRRR|TTTTT|AA|P|NMMMMMMMMMMMMMMMMM|
| :-:|:-:|:-:|:-:|:-:|:-:|:-:|
| Byte Format|N.R.F.T.M.|Receiver Serial-Key|Transmitter Serial-Key|Trashbin|Packet Information|Information


|Byte Location|Opcode|Description|
|:-:|:-:|:-|
|B0|z|NRFTHM(see above).|
|B1-B5|RRRRR|The UNIQUE Serial key of the RECEIVING device, that is also its PRIVATE Channel if it is a Slave, Public Channel if it is a Master.|
|B6-B10|TTTTT|The UNIQUE Serial key of the TRANSMITTING device, that is also its PRIVATE Channel if it is a Slave, Public Channel if it is a Master.|
|B11-B12|AA|The TRASHBIN is used by the receivers to detect whether the received information was a DUPLICATE of its last received informations(My Standard can identify up to 65535 trashbins which makes duplicates appear very less often).|
|B13|P|(See the Packet Information System below)|
|B14-B31|NMMMMMMMMMMMMMMMMM| Can store 18 characters max. The "N" which is B14 is a "Hybrid Byte" the is Either a "Packet or an Information". This Hybrid Byte is always used, while the remaining 17 bytes are supporting bytes when one byte(255) is not enough to store the information(eg. Power Consumption consumes CHARACTERERS that requires 1 byte each character).|


[========]

[========]

### Packet Information System:
###### Format: 1-Byte or 1-Word

------------

#### Byte Mode:
- Used by normal transmitted packet that does not require any special parameters(single variable information).
- Can transmit up to 15 packets of information per transmission.

b7<<<<<b0

0bKKKKkkkk

|0b|KKKK|kkkk|
|:-:|:-:|:-:|
| Binary Format|Packets Count|Current Packet|

##### KKKK - How many packets the receiver will expect to fetch. 0000 means only one packet was used, 0011 means 4 packets was used - max of 16 packets an information can use.
##### kkkk - The current packet number of the received/transmitted packet. This is used by the receiver to "Construct" the information when all packets are received. 0000 is the first packet and so on... Since NRF24 reception can only receive one packet(32bytes) at a time, this can tells the current packet number. and then merge all packets from 0th up to 15th packet as one information message

------------

##### Word Mode(Implemented but NOT USED):
- This is special for constructing the WiFi SSID which is a long string that might take more than 18 characters, same as with the WiFi Password. 
- Can transmit up to 31 packets of information per transmission. 

b15<<<<<<<<<<b0

0bpppppsssssCCCCCW

|0b|ppppp|sssss|CCCCC|W
|:-:|:-:|:-:|:-:|:-:|
| Binary Format|Password Packets Count|SSID Packets Count|Current Packet|Switch

##### ppppp - Tells the receiver how many packets does the Password occupied. So that the receiver will expect this amount of packets to construct the password.
##### sssss - Tells the receiver how many packets does the SSID occupied. So that the receiver will expect this amount of packets to construct the SSID.
##### CCCCC - Tells the receiver the current packet it receives.
##### W - Tells the receiver that the packet was owned by SSID if 0, Password if 1.
