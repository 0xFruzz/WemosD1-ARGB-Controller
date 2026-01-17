#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Constants
#define LED_PIN     D4    
#define NUM_LEDS    250   
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB   

CRGB leds[NUM_LEDS];

// Wifi Data
const char* ssid = "";
const char* password = "";

ESP8266WebServer server(80);

struct Config {
  uint8_t brightness;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t magic;
};

Config config;

void setup() {
  Serial.begin(115200);
  
  EEPROM.begin(sizeof(Config));
  EEPROM.get(0, config);

  if (config.magic != 0xAA) {
    config = {128, 255, 255, 255, 0xAA};
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(config.brightness);
  fill_solid(leds, NUM_LEDS, CRGB(config.r, config.g, config.b));
  FastLED.show();
  
  server.on("/stats", HTTP_GET, handleRoot);
  server.on("/setup", HTTP_GET, handleSetup);
  server.begin();
}

void loop() {
  server.handleClient();
}

void applyAndSave() {
  bool changed = false;

  if (FastLED.getBrightness() != config.brightness) changed = true;
  if (leds[0].r != config.r || leds[0].g != config.g || leds[0].b != config.b) changed = true;

  if (changed) {
    FastLED.setBrightness(config.brightness);
    fill_solid(leds, NUM_LEDS, CRGB(config.r, config.g, config.b));
    FastLED.show();
    
    EEPROM.put(0, config);
    EEPROM.commit();
  }
}

void handleRoot() {
  char buf[256];
  snprintf(buf, sizeof(buf), 
    "{\"leds\":{\"count\":%d,\"brightness\":%d,\"color\":{\"r\":%d,\"g\":%d,\"b\":%d}}}",
    NUM_LEDS, config.brightness, config.r, config.g, config.b);
  server.send(200, "application/json", buf);
}

void handleSetup() {
  if (server.hasArg("brightness")) {
    config.brightness = constrain(server.arg("brightness").toInt(), 0, 255);
  }
  
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    config.r = constrain(server.arg("r").toInt(), 0, 255);
    config.g = constrain(server.arg("g").toInt(), 0, 255);
    config.b = constrain(server.arg("b").toInt(), 0, 255);
  }

  applyAndSave();

  char buf[128];
  snprintf(buf, sizeof(buf), "{\"status\":\"ok\",\"brightness\":%d}", config.brightness);
  server.send(200, "application/json", buf);
}