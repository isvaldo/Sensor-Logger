

boolean connected = false;

void connectToWifi(){
	/*
		Connect to Wifi
	*/

	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print("Connecting to WiFi...[");
    Serial.print(WiFi.status());
    Serial.println("]");
		connected = true;
	}
	Serial.println("Connected");
}



void WifiHealthCheck(){
    /* reconection routine */
	if (connected == false){
    Serial.println("RESTARTING ...Try [");
     Serial.print(WiFi.status());
    Serial.println("]");
    //espClient.flush();

    while (WiFi.status() != WL_CONNECTED) {
      if(WiFi.status() == WL_CONNECT_FAILED ||WiFi.status() == WL_DISCONNECTED){
        Serial.println("Disconnected!");
        WiFi.disconnect();
        WiFi.reconnect();
        connectToWifi();
      }
      
		delay(500);
    Serial.print("Connecting to WiFi...[");
    Serial.print(WiFi.status());
    Serial.println("]");
		connected = true;
	}
    WiFi.disconnect();
    delay(2000);
    WiFi.reconnect();
		Serial.println("RESTARTING ... Wait");
    connected = true;
    delay(5000);
	}
}