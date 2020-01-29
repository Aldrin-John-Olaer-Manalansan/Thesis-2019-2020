#define HOMEPACKAGE_TRANSCEIVER_PHYSICALCHANNEL 97 // value must be within 0-125 ~ DEFAULT: 76
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
#define TRANSCEIVER_REQUEST_GETAVERAGECURRENT 3
#define TRANSCEIVER_REQUEST_GETRELAYSTATE 4
#define TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION 5
#define TRANSCEIVER_REQUEST_GETWIFICREDENTIALS 6
#define TRANSCEIVER_REQUEST_GETWIFILOSTPACKET 7
#define TRANSCEIVER_REQUEST_GETIPADDRESS 8
#define TRANSCEIVER_REQUEST_PINGPONG 64
#define TRANSCEIVER_REQUEST_RESTART 128
//
#define TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST 0
#define TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO 1
#define TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION 2
#define TRANSCEIVER_COMMAND_SMARTSOCKET_AVERAGECURRENT 3
#define TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATE 4
#define TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION 5
#define TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS 6
#define TRANSCEIVER_COMMAND_IPCAMERA_IPADDRESS 7
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
#define TRANSCEIVER_BYTECOUNT_WORDINFO 17
//
#define TRANSCEIVER_MAXPACKETSCOUNT 31 // 20 packets is the safest for arduinos
//
#define TRANSCEIVER_HUB_FETCHINGINTERVAL 5000 // every 5 seconds he Hub fetches data

byte __loopindex__;

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

#define publictrashbin_instances 4 // 3 usable trashbins while 1 dynamic trashbin
bool wasintrashbin(byte * passedtrashbin)
{
  static byte publictrashbin[publictrashbin_instances][TRANSCEIVER_BYTECOUNT_TRASHBIN]; // stores the old message to avoid repetitive seding of message, up to 4 trashbins, the higher the trashbin the higher the avoidance of repetitive message sending
  static byte publictrashbin_switch = 0;
  for (__loopindex__ = 0; __loopindex__ < publictrashbin_instances; __loopindex__++)
  {
    if (!memcmp(publictrashbin[__loopindex__], passedtrashbin, TRANSCEIVER_BYTECOUNT_TRASHBIN)) // return true if they are the same
      return true;// was at the trashbin
  }
  if (publictrashbin_switch == 0)
    publictrashbin_switch = publictrashbin_instances - 1;
  else
    publictrashbin_switch--;
  memcpy(publictrashbin[publictrashbin_switch], passedtrashbin, TRANSCEIVER_BYTECOUNT_TRASHBIN);
  return false;
}

void constructtrashbin(byte* packet)
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

void printpacketdetails(byte* packet)
{
  //Serial.println("Received a Message:");

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
    case TRANSCEIVER_COMMAND_SMARTSOCKET_AVERAGECURRENT:
      Serial.println("Average Current Info");
      break;
    case TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION:
      Serial.println("Relay State and Power Consumption Info");
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

  Serial.print("TrashBin: ");
  for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_TRASHBIN; __loopindex__++)
    Serial.print(String(packet[__loopindex__ + TRANSCEIVER_BYTELOC_TRASHBIN]) + String("  "));
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