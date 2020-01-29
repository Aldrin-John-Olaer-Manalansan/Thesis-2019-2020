//WiFi Related Libraries
#include <WiFi.h>
#include "WebSocketsServer.h"
#include <ESPmDNS.h>

//Bluetooth Related Libraries
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

//Servo Library
#include "ThesisServo.h"

//Time Library
//#include <NTPClient.h>
//#include <WiFiUdp.h>

//Camera Related Libraries
#include <esp_camera.h>

#define WiFi_TryConnect_TimeOut 20000 // ilang  milliseconds maghihintay ang esp32 sa pag connect

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define IPCAMERA_SERVICEUUID                         "eca9c05a-1250-41a6-9869-b455f8c96f80"
#define IPCAMERA_CHARACTERISTICSUUID_WIFICREDENTIALS "769c9614-8180-4722-9ce5-55a03b30fb5a"

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
//#define builtin_serialkey "mWQa!e6IqCISo%3" // eto yung UNIQUE Serial Key ng Device nato
//#define builtin_length_serialkey 15 // length ng serial key sa taas
#define builtin_serialkey "z8fqa2j6" // eto yung UNIQUE Serial Key ng Device nato
#define builtin_length_serialkey 8 // length ng serial key sa taas

#define builtin_AP_SSID "IP Camera: z8fqa2j6" // eto yung SSID ng WiFi na gagawin ng device nato
#define builtin_AP_Password "Ba0!*sL8" // eto yung password ng WiFi na gagawin ng device nato

#define Port_WebSocketServer 80 // set the websocket listen port to port 80(do not change this port because you will need to modify my html found at "WebSocketsServer.h" if you want to...)

#define standby_timer 500

#define flashlight_ledc_freq 5000
#define flashlight_ledc_channel 11
#define flashlight_ledc_resolution 16
#define flashlight 4

#define PIN_servo_X 12
#define PIN_servo_Y 13
#define ServoX_Channel 7
#define ServoY_Channel 9
#define ServoX_defaultAngle 90 // 0-180
#define ServoY_defaultAngle 90 // 0-180

//#define PIN_clientindicator_LED 12
#define PIN_clientindicator_RED 15 // 2
#define PIN_clientindicator_GREEN 14
#define PIN_clientindicator_BLUE 2

#define ledhasclient digitalWrite(PIN_clientindicator_RED,HIGH);digitalWrite(PIN_clientindicator_GREEN,HIGH);digitalWrite(PIN_clientindicator_BLUE,LOW);
#define ledwithsignal digitalWrite(PIN_clientindicator_RED,HIGH);digitalWrite(PIN_clientindicator_GREEN,LOW);digitalWrite(PIN_clientindicator_BLUE,HIGH);
#define ledbusy digitalWrite(PIN_clientindicator_RED,LOW);digitalWrite(PIN_clientindicator_GREEN,HIGH);digitalWrite(PIN_clientindicator_BLUE,HIGH);

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

Servo ServoX;  // create the X-Axis servo object to control a servo
Servo ServoY;  // create the Y-Axis servo object to control a servo

/* WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 3600e3);
  unsigned long timesincelastupdate = 0; */

IPAddress staticip;

sensor_t * camerasensor;
camera_config_t cameraconfig;

word customtimer = 0;
bool wasSTA = false;
bool camerainitialized = false;
// wifi_power_t defaulttxpower=WiFi.getTxPower();

//String ssid = "Manalansan";
//String password = "Manalansan@123!";
String ssid = "Jalibee";
String password = "3.1415926536";
//String ssid = "SKYWORTH-C29A";
//String password="176684990";
//String ssid, password; // empty for demonstration of ssid fetching via bluetooth

WebSocketsServer webSocket = WebSocketsServer(Port_WebSocketServer, "/ws"); //initialize websocket

#define dutycycle_spacing 32
void flashlightsetting(byte setting = 2)
{
  static byte dutycycle = 0;
  static byte booleanswitches = 0b00000001;

  if (setting >= 2)
  {
    if (!bitRead(booleanswitches, 1))
    {
      ledcSetup(flashlight_ledc_channel, flashlight_ledc_freq, flashlight_ledc_resolution);
      ledcAttachPin(flashlight, flashlight_ledc_channel);
      bitWrite(booleanswitches, 1, 1);
    }
    bitWrite(booleanswitches, 2, 0);

    if (dutycycle == 255)
      bitWrite(booleanswitches, 0, 0);
    else if (dutycycle == 0)
      bitWrite(booleanswitches, 0, 1);

    if (bitRead(booleanswitches, 0))
    {
      if (dutycycle + dutycycle_spacing <= 255)
        dutycycle += dutycycle_spacing;
      else
        dutycycle = 255;
    }
    else
    {
      if (dutycycle - dutycycle_spacing >= 0)
        dutycycle -= dutycycle_spacing;
      else
        dutycycle = 0;
    }

    ledcWrite(flashlight_ledc_channel, dutycycle);
  }
  else
  {
    if (bitRead(booleanswitches, 1))
    {
      ledcDetachPin(flashlight);
      ledcSetup(flashlight_ledc_channel, 0, 1);
      bitWrite(booleanswitches, 1, 0);
    }
    if (!bitRead(booleanswitches, 2))
    {
      pinMode(flashlight, OUTPUT);
      bitWrite(booleanswitches, 2, 1);
    }
    digitalWrite(flashlight, setting);
  }
}

bool initBluetooth()
{
  if (!btStart())
  {
    Serial.println("Failed to initialize controller");
    return false;
  }

  if (esp_bluedroid_init() != ESP_OK)
  {
    Serial.println("Failed to initialize bluedroid");
    return false;
  }

  if (esp_bluedroid_enable() != ESP_OK)
  {
    Serial.println("Failed to enable bluedroid");
    return false;
  }
  return true;
}

void GetThisDeviceBLEAddress(char* BLE_Address)
{
  const uint8_t* pointer = esp_bt_dev_get_address();

  for (int i = 0; i < 6; i++)
  {
    sprintf(BLE_Address + (3 * i), "%02x", (int)pointer[i]);

    if (i < 6)
      BLE_Address[(3 * i) - 1] = ':';
  }

}

TaskHandle_t Core0_Handle;
void setup()
{
  pinMode(PIN_clientindicator_RED, OUTPUT);
  pinMode(PIN_clientindicator_GREEN, OUTPUT);
  pinMode(PIN_clientindicator_BLUE, OUTPUT);
  ledbusy

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //WiFi.setTxPower(WIFI_POWER_17dBm); // weakest signal
  //Serial.println(String("Initial Power:")+String(WiFi.getTxPower()));

  //BLE SYSTEM
  char BLEAddress[(3 * 6) - 1] = {0};
  if (initBluetooth())
    GetThisDeviceBLEAddress(BLEAddress);

  Serial.println("Starting BLE work!");

  BLEDevice::init(BLEAddress);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(IPCAMERA_SERVICEUUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         IPCAMERA_CHARACTERISTICSUUID_WIFICREDENTIALS,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  String BLE_message = String(builtin_serialkey) + String(BLE_RequestCredentials_KeyWord) + String(builtin_serialkey);
  pCharacteristic->setValue(BLE_message.c_str()); // this tells the Master that I want the WiFi Credentials
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(IPCAMERA_SERVICEUUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Advertisement Initialted!");
BLEGetWiFi:
  while (ssid == "" && password == "")
  {
    flashlightsetting(); // make a dimming animation on the flashlight
    BLE_message = pCharacteristic->getValue().c_str();
    Serial.println(String("Message:") + BLE_message);
    if ( BLE_message.startsWith(builtin_serialkey) && BLE_message.endsWith(builtin_serialkey)
         && BLE_message.indexOf(BLE_SSID_KeyWord) > 0 && BLE_message.indexOf(BLE_Password_KeyWord) > 0 )
    { // to enter this block, the fetched serial must have the format of: SK SSIDKW SSID PWDKW PWD SK
      ssid = BLE_message.substring(BLE_message.indexOf(BLE_SSID_KeyWord) + BLE_SSID_Length_KeyWord, BLE_message.indexOf(BLE_Password_KeyWord));
      password = BLE_message.substring(BLE_message.indexOf(BLE_Password_KeyWord) + BLE_Password_Length_KeyWord, BLE_message.length() - builtin_length_serialkey);
      Serial.println(String("SSID:") + ssid);
      Serial.println(String("Password:") + password);
      break; // since the wifi credentials was fetched, terminate the loop
    }
    else if (BLE_message != String(builtin_serialkey) + String(BLE_RequestCredentials_KeyWord) + String(builtin_serialkey))
    {
      String BLE_message = String(builtin_serialkey) + String(BLE_RequestCredentials_KeyWord) + String(builtin_serialkey);
      pCharacteristic->setValue(BLE_message.c_str()); // this tells the Master that I want the WiFi Credentials
    }
    else
      delay(250);
  }
  pCharacteristic->setValue(""); // emptry string so that hackers will not have the time to see the credentials(ultra fast)
  //END OF BLE SYSTEM

connecttowifi:
  WiFi.begin(ssid.c_str(), password.c_str());
  customtimer = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    flashlightsetting(); // make a dimming animation on the flashlight
    delay(250);
    Serial.print(".");
    if ( (WiFi.status() == WL_CONNECT_FAILED) || (millis() - customtimer > WiFi_TryConnect_TimeOut) )
    { // create an access point to view the ip camera
      if (millis() < 120000) // if after 2 minutes and still not connected, lets create an access point
      {
        BLE_message = String(builtin_serialkey) + String(BLE_WrongCredential_KeyWord) + String(builtin_serialkey);
        pCharacteristic->setValue(BLE_message.c_str()); // this tells the Master that the given WiFi Credentials are Invalid/cant connect
        ssid = ""; password = "";
        goto BLEGetWiFi;
      }
      //////this will never be reached unless 2 minutes have passed
      WiFi.softAPConfig(IPAddress(192, 168, 1, 184), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0)); // set ip infos
      if (WiFi.softAP(builtin_AP_SSID, builtin_AP_Password) == true); // create our own wifi
      {
        if (WiFi.softAPIP() != IPAddress(192, 168, 1, 184))
          WiFi.softAPConfig(IPAddress(192, 168, 1, 184), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0)); // set ip infos
        Serial.println(String("Access Point created!"));
        Serial.println(String("SSID:") + builtin_AP_SSID);
        Serial.println(String("Password:") + builtin_AP_Password);
        break;
      }
    }
    if (Serial.available() > 0)
    {
      if ((Serial.readString()).indexOf("WiFi Setup") >= 0)
      {
        Serial.println("Type the SSID");
        while (Serial.available() <= 0) { }
        ssid = Serial.readString();
        if (ssid.endsWith("\r\n"))
          ssid.remove(ssid.length() - 2, 2);
        else if (ssid.endsWith("\r") || ssid.endsWith("\n"))
          ssid.remove(ssid.length() - 1, 1);

        Serial.println(String("SSID changed into ") + ssid);

        Serial.println("Type the Password");
        while (Serial.available() <= 0) { }
        password = Serial.readString();
        if (password.endsWith("\r\n"))
          password.remove(password.length() - 2, 2);
        else if (password.endsWith("\r") || password.endsWith("\n"))
          password.remove(password.length() - 1, 1);

        Serial.println(String("Password changed into ") + password);

        goto connecttowifi;
      }
    }
    //increasetxpower();
  }

  wasSTA = !MDNS.begin((String("ipcamera_") + String(builtin_serialkey)).c_str()); // recycle variable and change later
  //wasSTA = !MDNS.begin("thesiscam"); // recycle variable and change later
  if (!wasSTA)
  {
    MDNS.addService("http", "tcp", Port_WebSocketServer);
    Serial.println("mDNS responder started");
  }
  else
    Serial.println("Error setting up MDNS responder!");

  //increasetxpower(); // increase the power 1 time
  //WiFi.setTxPower(defaulttxpower);
  if (WiFi.status() == WL_CONNECTED )// kapag nakaconnect tayo sa ibang wifi, kelangan nakaconfigure ang static IP address
  {
    Serial.print(String("\nWiFi connected using:\nSSID:") + ssid + String("\nPassword:") + password + String("\n"));
    //if (wasSTA)
    //{
      staticip = WiFi.localIP(); // get the current ip of this device assigned by the router
      //staticip[3] = 184; // set the fourth octet to 184
      if (!WiFi.config(staticip, WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(8, 8, 8, 8), IPAddress(1, 1, 1, 1))) // set the static ip address
        Serial.println("STA Failed to configure");
    //}
    wasSTA = true; // force to become true
  }
  else
    staticip = WiFi.softAPIP(); // get the current ip of this device assigned by the router

  /* timeClient.begin(); // Initialize a NTPClient to get time
    timeClient.update();
    Serial.println(timeClient.getHours());
    Serial.println(timeClient.getFormattedTime()); */

  //BLE Fetching finished at this point... deliver the IP address to the Master
  /* customtimer = millis();
  while (millis() - customtimer <= 6000)
  {
    flashlightsetting(); // make a dimming animation on the flashlight
    String BLE_message = pCharacteristic->getValue().c_str();
    Serial.println(String("Message:") + BLE_message);
    if ( BLE_message.startsWith(builtin_serialkey) && BLE_message.endsWith(builtin_serialkey)
         && BLE_message.indexOf(BLE_YouAreFree_KeyWord) > 0)
    { // to enter this block, the fetched serial must have the format of: SK YAFKW SK
      break; // since the wifi credentials was fetched, terminate the loop
    }
    else if (BLE_message != String(builtin_serialkey) +
             String(BLE_IPAddress_KeyWord) +
             String(staticip[0]) + String(BLE_Dot_KeyWord) +
             String(staticip[1]) + String(BLE_Dot_KeyWord) +
             String(staticip[2]) + String(BLE_Dot_KeyWord) +
             String(staticip[3]) +
             String(builtin_serialkey))
    {
      BLE_message = String(builtin_serialkey)
                    + String(BLE_IPAddress_KeyWord)
                    + String(staticip[0]) + String(BLE_Dot_KeyWord)
                    + String(staticip[1]) + String(BLE_Dot_KeyWord)
                    + String(staticip[2]) + String(BLE_Dot_KeyWord)
                    + String(staticip[3])
                    + String(builtin_serialkey);
      pCharacteristic->setValue(BLE_message.c_str()); // this tells the Master my Private IP Address
    }
    else
      delay(250);
  } */
  // stop bluetooth since we do not need it anymore //this occupies almost 50MB of space
  pAdvertising->stop();
  pService->stop();
  BLEDevice::deinit(true);
  btStop();
  //
  flashlightsetting(LOW);

  webSocket.begin();

  Serial.println("Camera Ready! type this URL to access the Video Stream:");
  Serial.println(String(staticip[0])+ '.'
                +String(staticip[1])+ '.'
                +String(staticip[2])+ '.'
                +String(staticip[3])+ ':' + String(Port_WebSocketServer)
  			+ " or http://ipcamera_" + String(builtin_serialkey) + ".local");

  //pinMode(flashlight, OUTPUT);

  cameraconfig.ledc_channel = LEDC_CHANNEL_0;
  cameraconfig.ledc_timer = LEDC_TIMER_0;
  cameraconfig.pin_d0 = Y2_GPIO_NUM;
  cameraconfig.pin_d1 = Y3_GPIO_NUM;
  cameraconfig.pin_d2 = Y4_GPIO_NUM;
  cameraconfig.pin_d3 = Y5_GPIO_NUM;
  cameraconfig.pin_d4 = Y6_GPIO_NUM;
  cameraconfig.pin_d5 = Y7_GPIO_NUM;
  cameraconfig.pin_d6 = Y8_GPIO_NUM;
  cameraconfig.pin_d7 = Y9_GPIO_NUM;
  cameraconfig.pin_xclk = XCLK_GPIO_NUM;
  cameraconfig.pin_pclk = PCLK_GPIO_NUM;
  cameraconfig.pin_vsync = VSYNC_GPIO_NUM;
  cameraconfig.pin_href = HREF_GPIO_NUM;
  cameraconfig.pin_sscb_sda = SIOD_GPIO_NUM;
  cameraconfig.pin_sscb_scl = SIOC_GPIO_NUM;
  cameraconfig.pin_pwdn = PWDN_GPIO_NUM;
  cameraconfig.pin_reset = RESET_GPIO_NUM;

  //cameraconfig.xclk_freq_hz = 20000000;
  cameraconfig.xclk_freq_hz = 10000000; // accourding sa research ko ito 10MHz ang nagbibigay ng best FPS

  cameraconfig.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {

    Serial.println("PSRAM was FOUND!!!");
    cameraconfig.frame_size = FRAMESIZE_VGA;
    cameraconfig.jpeg_quality = 6;

    cameraconfig.fb_count = 2;
  } else {
    cameraconfig.frame_size = FRAMESIZE_VGA;
    cameraconfig.jpeg_quality = 12;
    cameraconfig.fb_count = 1;
  }

  // camera init
  if (esp_camera_init(&cameraconfig) != ESP_OK) {
    camerainitialized = false;
    Serial.println("Camera init failed!");
    return;
  }
  else camerainitialized = true;

  camerasensor = esp_camera_sensor_get();

  camerasensor->set_framesize(camerasensor, FRAMESIZE_VGA); //drop down frame size for higher initial frame rate
  camerasensor->set_quality(camerasensor, 6);
  ////camerasensor->set_brightness(camerasensor,0);
  ////camerasensor->set_contrast(camerasensor,0);
  ////camerasensor->set_saturation(camerasensor, 2);
  ////camerasensor->set_special_effect(camerasensor,0);
  //camerasensor->set_whitebal(camerasensor, 1);
  //camerasensor->set_awb_gain(camerasensor, 1);
  ////camerasensor->set_wb_mode(camerasensor,0);
  //camerasensor->set_exposure_ctrl(camerasensor, 1);
  //camerasensor->set_aec2(camerasensor, 1);
  ////camerasensor->set_ae_level(camerasensor,0);
  //camerasensor->set_aec_value(camerasensor, 204);
  //camerasensor->set_gain_ctrl(camerasensor, 1);
  //camerasensor->set_agc_gain(camerasensor, 5);
  ////camerasensor->set_gainceiling(camerasensor,0);
  ////camerasensor->set_bpc(camerasensor,0);
  //camerasensor->set_wpc(camerasensor, 1);
  //camerasensor->set_raw_gma(camerasensor, 1);
  //camerasensor->set_lenc(camerasensor, 1);
  ////camerasensor->set_hmirror(camerasensor,0);
  ////camerasensor->set_vflip(camerasensor,0);
  //camerasensor->set_dcw(camerasensor, 1);
  ////camerasensor->set_colorbar(camerasensor,0); // recommend not to on, only turn this on when callibrating colors
  ////camerasensor->detection_enabled=0;
  ////camerasensor->recognition_enabled=0;

  /* Serial.println(String("framesize:") + String(camerasensor->status.framesize));
  Serial.println(String("quality:") + String(camerasensor->status.quality));
  Serial.println(String("brightness:") + String(camerasensor->status.brightness));
  Serial.println(String("contrast:") + String(camerasensor->status.contrast));
  Serial.println(String("saturation:") + String(camerasensor->status.saturation));
  Serial.println(String("sharpness:") + String(camerasensor->status.sharpness));
  Serial.println(String("special_effect:") + String(camerasensor->status.special_effect));
  Serial.println(String("wb_mode:") + String(camerasensor->status.wb_mode));
  Serial.println(String("awb:") + String(camerasensor->status.awb));
  Serial.println(String("awb_gain:") + String(camerasensor->status.awb_gain));
  Serial.println(String("aec:") + String(camerasensor->status.aec));
  Serial.println(String("aec2:") + String(camerasensor->status.aec2));
  Serial.println(String("ae_level:") + String(camerasensor->status.ae_level));
  Serial.println(String("aec_value:") + String(camerasensor->status.aec_value));
  Serial.println(String("agc:") + String(camerasensor->status.agc));
  Serial.println(String("agc_gain:") + String(camerasensor->status.agc_gain));
  Serial.println(String("gainceiling:") + String(camerasensor->status.gainceiling));
  Serial.println(String("bpc:") + String(camerasensor->status.bpc));
  Serial.println(String("wpc:") + String(camerasensor->status.wpc));
  Serial.println(String("raw_gma:") + String(camerasensor->status.raw_gma));
  Serial.println(String("lenc:") + String(camerasensor->status.lenc));
  Serial.println(String("vflip:") + String(camerasensor->status.vflip));
  Serial.println(String("hmirror:") + String(camerasensor->status.hmirror));
  Serial.println(String("dcw:") + String(camerasensor->status.dcw));
  Serial.println(String("colorbar:") + String(camerasensor->status.colorbar)); */

  //initial sensors are flipped vertically and colors are a bit saturated
  if (camerasensor->id.PID == OV3660_PID) {
    camerasensor->set_vflip(camerasensor, 1);//flip it back
    camerasensor->set_brightness(camerasensor, 1);//up the blightness just a bit
    camerasensor->set_saturation(camerasensor, -2);//lower the saturation
  }

  //Serial.println(String("public ip:") + getIp());
  xTaskCreatePinnedToCore(
    Core0Cater,              /* Task function. */
    "Core0 Catering System", /* name of task. */
    10000,                    /* Stack size of task */
    NULL,                    /* parameter of the task */
    0,                       /* priority of the task */
    & Core0_Handle,          /* Task handle to keep track of created task */
    0);                      /* pin task to core 0 */
}
word fps;
word fpstimer = millis();
void loop()
{
  //Serial.println("ThisWater:"+String(uxTaskGetStackHighWaterMark(NULL)));
  //Serial.println("ThisHeap"+String(ESP.getFreeHeap()));
  if (WiFi.status() != WL_CONNECTED && wasSTA == true)
  {
    gostandbymode();
    ESP.restart();
  }

  byte clientscount = webSocket.connectedClients(false);
  if (clientscount <= 0 || !camerainitialized)
  {
    gostandbymode();
    if (millis() > 3600e3) // make esp32 restart if it has been running for one hour, to refresh its memory
      ESP.restart();
    return;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    gostandbymode();
    return;
  }

  if (fb->format != PIXFORMAT_JPEG)
  {
    Serial.println("Non-JPEG data not implemented");
    gostandbymode();
    return;
  }

  if (digitalRead(PIN_clientindicator_GREEN) == LOW || digitalRead(PIN_clientindicator_RED) == LOW)
  {
    Serial.println("hasclient");
    ledhasclient
  }

  webSocket.broadcastBIN((const uint8_t*) fb->buf, fb->len);
  esp_camera_fb_return(fb);

  fps = 1000 / (millis() - fpstimer);
  fpstimer = millis();
  if ((clientscount > 0 || digitalRead(PIN_clientindicator_BLUE) == LOW) && millis() - customtimer > 1000 / clientscount)
  {
    Serial.println(String("Connected Clients:") + String(clientscount));
    Serial.println(String(fps) + String("fps"));

    /* if (millis() - timesincelastupdate >= 3600e3)
      {
      timeClient.update();
      timesincelastupdate = millis();
      Serial.println("Time was Updated!");
      } */

    //byte currenthour = timeClient.getHours();
    //if ((currenthour < 22 && currenthour >= 6) && digitalRead(flashlight) == HIGH)
    //  digitalWrite(flashlight, LOW);
    //else if ((currenthour >= 22 || currenthour < 6) && digitalRead(flashlight) == LOW)
    //  digitalWrite(flashlight, HIGH);

    if (digitalRead(PIN_clientindicator_BLUE) != HIGH)
    {
      digitalWrite(PIN_clientindicator_BLUE, HIGH);
      Serial.println("HIGH");
    }
    else
    {
      digitalWrite(PIN_clientindicator_BLUE, LOW);
      Serial.println("LOW");
    }
    customtimer = millis();
  }
}

/* void increasetxpower()
  {
	switch(WiFi.getTxPower())
    {
    	case WIFI_POWER_MINUS_1dBm:
    		WiFi.setTxPower(WIFI_POWER_2dBm);
    		break;
    	case WIFI_POWER_2dBm:
    		WiFi.setTxPower(WIFI_POWER_5dBm);
    		break;
    	case WIFI_POWER_5dBm:
    		WiFi.setTxPower(WIFI_POWER_7dBm);
    		break;
    	case WIFI_POWER_7dBm:
    		WiFi.setTxPower(WIFI_POWER_8_5dBm);
    		break;
    	case WIFI_POWER_8_5dBm:
    		WiFi.setTxPower(WIFI_POWER_11dBm);
    		break;
    	case WIFI_POWER_11dBm:
    		WiFi.setTxPower(WIFI_POWER_13dBm);
    		break;
    	case WIFI_POWER_13dBm:
    		WiFi.setTxPower(WIFI_POWER_15dBm);
    		break;
    	case WIFI_POWER_15dBm:
    		WiFi.setTxPower(WIFI_POWER_17dBm);
    		break;
    	case WIFI_POWER_17dBm:
    		WiFi.setTxPower(WIFI_POWER_18_5dBm);
    		break;
    	case WIFI_POWER_18_5dBm:
    		WiFi.setTxPower(WIFI_POWER_19dBm);
    		break;
    	case WIFI_POWER_19dBm:
    		WiFi.setTxPower(WIFI_POWER_19_5dBm);
    		break;
    	default:
    		break;
    }
  Serial.println(String("Power Changed from ")+String(defaulttxpower)+String(" to ")+String(WiFi.getTxPower()));
  } */

void gostandbymode()
{
  if (digitalRead(flashlight) == HIGH)
    flashlightsetting(LOW);
  if (digitalRead(PIN_clientindicator_BLUE) == LOW || digitalRead(PIN_clientindicator_RED) == LOW)
  {
    Serial.println("withsignal");
    ledwithsignal
  }
  if (ServoX.read() != ServoX_defaultAngle)
    ServoX.write(ServoX_defaultAngle);
  if (ServoY.read() != ServoY_defaultAngle)
    ServoY.write(ServoY_defaultAngle);
  //delay(standby_timer);
}

/* String getIp()
  {
  WiFiClient client;
  if (client.connect("api.ipify.org", 80))
  {
    client.println("GET / HTTP/1.0");
    client.println("Host: api.ipify.org");
    client.println();
  }
  else return String();

  while (!client.available()) {
    delay(500);
  }

  String line;

  while (client.available())
    line = client.readStringUntil('\n');

  return line;
  } */

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  static unsigned long cooldown = 0;
  ledbusy
  switch (type)
  {
    case WStype_TEXT:
      {
        switch (payload[0])
        {
          case 'X':
            {
              if (millis() - cooldown > 250)
              {
                byte Servo_Axis = atoi((char*)&payload[1]);
                Serial.println("Changing X-Servo to " + String(Servo_Axis));
                ServoX.write(Servo_Axis);
              }
              break;
            }
          case 'Y':
            {
              if (millis() - cooldown > 250)
              {
                byte Servo_Axis = atoi((char*)&payload[1]);
                Serial.println("Changing Y-Servo to " + String(Servo_Axis));
                ServoY.write(Servo_Axis);
              }
              break;
            }
          case '#':
            {
              Serial.println(String("Flashlight State: ") + String(payload[1]));
              flashlightsetting(payload[1] - 48); // ascii of 0 is 48... and ascii of 1 is 49
              break;
            }
          case '@':
            {
              Serial.println(String("Flipping Camera"));
              if (camerasensor->status.vflip == 0)
              {
                camerasensor->set_vflip(camerasensor, 1);
                camerasensor->set_hmirror(camerasensor, 1);
              }
              else
              {
                camerasensor->set_vflip(camerasensor, 0);
                camerasensor->set_hmirror(camerasensor, 0);
              }
              break;
            }
          case '%':
            {
              cameraconfig.jpeg_quality = atoi((char*)&payload[1]);
              camerasensor->set_quality(camerasensor, atoi((char*)&payload[1]));
              Serial.println("Quality set to:" + String(camerasensor->status.quality));
              break;
            }
          case '!':
            {
              ServoX.detach();
              ServoY.detach();
              byte chosenframesize = atoi((char*)&payload[1]);
              byte savedsettings;

              bitWrite(savedsettings, 0, camerasensor->status.vflip);
              //bitWrite(savedsettings,1,camerasensor->status.hmirror);

              camerainitialized = false;
              esp_camera_deinit();

              cameraconfig.frame_size = (framesize_t)chosenframesize;

              /* if (chosenframesize > 6)
                cameraconfig.jpeg_quality = 16-chosenframesize;
                else
                cameraconfig.jpeg_quality = 10; */
              cameraconfig.jpeg_quality = chosenframesize;

              // camera init
              if (esp_camera_init(&cameraconfig) != ESP_OK) {
                camerainitialized = false;
                Serial.println("Camera init failed!");
                return;
              }
              else camerainitialized = true;

              camerasensor->set_vflip(camerasensor, bitRead(savedsettings, 0));
              camerasensor->set_hmirror(camerasensor, bitRead(savedsettings, 0));

              /* if (chosenframesize > 6)
                camerasensor->set_quality(camerasensor,16-chosenframesize);
                else
                camerasensor->set_quality(camerasensor, 10); */
              camerasensor->set_quality(camerasensor, chosenframesize);

              camerasensor->set_framesize(camerasensor, (framesize_t)chosenframesize);
              Serial.println(String("framesize:") + String(camerasensor->status.framesize));
              Serial.println(String("quality:") + String(camerasensor->status.quality));

              ServoX.attach(PIN_servo_X, ServoX_Channel); // attaches the servo on pin 18 to the servo object
              ServoY.attach(PIN_servo_Y, ServoY_Channel); // attaches the servo on pin 18 to the servo object

              break;
            }
          case '$':
            {
              ESP.restart();
              break;
            }
          default:
            break;
        }
        break;
      }
    case WStype_CONNECTED:
      Serial.println(String("A Client was Connected with Number: ") + String(num));
      break;
    case WStype_DISCONNECTED:
      Serial.println(String("A Client was Disconnected with Number: ") + String(num));
      break;
    default:
      break;
  }
  //ledhasclient
}

void Core0Cater(void * pvParameters)
{
  // setup
  ServoX.attach(PIN_servo_X, ServoX_Channel); // attaches the servo on pin 18 to the servo object
  ServoY.attach(PIN_servo_Y, ServoY_Channel); // attaches the servo on pin 18 to the servo object
  ServoX.write(ServoX_defaultAngle);
  ServoY.write(ServoY_defaultAngle);
  webSocket.onEvent(webSocketEvent);
  //
  for (;;)
  {
    webSocket.loop();
    vTaskDelay(1 / portTICK_PERIOD_MS); // cater other task pinned at core 0
  }
}
