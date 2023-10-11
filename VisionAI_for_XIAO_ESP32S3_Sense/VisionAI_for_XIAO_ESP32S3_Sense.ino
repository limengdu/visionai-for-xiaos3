#include "esp_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"
#include <TJpg_Decoder.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define GFXFF 1
#define FSB9 &FreeSerifBold9pt7b

TFT_eSPI tft = TFT_eSPI();

const char* ssid = "mengdu-H68K";
const char* password = "15935700";
const unsigned long timeout = 30000; // 30 seconds

//const int buttonPin = 4;    // the number of the pushbutton pin
//int buttonState;             
//int lastButtonState = LOW;   
//unsigned long lastDebounceTime = 0;  // 最后一次拨动输出引脚的时间
//unsigned long debounceDelay = 50;    // 消抖时间；如果输出闪烁，则增加消抖时间
//bool isNormalMode = true;


bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // 图像偏离屏幕底部，停止进一步解码
  if ( y >= tft.height() ) return 0;

  // 该功能将在 TFT 边界自动剪切图像块渲染
  tft.pushImage(x, y, w, h, bitmap);

  // 如果将草图调整为使用 Adafruit_GFX 库，这个方法也许可以替代它
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}



void setup() {
  Serial.begin(115200);
//  delay(1000);
  while(!Serial);
  
  Serial.println();
//  pinMode(buttonPin, INPUT);

  Serial.println("INIT DISPLAY");
  tft.begin();
  tft.setRotation(1);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(TFT_YELLOW);
  tft.setFreeFont(FSB9);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  
  Serial.println("INIT CAMERA");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }


  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

bool wifiConnect(){
  unsigned long startingTime = millis();
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    if((millis() - startingTime) > timeout){
      return false;
    }
  }
  return true;
}

//void timerIsr(){
//  
//  int reading = digitalRead(buttonPin);
//  if (reading != lastButtonState) {
//    lastDebounceTime = millis();
//  }
//
//  if ((millis() - lastDebounceTime) > debounceDelay) {
//    if (reading != buttonState) {
//      buttonState = reading;
//      
//      if (buttonState == HIGH) {
//        isNormalMode = !isNormalMode;
//
//        //Additional Code 
//        if(!isNormalMode)
//          sendingImage();
//        //   
//      }
//    }
//  }
//  lastButtonState = reading;
//}

camera_fb_t* capture(){
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  return fb;
}

void showingImage(){
  camera_fb_t *fb = capture();
  if(!fb || fb->format != PIXFORMAT_JPEG){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }else{
    TJpgDec.drawJpg(0,0,(const uint8_t*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}


void parsingResult(String response){
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response);
  JsonArray array = doc.as<JsonArray>();
  int yPos = 4;
  for(JsonVariant v : array){
    JsonObject object = v.as<JsonObject>();
    const char* description = object["description"];
    float score = object["score"];
    String label = "";
    label += description;
    label += ":";
    label += score;

    Serial.println(label);
    tft.drawString(label, 8, yPos, GFXFF);
    yPos += 16;
  }
}

void postingImage(camera_fb_t *fb){
  HTTPClient client;
  client.begin("http://192.168.1.208:8888/imageUpdate");
  client.addHeader("Content-Type", "image/jpeg");
  int httpResponseCode = client.POST(fb->buf, fb->len);
  if(httpResponseCode == 200){
    String response = client.getString();
    parsingResult(response);
  }else{
    //Error
    tft.drawString("Check Your Server!!!", 8, 4, GFXFF);
  }

  client.end();
  WiFi.disconnect();
}

void sendingImage(){
  camera_fb_t *fb = capture();
  if(!fb || fb->format != PIXFORMAT_JPEG){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }else{
    TJpgDec.drawJpg(0,0,(const uint8_t*)fb->buf, fb->len);

    tft.drawString("Wifi Connecting!", 8, 4, GFXFF);

    if(wifiConnect()){
      //tft.drawString("Wifi Connected!", 8, 4, GFXFF);
      TJpgDec.drawJpg(0,0,(const uint8_t*)fb->buf, fb->len);
      postingImage(fb);
    }else{
      tft.drawString("Check Wifi credential!", 8, 4, GFXFF);
    }
    esp_camera_fb_return(fb);
  }
}

unsigned long lastTime = 0;  // 记录上次调用sendingImage的时间
unsigned long interval = 5000;  // 函数调用的间隔（毫秒）

void loop() {
//  buttonEvent();
  
//  if(isNormalMode)
  unsigned long currentTime = millis();
  // 检查是否已经过了足够的时间
  if (currentTime - lastTime >= interval) {
    // 调用sendingImage函数
    sendingImage();

    // 更新上次调用函数的时间
    lastTime = currentTime;
  }

  showingImage();
  
}
