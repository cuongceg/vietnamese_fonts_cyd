#include <WiFiManager.h> 
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <FontMaker.h>

using namespace websockets;

TFT_eSPI tft = TFT_eSPI();
WebsocketsClient client;

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS
// Color of text 
#define BLACK  0x0000
#define WHITE  0xFFFF
#define RED    0x07FF

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
// #define FONT_SIZE 3

// // Slider UI
// #define SLIDER_X 40
// #define SLIDER_Y 200
// #define SLIDER_W 240
// #define SLIDER_H 20
// #define SLIDER_THUMB_W 10

// int x,y,z;
// int scrollSpeed = 50; // Độ trễ giữa các lần di chuyển chữ
// int textX = SCREEN_WIDTH; // Vị trí x bắt đầu
String marqueeText = "Xin chào đến lớp học"; // Dòng chữ chạy
// int sliderValue = 50; // 0-100
// int minSpeed = 10; // nhỏ hơn thì chạy nhanh hơn
// int maxSpeed = 150;
bool isWsConnected = false;
char websocket_server[40] = "ws://171.244.142.74:8080";

// void drawSlider(int value) {
//   // Vẽ thanh trượt nền
//   tft.fillRect(SLIDER_X, SLIDER_Y, SLIDER_W, SLIDER_H, TFT_WHITE);
//   // Vẽ thumb theo giá trị
//   int thumbX = map(value, 0, 100, SLIDER_X, SLIDER_X + SLIDER_W - SLIDER_THUMB_W);
//   tft.fillRect(thumbX, SLIDER_Y, SLIDER_THUMB_W, SLIDER_H, TFT_BLUE);
// }

// update scroll speed
// void updateScrollSpeedFromSlider() {
//   scrollSpeed = map(sliderValue, 0, 100, minSpeed, maxSpeed);
// }

void onMessageCallback(WebsocketsMessage message) {
  String payload = message.data();
  Serial.println("New Message: " + payload);

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.println("JSON parse failed!");
    return;
  }

  if (!doc.containsKey("content")) {
    Serial.println("No content field!");
    return;
  }

  // take content
  String contentStr = doc["content"].as<String>();
  DynamicJsonDocument contentDoc(2048);
  DeserializationError err2 = deserializeJson(contentDoc, contentStr);

  if (err2) {
    Serial.println("Inner JSON parse failed!");
    return;
  }

  // Combine text
  String combinedText = "";
  for (JsonObject obj : contentDoc.as<JsonArray>()) {
    if (obj["insert"].is<String>()) {
      combinedText += obj["insert"].as<String>();
    }
  }

  marqueeText = combinedText;
  drawWebSocketText();
//  textX = SCREEN_WIDTH;
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("WebSocket Connected");
    isWsConnected = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("WebSocket Disconnected");
    isWsConnected = false;
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Ping received");
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Pong received");
  }
}

void setpx(int16_t x,int16_t y,uint16_t color)
{
  tft.drawPixel(x,y,color);
}
MakeFont myfont(&setpx);

void drawWebSocketText(){
  tft.fillRect(0, 50, SCREEN_WIDTH,SCREEN_HEIGHT-50, TFT_BLACK);

  if (marqueeText.length() > 0) {
    myfont.print(10,50,marqueeText.c_str(),WHITE,BLACK);
  } else {
    myfont.print(10,50,"Chờ đợi dữ liệu bài giảng...",WHITE,BLACK);
  }
}
//update running words
// void updateMarqueeText() {
//   // Tính chiều rộng dòng chữ
//   int textWidth = tft.textWidth(marqueeText, 2);

//   // Xóa vùng cũ bằng cách xóa đúng chiều rộng cần thiết
//   tft.fillRect(0, 20, SCREEN_WIDTH, tft.fontHeight(2), TFT_BLACK);

//   // Vẽ chữ mới
//   tft.setTextSize(2);
//   tft.setTextColor(TFT_WHITE, TFT_BLACK);
//   tft.setCursor(textX, 20);
//   tft.print(marqueeText);

//   delay(scrollSpeed);

//   // Di chuyển chữ
//   textX -= 2;
//   if (textX < -textWidth) {
//     textX = SCREEN_WIDTH;
//   }
// }

void setup() {
  Serial.begin(115200);
  WiFiManager wm;
  //wm.resetSettings();
  bool res = wm.autoConnect("AutoConnectAP","12345678");

  //Wait for WiFi to connect to AP
  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else {
    Serial.println("connected...yeey :)");
  }
  Serial.print("📡 WebSocket Server: ");
  Serial.println(websocket_server);
  
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  myfont.set_font(MakeFont_Font1);
  myfont.print(0,30,"Xin chào các bạn !",RED,BLACK);
  drawWebSocketText();
  //drawSlider(sliderValue);
  
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);
  client.connect(websocket_server);
}

void loop() {
    client.poll();
//   //Cập nhật dòng chữ chạy
//  updateMarqueeText();

//   if (touchscreen.tirqTouched() && touchscreen.touched()) {
//     TS_Point p = touchscreen.getPoint();
//     x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
//     y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
//     z = p.z;

//     // printTouchToSerial(x, y, z);

//     // SLIDER LOGIC
//     if (y > SLIDER_Y && y < SLIDER_Y + SLIDER_H && x > SLIDER_X && x < SLIDER_X + SLIDER_W) {
//       sliderValue = map(x, SLIDER_X, SLIDER_X + SLIDER_W, 0, 100);
//       drawSlider(sliderValue);
//       updateScrollSpeedFromSlider();
//     }
//   }

  if (!isWsConnected) {
    Serial.println("Not connected, retrying...");
    client.connect("ws://171.244.142.74:8080");  // optional reconnect logic
    delay(2000);
  }
}

