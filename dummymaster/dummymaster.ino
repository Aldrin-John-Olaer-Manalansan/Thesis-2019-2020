#include <SPI.h>
#include <RF24.h>

#define NRF_PIN_CE 25
#define NRF_PIN_CSN 26

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

RF24 radio(NRF_PIN_CE, NRF_PIN_CSN); // CE, CSN
const byte __public_channel__[5] = {102, 32, 92, 192, 97};
byte __private_channel__[5] = {0b01001100, 0b11100000, 0b00010111, 0b01101100, 0b10101100}; // 76,224,23,108,172

unsigned long __timesaver__=millis();
byte __packet__[32] = {0};
byte __loopindex__;
void setup() {
  Serial.begin(115200);
  if (radio.begin()) // buhayin na yung module, magkainin na ng current yung nrf
    Serial.println("Radio was Successful");
  else
    Serial.println("Radio failed to begin");

  radio.setPALevel(RF24_PA_MAX); // 0-3 , 0=lowest , 3=max
  radio.setDataRate(RF24_250KBPS); // slowest data rate for improved range of transmission and more low power consumption

  // set the 1st pipe(0) sa private channel
  radio.stopListening();
  //radio.openReadingPipe(0, __public_channel__);
  radio.openReadingPipe(1, __private_channel__);
  radio.startListening();
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

void transmitmessage(byte * packet)
{
  radio.stopListening(); // gawing tagadaldal yung arduino gamit ang bibig(NRF24)

  radio.openWritingPipe(__private_channel__); // eto yung target channel(room eg. B103 sa LSB) na papasukan ng mensahe(message)
  __timesaver__=millis();
  while (!radio.write(packet, 32) && millis()-__timesaver__<=1000)
    // broadcast yung message
  {
    //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW);
    datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_REPEATER, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
    radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
    if (radio.write(packet, 32)) // broadcast yung message
      break;
    else
    {
      datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
      Serial.println("Failed to Communicate");
    }
  }
  //else
  //  digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, HIGH);

  //radio.openReadingPipe(0, __private_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
  radio.startListening(); // makikinig lang yung NRF24 gamit ang anim(6) na tenga nya
}

void loop()
{
  if (Serial.available() > 0)
    /*
       Known commands:
       get power consumption of the device - byte[0]=128 - byte[14]=2
       get relay state of the device - byte[0]=128 - byte[14]=3
       get ip address of the device - byte[0]=128 - byte[14]=5
       ping-pong communication - byte[0]=128 - byte[14]=64
       SET relay state of the device - byte[0]=131 - byte[14]=(0 or 1)
       byte communication - byte[0]=142
    */
  {
    String serialstr = Serial.readString();
    if (serialstr.indexOf("byte") >= 0)
    {
      memset(__packet__, 0, 32);
      for (__loopindex__ = 0; __loopindex__ < 32; __loopindex__++)
      {
        if (__loopindex__ > 0 && __loopindex__ < 14)
          continue;
        Serial.print(String("Set Byte ") + String(__loopindex__) + String(": "));
        while (Serial.available() <= 0) { }
        serialstr = Serial.readString();
        if (serialstr.endsWith("\r\n"))
          serialstr.remove(serialstr.length() - 2, 2);
        else if (serialstr.endsWith("\r") || serialstr.endsWith("\n"))
          serialstr.remove(serialstr.length() - 1, 1);
        if (serialstr == "stop")
          break;
        __packet__[__loopindex__] = serialstr.toInt();
        Serial.println(__packet__[__loopindex__]);
      }
      memcpy(__packet__ + 1, __private_channel__, 5);
      memcpy(__packet__ + 6, __public_channel__, 5);
      Serial.println("Reciting Message");
      for (__loopindex__ = 0; __loopindex__ < 32; __loopindex__++)
      {
        Serial.print(String("Byte ") + String(__loopindex__) + String(':'));
        Serial.println(__packet__[__loopindex__]);
      }

      radio.stopListening();
      radio.openWritingPipe(__private_channel__);
  __timesaver__=millis();
  while (!radio.write(__packet__, 32) && millis()-__timesaver__<=1000)
    // broadcast yung message
  {
    //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW);
    datatypewritebits(__packet__ + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_REPEATER, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
    radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
    if (radio.write(__packet__, 32)) // broadcast yung message
      break;
    else
    {
      datatypewritebits(__packet__ + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
        Serial.println("Failed to Send the Data");
    }
  }
      //radio.openReadingPipe(0, __private_channel__);
      radio.startListening();
    }
    else if (serialstr.indexOf("string") >= 0)
    {
      Serial.println(String("Set String Message: "));
      memset(__packet__, 0, 32);
      while (Serial.available() <= 0) { }
      serialstr = Serial.readString();
      if (serialstr.endsWith("\r\n"))
        serialstr.remove(serialstr.length() - 2, 2);
      else if (serialstr.endsWith("\r") || serialstr.endsWith("\n"))
        serialstr.remove(serialstr.length() - 1, 1);

      Serial.println(serialstr);
      byte length = serialstr.length() < 18 ? serialstr.length() : 18;
      for (__loopindex__ = 0; __loopindex__ < length; __loopindex__++)
      {
        __packet__[14 + __loopindex__] = serialstr[__loopindex__];
        Serial.print((char)__packet__[14 + __loopindex__]);
      }
      Serial.println("");

      __packet__[0] = 0b10001111;
      memcpy(__packet__ + 1, __private_channel__, 5);
      memcpy(__packet__ + 6, __public_channel__, 5);
      Serial.println("Reciting Message");
      for (__loopindex__ = 0; __loopindex__ < 32; __loopindex__++)
      {
        Serial.print(String("Byte ") + String(__loopindex__) + String(':'));
        Serial.println(__packet__[__loopindex__]);
      }

      radio.stopListening();
      radio.openWritingPipe(__private_channel__);
  __timesaver__=millis();
  while (!radio.write(__packet__, 32) && millis()-__timesaver__<=1000)
    // broadcast yung message
  {
    //digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW);
    datatypewritebits(__packet__ + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_REPEATER, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
    radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
    if (radio.write(__packet__, 32)) // broadcast yung message
      break;
    else
    {
      datatypewritebits(__packet__ + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
        Serial.println("Failed to Send this Data");
    }
  }
      //radio.openReadingPipe(0, __private_channel__);
      radio.startListening();
    }
  }
  else if (radio.available())
  {
    radio.read(&__packet__, 32);

    printpacketdetails(__packet__);

    switch (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND))
    {
      case TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST:
        if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] == TRANSCEIVER_REQUEST_GETPUBLICCHANNEL)
        {
          datatypewritebits(&__packet__[TRANSCEIVER_BYTELOC_NRFTM], datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT), TRANSCEIVER_NRFTM_BITCOUNT_RDT, TRANSCEIVER_NRFTM_BITLOC_RDT); // become the receiver device type on our transmission
          datatypewritebits(&__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
          datatypewritebits(&__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND, TRANSCEIVER_NRFTM_BITLOC_COMMAND);
          memcpy(__private_channel__, __packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
          memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
          memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
          memset(__packet__ + TRANSCEIVER_BYTELOC_TRASHBIN, 0, TRANSCEIVER_BYTECOUNT_TRASHBIN);
          memset(__packet__ + TRANSCEIVER_BYTELOC_PACKETINFO, 0, TRANSCEIVER_BYTECOUNT_PACKETINFO);
          memcpy(__packet__ + TRANSCEIVER_BYTELOC_INFORMATION, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
          transmitmessage(__packet__);
        }
        break;
      case TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION:
        Serial.print("Total Wh since last fetch:");

        for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
          Serial.print((char)__packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__]);
        Serial.println("");
        break;
      case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE:
        Serial.print("Relay State: ");
        if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION])
          Serial.println("HIGH");
        else
          Serial.println("LOW");
        break;
      case TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS:
        Serial.println(String("IP Address: ")
                       + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION])
                       + String('.')
                       + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + 1])
                       + String('.')
                       + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + 2])
                       + String('.')
                       + String(__packet__[TRANSCEIVER_BYTELOC_INFORMATION + 3]));
        break;
      case TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG:
        Serial.println("The Device was Reachable!");
        break;
      default:
        break;
    }

  }
}
