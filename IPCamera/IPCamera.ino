#include <WiFi.h>
#include "WebSocketsServer.h"

//#include <NTPClient.h>
//#include <WiFiUdp.h>

#include <esp_camera.h>
#include "camera_pins.h"

#define WiFi_TryConnect_TimeOut 20000.0 // ilang  milliseconds maghihintay ang esp32 sa pag connect
#define builtin_serialkey "Qa!e6" // eto yung UNIQUE Serial Key ng Device nato
#define builtin_AP_SSID "IP Camera: Qa!e6" // eto yung SSID ng WiFi na gagawin ng device nato
#define builtin_AP_Password "Ba0!*sL8" // eto yung password ng WiFi na gagawin ng device nato

#define Port_WebSocket 80 // set the websocket listen port to port 80(do not change this port because you will need to modify my html found at "WebSocketsServer.h" if you want to...)

#define standby_timer 500

#define flashlight 4
#define clientindicator_LED 12

/* WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 3600e3);
unsigned long timesincelastupdate = 0; */

IPAddress staticip;

sensor_t * camerasensor;
camera_config_t cameraconfig;

word customtimer = 0;
bool wasSTA = false;

String ssid = "Manalansan";
String password = "Manalansan@123!";

WebSocketsServer webSocket = WebSocketsServer(Port_WebSocket, "/ws"); //initialize websocket

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

connecttowifi:
  WiFi.begin(ssid.c_str(), password.c_str());
  customtimer = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if ( (WiFi.status() == WL_CONNECT_FAILED) || (millis() - customtimer > WiFi_TryConnect_TimeOut) )
    { // create an access point to view the ip camera
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
      String dynamicstring = Serial.readString();
      Serial.print(dynamicstring);
      if (dynamicstring.indexOf("WiFi Setup") >= 0)
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
  }
  if (WiFi.status() == WL_CONNECTED )// kapag nakaconnect tayo sa ibang wifi, kelangan nakaconfigure ang static IP address
  {
    Serial.print(String("\nWiFi connected using:\nSSID:") + ssid + String("\nPassword:") + password + String("\n"));
    staticip = WiFi.localIP(); // get the current ip of this device assigned by the router
    staticip[3] = 184; // set the fourth octet to 184
    if (!WiFi.config(staticip, WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(8, 8, 8, 8), IPAddress(1, 1, 1, 1))) { // set the static ip address
      Serial.println("STA Failed to configure");
    }
    wasSTA = true;
  }
  else
    staticip = WiFi.softAPIP(); // get the current ip of this device assigned by the router

  /* timeClient.begin(); // Initialize a NTPClient to get time
  timeClient.update();
  Serial.println(timeClient.getHours());
  Serial.println(timeClient.getFormattedTime()); */

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Camera Ready! type this URL to access the Video Stream:");
  Serial.print(staticip);
  Serial.println(String(":") + Port_WebSocket);

  pinMode(clientindicator_LED, OUTPUT);
  pinMode(flashlight, OUTPUT);

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
    Serial.println("Camera init failed!");
    return;
  }

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

  Serial.println(String("framesize:") + String(camerasensor->status.framesize));
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
  Serial.println(String("colorbar:") + String(camerasensor->status.colorbar));

  //initial sensors are flipped vertically and colors are a bit saturated
  if (camerasensor->id.PID == OV3660_PID) {
    camerasensor->set_vflip(camerasensor, 1);//flip it back
    camerasensor->set_brightness(camerasensor, 1);//up the blightness just a bit
    camerasensor->set_saturation(camerasensor, -2);//lower the saturation
  }

  //Serial.println(String("public ip:") + getIp());
}

word fps;
word fpstimer=millis();
void loop()
{
  if (WiFi.status() != WL_CONNECTED && wasSTA == true)
  {
    gostandbymode();
    ESP.restart();
  }

  webSocket.loop();
  byte clientscount = webSocket.connectedClients(false);
  if (clientscount <= 0)
  {
    gostandbymode();
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

  webSocket.broadcastBIN((const uint8_t*) fb->buf, fb->len);
  esp_camera_fb_return(fb);

  fps=1000/(millis()-fpstimer);
  fpstimer=millis();
  if ((clientscount > 0 || digitalRead(clientindicator_LED) == HIGH) && millis() - customtimer > 1000 / clientscount)
  {
    Serial.println(String("Connected Clients:") + String(clientscount));
    Serial.println(String(fps)+String("fps"));

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

    if (digitalRead(clientindicator_LED) != HIGH)
    {
      digitalWrite(clientindicator_LED, HIGH);
      Serial.println("HIGH");
    }
    else
    {
      digitalWrite(clientindicator_LED, LOW);
      Serial.println("LOW");
    }
    customtimer = millis();
  }
}

void gostandbymode()
{
  if (digitalRead(flashlight) == HIGH)
    digitalWrite(flashlight, LOW);
  if (digitalRead(clientindicator_LED) == HIGH)
    digitalWrite(clientindicator_LED, LOW);
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
  switch (type)
  {
    case WStype_TEXT:
      if (payload[0] == '#')
      {
        Serial.println(String("Flashlight State: ") + String(payload[1]));
        digitalWrite(flashlight, payload[1] - 48); // ascii of 0 is 48... and ascii of 1 is 49
      }
      else if (payload[0] == '@')
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
      }
      else if (payload[0] == '!')
      {
        byte chosenframesize = atoi((char*)&payload[1]);
        byte savedsettings;


        bitWrite(savedsettings, 0, camerasensor->status.vflip);
        //bitWrite(savedsettings,1,camerasensor->status.hmirror);

        esp_camera_deinit();

        cameraconfig.frame_size = (framesize_t)chosenframesize;

        /* if (chosenframesize > 6)
          cameraconfig.jpeg_quality = 16-chosenframesize;
        else
          cameraconfig.jpeg_quality = 10; */
        cameraconfig.jpeg_quality = chosenframesize;

        // camera init
        if (esp_camera_init(&cameraconfig) != ESP_OK) {
          Serial.println("Camera init failed!");
          return;
        }

        camerasensor->set_vflip(camerasensor, bitRead(savedsettings, 0));
        camerasensor->set_hmirror(camerasensor, bitRead(savedsettings, 0));
        
        /* if (chosenframesize > 6)
          camerasensor->set_quality(camerasensor,16-chosenframesize);
        else
          camerasensor->set_quality(camerasensor, 10); */
        camerasensor->set_quality(camerasensor,chosenframesize);

        camerasensor->set_framesize(camerasensor, (framesize_t)chosenframesize);
        Serial.println(String("framesize:") + String(camerasensor->status.framesize));
        Serial.println(String("quality:") + String(camerasensor->status.quality));
      }
      break;
    case WStype_CONNECTED:
      Serial.println(String("A Client was Connected with Number: ") + String(num));
      break;
    case WStype_DISCONNECTED:
      Serial.println(String("A Client was Disconnected with Number: ") + String(num));
      break;
    default:
      break;
  }
}
