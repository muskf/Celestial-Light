#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>

// WiFi配置
const char* ssid = "Wifi Name";
const char* password = "password";
const char* apiUrl = "http://lunarclient.top/api-next/analysis/now";

// WS2812配置 - 16个灯珠
#define LED_PIN D4
#define LED_COUNT 16
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  Serial.println("\n\nStarting System...");
  Serial.println("\n\nMade by ggken");

  // 初始化灯带
  strip.begin();
  strip.setBrightness(255);  //最大亮度

  // 1. 灯带硬件测试
  testLEDs();

  // 2. 连接WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");

    // 连接状态指示灯（蓝色呼吸效果）
    static uint8_t brightness = 0;
    static bool ascending = true;
    strip.fill(strip.Color(0, 0, brightness));
    strip.show();

    if (ascending) {
      brightness += 10;
      if (brightness >= 200) ascending = false;
    } else {
      brightness -= 10;
      if (brightness <= 10) ascending = true;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nConnected! IP: ");
    Serial.println(WiFi.localIP());

    // 连接成功提示 - 绿色闪烁
    for (int i = 0; i < 5; i++) {
      strip.fill(strip.Color(0, 255, 0));
      strip.show();
      delay(150);
      strip.clear();
      strip.show();
      delay(150);
    }
  } else {
    Serial.println("\nWiFi Connection Failed");
    // 红色闪烁表示失败
    for (int i = 0; i < 10; i++) {
      strip.fill(strip.Color(255, 0, 0));
      strip.show();
      delay(150);
      strip.clear();
      strip.show();
      delay(150);
    }
    ESP.restart();
  }
}

void loop() {
  static unsigned long lastUpdate = 0;
  const long interval = 30000;  // 30秒更新一次

  if (millis() - lastUpdate >= interval) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Fetching online count...");
      int onlineCount = getOnlineCount();

      if (onlineCount >= 0) {
        Serial.print("Online count: ");
        Serial.println(onlineCount);
        updateLights(onlineCount);
      } else {
        // 获取失败时橙色闪烁
        errorFlash(strip.Color(255, 165, 0));  // 橙色
      }
    } else {
      Serial.println("WiFi disconnected");
      // 黄色闪烁
      errorFlash(strip.Color(255, 255, 0));  // 黄色

      // 尝试重新连接
      WiFi.reconnect();
      delay(5000);
      if (WiFi.status() != WL_CONNECTED) {
        ESP.restart();
      }
    }
    lastUpdate = millis();
  }

  delay(100);
}

// 灯带硬件测试函数
void testLEDs() {
  Serial.println("Starting LED Hardware Test...");

  // 测试所有灯珠 - 红色
  Serial.println("Testing RED");
  strip.fill(strip.Color(255, 0, 0));
  strip.show();
  delay(2000);

  // 测试所有灯珠 - 绿色
  Serial.println("Testing GREEN");
  strip.fill(strip.Color(0, 255, 0));
  strip.show();
  delay(2000);

  // 测试所有灯珠 - 蓝色
  Serial.println("Testing BLUE");
  strip.fill(strip.Color(0, 0, 255));
  strip.show();
  delay(2000);

  // 测试所有灯珠 - 白色
  Serial.println("Testing WHITE");
  strip.fill(strip.Color(255, 255, 255));
  strip.show();
  delay(2000);

  // 逐个点亮测试
  Serial.println("Testing Individual LEDs");
  for (int i = 0; i < LED_COUNT; i++) {
    strip.clear();
    strip.setPixelColor(i, strip.Color(255, 255, 255));
    strip.show();
    Serial.print("LED ");
    Serial.println(i);
    delay(300);
  }

  strip.clear();
  strip.show();
  Serial.println("LED Hardware Test Complete");
}

int getOnlineCount() {
  WiFiClient client;
  HTTPClient http;

  Serial.print("Connecting to API: ");
  Serial.println(apiUrl);

  if (!http.begin(client, apiUrl)) {
    Serial.println("HTTP begin failed");
    return -1;
  }

  http.setTimeout(3000);

  int httpCode = http.GET();
  Serial.print("HTTP code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.print("API response: ");
    Serial.println(payload);

    // 查找onlineCount值
    int startPos = payload.indexOf("\"onlineCount\":");
    if (startPos != -1) {
      startPos += 14;  // 跳过"onlineCount":
      int endPos = payload.indexOf(',', startPos);
      if (endPos == -1) endPos = payload.indexOf('}', startPos);

      if (endPos > startPos) {
        String countStr = payload.substring(startPos, endPos);
        Serial.print("Parsed count: ");
        Serial.println(countStr);
        return countStr.toInt();
      }
    }
    Serial.println("Failed to parse onlineCount");
  }

  http.end();
  return -1;
}

void updateLights(int count) {
  uint32_t color;

  // 根据在线人数设置颜色（高亮度）
  if (count == 0) {
    color = strip.Color(0, 0, 255);  // 蓝色
    Serial.println("Setting BLUE");
  } else if (count <= 45) {
    color = strip.Color(255, 255, 255);  // 白色
    Serial.println("Setting WHITE");
  } else {
    color = strip.Color(255, 0, 0);  // 红色
    Serial.println("Setting RED");
  }

  // 设置所有灯珠为相同颜色
  strip.fill(color);
  strip.show();

  // 添加呼吸效果确认更新
  for (int i = 0; i < 3; i++) {
    strip.setBrightness(50);
    strip.show();
    delay(150);
    strip.setBrightness(255);
    strip.show();
    delay(150);
  }
}

// 错误闪烁提示
void errorFlash(uint32_t color) {
  Serial.println("Error flash indication");
  for (int i = 0; i < 5; i++) {
    strip.fill(color);
    strip.show();
    delay(200);
    strip.clear();
    strip.show();
    delay(200);
  }
}
