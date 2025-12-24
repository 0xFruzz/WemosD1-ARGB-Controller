#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// led conf
#define LED_PIN     D4    
#define NUM_LEDS    60   
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB   
#define BRIGHTNESS 255

CRGB leds[NUM_LEDS];

// wifi and web
const char* ssid = "";
const char* password = "";

ESP8266WebServer server(80);

//EEPROM Configuration

struct Config {
  uint8_t brightness;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

Config config;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  EEPROM.begin(sizeof(Config) + 10);
  EEPROM.get(0, config);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  Serial.print("IP ");
  Serial.println(WiFi.localIP());

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(config.brightness);
  
  server.on("/stats", handleRoot);
  server.on("/setup", handleSetup);
  server.begin();
}

void loop() {
  server.handleClient();
  colorist();
  delay(20);
}

void colorist() {
  CRGB cur = leds[0];
  if(config.r != cur.r && config.g != cur.g && config.b != cur.b) {
      fillSolid(CRGB(config.r, config.g, config.b));
  }
}

void saveConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
}

void updateConfig() {
  config.brightness = FastLED.getBrightness();
  
  CRGB cur = leds[0];
  config.r = cur.r;
  config.g = cur.g;
  config.b = cur.b;

  saveConfig();
}

void fillSolid(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

void handleRoot() {
  String json = "{";
  
  json += "\"leds\":{";
  json += "\"count\":" + String(NUM_LEDS) + ",";
  json += "\"brightness\":" + String(FastLED.getBrightness()) + ",";
  json += "\"color\":{";
  json += "\"r\":" + String(config.r) + ",";
  json += "\"g\":" + String(config.g) + ",";
  json += "\"b\":" + String(config.b) + ",";
  json += "},";
  
  json += "\"config\":{";
  json += "\"brightness\":" + String(config.brightness) + ",";
  json += "\"color\":{";
  json += "\"r\":" + String(config.r) + ",";
  json += "\"g\":" + String(config.g) + ",";
  json += "\"b\":" + String(config.b);
  json += "}";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleSetup() {
  String json = "{";
  
  if (server.hasArg("brightness")) {
    int brightness = server.arg("brightness").toInt();
    brightness = constrain(brightness, 0, 255);
    FastLED.setBrightness(brightness);
    FastLED.show();
    
    json += "\"brightness\":" + String(brightness) + ",";
  }
  
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);
    
    fillSolid(CRGB(r, g, b));
    
    json += "\"color\":{";
    json += "\"r\":" + String(r) + ",";
    json += "\"g\":" + String(g) + ",";
    json += "\"b\":" + String(b);
    json += "},";
  }

  updateConfig();
  
  if (json.endsWith(",")) 
    json.remove(json.length() - 1);
  
  json += "}";
  
  FastLED.show();
  server.send(200, "application/json", json);
}