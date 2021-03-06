/*
  COMPILE PASSED
  TRANSMISSION AND RECEPTION : TESTED and WORKING
  Communication Script:
  Slave	 - SmartSocket.ino
  Master - dummymaster.ino
*/
// dito mag-Include ng Libraries
#include <SPI.h>
// #include <nRF24L01.h> // for old radios without plus(+)
#include <RF24.h>
#include "C:\Users\acer\Desktop\Thesis\Project\Source_Codes\ThesisLibraries\transceiver_library\transceiver_library.cpp"
#include <Filters.h>
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
// sample serial keys// can be changed
// device 0
const byte __private_channel__[5] = {76,224,23,108,172}; // 1byte=8bits ; 5bytes=40bits
#define TRANSCEIVER_SMARTSOCKET_HASRELAY true
//TRANSCEIVER_SMARTSOCKET_HASRELAY false
// device 1
//const byte __private_channel__[5] = {162, 111, 4, 54, 92}; // 1byte=8bits ; 5bytes=40bits
//#define TRANSCEIVER_SMARTSOCKET_HASRELAY false
// device 2
// const byte __private_channel__[5] = {92 , 255, 134, 222, 0  };
// device 3
// const byte __private_channel__[5]={0  ,423,111,32 ,42 };
// device 4
// const byte __private_channel__[5]={12 ,182,33 ,12 ,87 };
// device 5
// const byte __private_channel__[5]={16 ,11 ,40 ,49 ,74 };
// device 6
// const byte __private_channel__[5]={122,199,153,163,102};
// device 7
// const byte __private_channel__[5]={136,0  ,0  ,12 ,192};
// device type
#define TRANSCEIVER_THISDEVICE_TYPE TRANSCEIVER_DEVICETYPE_SMARTSOCKET
// #define TRANSCEIVER_THISDEVICE_TYPE TRANSCEIVER_DEVICETYPE_IPCAMERA
//
//callibrate the acs using ajoms callibrator
const double ACS712_slope[2]={0.038326692,0.037941106}; // to be adjusted based on calibration testing using ajoms callibrator.xlx
const double ACS712_intercept[2]={-0.065150005,-0.06835347}; // to be adjusted based on calibration testing using ajoms callibrator.xlx
#define NOISECURRENT_LEVEL 9.0 // 4.545454545454 // (in mA) // 1W noise as error of the ACS device
//
// arduino pins
#define SMARTSOCKET_PIN_RELAY1 6
#define SMARTSOCKET_PIN_RELAY2 7
const byte SMARTSOCKET_PIN_CURRENTSENSOR[2]={A0,A1};
#define TRANSCEIVER_SIGNALPIN_RED 5 // optional
#define TRANSCEIVER_SIGNALPIN_GREEN 4  // optional
#define TRANSCEIVER_SIGNALPIN_BLUE 3 // optional
#define NRF_PIN_CE 9
#define NRF_PIN_CSN 8
// esp32
/* #define SMARTSOCKET_PIN_RELAY1 18
  #define SMARTSOCKET_PIN_RELAY2 19
  #define SMARTSOCKET_PIN_CURRENTSENSOR1 34
  #define SMARTSOCKET_PIN_CURRENTSENSOR2 35
  #define TRANSCEIVER_SIGNALPIN_RED 4 // optional
  #define TRANSCEIVER_SIGNALPIN_GREEN 16  // optional
  #define TRANSCEIVER_SIGNALPIN_BLUE 17 // optional
  #define NRF_PIN_CE 25
  #define NRF_PIN_CSN 26 */
//
// window sampler settings
#define ACS712_testfrequency 60.0
#define ACS712_windowlength 20.0
RunningStatistics CurrentSensor_inputStats[2]; // create statistics to look at the raw test signal
//
// byte staticip[4] = {192, 168, 1, 111}; // aldrin test
#define ledwithsignal digitalWrite(TRANSCEIVER_SIGNALPIN_GREEN,LOW);digitalWrite(TRANSCEIVER_SIGNALPIN_RED,HIGH);digitalWrite(TRANSCEIVER_SIGNALPIN_BLUE,HIGH);
#define ledbusy digitalWrite(TRANSCEIVER_SIGNALPIN_GREEN,HIGH);digitalWrite(TRANSCEIVER_SIGNALPIN_RED,HIGH);digitalWrite(TRANSCEIVER_SIGNALPIN_BLUE,LOW);
#define lednosignal digitalWrite(TRANSCEIVER_SIGNALPIN_GREEN,HIGH);digitalWrite(TRANSCEIVER_SIGNALPIN_RED,LOW);digitalWrite(TRANSCEIVER_SIGNALPIN_BLUE,HIGH);
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
byte __public_channel__[5] =
{
  0
};

// //////////////////////
unsigned long led_indicator_timeout = 0; // timeout indicator of the led
unsigned long __timesaver__ = 0; // used for timers
byte __packet__[32] =
{
  0
}; // globally shared information buffer

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
char * __WiFi_SSID__ = NULL;
char * __WiFi_Password__ = NULL;
#endif

// uint8_t __TRANSCEIVER_PALevel__;
// rf24_datarate_e __TRANSCEIVER_DataRate__;
// //////////////Function Prototypes para sa Compiler////////////////

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
void(* resetFunc) (void) = 0; // only declare the reset function if it is an arduino chip AKA smart socket chip
void energyconsumption(double * getWh = NULL, double * getmA = NULL);
#endif

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
  pinMode(SMARTSOCKET_PIN_RELAY1, OUTPUT);
  pinMode(SMARTSOCKET_PIN_RELAY2, OUTPUT);
  pinMode(TRANSCEIVER_SIGNALPIN_RED, OUTPUT);
  pinMode(TRANSCEIVER_SIGNALPIN_GREEN, OUTPUT);
  pinMode(TRANSCEIVER_SIGNALPIN_BLUE, OUTPUT);
  digitalWrite(SMARTSOCKET_PIN_RELAY1, LOW);
  digitalWrite(SMARTSOCKET_PIN_RELAY2, LOW);
  //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW);
  ledbusy
  if (radio.begin())
    // buhayin na yung module, magkainin na ng current yung nrf
    Serial.println("Radio Begin Successfully");
  else
    Serial.println("Radio Failed Begin");
  // __TRANSCEIVER_PALevel__=radio.getPALevel();
  // __TRANSCEIVER_DataRate__=radio.getDataRate();
  radio.setChannel(HOMEPACKAGE_TRANSCEIVER_PHYSICALCHANNEL);
  radio.setPALevel(RF24_PA_MAX); // 0-3 , 0=lowest , 3=max
  radio.setDataRate(RF24_250KBPS); // slowest data rate for improved range of transmission and more low power consumption
  radio.setRetries(7, 15); // 2000us delay each failure, 15 retries(max already)
  // set the 1st pipe(0) sa private channel
  radio.openReadingPipe(1, __private_channel__); // eto yung private channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
  radio.stopListening();
  if (radio.getDataRate() == RF24_250KBPS)
    radio.flush_tx();
  radio.openWritingPipe(__private_channel__);
redotransceiversetup:
  Serial.println("Waiting to Receive Public Channel.");
  radio.stopListening();
  if (radio.getDataRate() == RF24_250KBPS)
    radio.flush_tx();
  memset(__packet__, 0, 32); // yung data na kailangan natin is just 14 bytes... 3 bytes para sa header redundancy, 1 byte para sa packets info, 5 bytes para sa target private channel ng target device, 5 bytes para sa public channel info
  requestmessage(__packet__, TRANSCEIVER_REQUEST_GETPUBLICCHANNEL);
  printpacketdetails(__packet__);
  ledbusy
  radio.write(& __packet__, 32);
  //while (!radio.write(& __packet__, 32))
  //{
  //}
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
          //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH); // signal found
          ledwithsignal
          led_indicator_timeout = millis(); // for the led indicator
          memcpy(__public_channel__, __packet__ + TRANSCEIVER_BYTELOC_INFORMATION, TRANSCEIVER_BYTECOUNT_RECEIVERSK); // extract 5 bytes starting from the 15th byte of __packet__ and store to __public_channel__
          break; // break the infinite loop
        }
      }
    }
    else if (millis() - __timesaver__ > 1500)
      // was 5000
      goto redotransceiversetup;
  }
  // set the 2nd pipe(1) sa target channel
  radio.openReadingPipe(0, __public_channel__); // eto yung public channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
  Serial.print("Now Listening at Public Address: ");
  for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_RECEIVERSK; __loopindex__++)
    Serial.print(String(__public_channel__[__loopindex__]) + String("  "));
  Serial.println("");

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
  ledbusy
redogetwificredentials:
  radio.stopListening();
  if (radio.getDataRate() == RF24_250KBPS)
    radio.flush_tx();
  memset(__packet__, 0, 32); // yung data na kailangan natin is just 14 bytes... 3 bytes para sa header redundancy, 1 byte para sa packets info, 5 bytes para sa target private channel ng target device, 5 bytes para sa public channel info
  requestmessage(__packet__, TRANSCEIVER_REQUEST_GETWIFICREDENTIALS);
  printpacketdetails(__packet__);
  radio.write(& __packet__, 32);
  // Set module as receiver
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
  __timesaver__ = millis();
  while (true)
  {
    getrequest();
    if (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND) == TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS)
    {
      Serial.println("Got the Credentials!");
      Serial.println(String("SSID:") + String(__WiFi_SSID__));
      Serial.println(String("Password:") + String(__WiFi_Password__));
      break;
    }
    else if (millis() - __timesaver__ > 5000)
      goto redogetwificredentials;
  }
  ledwithsignal
  led_indicator_timeout = millis();
#endif
  #if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
    CurrentSensor_inputStats[0].setWindowSecs( ACS712_windowlength/ACS712_testfrequency );
    CurrentSensor_inputStats[1].setWindowSecs( ACS712_windowlength/ACS712_testfrequency );
    __timesaver__=millis();
    while (millis()-__timesaver__<3000) // give 3 seconds to callibrate the signal fetched
    {
      for(__loopindex__=0;__loopindex__<=1;__loopindex__++)
        CurrentSensor_inputStats[__loopindex__].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[__loopindex__]));  // log to Stats function
    }
  #endif
}

void loop()
{

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
  energyconsumption(); // make the current sensor measure the energy consumption inside the function
  if (millis() - led_indicator_timeout > TRANSCEIVER_HUB_FETCHINGINTERVAL + 2000 && digitalRead(TRANSCEIVER_SIGNALPIN_RED))
    // if within 6 seconds there was no
  {
    //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW); // no signal found
    lednosignal
    Serial.println(String("NO Operations done since ") + String((millis() - led_indicator_timeout) / 1000) + String(" seconds ago"));
  }
#endif

  processrequest(); // do all the job of the relay and current sensor monitoring and cntrolling
}

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
// usage:
// energyconsumption(); - run/put this function indefinitely at the loop() section
// energyconsumption(&WhData); // store the kwH since the last call at "WhData"
//void energyconsumption(double * getWh = NULL,double * getmA=NULL)
void energyconsumption(double * getWh, double * getmA)
{
  static double average_mA[2] = {0.0,0.0},sampled_average_mA[2] = {0.0,0.0},Total_Energy_Consumption_Wh[2] = {0.0,0.0};
  static unsigned long timeout = millis();
  static word Current_Summation_SummedCount[2] ={0,0};
  
  for (byte index = 0; index <= 1; index++)
  {
    CurrentSensor_inputStats[index].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[index]));  // log to Stats function
    if (!TRANSCEIVER_SMARTSOCKET_HASRELAY || digitalRead(index==0?SMARTSOCKET_PIN_RELAY1:SMARTSOCKET_PIN_RELAY2))
      sampled_average_mA[index] += 1000.0 * (ACS712_intercept[index] + (ACS712_slope[index] * CurrentSensor_inputStats[index].sigma()));
    Current_Summation_SummedCount[index]++;
    
    //if (index==1) //used for callibration
    //  Serial.println(CurrentSensor_inputStats[index].sigma(),8);
    
    if (millis() - timeout >= 1000)
      // get Ws
    {

      average_mA[index] = sampled_average_mA[index]/Current_Summation_SummedCount[index];
      if (average_mA[index]<=NOISECURRENT_LEVEL)
        average_mA[index]=0.0;
      //Serial.println("sigma"+String(index)+'='+String(CurrentSensor_inputStats[index].sigma()));
      //Serial.println("mA"+String(index)+'='+String(average_mA[index]));

      Total_Energy_Consumption_Wh[index] += average_mA[index]/16363.63636; // Wh unit // 16363.63636 = 3600*1000/220 s/h
      // Serial.print("Total Wh:");
      // Serial.println(Total_Energy_Consumption_Wh[index], 6);
      sampled_average_mA[index]=0.0;Current_Summation_SummedCount[index] = 0;
      if (index>=1)
        timeout = millis();
    }
    if (getmA != NULL)
    {
      getmA[index] = average_mA[index];
    }
    if (getWh != NULL)
    {
      getWh[index] = Total_Energy_Consumption_Wh[index];
      Total_Energy_Consumption_Wh[index] = 0.0;
    }
    /* char datapiece[18];
      dtostrf(Total_Energy_Consumption_Wh[index],18,4,datapiece);
      Serial.print("char Total Wh:");
      Serial.println(datapiece);
      Energy_Consumption_Wh=atof(datapiece);
      Serial.print("going back:");
      Serial.println(Energy_Consumption_Wh,6); */
  }
}

#endif

bool transmitmessage(byte * packet)
{
  Serial.print("Transmitting:");
  for (byte i=0;i<32;i++)
    Serial.print(' '+String(packet[i]));
  Serial.println();
  bool transmitted = false;
  radio.stopListening(); // gawing tagadaldal yung arduino gamit ang bibig(NRF24)
  // if (radio.getDataRate() == RF24_250KBPS)
  // radio.flush_tx();
  if (datatypereadbits(packet[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER)
  {
    byte target_channel[5];
    memcpy(target_channel, packet + TRANSCEIVER_BYTELOC_RECEIVERSK, 5); // get the target channel of the source
    unsigned long timeout = millis();
    ledbusy
    do
    {
      CurrentSensor_inputStats[0].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[0]));  // log to Stats function
      CurrentSensor_inputStats[1].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[1]));  // log to Stats function
      radio.openWritingPipe(target_channel); // eto yung target channel( hallway ng llyce ) na papasukan ng mensahe(message)
      transmitted = radio.write(packet, 32);
      if (transmitted)
        break;
      radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
      transmitted = radio.write(packet, 32);
    }
    while (!transmitted && millis() - timeout < 1000);
    radio.openWritingPipe(__public_channel__); // ibalik natin sa default writing channel
    led_indicator_timeout = millis();
    ledwithsignal
    if (transmitted)
    {
      //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH);
      led_indicator_timeout = millis();
      Serial.println("Requested Signal to be Repeated on Public Channel was Successful!");
    }
  }
  else
  {
    constructtrashbin(packet);
    radio.openWritingPipe(__private_channel__); // eto yung target channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
    ledbusy
    __timesaver__ = millis();
    do
    {
      CurrentSensor_inputStats[0].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[0]));  // log to Stats function
      CurrentSensor_inputStats[1].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[1]));  // log to Stats function
      transmitted = radio.write(packet, 32);
      if (transmitted)
        // broadcast yung message
        break;
      datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_REPEATER, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
      radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
      transmitted = radio.write(packet, 32);
      if (transmitted)
        // broadcast yung message
      {
        //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH);
        ledwithsignal
        led_indicator_timeout = millis();
        break;
      }
      // constructtrashbin(packet); // commented since this results spam of info to receivers
      radio.setDataRate(radio.getDataRate() == RF24_250KBPS ? RF24_1MBPS : RF24_250KBPS);
      radio.openWritingPipe(__private_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
      //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW);
      lednosignal
      datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_THISDEVICE_TYPE, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
    }
    while (!transmitted && millis() - __timesaver__ <= 1000);
    led_indicator_timeout = millis();
    ledwithsignal
    radio.setDataRate(RF24_250KBPS);
  }
  radio.openReadingPipe(0, __public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
  return transmitted;
}

void processrequest()
{
  byte request = getrequest();
  if (request == 0)
    return; // if no requests
  //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH);
  ledwithsignal
  led_indicator_timeout = millis();
  // memset(__packet__, 0, 32); // empty the message buffer
  switch (request)
  {
      // # are compile time directives, if this conditions are stisfied, then they will be compiled, else not compiled

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
    case TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION:
      {
        constructmesssage(__packet__, TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION);
        break;
      }
    case TRANSCEIVER_REQUEST_GETPOWERCONSUMPTION:
      {
        constructmesssage(__packet__, TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION);
        break;
      }
    case TRANSCEIVER_REQUEST_GETAVERAGECURRENT:
      {
        constructmesssage(__packet__, TRANSCEIVER_COMMAND_SMARTSOCKET_AVERAGECURRENT);
        break;
      }
    case TRANSCEIVER_REQUEST_GETRELAYSTATE:
      {
        constructmesssage(__packet__, TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE);
        break;
      }
#elif(TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
    case TRANSCEIVER_REQUEST_GETIPADDRESS:
      {
        constructmesssage(__packet__, TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS);
        break;
      }
#endif

    case TRANSCEIVER_REQUEST_PINGPONG:
      {
        // if ((datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_HUB)
        // || (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER))
        // {
        constructmesssage(__packet__, TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG);
        break;
        // }
        // else
        // return; // do nothing
      }
    case TRANSCEIVER_REQUEST_RESTART:
      {
        Serial.println("Restarting Device!");

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
        resetFunc();
#elif(TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
        ESP.restart(); // aldrin test
#endif

        break;
      }
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
  constructtrashbin(message);
  // memset(message + TRANSCEIVER_BYTELOC_INFORMATION,0, TRANSCEIVER_BYTECOUNT_INFORMATION);
  message[TRANSCEIVER_BYTELOC_INFORMATION] = requestswitch;
}

void constructmesssage(byte * packet, byte commandswitch)
{
  memset(packet + TRANSCEIVER_BYTELOC_PACKETINFO, 0, TRANSCEIVER_BYTECOUNT_PACKETINFO);
  memset(packet + TRANSCEIVER_BYTELOC_INFORMATION, 0, TRANSCEIVER_BYTECOUNT_INFORMATION);
  switch (commandswitch)
  {

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
    case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION:
      {
        Serial.println("Sending Relay State and Power Consumption.");
        packet[TRANSCEIVER_BYTELOC_INFORMATION] = digitalRead(SMARTSOCKET_PIN_RELAY1);
        bitWrite(packet[TRANSCEIVER_BYTELOC_INFORMATION], 1, digitalRead(SMARTSOCKET_PIN_RELAY2));
        bitWrite(packet[TRANSCEIVER_BYTELOC_INFORMATION], 7, TRANSCEIVER_SMARTSOCKET_HASRELAY);
        double WhData[2], mAData[2];
        energyconsumption(WhData, mAData); // store the kwH since the last call at "WhData"
        Serial.println("Relay1:" + String(bitRead(packet[TRANSCEIVER_BYTELOC_INFORMATION], 0) == 0 ? "LOW" : "HIGH"));
        Serial.println("Relay2:" + String(bitRead(packet[TRANSCEIVER_BYTELOC_INFORMATION], 1) == 0 ? "LOW" : "HIGH"));
        Serial.println("WhData1:" + String(WhData[0]));
        Serial.println("WhData2:" + String(WhData[1]));
        Serial.println("mAData:" + String(mAData[0]));
        Serial.println("mAData:" + String(mAData[1]));
        word whole, decimal;
        for (byte index = 0; index <= 1; index++)
        {
          // power consumption allocation
          whole = WhData[index];
          decimal = (WhData[index] - whole) * pow(10, 4);
          for (byte i = 0; i < 2; i++)
          {
            packet[TRANSCEIVER_BYTELOC_INFORMATION + 2 + i + (8 * index)] = whole >> (8 * i);
          }
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 4 + (8 * index)] = decimal;
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 5 + (8 * index)] = decimal >> 8;
          //
          // average current allocation
          whole = mAData[index];
          decimal = (mAData[index] - whole) * pow(10, 4);
          for (byte i = 0; i < 2; i++)
          {
            packet[TRANSCEIVER_BYTELOC_INFORMATION + 6 + i + (8 * index)] = whole >> (8 * i);
          }
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 8 + (8 * index)] = decimal;
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 9 + (8 * index)] = decimal >> 8;
          //
        }
        break;
      }
    case TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION:
      {
        double WhData[2], mAData[2];
        energyconsumption(WhData, NULL);
        Serial.println("WhData1:" + String(WhData[0]));
        Serial.println("WhData2:" + String(WhData[1]));
        word whole, decimal;
        for (byte index = 0; index <= 1; index++)
        {
          // power consumption allocation
          whole = WhData[index];
          decimal = (WhData[index] - whole) * pow(10, 4);
          for (byte i = 0; i < 2; i++)
          {
            packet[TRANSCEIVER_BYTELOC_INFORMATION + 2 + i + (8 * index)] = whole >> (8 * i);
          }
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 4 + (8 * index)] = decimal;
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 5 + (8 * index)] = decimal >> 8;
          //
          // average current allocation
          whole = mAData[index];
          decimal = (mAData[index] - whole) * pow(10, 4);
          for (byte i = 0; i < 2; i++)
          {
            packet[TRANSCEIVER_BYTELOC_INFORMATION + 6 + i + (8 * index)] = whole >> (8 * i);
          }
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 8 + (8 * index)] = decimal;
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 9 + (8 * index)] = decimal >> 8;
          //
        }
        break;
      }
    case TRANSCEIVER_COMMAND_SMARTSOCKET_AVERAGECURRENT:
      {
        double mAData[2];
        energyconsumption(NULL, mAData);
        Serial.println("mAData:" + String(mAData[0]));
        Serial.println("mAData:" + String(mAData[1]));
        word whole, decimal;
        for (byte index = 0; index <= 1; index++)
        {
          // average current allocation
          whole = mAData[index];
          decimal = (mAData[index] - whole) * pow(10, 4);
          for (byte i = 0; i < 2; i++)
          {
            packet[TRANSCEIVER_BYTELOC_INFORMATION + 6 + i + (8 * index)] = whole >> (8 * i);
          }
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 8 + (8 * index)] = decimal;
          packet[TRANSCEIVER_BYTELOC_INFORMATION + 9 + (8 * index)] = decimal >> 8;
          //
        }
        break;
      }
    case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
      {
        packet[TRANSCEIVER_BYTELOC_INFORMATION] = digitalRead(SMARTSOCKET_PIN_RELAY1);
        bitWrite(packet[TRANSCEIVER_BYTELOC_INFORMATION], 1, digitalRead(SMARTSOCKET_PIN_RELAY2));
        bitWrite(packet[TRANSCEIVER_BYTELOC_INFORMATION], 7, TRANSCEIVER_SMARTSOCKET_HASRELAY);
        Serial.println(String("Relay State Sent: ") + String(packet[TRANSCEIVER_BYTELOC_INFORMATION]));
        break;
      }
#elif(TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
    case TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS:
      {
        Serial.print("IP Sent: ");
        for (__loopindex__ = 0; __loopindex__ <= 3; __loopindex__++)
        {
          Serial.print(String(staticip[__loopindex__]) + String("  "));
          packet[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__] = staticip[__loopindex__];
        }
        Serial.println();
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
  memcpy(packet + TRANSCEIVER_BYTELOC_RECEIVERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
  memcpy(packet + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
}

#define TRANSCEIVER_BYTECOUNT_STATICHEADER TRANSCEIVER_BYTECOUNT_NRFTM + TRANSCEIVER_BYTECOUNT_RECEIVERSK + TRANSCEIVER_BYTECOUNT_TRANSMITTERSK
byte getrequest()
{
  memset(__packet__, 0, 32); // empty string first
  // radio.flush_rx(); // flush rx messages
  // Set module as receiver
  //radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
  // Read the data if available in buffer
  __timesaver__ = millis();
  while (radio.available() && millis() - __timesaver__ <= 1000)
    // wait for the signal to reach
  {
    CurrentSensor_inputStats[0].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[0]));  // log to Stats function
    CurrentSensor_inputStats[1].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[1]));  // log to Stats function
    // sabi sa documentary ng nrf24, every packet, 32bytes ang laman kaya 32 bytes of array yung ating "text"
    radio.read(& __packet__, 32);
    byte publictrashbin[TRANSCEIVER_BYTECOUNT_TRASHBIN];
    // if (datatypereadbits(__packet__[0], 6, 2) == TRANSCEIVER_DEVICETYPE_REPEATER)
    // {
    memcpy(publictrashbin, __packet__ + TRANSCEIVER_BYTELOC_TRASHBIN, TRANSCEIVER_BYTECOUNT_TRASHBIN);
    if (wasintrashbin(publictrashbin))
    {
      Serial.println(String("Message Denied because of Trashbin Value: ")
                     + String(__packet__[TRANSCEIVER_BYTELOC_TRASHBIN]) + String("  ")
                     + String(__packet__[TRANSCEIVER_BYTELOC_TRASHBIN + 1]));
      continue;
    }
    // }
    if (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER
        && memcmp(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK)) // memcmp returns true when they are not the same
    {
      Serial.println("Public:");
      printpacketdetails(__packet__);
      radio.stopListening(); // gawing tagadaldal yung arduino gamit ang bibig(NRF24)
      // if (radio.getDataRate() == RF24_250KBPS)
      // radio.flush_tx();
      byte target_channel[5];
      memcpy(target_channel, __packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, 5); // get the target channel of the source
      unsigned long timeout = millis();
      boolean transmitted = false;
      ledbusy
      do
      {
        CurrentSensor_inputStats[0].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[0]));  // log to Stats function
        CurrentSensor_inputStats[1].input(analogRead(SMARTSOCKET_PIN_CURRENTSENSOR[1]));  // log to Stats function
        radio.openWritingPipe(target_channel); // eto yung target channel( hallway ng llyce ) na papasukan ng mensahe(message)
        transmitted = radio.write(__packet__, 32);
        if (transmitted)
          break;
        radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
        transmitted = radio.write(__packet__, 32);
      } while (!transmitted && millis() - timeout < 1000);
      radio.openWritingPipe(__public_channel__); // ibalik natin sa default writing channel
      led_indicator_timeout = millis();
      ledwithsignal
      if (transmitted)
      {
        Serial.println("Prior Requested Signal to be Repeated on Public Channel was Successful!");
        //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH);
        led_indicator_timeout = millis();
      }
      radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
    }
    else if ((datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT) == TRANSCEIVER_THISDEVICE_TYPE || datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT) == TRANSCEIVER_DEVICETYPE_ANYDEVICE)
             && (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_HUB || datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER)
             && !memcmp(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK)
             && !memcmp(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK))
    {
      Serial.println("Private:");
      printpacketdetails(__packet__);
      switch (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND))
      {
        case TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST:
          {

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
            if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] == TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION && __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 3] == 1)
            {
              if (digitalRead(SMARTSOCKET_PIN_RELAY1) != __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 1])
                digitalWrite(SMARTSOCKET_PIN_RELAY1, __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 1]);
              if (digitalRead(SMARTSOCKET_PIN_RELAY2) != __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 2])
                digitalWrite(SMARTSOCKET_PIN_RELAY2, __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 2]);
            }
#endif

            return __packet__[TRANSCEIVER_BYTELOC_INFORMATION]; // this is the requestswitch information
          }

#if (TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_SMARTSOCKET)
        case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
          {
            if (digitalRead(SMARTSOCKET_PIN_RELAY1) != __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 1] && (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] == 0 || __packet__[TRANSCEIVER_BYTELOC_INFORMATION] == 1))
            {
              digitalWrite(SMARTSOCKET_PIN_RELAY1, __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 1]);
              Serial.println("Relay 1 was Changed to:" + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + 1]));
            }
            if (digitalRead(SMARTSOCKET_PIN_RELAY2) != __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 2] && (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] == 0 || __packet__[TRANSCEIVER_BYTELOC_INFORMATION] == 2))
            {
              digitalWrite(SMARTSOCKET_PIN_RELAY2, __packet__[TRANSCEIVER_BYTELOC_INFORMATION + 2]);
              Serial.println("Relay 2 was Changed to:" + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + 2]));
            }
            break;
          }
#elif(TRANSCEIVER_THISDEVICE_TYPE == TRANSCEIVER_DEVICETYPE_IPCAMERA)
        case TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS:
          {
            static word WiFi_CharLength[2] =
            {
              0
            };
            unsigned long packetwasreceived[2] =
            {
              0
            }; // initially set all packet was received to false // max of 32 packets or 2^5
            packetwasreceived[0] = __packet__[TRANSCEIVER_BYTELOC_INFORMATION] + (__packet__[TRANSCEIVER_BYTELOC_PACKETINFO] << 8); // temporarily use this variable, i will empty it again
            byte packetscount[2];
            packetscount[0] = datatypereadbits(packetwasreceived[0], 6, 5);
            packetscount[1] = datatypereadbits(packetwasreceived[0], 11, 5);
            packetwasreceived[0] = 0xFFFFFFFF << packetscount[0]; // set all packets on the right as RECEIVED while the number of packet count is zero
            packetwasreceived[1] = 0xFFFFFFFF << packetscount[1]; // set all packets on the right as RECEIVED while the number of packet count is zero
            if (packetscount[0] > 0 || packetscount[1] > 0)
            {
              if (__WiFi_SSID__ != NULL)
              {
                memset(__WiFi_SSID__, 0, WiFi_CharLength[0]);
                delete[] __WiFi_SSID__;
                __WiFi_SSID__ = NULL;
              }
              if (__WiFi_Password__ != NULL)
              {
                memset(__WiFi_SSID__, 0, WiFi_CharLength[1]);
                delete[] __WiFi_Password__;
                __WiFi_Password__ = NULL;
              }
              Serial.println(String("packet0:") + String(packetscount[0]));
              Serial.println(String("packet1:") + String(packetscount[1]));
              WiFi_CharLength[0] = TRANSCEIVER_BYTECOUNT_WORDINFO * packetscount[0];
              WiFi_CharLength[1] = TRANSCEIVER_BYTECOUNT_WORDINFO * packetscount[1];
              if (packetscount[0] > 0)
                __WiFi_SSID__ = new char[WiFi_CharLength[0]];
              if (packetscount[1] > 0)
                __WiFi_Password__ = new char[WiFi_CharLength[1]];
              char datapiece[TRANSCEIVER_BYTECOUNT_WORDINFO];
              byte currentpacket = datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_INFORMATION], 1, 5);
              byte datawasfor = bitRead(__packet__[TRANSCEIVER_BYTELOC_INFORMATION], 0);
              if (datawasfor)
              {
                if (__WiFi_Password__ != NULL)
                  memcpy(__WiFi_Password__ + (TRANSCEIVER_BYTECOUNT_WORDINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WORDINFO);
              }
              else
              {
                if (__WiFi_SSID__ != NULL)
                  memcpy(__WiFi_SSID__ + (TRANSCEIVER_BYTECOUNT_WORDINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WORDINFO);
              }
              bitWrite(packetwasreceived[datawasfor], currentpacket, 1); // set the current packet was received(true or 1)
              __timesaver__ = millis();
              byte dynamicheader[TRANSCEIVER_BYTECOUNT_STATICHEADER];
              byte constantheader[TRANSCEIVER_BYTECOUNT_STATICHEADER];
              memcpy(constantheader, __packet__, TRANSCEIVER_BYTECOUNT_STATICHEADER);
              if (packetscount[datawasfor] > 0)
                packetscount[datawasfor] --;
recoverlostpackets:
              while ((packetscount[0] > 0 || packetscount[1] > 0))
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
                        if (__WiFi_Password__ != NULL)
                          memcpy(__WiFi_Password__ + (TRANSCEIVER_BYTECOUNT_WORDINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WORDINFO);
                      }
                      else
                      {
                        if (__WiFi_SSID__ != NULL)
                          memcpy(__WiFi_SSID__ + (TRANSCEIVER_BYTECOUNT_WORDINFO * currentpacket), __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, TRANSCEIVER_BYTECOUNT_WORDINFO);
                      }
                      bitWrite(packetwasreceived[datawasfor], currentpacket, 1); // set the current packet was received(true or 1)
                      packetscount[datawasfor] --;
                      __timesaver__ = millis();
                    }
                  }
                }
                else if (millis() - __timesaver__ > 5000 && (packetwasreceived[0] != 0xFFFFFFFF || packetwasreceived[1] != 0xFFFFFFFF))
                {
                  Serial.println("Requesting Lost packets...");
                  memset(__packet__, 0, 32); // empty message buffer
                  datatypewritebits(__packet__ + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND, TRANSCEIVER_NRFTM_BITLOC_COMMAND);
                  datatypewritebits(__packet__ + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_RDT, TRANSCEIVER_NRFTM_BITLOC_RDT);
                  datatypewritebits(__packet__ + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_THISDEVICE_TYPE, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
                  memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
                  memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
                  __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = TRANSCEIVER_REQUEST_GETWIFILOSTPACKET;
                  for (__loopindex__ = 1; __loopindex__ <= 4; __loopindex__++)
                  {
                    __packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__] = datatypereadbits(packetwasreceived[0] >> (8 * (__loopindex__ - 1)), 0, 8);
                    __packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__ + 4] = datatypereadbits(packetwasreceived[1] >> (8 * (__loopindex__ - 1)), 0, 8);
                  }
                  transmitmessage(__packet__);
                  __timesaver__ = millis(); // reset timeout
                  goto recoverlostpackets;
                }
              }
              if (__WiFi_SSID__ != NULL)
                // ssid exist and not had timeout
              {
                Serial.print("\nSSID was Set to:");
                for (word i = 0; i < WiFi_CharLength[0]; i++)
                  Serial.print(__WiFi_SSID__[i]);
                Serial.print("\nPassword was Set to:");
                for (word i = 0; i < WiFi_CharLength[1]; i++)
                  Serial.print(__WiFi_Password__[i]);
                WiFi.begin(__WiFi_SSID__, __WiFi_Password__); // aldrin test
                __timesaver__ = millis();
                while (WiFi.status() != WL_CONNECTED)
                {
                  delay(500);
                  Serial.print(".");
                  if ((WiFi.status() == WL_CONNECT_FAILED) || (millis() - __timesaver__ > 10000))
                    // maghintay 10 sec kung di prin connected, request ulit wifi password
                  {
                    // some code here
                    break;
                  }
                }
              }
              /*if (__WiFi_SSID__ != NULL)
                {
                memset(__WiFi_SSID__,0,WiFi_CharLength[0]);
                delete[] __WiFi_SSID__;
                __WiFi_SSID__ = NULL;
                }
                if (__WiFi_Password__ != NULL)
                {
                memset(__WiFi_SSID__,0,WiFi_CharLength[1]);
                delete[] __WiFi_Password__;
                __WiFi_Password__ = NULL;
                }*/
            }
            break;
          }
#endif

        case TRANSCEIVER_COMMAND_DEBUG_BYTEMESSAGE:
          {
            Serial.println("You got a Byte Message:");
            for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
            {
              Serial.println(String("Byte ") + String(__loopindex__) + String(" = ") + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__]));
            }
            break;
          }
        case TRANSCEIVER_COMMAND_DEBUG_CHARMESSAGE:
          {
            Serial.print("You got a Char Message:");
            for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
            {
              Serial.print("  " + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__]));
            }
            Serial.println("");
            break;
          }
        default:
          break;
      }
    }
  }
  return 0;
}
