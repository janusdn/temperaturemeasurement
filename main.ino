// This #include statement was automatically added by the Spark IDE.
#include "Tokens.h"

#include "HttpClient/HttpClient.h"
#include "TemperatureReaderOneWire.h"
#include "OneWire.h"

#define DEBUG true

/**
* Declaring the variables.
*/
HttpClient http;

int lightLevel = 0;
bool shouldGoToSleep = false;

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();

OneWire ds = OneWire(D0);  // on pin 0 (a 4.7K resistor is necessary)

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
      { "Content-Type", "application/json" },
      { "X-Auth-Token" , TOKEN },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

http_request_t request;

void setup() {
    //pinMode(A0, INPUT);
    Serial.begin(9600);
    request.hostname = "things.ubidots.com";
    request.port = 80;
    shouldGoToSleep = false;
}

void loop() {

    if (shouldGoToSleep) {
        if (DEBUG) {
            Serial.println("Should go to sleep");
        }
        // Delay 20 seconds to allow flashing updates.
        delay(20000);
        shouldGoToSleep = false;
        Spark.sleep(SLEEP_MODE_DEEP, 1200);
        delay(2000);
    }

    http_response_t response;
    
    if (millis() - lastSync > ONE_DAY_MILLIS) {
        // Request time synchronization from the Spark Cloud
        Spark.syncTime();
        lastSync = millis();
    }

    if (WiFi.ready()) {
        // Get temperature


        float celsius;
        int r;
        r = read_temperature(celsius, ds);
        // Serial.println(r);
        // Serial.println(celsius);
   
        if (DEBUG) {
            switch (r) {
                case -1:
                    Serial.println("No more addresses.");
                    Serial.println();
                    Serial.println("Converting temp on all probes");
                    Serial.println();
                    break;
                case -2:
                    Serial.println("CRC is not valid!");
                    Serial.println();
                    break;
                case -3:
                    Serial.println("Device is not a DS18x20 family device.");
                    Serial.println();
                    break;
                case 0:
                    Serial.print("  Temperature = ");
                    Serial.print(celsius);
                    Serial.println();
                    break;
                default:
                    Serial.println(r);
                    Serial.println();
                    break;
            }
        }

        if (r == 0) {

          // Send to Ubidots

            request.path = "/api/v1.6/variables/"VARIABLE_ID"/values";
            request.body = "{\"value\":" + String(celsius) + "," +
                        "\"context\": { " +
                            "\"lat\": 55.8459920," +
                            "\"lng\": 9.7952940 }}";    

            http.post(request, response, headers);

            if (DEBUG) {
                delay(2000);
                Serial.println(response.status);
                Serial.println(response.body);
            }    
        
            if ((response.status == 200) || (response.status == 201)) {
                shouldGoToSleep = true;
            }
        }
        
        delay(2000);
    } 
    
}