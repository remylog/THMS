/***************************************************
  This is an example for the SHT4x Humidity & Temp Sensor

  Designed specifically to work with the SHT4x sensor from Adafruit
  ----> https://www.adafruit.com/products/4885

  These sensors use I2C to communicate, 2 pins are required to
  interface
 ****************************************************/

#include "Adafruit_SHT4x.h"
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char *ssid = "ssid";   // WiFi名を入力
const char *password = "password"; // WiFiのパスワード

// MQTTブローカー
const char *mqtt_broker = "address";
const char *mqtt_temp = "mqtt/temp";
const char *mqtt_hum = "mqtt/hum";
const char *mqtt_hin = "mqtt/hin";
const char *mqtt_username = "username";
const char *mqtt_password = "password";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void setup()
{
  Serial.begin(115200);

  // WiFiネットワークへの接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("WiFiへ接続中...");
  }
  Serial.println("WiFiネットワークに接続しました");
  // MQTTブローカーへの接続
  client.setServer(mqtt_broker, mqtt_port);
  // client.setCallback(callback);
  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("クライアント %s がパブリックMQTTブローカーに接続します\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("EMQXのパブリックMQTTブローカーに接続しました");
    }
    else
    {
      Serial.print("状態で失敗しました ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit SHT4x test");
  if (!sht4.begin())
  {
    Serial.println("Couldn't find SHT4x");
    while (1)
      delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  switch (sht4.getPrecision())
  {
  case SHT4X_HIGH_PRECISION:
    Serial.println("High precision");
    break;
  case SHT4X_MED_PRECISION:
    Serial.println("Med precision");
    break;
  case SHT4X_LOW_PRECISION:
    Serial.println("Low precision");
    break;
  }

  // You can have 6 different heater settings
  // higher heat and longer times uses more power
  // and reads will take longer too!
  sht4.setHeater(SHT4X_NO_HEATER);
  switch (sht4.getHeater())
  {
  case SHT4X_NO_HEATER:
    Serial.println("No heater");
    break;
  case SHT4X_HIGH_HEATER_1S:
    Serial.println("High heat for 1 second");
    break;
  case SHT4X_HIGH_HEATER_100MS:
    Serial.println("High heat for 0.1 second");
    break;
  case SHT4X_MED_HEATER_1S:
    Serial.println("Medium heat for 1 second");
    break;
  case SHT4X_MED_HEATER_100MS:
    Serial.println("Medium heat for 0.1 second");
    break;
  case SHT4X_LOW_HEATER_1S:
    Serial.println("Low heat for 1 second");
    break;
  case SHT4X_LOW_HEATER_100MS:
    Serial.println("Low heat for 0.1 second");
    break;
  }
}

void loop()
{
    while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("EMQXのパブリックMQTTブローカーに接続しました");
    } else {
      Serial.print("状態で失敗しました ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  sensors_event_t humidity, temp;

  uint32_t timestamp = millis();
  sht4.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  timestamp = millis() - timestamp;

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degrees C");
  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println("% rH");

  Serial.print("Read duration (ms): ");
  Serial.println(timestamp);

  client.publish(mqtt_temp, String(temp.temperature).c_str());
  client.publish(mqtt_hum, String(humidity.relative_humidity).c_str());
  int hin = -8.77+0.148*humidity.relative_humidity+0.907*temp.temperature;  //暑さ指数計算式
  client.publish(mqtt_hin, String(hin).c_str());    //暑さ指数をMQTTブローカーに送信

  client.subscribe(mqtt_temp);
  client.subscribe(mqtt_hum);
  client.subscribe(mqtt_hin);

  client.disconnect();

  delay(3600000); // 1時間ごとにデータを取得
}