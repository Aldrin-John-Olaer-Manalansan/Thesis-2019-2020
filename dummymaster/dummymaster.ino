#include <SPI.h>
#include <RF24.h>
#include "C:\Users\acer\Desktop\Thesis\Project\Source_Codes\ThesisLibraries\transceiver_library\transceiver_library.cpp"
#include <BLEDevice.h> //Header file for BLE
//
#define IPCAMERA_SERVICEUUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define IPCAMERA_CHARACTERISTICSUUID_WIFICREDENTIALS "beb5483e-36e1-4688-b7f5-ea07361b26a8"
//
#define BLE_Dot_KeyWord "nU)7S"
#define BLE_Dot_Length_KeyWord 5 // length ng serial key sa taas
#define BLE_YouAreFree_KeyWord "oE9*3&#M0*a2vcA"
#define BLE_YouAreFree_Length_KeyWord 15 // length ng serial key sa taas
#define BLE_RequestCredentials_KeyWord "I75^SuSwIoNv&6y"
#define BLE_RequestCredentials_Length_KeyWord 15 // length ng serial key sa taas
#define BLE_WrongCredential_KeyWord "iU8#9Dv_dieYb*3"
#define BLE_WrongCredential_Length_KeyWord 15 // length ng serial key sa taas
#define BLE_IPAddress_KeyWord "m*Ay@9X7&4Sp,u8"
#define BLE_IPAddress_Length_KeyWord 15 // length ng serial key sa taas
#define BLE_SSID_KeyWord "Mq83Uc_3ZvSeLVe"
#define BLE_SSID_Length_KeyWord 15 // length ng serial key sa taas
#define BLE_Password_KeyWord "p!i^p##0&eOsqm3"
#define BLE_Password_Length_KeyWord 15 // length ng serial key sa taas
#define builtin_serialkey "mWQa!e6IqCISo%3" // eto yung UNIQUE Serial Key ng Device nato
#define builtin_length_serialkey 15 // length ng serial key sa taas
//
String __Router_SSID__ = "Manalansan";
String __Router_Password__ = "Manalansan@123!";
//
byte staticip[4];
//
static BLEUUID serviceUUID(IPCAMERA_SERVICEUUID); // Service UUID of IPCAM obtained through nRF connect application
static BLEUUID charUUID(IPCAMERA_CHARACTERISTICSUUID_WIFICREDENTIALS); // Characteristic  UUID of IPCAM obtained through nRF connect application
String My_BLE_Address = "30:ae:a4:f5:04:fe"; // Hardware Bluetooth MAC of the IP-Camera, will vary for every band obtained through nRF connect application
BLERemoteCharacteristic * pRemoteCharacteristic = NULL;
BLEScan * pBLEScan = NULL; // Name the scanning device as pBLEScan;
BLEAddress * Server_BLE_Address = NULL;
BLEClient * pClient;
static BLEScanResults foundDevices;
String Scaned_BLE_Address;
bool connectToserver(BLEAddress pAddress)
{
  pClient = BLEDevice::createClient();
  Serial.println(" - Created client");
  // Connect to the BLE Server.
  pClient -> connect(pAddress);
  Serial.println(" - Connected to IP-Camera");
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService * pRemoteService = pClient -> getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient -> disconnect();
    return false;
  }
  Serial.println(" - Found our service");
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  if (pRemoteCharacteristic != NULL)
    delete pRemoteCharacteristic;
  pRemoteCharacteristic = pRemoteService -> getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient -> disconnect();
    return false;
  }
  else
    Serial.println(" - Found our characteristic");
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.printf("Scan Result: %s \n", advertisedDevice.toString().c_str());
      if (Server_BLE_Address != NULL)
      {
        delete Server_BLE_Address;
        Server_BLE_Address = NULL;
      }
      Server_BLE_Address = new BLEAddress(advertisedDevice.getAddress());
      Scaned_BLE_Address = Server_BLE_Address -> toString().c_str();
      if (Scaned_BLE_Address == My_BLE_Address)
      {
        Serial.println("Target Device FOUND!");
        pBLEScan -> stop();
        pBLEScan -> clearResults();
      }
    }
};

TaskHandle_t NRF_CoreHandle;
void setup()
{
  Serial.begin(115200); // Start serial monitor
  xTaskCreatePinnedToCore(
    NRFMonitor,
    /* Task function. */
    "NRF Monitoring System",
    /* name of task. */
    5000,
    /* Stack size of task */
    NULL,
    /* parameter of the task */
    0,
    /* priority of the task */
    & NRF_CoreHandle,
    /* Task handle to keep track of created task */
    0);
  /* pin task to core 0 */
  delay(500);
  Serial.println("ESP32 BLE Server program"); // Intro message
  pinMode(LED_BUILTIN, OUTPUT); // Declare the in-built LED pin as output
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  BLEAdvertisedDeviceCallbacks * pcustomcallback = new MyAdvertisedDeviceCallbacks();
  pBLEScan -> setAdvertisedDeviceCallbacks(pcustomcallback); // Call the class that is defined above
  pBLEScan -> setActiveScan(true); // active scan uses more power, but get results faster
  foundDevices = pBLEScan -> start(3); // Scan for 3 seconds to find the IP Camera
}

void loop()
{
  static bool paired = false; // boolean variable as paring indicator
  // Serial.println(ESP.getFreeHeap());
  if (foundDevices.getCount() > 0 && Server_BLE_Address != NULL && Scaned_BLE_Address == My_BLE_Address)
  {
    // Serial.println("Trying");
    if (!paired)
    {
      Serial.println(String("Found Device " "") + Scaned_BLE_Address + String("" " connecting to its Server as client"));
      paired = connectToserver(* Server_BLE_Address);
    }
    if (paired && pClient -> isConnected())
    {
      if (!digitalRead(LED_BUILTIN))
      {
        Serial.println("********************LED Turned ON************************");
        digitalWrite(LED_BUILTIN, HIGH);
      }
      if (pRemoteCharacteristic != NULL)
      {
        // if (pRemoteCharacteristic->canWrite() && pRemoteCharacteristic->canRead())
        // {
        String BLE_message = pRemoteCharacteristic -> readValue().c_str(); // read the ble characteristics of the server
        if (BLE_message.startsWith(builtin_serialkey) && BLE_message.endsWith(builtin_serialkey))
        {
          if (BLE_message.indexOf(BLE_RequestCredentials_KeyWord) > 0 || BLE_message.indexOf(BLE_WrongCredential_KeyWord) > 0)
          {
            Serial.println(("Received WiFi Credential Request:") + BLE_message);
            if (BLE_message.indexOf(BLE_WrongCredential_KeyWord) > 0)
            {
              // inform the user that the ip camera cannot connect to the wifi
              Serial.println("ALERT!!! An IP-Camera claims that the WiFi Credentials are Invalid! Take a Look:");
              Serial.println(String("SSID:") + __Router_SSID__);
              Serial.println(String("Password:") + __Router_Password__);
            }
            BLE_message = String(builtin_serialkey) +
                          String(BLE_SSID_KeyWord) + __Router_SSID__ +
                          String(BLE_Password_KeyWord) + __Router_Password__ +
                          String(builtin_serialkey);
            Serial.println(("Sending the WiFi Credentials:") + BLE_message);
            pRemoteCharacteristic -> writeValue(BLE_message.c_str(), BLE_message.length());
          }
          else if (BLE_message.indexOf(BLE_IPAddress_KeyWord) > 0)
          {
            Serial.println(("Received the IP-Address of the IP-Camera:") + BLE_message);
            __loopindex__ = BLE_message.indexOf(BLE_Dot_KeyWord);
            staticip[0] = (BLE_message.substring(BLE_message.indexOf(BLE_IPAddress_KeyWord) + BLE_IPAddress_Length_KeyWord, __loopindex__)).toInt();
            staticip[1] = (BLE_message.substring(__loopindex__ + BLE_Dot_Length_KeyWord, BLE_message.indexOf(BLE_Dot_KeyWord, __loopindex__ + 1))).toInt();
            __loopindex__ = BLE_message.indexOf(BLE_Dot_KeyWord, __loopindex__ + 1);
            staticip[2] = (BLE_message.substring(__loopindex__ + BLE_Dot_Length_KeyWord, BLE_message.indexOf(BLE_Dot_KeyWord, __loopindex__ + 1))).toInt();
            __loopindex__ = BLE_message.indexOf(BLE_Dot_KeyWord, __loopindex__ + 1);
            staticip[3] = (BLE_message.substring(__loopindex__ + BLE_Dot_Length_KeyWord, BLE_message.indexOf(builtin_serialkey, builtin_length_serialkey))).toInt();
            Serial.println(String("IP-Camera Fetched IP-Address: ") + String(staticip[0]) + '.' + String(staticip[1]) + '.' + String(staticip[2]) + '.' + String(staticip[3]));
            BLE_message = String(builtin_serialkey) +
                          String(BLE_YouAreFree_KeyWord) +
                          String(builtin_serialkey);
            Serial.println(("Freeing the Caller:") + BLE_message);
            pRemoteCharacteristic -> writeValue(BLE_message.c_str(), BLE_message.length());
          }
        }
        // }
      }
    }
    else
    {
      if (digitalRead(LED_BUILTIN) || paired == true)
      {
        Serial.println("Pairing Failed");
        paired = false;
        Serial.println("********************LED Turned OFF***********************");
        digitalWrite(LED_BUILTIN, LOW);
      }
      if (Server_BLE_Address != NULL)
      {
        // delete[] Server_BLE_Address;
        delete Server_BLE_Address;
        Server_BLE_Address = NULL;
      }
      pBLEScan -> stop();
      pBLEScan -> clearResults();
      foundDevices = pBLEScan -> start(1); // Scan for 3 seconds to find the IP Camera
      // Serial.print("Count:");
      // Serial.println(foundDevices.getCount());
    }
  }
  else
  {
    if (digitalRead(LED_BUILTIN) || paired == true)
    {
      Serial.println("Our device went out of range");
      paired = false;
      Serial.println("********************LED OOOFFFFF************************");
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (Server_BLE_Address != NULL)
    {
      // delete[] Server_BLE_Address;
      delete Server_BLE_Address;
      Server_BLE_Address = NULL;
    }
    pBLEScan -> stop();
    pBLEScan -> clearResults();
    foundDevices = pBLEScan -> start(1); // Scan for 3 seconds to find the IP Camera
    // Serial.print("Count:");
    // Serial.println(foundDevices.getCount());
  }
}

void NRFMonitor(void * pvParameters)
{
  // setup
  NRFsetup();
  //
  for (;;)
  {
    NRFloop();
    /*Serial.print("NRF Watermark:");
      Serial.println(uxTaskGetStackHighWaterMark(NRF_CoreHandle));
      Serial.print("ESP Free Heap:");
      Serial.println(ESP.getFreeHeap());*/
    vTaskDelay(1); // cater other task pinned at core 0
  }
}

#define NRF_PIN_CE 25
#define NRF_PIN_CSN 26
//
RF24 radio(NRF_PIN_CE, NRF_PIN_CSN); // CE, CSN
const byte __public_channel__[5] =
{
  102, 32, 92, 192, 97
};

byte __registered_channels__[8][5] =
{
  {
    0b01001100, 0b11100000, 0b00010111, 0b01101100, 0b10101100
  },
  {
    162, 111, 4, 54, 92
  },
  {
    92, 255, 134, 222, 0
  },
  {
    0, 423, 111, 32, 42
  },
  {
    12, 182, 33, 12, 87
  },
  {
    16, 11, 40, 49, 74
  },
  {
    122, 199, 153, 163, 102
  },
  {
    136, 0, 0, 12, 192
  }
}; // 76,224,23,108,172 and 162,111,4,54,92
byte __registered_wasactive__ = 0b00000001; // only registeredchannel[0] is actived

byte __private_channel__[5] =
{
  0b01001100, 0b11100000, 0b00010111, 0b01101100, 0b10101100
};

byte __target_devicetype__;
unsigned long __timesaver__ = millis();
byte __packet__[32] =
{
  0
};

void NRFsetup()
{
  if (radio.begin())
    // buhayin na yung module, magkainin na ng current yung nrf
    Serial.println("Radio was Successful");
  else
    Serial.println("Radio failed to begin");
  radio.setChannel(HOMEPACKAGE_TRANSCEIVER_PHYSICALCHANNEL);
  radio.setPALevel(RF24_PA_MAX); // 0-3 , 0=lowest , 3=max
  radio.setDataRate(RF24_250KBPS); // slowest data rate for improved range of transmission and more low power consumption
  radio.setRetries(7, 15); // 2000us delay each failure, 15 retries(max already)
  // set the 1st pipe(0) sa private channel
  radio.stopListening();
  if (radio.getDataRate() == RF24_250KBPS)
    radio.flush_tx();
  radio.openReadingPipe(0, __public_channel__);
  radio.openReadingPipe(1, __private_channel__);
  radio.startListening();
  /* while (true) // only use this for channel callibration, else comment this whole while loop
    {
    for (__loopindex__=0;__loopindex__<=125;__loopindex__++)
    {
    radio.setChannel(__loopindex__);
    Serial.println(String("Channel ")+String(__loopindex__)
    +String(": TestCarrier=")+String(radio.testCarrier())
    +String("  TestRPD=")+String(radio.testRPD()));
    }
    delay(10000); // after 10 sec, retest channels
    } */
}

bool transmitpacket(byte * packet, bool longtimeout = false)
{
  bool transmitted;
  radio.stopListening();
  if (radio.getDataRate() == RF24_250KBPS)
    radio.flush_tx();
  constructtrashbin(packet);
  radio.openWritingPipe(__private_channel__);
  __timesaver__ = millis();
  do // broadcast yung message
  {
    transmitted = radio.write(packet, 32); // broadcast yung message
    if (transmitted)
      break;
    // digitalWrite(TRANSCEIVER_PIN_SIGNALINDICATOR, LOW);
    datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_REPEATER, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
    radio.openWritingPipe(__public_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
    transmitted = radio.write(packet, 32); // broadcast yung message
    if (transmitted)
      break;
    Serial.println("Failed to Send the Data");
    //constructtrashbin(packet); // commented since this results spam of info to receivers
    radio.setDataRate(radio.getDataRate() == RF24_250KBPS ? RF24_1MBPS : RF24_250KBPS);
    radio.openWritingPipe(__private_channel__); // eto yung target public channel( hallway ng llyce ) na papasukan ng mensahe(message)
    datatypewritebits(packet + TRANSCEIVER_BYTELOC_NRFTM, TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
  }
  while (!transmitted && millis() - __timesaver__ <= 1000 * (longtimeout == true ? 10 : 1));
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, __public_channel__);
  radio.startListening();
  return transmitted;
}

void NRFloop()
{
  static unsigned long requestpowertimeout;
  static byte deviceswitch = 0;
  if (millis() - requestpowertimeout >= TRANSCEIVER_HUB_FETCHINGINTERVAL) // every 5 seconds, fetch the power consumption of all devices
  {
    for (byte index = 0; index < sizeof(__registered_channels__) / 5; index++) // there are 5 bytes every registered channels that is why we need to divide it by 5
    {
      if (!bitRead(__registered_wasactive__, index))
        continue; // means that the registeredchannel was not actived
      radio.stopListening();
      memcpy(__private_channel__, __registered_channels__[index], 5); // switch the listened private channel
      radio.openReadingPipe(1, __private_channel__);
      bool received = false;
      requestpowertimeout = millis();
      do
      {
        memset(__packet__, 0, 32);
        __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 128;
        __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION;
        memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
        memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
        bool transmitted = transmitpacket(__packet__); // transmit the power consumption request
        unsigned long waittimeout = millis();
        while (transmitted && !received && millis() - waittimeout < 1000)
        {
          if (radio.available())
          {
            radio.read(& __packet__, 32);
            byte publictrashbin[TRANSCEIVER_BYTECOUNT_TRASHBIN];
            memcpy(publictrashbin, __packet__ + TRANSCEIVER_BYTELOC_TRASHBIN, TRANSCEIVER_BYTECOUNT_TRASHBIN);
            if (wasintrashbin(publictrashbin))
            {
              Serial.println(String("Stated Message Denied because of Trashbin Value: ")
                             + String(__packet__[TRANSCEIVER_BYTELOC_TRASHBIN]) + String("  ")
                             + String(__packet__[TRANSCEIVER_BYTELOC_TRASHBIN + 1]));
              continue;
            }
            if (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_RDT, TRANSCEIVER_NRFTM_BITCOUNT_RDT) == TRANSCEIVER_DEVICETYPE_HUB
                && (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_SMARTSOCKET || datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT) == TRANSCEIVER_DEVICETYPE_REPEATER)
                && datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND) == TRANSCEIVER_COMMAND_SMARTSOCKET_RELAYSTATEANDPOWERCONSUMPTION
                && !memcmp(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK)
                && !memcmp(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK))
            {
              Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
              Serial.println(String("Response from Smart Socket (")
                             + String(__private_channel__[0]) + ','
                             + String(__private_channel__[1]) + ','
                             + String(__private_channel__[2]) + ','
                             + String(__private_channel__[3]) + ','
                             + String(__private_channel__[4])
                             + String("):"));
              Serial.println(String("Relay State: ") + String(!__packet__[TRANSCEIVER_BYTELOC_INFORMATION] ? "LOW" : "HIGH"));
              Serial.print("Total Wh since last fetch:");
              for (byte i = 1; i < TRANSCEIVER_BYTECOUNT_INFORMATION; i++)
              {
                Serial.print((char) __packet__[TRANSCEIVER_BYTELOC_INFORMATION + i]);
              }
              Serial.println("");
              Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
              received = true;
              radio.flush_rx();
              break;
            }
          }
        }
      }
      while (!received && millis() - requestpowertimeout < 5000);
    }
    memcpy(__private_channel__, __registered_channels__[deviceswitch], 5); // recover back the listened private channel
    requestpowertimeout = millis();
  }
  else if (Serial.available() > 0)
    /*
      commands:
      UNIVERSAL:
      pingpong
      thatdevicerestart
      
      SMART SOCKET:
      getpower
      getrelay
      onrelay
      offrelay
      
      IP Camera:
      getip
      setupwifi

      Setup:
      getlisteneddevice
      listentodevice
      setdevicestate
      
      Known commands:
      get power consumption of the device - byte[0]=128 - byte[14]=2
      get relay state of the device - byte[0]=128 - byte[14]=3
      get ip address of the device - byte[0]=144 - byte[14]=5
      ping-pong communication - byte[0]=128 - byte[14]=64
      SET relay state of the device - byte[0]=131 - byte[14]=(0 or 1)
      byte communication - byte[0]=142
    */
  {
    String serialstr = Serial.readString();
    if (serialstr.indexOf("getpower") >= 0)
    {
      Serial.println("Fetching Energy Consumption on that Device...");
      memset(__packet__, 0, 32);
      __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 128;
      __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = TRANSCEIVER_REQUEST_GETPOWERCONSUMPTION;
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("getrelay") >= 0)
    {
      Serial.println("Getting that Device's Relay State...");
      memset(__packet__, 0, 32);
      __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 128;
      __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = TRANSCEIVER_REQUEST_GETRELAYSTATE;
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("getip") >= 0)
    {
      Serial.println("Getting that Device's IP Address...");
      memset(__packet__, 0, 32);
      __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 144;
      __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = TRANSCEIVER_REQUEST_GETIPADDRESS;
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("pingpong") >= 0)
    {
      Serial.println("Initiating Ping-Pong Communication...");
      memset(__packet__, 0, 32);
      __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 128 + __target_devicetype__;
      __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = TRANSCEIVER_REQUEST_PINGPONG;
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("offrelay") >= 0)
    {
      Serial.println("Turning OFF that Device's Relay...");
      memset(__packet__, 0, 32);
      __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 131;
      __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = 0;
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("onrelay") >= 0)
    {
      Serial.println("Turning ON that Device's Relay...");
      memset(__packet__, 0, 32);
      __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 131;
      __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = 1;
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("thatdevicerestart") >= 0)
    {
      Serial.println("Commanding that Device to Restart...");
      memset(__packet__, 0, 32);
      __packet__[TRANSCEIVER_BYTELOC_NRFTM] = 128 + __target_devicetype__;
      __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = TRANSCEIVER_REQUEST_RESTART;
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
      memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("getlisteneddevice") >= 0)
      Serial.println(String("Currently listening to Device: ") + String(deviceswitch));
    else if (serialstr.indexOf("listentodevice") >= 0)
    {
      Serial.println("Which Device you want to listen to? typed value must be within 0 - 7: ");
      while (Serial.available() <= 0)
      {
      }
      serialstr = Serial.readString();
      if (serialstr.endsWith("\r\n"))
        serialstr.remove(serialstr.length() - 2, 2);
      else if (serialstr.endsWith("\r") || serialstr.endsWith("\n"))
        serialstr.remove(serialstr.length() - 1, 1);
      __loopindex__=serialstr.toInt();
      if (bitRead(__registered_wasactive__,__loopindex__))
      {
        deviceswitch=__loopindex__;
        memcpy(__private_channel__, __registered_channels__[deviceswitch], 5); // set the listened private channel
        Serial.println(String("Now listening to device ") + String(deviceswitch));
      }
      else
        Serial.println(String("Failed to listen to device ") + String(__loopindex__) + String(" because it was not yet activated! Activate it first using command 'setdevicestate' before using this command again."));
    }
    else if (serialstr.indexOf("setdevicestate") >= 0)
    {
      Serial.println("Which Device? typed value must be within 0 - 7: ");
      while (Serial.available() <= 0)
      {
      }
      serialstr = Serial.readString();
      if (serialstr.endsWith("\r\n"))
        serialstr.remove(serialstr.length() - 2, 2);
      else if (serialstr.endsWith("\r") || serialstr.endsWith("\n"))
        serialstr.remove(serialstr.length() - 1, 1);
      __loopindex__=serialstr.toInt();
      Serial.println(__loopindex__);
      Serial.print("Enable the Device? 0 or 1: ");
      while (Serial.available() <= 0)
      {
      }
      serialstr = Serial.readString();
      if (serialstr.endsWith("\r\n"))
        serialstr.remove(serialstr.length() - 2, 2);
      else if (serialstr.endsWith("\r") || serialstr.endsWith("\n"))
        serialstr.remove(serialstr.length() - 1, 1);
      bitWrite(__registered_wasactive__,__loopindex__,serialstr.toInt());
      Serial.println(bitRead(__registered_wasactive__,__loopindex__));
    }
    else if (serialstr.indexOf("setupwifi") >= 0)
    {
      Serial.print(String("Set SSID: "));
      while (Serial.available() <= 0)
      {
      }
      __Router_SSID__ = Serial.readString();
      if (__Router_SSID__.endsWith("\r\n"))
        __Router_SSID__.remove(__Router_SSID__.length() - 2, 2);
      else if (__Router_SSID__.endsWith("\r") || __Router_SSID__.endsWith("\n"))
        __Router_SSID__.remove(__Router_SSID__.length() - 1, 1);
      Serial.println(__Router_SSID__);
      Serial.print(String("Set Password: "));
      while (Serial.available() <= 0)
      {
      }
      __Router_Password__ = Serial.readString();
      if (__Router_Password__.endsWith("\r\n"))
        __Router_Password__.remove(__Router_Password__.length() - 2, 2);
      else if (__Router_Password__.endsWith("\r") || __Router_Password__.endsWith("\n"))
        __Router_Password__.remove(__Router_Password__.length() - 1, 1);
      Serial.println(__Router_Password__);
      Serial.println("WiFi Credentials was changed successfully");
    }
    else if (serialstr.indexOf("byte") >= 0)
    {
      memset(__packet__, 0, 32);
      for (__loopindex__ = 0; __loopindex__ < 32; __loopindex__++)
      {
        if (__loopindex__ > 0 && __loopindex__ < 14)
          continue;
        Serial.print(String("Set Byte ") + String(__loopindex__) + String(": "));
        while (Serial.available() <= 0)
        {
        }
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
      transmitpacket(__packet__);
    }
    else if (serialstr.indexOf("string") >= 0)
    {
      Serial.println(String("Set String Message: "));
      memset(__packet__, 0, 32);
      while (Serial.available() <= 0)
      {
      }
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
        Serial.print((char) __packet__[14 + __loopindex__]);
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
      transmitpacket(__packet__);
    }
  }
  else if (radio.available())
  {
    radio.read(& __packet__, 32);
    byte publictrashbin[TRANSCEIVER_BYTECOUNT_TRASHBIN];
    memcpy(publictrashbin, __packet__ + TRANSCEIVER_BYTELOC_TRASHBIN, TRANSCEIVER_BYTECOUNT_TRASHBIN);
    if (wasintrashbin(publictrashbin))
    {
      Serial.println(String("Fetched Message Denied because of Trashbin Value: ")
                     + String(__packet__[TRANSCEIVER_BYTELOC_TRASHBIN]) + String("  ")
                     + String(__packet__[TRANSCEIVER_BYTELOC_TRASHBIN + 1]));
      return;
    }
    printpacketdetails(__packet__);
    switch (datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_COMMAND, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND))
    {
      case TRANSCEIVER_COMMAND_UNIVERSAL_REQUEST:
        {
          if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] != 0)
          {
            datatypewritebits(& __packet__[TRANSCEIVER_BYTELOC_NRFTM], datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT), TRANSCEIVER_NRFTM_BITCOUNT_RDT, TRANSCEIVER_NRFTM_BITLOC_RDT); // become the receiver device type on our transmission
            datatypewritebits(& __packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_DEVICETYPE_HUB, TRANSCEIVER_NRFTM_BITCOUNT_TDT, TRANSCEIVER_NRFTM_BITLOC_TDT);
            memcpy(__private_channel__, __packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK); // renew global variable
            __target_devicetype__ = datatypereadbits(__packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_NRFTM_BITLOC_TDT, TRANSCEIVER_NRFTM_BITCOUNT_TDT); // renew global variable
            memcpy(__packet__ + TRANSCEIVER_BYTELOC_RECEIVERSK, __private_channel__, TRANSCEIVER_BYTECOUNT_RECEIVERSK);
            memcpy(__packet__ + TRANSCEIVER_BYTELOC_TRANSMITTERSK, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
            memset(__packet__ + TRANSCEIVER_BYTELOC_TRASHBIN, 0, TRANSCEIVER_BYTECOUNT_TRASHBIN);
            if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] == TRANSCEIVER_REQUEST_GETPUBLICCHANNEL)
            {
              datatypewritebits(& __packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_COMMAND_UNIVERSAL_PUBLICCHANNELINFO, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND, TRANSCEIVER_NRFTM_BITLOC_COMMAND);
              memset(__packet__ + TRANSCEIVER_BYTELOC_PACKETINFO, 0, TRANSCEIVER_BYTECOUNT_PACKETINFO);
              memcpy(__packet__ + TRANSCEIVER_BYTELOC_INFORMATION, __public_channel__, TRANSCEIVER_BYTECOUNT_TRANSMITTERSK);
              transmitpacket(__packet__);
            }
            else if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] == TRANSCEIVER_REQUEST_GETWIFICREDENTIALS || __packet__[TRANSCEIVER_BYTELOC_INFORMATION] == TRANSCEIVER_REQUEST_GETWIFILOSTPACKET)
            {
              datatypewritebits(& __packet__[TRANSCEIVER_BYTELOC_NRFTM], TRANSCEIVER_COMMAND_IPCAMERA_WIFICREDENTIALS, TRANSCEIVER_NRFTM_BITCOUNT_COMMAND, TRANSCEIVER_NRFTM_BITLOC_COMMAND);
              unsigned long packetwassent[2] =
              {
                0
              };
              word strinfolength[2];
              byte packet_count[2];
              if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION] == TRANSCEIVER_REQUEST_GETWIFILOSTPACKET)
              {
                for (__loopindex__ = 1; __loopindex__ <= 4; __loopindex__++)
                {
                  packetwassent[0] += (__packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__]) << (8 * (__loopindex__ - 1));
                  packetwassent[1] += (__packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__ + 4]) << (8 * (__loopindex__ - 1));
                }
              }
              Serial.println(packetwassent[0]);
              Serial.println(packetwassent[1]);
              strinfolength[0] = __Router_SSID__.length();
              if (strinfolength[0] > TRANSCEIVER_BYTECOUNT_WORDINFO * TRANSCEIVER_MAXPACKETSCOUNT)
              {
                strinfolength[0] = TRANSCEIVER_BYTECOUNT_WORDINFO * TRANSCEIVER_MAXPACKETSCOUNT;
                packet_count[0] = TRANSCEIVER_MAXPACKETSCOUNT;
                Serial.println("WARNING!SSID was too tong! the system can only send 31 packets and that is 31*17=527 characters MAX!");
              }
              else
                packet_count[0] = ceil(strinfolength[0] / (float) TRANSCEIVER_BYTECOUNT_WORDINFO);
              strinfolength[1] = __Router_Password__.length();
              if (strinfolength[1] > TRANSCEIVER_BYTECOUNT_WORDINFO * TRANSCEIVER_MAXPACKETSCOUNT)
              {
                strinfolength[1] = TRANSCEIVER_BYTECOUNT_WORDINFO * TRANSCEIVER_MAXPACKETSCOUNT;
                packet_count[1] = TRANSCEIVER_MAXPACKETSCOUNT;
                Serial.println("WARNING!Password was too tong! the system can only send 31 packets and that is 31*17=527 characters MAX!");
              }
              else
                packet_count[1] = ceil(strinfolength[1] / (float) TRANSCEIVER_BYTECOUNT_WORDINFO);
              __packet__[TRANSCEIVER_BYTELOC_PACKETINFO] = (packet_count[0] >> 2) + (packet_count[1] << 3);
              __packet__[TRANSCEIVER_BYTELOC_INFORMATION] = (packet_count[0] << 6);
              // multi-packet handling system
              for (byte i = 0; i <= 1; i++)
                // 0=ssid,1=password
              {
                Serial.print(String("\nTransmitting ") + String(i == 0 ? "SSID:" : "Password:"));
                bitWrite(__packet__[TRANSCEIVER_BYTELOC_INFORMATION], 0, i); // indicates that the info was an ssid(0) or password(1) packet
                byte lengthfinal = strinfolength[i] % TRANSCEIVER_BYTECOUNT_WORDINFO;
                if (lengthfinal == 0)
                  lengthfinal = TRANSCEIVER_BYTECOUNT_WORDINFO;
                for (byte index = 0; index < packet_count[i]; index++)
                {
                  if (bitRead(packetwassent[i], index))
                    continue; // do not try to send this data if this packet was already sent
                  datatypewritebits(& __packet__[TRANSCEIVER_BYTELOC_INFORMATION], index, 5, 1); // write the current packet
                  memset(__packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, 0, TRANSCEIVER_BYTECOUNT_WORDINFO);
                  // memset(__packet__ + TRANSCEIVER_BYTELOC_INFORMATION+1,255,TRANSCEIVER_BYTECOUNT_WORDINFO); // fill the array with HOLES(255), this is an indicator that if this was received at the receiver, the packet was LOST
                  byte packetlength;
                  if (index + 1 < packet_count[i])
                    packetlength = TRANSCEIVER_BYTECOUNT_WORDINFO;
                  else
                  {
                    packetlength = lengthfinal;
                    // memset(__packet__ + TRANSCEIVER_BYTELOC_INFORMATION+1+lengthfinal,0,TRANSCEIVER_BYTECOUNT_WORDINFO-lengthfinal); // fill the end of the packet with a TERMINATOR mark(0), this is the indicator that here ends the information
                  }
                  if (i == 0)
                    memcpy(__packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, __Router_SSID__.c_str() + (TRANSCEIVER_BYTECOUNT_WORDINFO * index), packetlength);
                  else
                    memcpy(__packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1, __Router_Password__.c_str() + (TRANSCEIVER_BYTECOUNT_WORDINFO * index), packetlength);
                  Serial.print((char *) __packet__ + TRANSCEIVER_BYTELOC_INFORMATION + 1);
                  transmitpacket(__packet__, true); // transmit with long autoretry timeout of 10 seconds
                }
              }
              Serial.print("\nTransmission Complete\n");
              //
            }
          }
          break;
        }
      case TRANSCEIVER_REQUEST_GETRELAYSTATEANDPOWERCONSUMPTION:
        Serial.print("Relay State: ");
        if (__packet__[TRANSCEIVER_BYTELOC_INFORMATION])
          Serial.println("HIGH");
        else
          Serial.println("LOW");
        Serial.print("Total Wh since last fetch:");
        for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
          Serial.print((char) __packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__+1]);
        Serial.println("");
        break;
      case TRANSCEIVER_COMMAND_SMARTSOCKET_POWERCONSUMPTION:
        Serial.print("Total Wh since last fetch:");
        for (__loopindex__ = 0; __loopindex__ < TRANSCEIVER_BYTECOUNT_INFORMATION; __loopindex__++)
          Serial.print((char) __packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__]);
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
        {
          for (__loopindex__ = 0; __loopindex__ < 4; __loopindex__++)
            staticip[__loopindex__] = __packet__[TRANSCEIVER_BYTELOC_INFORMATION + __loopindex__];
          Serial.println(String("IP Address: ") + String(staticip[0]) + '.' + String(staticip[1]) + '.' + String(staticip[2]) + '.' + String(staticip[3]));
          break;
        }
      case TRANSCEIVER_COMMAND_UNIVERSAL_PINGPONG:
        Serial.println("The Device was Reachable!");
        break;
      default:
        break;
    }
  }
}
