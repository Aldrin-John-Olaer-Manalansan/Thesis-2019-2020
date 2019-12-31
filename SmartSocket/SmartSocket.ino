/*
  COMPILE PASSED
  TRANSMISSION AND RECEPTION : TESTED and WORKING

  Communication Script:
  Slave	 - SmartSocket.ino
  Master - dummymaster.ino
*/
// dito mag-Include ng Libraries
#include <SPI.h>
//#include <nRF24L01.h> // for old radios without plus(+)
#include <RF24.h>
//
/* global definition,constants,variables,class,objects,structures dito ntin ilagay
  Important parameters the programmer need to setup definition before compile:
  ~~CRITICALLY REQUIRED~~
  __private_channel__[5]  - The private channel of this device. This is also the "UNIQUE SERIAL KEY" of this device! - consist of 5 bytes
  NRF_PIN_CE  - CE pin of the NRF
  NRF_PIN_CSN - CSN pin of the NRF
  TRANSCEIVER_THISDEVICE_TYPE - see settings below
  ~~SMART SOCKET~~
  SMARTSOCKET_PIN_RELAY         - ilagay nyo dito yung pin number ng relay
  SMARTSOCKET_PIN_CURRENTSENSOR - ilagay nyo dito yung pin number ng current-sensor
*/
const byte __private_channel__[5] = {0b01001100, 0b11100000, 0b00010111, 0b01101100, 0b10101100}; // 1byte=8bits ; 5bytes=40bits

#define SMARTSOCKET_PIN_RELAY 7
#define SMARTSOCKET_PIN_CURRENTSENSOR A0
#define TRANSCEIVER_THISDEVICE_TYPE TRANSCEIVER_DEVICETYPE_SMARTSOCKET
#define TRANSCEIVER_PIN_SIGNALINDICATOR 6 // optional
// arduino nano
#define NRF_PIN_CE 9
#define NRF_PIN_CSN 8
// esp32
//#define NRF_PIN_CE 25
//#define NRF_PIN_CSN 26
//
//
//
#define TRANSCEIVER_DEVICETYPE_SMARTSOCKET 0
#define TRANSCEIVER_DEVICETYPE_IPCAMERA 1
#define TRANSCEIVER_DEVICETYPE_HUB 2
#define TRANSCEIVER_DEVICETYPE_ANYDEVICE 3
#define TRANSCEIVER_DEVICETYPE_REPEATER 3
//
#define TRANSCEIVER_NRFTM_BITLOC_COMMAND 0
#define TRANSCEIVER_NRFTM_BITLOC_RDT 4
#define TRANSCEIVER_NRFTM_BITLOC_TDT 6
//
#define TRANSCEIVER_NRFTM_BITCOUNT_COMMAND 4
#define TRANSCEIVER_NRFTM_BITCOUNT_RDT 2
#define TRANSCEIVER_NRFTM_BITCOUNT_TDT 2
//
#define TRANSCEIVER_REQUEST_NOREQUEST 0
#define TRANSCEIVER_REQUEST_GETPUBLICCHANNEL 1
#define TRANSCEIVER_REQUEST_GETPOWERCONSUMPTION 2
#define TRANSCEIVER_REQUEST_GETRELAYSTATE 3
#define TRANSCEIVER_REQUEST_GETWIFICREDENTIALS 4
#define TRANSCEIVER_REQUEST_GETIPADDRESS 5
#define TRANSCEIVER_REQUEST_PINGPONG 64
//
#define TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST 0
#define TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO 1
#define TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION 2
#define TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE 3
#define TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS 4
#define TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS 5
#define TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG 13
#define TRANSCEIVER_COMMAND_DEBUG_BYTEMESSAGE 14
#define TRANSCEIVER_COMMAND_DEBUG_CHARMESSAGE 15
//
#define TRANSCEIVER_BYTELOC_NRFTM 0
#define TRANSCEIVER_BYTELOC_RECEIVERSK 1
#define TRANSCEIVER_BYTELOC_TRANSMITTERSK 6
#define TRANSCEIVER_BYTELOC_TRASHBIN 11
#define TRANSCEIVER_BYTELOC_PACKETINFO 13
#define TRANSCEIVER_BYTELOC_INFORMATION 14
//
#define TRANSCEIVER_BYTECOUNT_NRFTM 1
#define TRANSCEIVER_BYTECOUNT_RECEIVERSK 5
#define TRANSCEIVER_BYTECOUNT_TRANSMITTERSK 5
#define TRANSCEIVER_BYTECOUNT_TRASHBIN 2
#define TRANSCEIVER_BYTECOUNT_PACKETINFO 1
#define TRANSCEIVER_BYTECOUNT_INFORMATION 18
#define TRANSCEIVER_BYTECOUNT_WIFIINFO 17
// /////////////////////////////////////////////////
// create an RF24 object
// object definition map:
// RF24   - RF24.h ibig sabihin nyan
// radio  - pangalan ng function na nag cecreate ng object na kung ano man yan.
// (9,8)  - parameters na ipapasa sa function na "radio", yan yung value na tatandaan ng "radio" function bago nya gawin trabaho nya, para bang inutusan ka ng nanay mo bumili ng toyo(9) at suka(8)
// RF24 radio(9,8) - inutusan ko si "JB(radio)" ng "Ladiero Family"(RF24) na mag sign-up sa github.com gamit ang username na jblgwapo(9) at password na dniawedw(8)
RF24 radio(NRF_PIN_CE, NRF_PIN_CSN); // CE, CSN
// //////////////////////
/*
  sabi sa documentation 5 bytes lang ang channel na cinoconsider ng pipe 0 at 1, where yung first byte is unique sa lahat ng 6 pipes, kasi yung remaining 4 bytes, nagshashare silang anim dito...
  eg. pipe0channel={1,N,O,D,E};
  eg. pipe1channel={b,N,O,D,E};
  eg. pipe2channel={Y,N,O,D,E};
  eg. pipe3channel={Q,N,O,D,E};
  eg. pipe4channel={v,N,O,D,E};
  eg. pipe5channel={9,N,O,D,E};
  pero since di naman tayo "definite" sa dami ng modules, di natin susundin to... in other words, kahit ano na value ng limang bytes
  eto ang tatawagin nting "Primary Serial Key"
  1st byte     2nd byte     3rd byte     4th byte     5th byte
  byte address[5] = [ 0b00000000 , 0b00000000 , 0b00000000 , 0b00000000 , 0b00000000 ];
*/
// eto yung UNIQUE Channel ng device nato, WALA DAPAT ETO KAPAREHAS NA CHANNEL SA ANUMANG DEVICE
// eto narin yung serial key ng device nato
// Private communication // kapag gusto mo kausapin ng private yung device nato instead of PUBLIC na marami makakarinig, ito gagamitin na channel
// eg, ikaw binulong mo sa katabi mo na nagtatanong kung anong oras na, pwede karin nya bulungan na "its time to party"... secretly
// const byte __private_channel__[5]={0b00000000,0b00000000,0b00000000,0b00000000,0b00000000}; // 1byte=8bits ; 5bytes=40bits
// const byte __private_channel__[5] ={0b01001100, 0b11100000, 0b00010111, 0b01101100, 0b10101100}; // 1byte=8bits ; 5bytes=40bits
// eto yung RASPBERRY-PI initialized CHANNEL na makukuha sa raspberry-pi(HUB) natin para sa PUBLIC COMMUNICATION aka lahat ng rf devices ,maririnig kapag nasa loob nito.
// star topology ang inaapply ng channel nato sa thesis ntin, all devices transmits on all other devices, eg. ikaw sumigaw ka sa classroom lahat narinig ka
byte __public_channel__[5] = {0};
// //////////////////////
unsigned long __timesaver__ = 0; // used for timers
byte __loopindex__; // used at for loops
byte __packet__[32] = {0}; // globally shared information buffer
//uint8_t __TRANSCEIVER_PALevel__;
//rf24_datarate_e __TRANSCEIVER_DataRate__;

// //////////////Function Prototypes para sa Compiler////////////////
uint16_t datatypereadbits(uint16_t variable, byte from = 0, byte bitcount = 1);
void energyconsumption(double * getWh = NULL);
// //////////////////////////////////////////////////////////////////
/*
  isang beses lang to tatawagin, iniinitialize nito yung
  device types:
  0 - Smart Socket
  1 - CCTV
  2 - Raspberry-Pi
*/
void setup() // tawagin lang to one time // sa setup() lang to gamitin
{
  Serial.begin(115200);
  pinMode(SMARTSOCKET_PIN_RELAY, OUTPUT);
  pinMode(TRANSCEIVER_PIN_SIGNALINDICATOR, OUTPUT);
  radio.begin(); // buhayin na yung module, magkainin na ng current yung nrf

  //__TRANSCEIVER_PALevel__=radio.getPALevel();
  //__TRANSCEIVER_DataRate__=radio.getDataRate();
  radio.setPALevel(RF24_PA_MAX); // 0-3 , 0=lowest , 3=max
  radio.setDataRate(RF24_250KBPS); // slowest data rate for improved range of transmission and more low power consumption

  // set the 1st pipe(0) sa private channel
  radio.openReadingPipe(1, __private_channel__); // eto yung private channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
  radio.stopListening();
  radio.openWritingPipe(__private_channel__);
redotransceiversetup:
  Serial.println("Waiting to Receive Public Channel.");
  radio.stopListening();
  memset(__packet__, 0, 32); // yung data na kailangan natin is just 14 bytes... 3 bytes para sa header redundancy, 1 byte para sa packets info, 5 bytes para sa target private channel ng target device, 5 bytes para sa public channel info
  requestmessage(__packet__, TRANSCEIVER_REQUEST_GETPUBLICCHANNEL);
  printpacketdetails(__packet__);
  radio.write(& __packet__, 32);
  // Set module as receiver
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
  __timesaver__ = millis();
  while (true)
    // infinite loop
  {
    if (radio.available())
    {
      memset(__packet__, 0, 32); // empty array(fill with zeroes)
      radio.read(& __packet__, 32);
      if ((datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT) == TRANSCEIVER_THISDEVICE_TYPE || datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT) == TRANSCEIVER_DEVICETYPE_ANYDEVICE)
          && datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND) == TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO)
      {
        // 4th to 8th byte of __packet__ contained the public channel
        memcpy(__public_channel__, __packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, TRANSCEIVER_BYTECOUNT_RECEIVERSK); // extract 5 bytes starting from the 4th byte of __packet__ and store to target device private channel, this confirms that this is the device it was targeting to...
        if (!memcmp(__public_channel__, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK))
          // this device private channel and the received target private channel of the sender must be the same to point out the this is the device it was referring to...
        {
          memcpy(__public_channel__, __packet__ + TRANSCEIVER_BYTELOC_INFORMATION, TRANSCEIVER_BYTECOUNT_RECEIVERSK); // extract 5 bytes starting from the 15th byte of __packet__ and store to __public_channel__
          break; // break the infinite loop
        }
      }
    }
    else if (millis() - __timesaver__ > 5000)
      goto redotransceiversetup;
  }
  // set the 2nd pipe(1) sa target channel
  radio.openReadingPipe(0, __public_channel__); // eto yung public channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
  Serial.print("Now Listening at Public Address: ");
  for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_RECEIVERSK; __loopindex__++)
    Serial.print(String(__public_channel__[__loopindex__]) + String("  "));
  Serial.println("");
}

void loop()
{
  energyconsumption(); // make the current sensor measure the energy consumption inside the function
  processrequest(); // do all the job of the relay and current sensor monitoring and cntrolling
}

// usage:
// energyconsumption(); - run/put this function indefinitely at the loop() section
// energyconsumption(&kWhData); // store the kwH since the last call at "kWhData"
void energyconsumption(double * getWh = NULL)
{
  static unsigned long timeout = millis();
  static unsigned long Current_RawValue_Summation = 0;
  static word Current_Summation_SummedCount = 0;
  static double Total_Energy_Consumption_Wh = 0.0;
  int Current_RawValue = analogRead(SMARTSOCKET_PIN_CURRENTSENSOR);
  if (Current_RawValue == 512 || Current_RawValue == 511)
    Current_RawValue = 0;
  else
    Current_RawValue = abs(Current_RawValue - 511 - (Current_RawValue > 512 ? 1 : 0));
  Current_RawValue_Summation += Current_RawValue;
  Current_Summation_SummedCount++;

  if (millis() - timeout >= 5000 || getWh != NULL)
  {
    //Serial.println(String("Summation Ws:") + String(Current_RawValue_Summation));
    //Serial.println(String("Summation Count:") + String(Current_Summation_SummedCount));
    /* double Energy_Consumption_Wh=(Current_RawValue_Summation/511.0)*(6600.0/Current_Summation_SummedCount); //  Ws unit~~~6600W=220V*30A
      Serial.print("Ws:");
      Serial.println(Energy_Consumption_Wh,6);
      Energy_Consumption_Wh/=(3600.0); // 1kh/3600000=(1k/1000)*(1s*1h/3600s) - 1h=1s*1h/3600s
      Serial.print("Wh:");
      Serial.println(Energy_Consumption_Wh,6);
      Total_Energy_Consumption_Wh+=Energy_Consumption_Wh; // formula above is same as formula below(totalenergyconsumption)*/
    Total_Energy_Consumption_Wh += (Current_RawValue_Summation / 511.0) * (6600.0 / Current_Summation_SummedCount) / 3600.0; // Wh unit
    //Serial.print("Total Wh:");
    //Serial.println(Total_Energy_Consumption_Wh, 6);
    Current_RawValue_Summation = 0;
    Current_Summation_SummedCount = 0;
    timeout = millis();
  }
  if (getWh != NULL)
  {
    getWh[0] = Total_Energy_Consumption_Wh;
    Total_Energy_Consumption_Wh = 0.0;
  }
  /* char datapiece[18];
    dtostrf(Total_Energy_Consumption_Wh,18,4,datapiece);
    Serial.print("char Total Wh:");
    Serial.println(datapiece);
    Energy_Consumption_Wh=atof(datapiece);
    Serial.print("going back:");
    Serial.println(Energy_Consumption_Wh,6); */
}

uint16_t datatypereadbits(uint16_t variable, byte from = 0, byte bitcount = 1)
/*
  read number of bits on any unsigned integer data type, example:
  b7 b6 b5 b4 b3 b2 b1 b0 - byte format in bits
  usage:
  datatypereadbits(0b10100101,4,3) - get 3 bits starting from the 5th bit(b4) - b4 to b6 - extracts 0b010
  datatypereadbits(0b10100101,2,4) - get 4 bits starting from the 3rd bit(b2) - b2 to b5 - extracts 0b1001
*/
{
  variable = variable << 16 - bitcount - from;
  variable = variable >> 16 - bitcount;
  return variable;
}

void datatypewritebits(byte * variable, byte writer = 0, byte bitcount = 0, byte from = 0)
/*
  write number of bits on any unsigned integer data type, example:
  b7 b6 b5 b4 b3 b2 b1 b0 - byte format in bits
  usage:
  data=0b10011010;
  datatypewritebits(&data,0b110,3,4); - writes 110 starting from b4 with 3 bit length - data=0b11101010
  datatypewritebits(&data,0b110,4,4); - writes 0110 starting from b4 with 4 bit length - data=0b01101010
  datatypewritebits(&data,0,4,2); - writes 0b0000 starting from b2 with 4 bit length - data=0b10000010
  datatypewritebits(&data,0b11010100,4,2); - writes 0b0100 starting from b2 with 4 bit length - notice that the remaining 4 bits at writer was unused - data=0b10010010
  datatypewritebits(&data,0b11010100,4,2); - writes 0b0100 starting from b2 with 4 bit length - notice that the remaining 4 bits at writer was unused - data=0b10010010
  datatypewritebits(&data,0b11011,0,2); - activates SMART WRITE MODE(bitcount = 0) - writes 0b11011 starting from b2 with bitcount of 5 bits(because of SMART WRITE MODE) - data=0b11101110
*/
{
  if (bitcount == 0)
  {
    if (writer <= 0)
      return;
    do
    {
      bitcount++;
    }
    while (writer >= pow(2, bitcount) && bitcount < 8);
  }
  else if (from + bitcount > 8)
    bitcount = 8 - from;
  for (__loopindex__ = from; __loopindex__ < from + bitcount; __loopindex__++)
  {
    bitWrite(variable[0], __loopindex__, bitRead(writer, __loopindex__ - from));
  }
}

void transmitmessage(byte * packet)
{
  radio.stopListening(); // gawing tagadaldal yung arduino gamit ang bibig(NRF24)
  if (datatypereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER)
  {
    radio.openWritingPipe(packet); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
    radio.write(packet, 32); // broadcast yung message
  }
  else
  {
    radio.openWritingPipe(__private_channel__); // eto yung target channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
    __timesaver__=millis();
 	 while (!radio.write(packet, 32) && millis()-__timesaver__<=1000)
      // broadcast yung message
    {
      digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW);
      datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_REPEATER, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
      radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
      if (radio.write(packet, 32)) // broadcast yung message
      	break;
      else
      {
      	datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
      	digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH);
      }
    }
  }
  radio.openReadingPipe(0, __public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
}

void processrequest()
{
  byte request = getrequest();
  if (request == 0)
    return; // if no requests
  digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH);
  //memset(__packet__, 0, 32); // empty the message buffer
  switch (request)
  {
      // # are compile time directives, if this conditions are stisfied, then they will be compiled, else not compiled

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
    case TRANSCEIVER_REQUEST_GETPOWERCONSUMPTION:
      constructmesssage(__packet__, TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION);
      break;
    case TRANSCEIVER_REQUEST_GETRELAYSTATE:
      constructmesssage(__packet__, TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE);
      break;
#elif(TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
    case TRANSCEIVER_REQUEST_GETIPADDRESS:
      constructmesssage(__packet__, TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS);
      break;
#endif

    case TRANSCEIVER_REQUEST_PINGPONG:
      //if ((datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_HUB)
      //    || (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER))
      //{
      constructmesssage(__packet__, TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG);
      break;
    //}
    //else
    //  return; // do nothing
    default:
      return; // do nothing
  }
  transmitmessage(__packet__);
}

void requestmessage(byte * message, byte requestswitch)
{
  memset(message, 0, 32); // empty message
  switch (requestswitch)
  {
    case TRANSCEIVER_REQUEST_GETPUBLICCHANNEL:
      memcpy(message + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK); // transmitter=receiver indicates at the rpi side that it is a legit request
      break;
    case TRANSCEIVER_REQUEST_GETWIFICREDENTIALS:
      memcpy(message + TRANSCEIVER_BYTELOC_RECEIVERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      break;
    default:
      return; // if the request passed to this function is invalid, it will not reach the commands at the botton
  }
  // if success, it will be reached here
  message[TRANSCEIVER_BYTELOC_NRFTM] = 0b00100000 + (TRANSCEIVER_THISDEVICE_TYPE << TRANSCEIVER_NRFTM_BITLOC_TDT); // NRFTHM
  memcpy(message + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
  //memset(message + TRANSCEIVER_BYTELOC_INFORMATION,0, TRANSCEIVER_BYTECOUNT_INFORMATION);
  message[TRANSCEIVER_BYTELOC_INFORMATION] = requestswitch;
}

#define publictrashbin_instances 4 // 3 usable trashbins while 1 dynamic trashbin
bool wasintrashbin(byte * passedtrashbin)
{
  static byte publictrashbin[publictrashbin_instances][TRANSCEIVER_BYTECOUNT_TRASHBIN]; // stores the old message to avoid repetitive seding of message, up to 4 trashbins, the higher the trashbin the higher the avoidance of repetitive message sending
  static byte publictrashbin_switch;
  for (__loopindex__ = 0; __loopindex__ < publictrashbin_instances; __loopindex__++)
  {
    if (!memcmp(publictrashbin[__loopindex__], passedtrashbin, TRANSCEIVER_BYTECOUNT_TRASHBIN))
      // return true if they are the same
    {
      publictrashbin_switch = publictrashbin_instances;
      break;
    }
  }
  if (publictrashbin_switch == publictrashbin_instances)
    return true; // was at the trashbin
  if (publictrashbin_switch < publictrashbin_instances - 1)
    publictrashbin_switch++;
  else
    publictrashbin_switch = 0;
  memcpy(publictrashbin[publictrashbin_switch], passedtrashbin, TRANSCEIVER_BYTECOUNT_TRASHBIN);
  return false;
}

void constructmesssage(byte * packet, byte commandswitch)
{
  memset(packet + TRANSCEIVER_BYTELOC_PACKETINFO, 0, TRANSCEIVER_BYTECOUNT_PACKETINFO);
  memset(packet + TRANSCEIVER_BYTELOC_INFORMATION, 0, TRANSCEIVER_BYTECOUNT_INFORMATION);
  switch (commandswitch)
  {
#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
    case TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION:
    {
      double kWhData = 0.0;
      energyconsumption(& kWhData); // store the kwH since the last call at "kWhData"
      dtostrf(kWhData, 18, 4, packet + TRANSCEIVER_BYTELOC_INFORMATION);
      break;
    }
    case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
    {
      packet[TRANSCEIVER_BYTELOC_INFORMATION] = digitalRead(SMARTSOCKET_PIN_RELAY);
      Serial.println(String("Data was set to")+String(packet[TRANSCEIVER_BYTELOC_INFORMATION]));
      break;
    }
#elif(TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
    case TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS:
    {
      for (__loopindex__ = 0; __loopindex__ <= 3; __loopindex__++)
        packet[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__] = staticip[__loopindex__];
      break;
    }
#endif
    case TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG:
    {
      break;
    }
    default:
    {
      return;
    }
  }
  datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, commandswitch, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND, TRANSCEIVER_NRFTM_BITLOC_COMMAND);
  datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_RDT, TRANSCEIVER_NRFTM_BITLOC_RDT);
  if (datatypereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) != TRANSCEIVER_DEVICETYPE_REPEATER)
    datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_THISDEVICE_TYPE, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
  else
  {
    byte publictrashbin[TRANSCEIVER_BYTECOUNT_TRASHBIN];
    do
    {
      publictrashbin[0] = random(256);
      publictrashbin[1] = random(256);
      wasintrashbin(publictrashbin);
    }
    while (!wasintrashbin(publictrashbin));
    memcpy(packet + TRANSCEIVER_BYTELOC_TRASHBIN, publictrashbin, TRANSCEIVER_BYTECOUNT_TRASHBIN);
  }
  memcpy(packet + TRANSCEIVER_BYTELOC_RECEIVERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
  memcpy(packet + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);

  Serial.println("ended");
}

void printpacketdetails(byte* packet)
{
  Serial.println("Received a Message:");

  Serial.print("Transmitter Device: ");
  switch (datatypereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT))
  {
    case TRANSCEIVER_DEVICETYPE_SMARTSOCKET:
      Serial.println("Smart Socket");
      break;
    case TRANSCEIVER_DEVICETYPE_IPCAMERA:
      Serial.println("IP Camera");
      break;
    case TRANSCEIVER_DEVICETYPE_HUB:
      Serial.println("Hub");
      break;
    case TRANSCEIVER_DEVICETYPE_REPEATER:
      Serial.println("Repeater");
      break;
    default:
      break;
  }

  Serial.print("Receiver Device: ");
  switch (datatypereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT))
  {
    case TRANSCEIVER_DEVICETYPE_SMARTSOCKET:
      Serial.println("Smart Socket");
      break;
    case TRANSCEIVER_DEVICETYPE_IPCAMERA:
      Serial.println("IP Camera");
      break;
    case TRANSCEIVER_DEVICETYPE_HUB:
      Serial.println("Hub");
      break;
    case TRANSCEIVER_DEVICETYPE_REPEATER:
      Serial.println("Repeater");
      break;
    default:
      break;
  }

  Serial.print("Command: ");
  switch (datatypereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND))
  {
    case TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST:
      Serial.println("Request");
      break;
    case TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO:
      Serial.println("Public Channel Info");
      break;
    case TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION:
      Serial.println("Power Consumption Info");
      break;
    case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
      Serial.println("Relay State Info");
      break;
    case TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS:
      Serial.println("Wifi Credentials");
      break;
    case TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS:
      Serial.println("IP Address Info");
      break;
    case TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG:
      Serial.println("Ping-Pong Communication");
      break;
    case TRANSCEIVER_COMMAND_DEBUG_BYTEMESSAGE:
      Serial.println("DEBUG Byte Message");
      break;
    case TRANSCEIVER_COMMAND_DEBUG_CHARMESSAGE:
      Serial.println("DEBUG String Message");
      break;
    default:
      break;
  }

  Serial.print("Receiver Serial Key: ");
  for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_RECEIVERSK; __loopindex__++)
    Serial.print(String(packet[__loopindex__ + TRANSCEIVER_BYTELOC_RECEIVERSK]) + String("  "));
  Serial.println("");

  Serial.print("Transmitter Serial Key: ");
  for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_TRANSMITTERSK; __loopindex__++)
    Serial.print(String(packet[__loopindex__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK]) + String("  "));
  Serial.println("");

  if (datatypereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND) == TRANSCEIVER_COMMAND_DEBUG_CHARMESSAGE)
  {
    Serial.print("Information(String): ");
    for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
      Serial.print((char)packet[__loopindex__ + TRANSCEIVER_BYTELOC_INFORMATION]);
    Serial.println("");
  }
  else
  {
    Serial.print("Information(bytes): ");
    for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
      Serial.print(String(packet[__loopindex__ + TRANSCEIVER_BYTELOC_INFORMATION]) + String("  "));
    Serial.println("");
  }
}

#define TRANSCEIVER_BYTECOUNT_STATICHEADER TRANSCEIVER_BYTECOUNT_NRFTM + TRANSCEIVER_BYTECOUNT_RECEIVERSK + TRANSCEIVER_BYTECOUNT_TRANSMITTERSK
byte getrequest()
{
  memset(__packet__, 0, 32); // empty string first
  //radio.flush_rx(); // flush rx messages
  // Set module as receiver
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
  // Read the data if available in buffer
  __timesaver__ = millis();
  while (radio.available() && millis() - __timesaver__ <= 5000)
    // wait for the signal to reach
  {
    // sabi sa documentary ng nrf24, every packet, 32bytes ang laman kaya 32 bytes of array yung ating "text"
    radio.read(& __packet__, 32);

    byte publictrashbin[TRANSCEIVER_BYTECOUNT_TRASHBIN];
    if (datatypereadbits(__packet__[0], 6, 2) == TRANSCEIVER_DEVICETYPE_REPEATER)
    {
      memcpy(publictrashbin, __packet__ + TRANSCEIVER_BYTELOC_TRASHBIN, TRANSCEIVER_BYTECOUNT_TRASHBIN);
      if (wasintrashbin(publictrashbin))
        continue;
    }
    if (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER
        && memcmp(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK)) // memcmp returns true when they are not the same
    {
      printpacketdetails(__packet__);
      radio.stopListening(); // gawing tagadaldal yung arduino gamit ang bibig(NRF24)
      radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
      radio.write(__packet__, 32);
      radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
    }
    else if ((datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT) == TRANSCEIVER_THISDEVICE_TYPE || datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT) == TRANSCEIVER_DEVICETYPE_ANYDEVICE)
             && (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_HUB || datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER)
             && !memcmp(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK)
             && !memcmp(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK))
    {
      printpacketdetails(__packet__);
      switch (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND))
      {
        case TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST:
          return __packet__[TRANSCEIVER_BYTELOC_INFORMATION]; // this is the requestswitch information

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
        case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
          Serial.println(String("Relay was Changed to:") + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION]));
          digitalWrite(SMARTSOCKET_PIN_RELAY, __packet__[TRANSCEIVER_BYTELOC_INFORMATION]);
          break;
#elif(TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
        case TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS:
          word packetwasreceived[2] = {0}; // initially set all packet was received to false
          packetwasreceived[0] = __packet__[TRANSCEIVER_BYTELOC_INFORMATION] + (__packet__[TRANSCEIVER_BYTELOC_PACKETINFO] << 8); // temporarily use this variable, i will empty it again
          byte packetscount[2];
          packetscount[0] = datatypereadbits(packetwasreceived[0], 6, 5);
          packetscount[1] = datatypereadbits(packetwasreceived[0], 11, 5);
          if (packetscount[0] > 0 || packetscount[1] > 0)
          {
            char * wifi_ssid = NULL;
            char * wifi_password = NULL;
            if (packetscount[0] > 0)
              wifi_ssid = new char[TRANSCEIVER_BYTECOUNT_WIFIINFO * packetscount[0]];
            if (packetscount[1] > 0)
              wifi_password = new char[TRANSCEIVER_BYTECOUNT_WIFIINFO * packetscount[1]];
            char datapiece[TRANSCEIVER_BYTECOUNT_WIFIINFO];
            byte currentpacket = datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_INFORMATION], 1, 5);
            byte datawasfor = bitRead(__packet__[TRANSCEIVER_BYTELOC_INFORMATION], 0);
            if (datawasfor)
            {
              if (wifi_password != NULL)
                memcpy(wifi_password + (TRANSCEIVER_BYTECOUNT_WIFIINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WIFIINFO);
            }
            else
            {
              if (wifi_ssid != NULL)
                memcpy(wifi_ssid + (TRANSCEIVER_BYTECOUNT_WIFIINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WIFIINFO);
            }
            bitWrite(packetwasreceived[datawasfor], currentpacket, 1); // set the current packet was received(true or 1)
            __timesaver__ = millis();
            byte dynamicheader[TRANSCEIVER_BYTECOUNT_STATICHEADER];
            byte constantheader[TRANSCEIVER_BYTECOUNT_STATICHEADER];
            memcpy(constantheader, __packet__, TRANSCEIVER_BYTECOUNT_STATICHEADER);
            if (packetscount[datawasfor] > 0)
              packetscount[datawasfor] --;
            while (packetscount[0] > 0 || packetscount[1] > 0)
            {
              if (radio.available())
              {
                // sabi sa documentary ng nrf24, every packet, 32bytes ang laman kaya 32 bytes of array yung ating "text"
                radio.read(& __packet__, 32);
                memcpy(dynamicheader, __packet__, TRANSCEIVER_BYTECOUNT_STATICHEADER);
                if (!memcmp(constantheader, dynamicheader, TRANSCEIVER_BYTECOUNT_STATICHEADER))
                  // returns true if they are the same
                {
                  byte currentpacket = datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_INFORMATION], 1, 5);
                  byte datawasfor = bitRead(__packet__[TRANSCEIVER_BYTELOC_INFORMATION], 0);
                  if (bitRead(packetwasreceived[datawasfor], currentpacket) == 0 && packetscount[datawasfor] > 0)
                  {
                    if (datawasfor)
                    {
                      if (wifi_password != NULL)
                        memcpy(wifi_password + (TRANSCEIVER_BYTECOUNT_WIFIINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WIFIINFO);
                    }
                    else
                    {
                      if (wifi_ssid != NULL)
                        memcpy(wifi_ssid + (TRANSCEIVER_BYTECOUNT_WIFIINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WIFIINFO);
                    }
                    bitWrite(packetwasreceived[datawasfor], currentpacket, 1); // set the current packet was received(true or 1)
                    packetscount[datawasfor] --;
                    __timesaver__ = millis();
                  }
                }
              }
              else if (millis() - __timesaver__ > 5000)
                break;
            }
            if (wifi_ssid != NULL)
            {
              Serial.println(String("SSID was Set to:") + String(wifi_ssid));
              Serial.println(String("PasswordID was Set to:") + String(wifi_password));
              WiFi.begin(wifi_ssid, wifi_password);
              delete[] wifi_ssid;
              wifi_ssid = NULL;
            }
            if (wifi_password != NULL)
            {
              delete[] wifi_password;
              wifi_password = NULL;
            }
          }
          break;
#endif

        case TRANSCEIVER_COMMAND_DEBUG_BYTEMESSAGE:
          Serial.println("You got a Byte Message:");
          for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
          {
            Serial.println(String("Byte ") + String(__loopindex__) + String(" = ") + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__]));
          }
          break;
        case TRANSCEIVER_COMMAND_DEBUG_CHARMESSAGE:
          Serial.print("You got a Char Message:");
          for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
          {
            Serial.print(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__]);
          }
          Serial.println("");
          break;
        default:
          break;
      }
    }
  }
  return 0;
}

/* String RXProcessAll()
  {
  String information;
  byte packet[32] = {0}; // empty string first
  char* alldatapiece[18];
  char datapiece[18];
  word packetwasreceived = 0; //initially set all packet was received to false
  //Set module as receiver
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
  //Read the data if available in buffer
  if (radio.available())
  {
  // sabi sa documentary ng nrf24, every packet, 32bytes ang laman kaya 32 bytes of array yung ating "text"
  radio.read(&packet, 32);
  memcpy(packet, NRFTHMCorrection(packet), 32); // array 0,1,2 will become zero if corrupted
  if ( packet[0] != 0 && packet[1] != 0 && packet[2] != 0 ) // if the redundancy header was not corrupted
  {
  if (datatypereadbits(packet[0], 6, 2) == TRANSCEIVER_DEVICETYPE_REPEATER
  && !(packet[3] == __private_channel__[0]
  && packet[4] == __private_channel__[1]
  && packet[5] == __private_channel__[2]
  && packet[6] == __private_channel__[3]
  && packet[7] == __private_channel__[4]))
  {
  radio.stopListening(); // gawing tagadaldal yung arduino gamit ang bibig(NRF24)
  radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
  radio.write(&packet, sizeof(packet));
  radio.stopListening(); // gawing tagadaldal yung arduino gamit ang bibig(NRF24)
  radio.openWritingPipe(__private_channel__); // eto yung target channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
  }
  else if ((datatypereadbits(packet[0], 4, 2) == TRANSCEIVER_THISDEVICE_TYPE || datatypereadbits(packet[0], 4, 2) == TRANSCEIVER_DEVICETYPE_ANYDEVICE)
  && (datatypereadbits(packet[0], 0, 4) == TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST))
  {
  byte packetscount = datatypereadbits(packet[13], 4, 4);
  if (packetscount > 0)
  {
  byte currentpacket = datatypereadbits(packet[13], 0, 4);
  memcpy(datapiece, packet + 14, sizeof(datapiece)); // get the 15th to 32th byte
  alldatapiece[currentpacket] = datapiece; // append it at the packet constructor
  bitWrite(packetwasreceived, currentpacket, 1); // set the current packet was received(true or 1)
  __timesaver__ = millis();
  byte dynamicheader[13];
  byte constantheader[13];
  memcpy(constantheader, packet, 13);
  while (packetscount > 0)
  {
  if (radio.available())
  {
  // sabi sa documentary ng nrf24, every packet, 32bytes ang laman kaya 32 bytes of array yung ating "text"
  radio.read(&packet, 32);
  memcpy(dynamicheader, packet, 13);
  if (constantheader == dynamicheader)
  {
  byte currentpacket = datatypereadbits(packet[13], 0, 4);
  if (bitRead(packetwasreceived, currentpacket) == 0)
  {
  memcpy(datapiece, packet + 14, sizeof(datapiece)); // get the 15th to 32th byte
  alldatapiece[currentpacket] = datapiece; // append it at information
  bitWrite(packetwasreceived, currentpacket, 1); // set the current packet was received(true or 1)
  packetscount--;
  __timesaver__ = millis();
  }
  }
  }
  else if (millis() - __timesaver__ > 5000)
  break;
  }
  for (byte index = 0; index <= packetscount; index++)
  information += alldatapiece[index];
  }
  else
  {
  memcpy(datapiece, packet + 14, sizeof(datapiece)); // get the 15th to 32th byte
  information += datapiece; // append it at information
  }
  memset(packet, 0, 32); // empty array(fill with zeroes)
  memcpy(message + 3, __public_channel__, 5); // the receiver will be the raspberry pi serial key, publicchannel=raspberry pi's serial key
  memcpy(message + 8, __private_channel__, 5); // the transmitter this device,privatechannel=this device's serial key
  byte bytewriter = 0b00100000 + (TRANSCEIVER_THISDEVICE_TYPE << 6); // NRFTHM
  //unfinished
  switch (information[0])
  {
  case TRANSCEIVER_REQUEST_GETPOWERCONSUMPTION:
  bytewriter += TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION;
  break;
  case TRANSCEIVER_REQUEST_GETRELAYSTATE:
  bytewriter += TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE;
  break;
  case TRANSCEIVER_REQUEST_GETWIFICREDENTIALS:
  bytewriter += TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS;
  break;
  case TRANSCEIVER_REQUEST_GETIPADDRESS:
  bytewriter += TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS;
  break;
  default:
  break;
  }
  //unfinished
  packet[0] = bytewriter; packet[1] = bytewriter; packet[2] = bytewriter; // redundancy header message for error correction
  }
  }
  }
  return information;
  } */
