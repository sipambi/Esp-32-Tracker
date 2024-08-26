#include <TinyGPSPlus.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
// #include <TinyGPS++.h>
#include <HardwareSerial.h>

#include "DHT.h"
#define DHTPIN 19
#define DHTTYPE DHT22

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

void dhtTemp();

// WiFi credentials
const char* ssid = "MONITOR";
const char* password = "123456789";

// Telegram bot token and chat ID
#define BOTtoken "7482748956:AAFbJ7Jfdr6orVrgDwcZTOP3PdFpCVOIJqk"
// #define CHAT_ID "-4514181978"
#define CHAT_ID "5813186453"


// Create WiFi and Telegram bot clients
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// GPS setup
HardwareSerial GPSSerial(1);
TinyGPSPlus gps;

// Bot request delay and timing
int botRequestDelay = 100;
unsigned long lastTimeBotRan;

void handleNewMessages(int numNewMessages) 
{
  Serial.println("Handling new messages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) 
  {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) 
    {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;
    
    // Process GPS data
    while (GPSSerial.available()) 
    {
      gps.encode(GPSSerial.read());
    }
    
    if (gps.charsProcessed() > 10) 
    {
      float currentLat = gps.location.lat();
      float currentLng = gps.location.lng();

      if (text == "/start") 
      {
        String welcomeMessage = "Welcome, " + from_name + ".\n";
        welcomeMessage += "Use the commands below to monitor the GPS location\n\n";
        welcomeMessage += "/location to get the current location\n";
        bot.sendMessage(chat_id, welcomeMessage, "");
      }

      if (text == "/location") 
      {
        // String locationMessage = "Location: https://www.google.com/maps/@" + String(currentLat, 6) + "," + String(currentLng, 6) + ",21z?entry=ttu";
        String locationMessage = "Location: https://www.google.com/maps?q=loc:" + String(currentLat, 6) + "," + String(currentLng, 6);
        bot.sendMessage(chat_id, locationMessage, "");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH);
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  #endif

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("GPS SYSTEM");
  delay(2000);
  lcd.clear();

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  dht.begin();
  // Print ESP32 local IP address
  Serial.println(WiFi.localIP());
}

void loop() 
{
  if (millis() > lastTimeBotRan + botRequestDelay) 
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) 
    {
      Serial.println("Got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTimeBotRan = millis();
  }
  dhtTemp();
}

void dhtTemp()
{
  delay(2000);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) 
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  lcd.setCursor(0,0);
  lcd.print("HUMIDITY:");
  lcd.setCursor(9,0);
  lcd.print(h);

  lcd.setCursor(0,1);
  lcd.print("TEMPE:");
  lcd.setCursor(9,1);
  lcd.print(t);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));

  if(t >= 30)
  {
    digitalWrite(18, LOW);
  }
  else
  {
    digitalWrite(18, HIGH);
  }
}