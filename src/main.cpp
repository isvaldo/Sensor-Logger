#include <Arduino.h>
#include <Wire.h>
#include <SI7021.h>
#include "Adafruit_CCS811.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Secret.h>
#include <WifiHelper.h>

/// Whales Objects
Adafruit_CCS811 ccs;
SI7021 SI7021_Sensor;
WiFiClient espClient;
HTTPClient http;
//End

struct Collect
{
  String Temperature;
  String Humidity;
  String Co2;
  String Tvoc;
};

/**
 * Return false if fail
 */
bool sendData(Collect collect)
{
  http.begin(HubUrl);
  http.addHeader("Content-Type", "text/plain"); 
  String payload = "HUB," + collect.Co2 + "," + collect.Temperature + "," + collect.Humidity+","+collect.Tvoc;
  int httpCode = http.POST(payload);

  Serial.print("HttpCode:");
  Serial.println(httpCode);
  Serial.print("Url: ");
  Serial.println(HubUrl);
  Serial.print("Data: ");
  Serial.println(payload);

  if (httpCode != 200)
  {
    Serial.println(httpCode);
    return false;
  }
  http.end();
  return true;
}

String co2SafeCollect()
{
  if (ccs.available())
  {
    if (!ccs.readData())
    {
      return String(ccs.geteCO2());
    }
    else
    {
      return String("");
      while (1)
        ;
    }
  }
  return String("");
}

String TvocSafeCollect(){
  if (ccs.available())
  {
    if (!ccs.readData())
    {
      return String(ccs.getTVOC());
    }
    else
    {
      return String("");
      while (1)
        ;
    }
  }
  return String("");
}

Collect
GetSensorInfo()
{
  Collect collect = Collect{};
  collect.Temperature = String(SI7021_Sensor.getCelsiusHundredths() / 100.0);
  collect.Humidity = String(SI7021_Sensor.getHumidityPercent());
  delay(1500);
  collect.Co2 = co2SafeCollect();
  delay(1500);
  collect.Tvoc = TvocSafeCollect();

  Serial.print("CO2: ");
  Serial.print(ccs.geteCO2());
  Serial.print(" Temp: ");
  Serial.print(collect.Temperature);
  Serial.print(" Humidity:");
  Serial.println(collect.Humidity);
  Serial.print(" Tvoc");
  Serial.println(collect.Tvoc);
  delay(500);

  return collect;
}

void setup()
{
  // Load SI7021
  Wire.begin();
  Serial.begin(9600);

  if (!ccs.begin())
  {
    Serial.println("failed to start sensor! please check your wiring.");
    while (1)
      ;
  }

  //calibrate temperature sensor
  while (!ccs.available())
    ;
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);

  connectToWifi();
}

void loop()
{
  Collect data = GetSensorInfo();
  WifiHealthCheck();
  sendData(data);
}
