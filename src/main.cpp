
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <ErriezBH1750.h>
#include <SI7021.h>

#include <Secret.h>
#include <WiFi.h>
#include <HTTPClient.h>;

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)

BH1750 sensor(LOW);
Adafruit_BME680 bme; // I2C


WiFiClient espClient;
// PubSubClient client(espClient);
HTTPClient http;


float t;                                      
float h;                                      

long SensorenLetzteAktualisierung;            
int SensorenSampelTime = 1000;

SI7021 SI7021_Sensor; 

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int   getgasreference_count = 0;

void connectToWifi(){
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.println("Connecting to WiFi..");
	}
}

String produceStatus(uint16_t lux, float temp, float humi, String airQ){  
  http.begin(HubUrl
  
  );
  http.addHeader("Content-Type", "text/plain"); 
  int httpCode = http.POST("HUB,"+String(lux)+","+String(temp)+","+String(humi)+","+airQ);
  Serial.println(httpCode);
  String payload = http.getString();
  http.end();
  return payload;
}

String CalculateIAQ(float score){
  String IAQ_text = "Air quality is ";
  score = (100-score)*5;
  if      (score >= 301)                  IAQ_text += "Hazardous";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Very Unhealthy";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Unhealthy";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Unhealthy for Sensitive Groups";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Moderate";
  else if (score >=  00 && score <=  50 ) IAQ_text += "Good";
  return IAQ_text;
}

void GetGasReference(){
  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  Serial.println("Getting a new gas reference value");
  int readings = 10;
  for (int i = 1; i <= readings; i++){ // read gas for 10 x 0.150mS = 1.5secs
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
}


void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("BME680 test"));
  connectToWifi();
  pinMode(26,OUTPUT);
  pinMode(33,OUTPUT);
  pinMode(34,OUTPUT);
  
  
  Wire.begin();
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  } else Serial.println("Found a sensor");

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_16X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms
  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  GetGasReference();


  // Initialize sensor in continues mode, high 0.5 lx resolution
  sensor.begin(ModeContinuous, ResolutionHigh);

  // Start conversion
  sensor.startConversion();
}

void loop() {
  digitalWrite(33,LOW);
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println("°C");
  Serial.print("   Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");
  Serial.print("   Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println("%");
  Serial.print("        Gas = ");
  Serial.print(bme.readGas());
  Serial.println("R\n");

  
  //Calculate humidity contribution to IAQ index
  float current_humidity = bme.readHumidity();
  if (current_humidity >= 38 && current_humidity <= 42)
    hum_score = 0.25*100; // Humidity +/-5% around optimum 
  else
  { //sub-optimal
    if (current_humidity < 38) 
      hum_score = 0.25/hum_reference*current_humidity*100;
    else
    {
      hum_score = ((-0.25/(100-hum_reference)*current_humidity)+0.416666)*100;
    }
  }
  digitalWrite(26,LOW);
  //Calculate gas contribution to IAQ index
  float gas_lower_limit = 5000;   // Bad air quality limit
  float gas_upper_limit = 50000;  // Good air quality limit 
  if (gas_reference > gas_upper_limit) gas_reference = gas_upper_limit; 
  if (gas_reference < gas_lower_limit) gas_reference = gas_lower_limit;
  gas_score = (0.75/(gas_upper_limit-gas_lower_limit)*gas_reference -(gas_lower_limit*(0.75/(gas_upper_limit-gas_lower_limit))))*100;
  
  //Combine results for the final IAQ index value (0-100% where 100% is good quality air)
  float air_quality_score = hum_score + gas_score;

  Serial.println("Air Quality = "+String(air_quality_score,1)+"% derived from 25% of Humidity reading and 75% of Gas reading - 100% is good quality air");
  Serial.println("Humidity element was : "+String(hum_score/100)+" of 0.25");
  Serial.println(" Gas element was : "+String(gas_score/100)+" of 0.75");
  if (bme.readGas() < 120000) Serial.println("***** Poor air quality *****");
  Serial.println();
  if ((getgasreference_count++)%10==0) GetGasReference(); 
  Serial.println(CalculateIAQ(air_quality_score));
  Serial.println("------------------------------------------------");
  delay(2000);
  digitalWrite(34,LOW);

   uint16_t lux;

  // Wait for completion (blocking busy-wait delay)
  if (sensor.isConversionCompleted()) {
    // Read light
    lux = sensor.read();

    // Print light
    Serial.print(F("Light: "));
    Serial.print(lux / 2);
    Serial.print(F("."));
    Serial.print(lux % 10);
    Serial.println(F(" LUX"));
  }

  if (millis() - SensorenLetzteAktualisierung > SensorenSampelTime) {

    t = SI7021_Sensor.getCelsiusHundredths() / 100.0; // Temperatur aus SI7021 auslesen 
                                                      // und in Variable t speichern
                                                      
    h = SI7021_Sensor.getHumidityPercent();           // Luftfeuchtigkeit aus Si7021 auslesen
                                                      // und in Variable h speichern
    SensorenLetzteAktualisierung = millis();                                              
  
  Serial.print("temperature: ");
  Serial.println(t);
  Serial.print("Humidity: ");
  Serial.println(h);
  }
  digitalWrite(33,HIGH);
  digitalWrite(34,HIGH);
  digitalWrite(26,HIGH);
  
  produceStatus(lux,t,h,CalculateIAQ(air_quality_score));

   digitalWrite(36,LOW);
   delay(300);
   digitalWrite(36,HIGH);
   digitalWrite(33,LOW);
   delay(300);
   digitalWrite(33,HIGH);
   digitalWrite(34,LOW);
   delay(300);
   digitalWrite(33,HIGH);
   digitalWrite(34,HIGH);
   digitalWrite(26,HIGH);
   
}

