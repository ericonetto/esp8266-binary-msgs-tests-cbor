#include <Arduino.h>
#include <EEPROM.h>
#include <cstring>

#include <CBOR.h>
#include <CBOR_parsing.h>
#include <CBOR_streams.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#define MAX_MAP_KEY_SZ 8
#define MAX_MAP_VALUE_TXT_SZ 16

#define SSID ""
#define SSID_PASSWORD ""
#define SERVER_IP_ADDRESS ""


namespace cbor = ::qindesign::cbor;


// MyData contains important data.
struct MyData {
  uint8_t name[10]{0};  // UTF-8 encoded
  bool planet = true;
  int64_t number=0;
  double gravity=0;
} myData;


//EXPECT {'name':'earth', 'planet':true, 'number': 3, 'gravity': 9.807};
bool loadDataClassic(MyData* myData, Stream* stream) {
  uint64_t mapLength;
  bool isIndefiniteMapLength;

  cbor::Reader cbor{*stream};

  // is it a map?
  if (expectMap(cbor, &mapLength, &isIndefiniteMapLength) && !isIndefiniteMapLength) {

    // iterate over all key/value pairs
    for (int i = 0; i < mapLength; i++) {
        uint64_t keySz;
        bool iskeyIndefinite;

        // get map key
        if (expectText(cbor, &keySz, &iskeyIndefinite)) {
          char key[MAX_MAP_KEY_SZ];
          keySz = cbor.readBytes((uint8_t*)key, keySz);
          key[keySz] = '\0';


          // if the key is "name", read the boolean associated
          if (strcmp(key, "name") == 0) {
            uint64_t valSz;
            bool isValIndefinite;
            if (expectText(cbor, &valSz, &isValIndefinite)) {
              // TODO: carefull with buffer overflow...
              cbor.readBytes((uint8_t*)&(myData->name), valSz);
              myData->name[valSz] = '\0';
            } else {
              Serial.println("Text value was not text!" );
              return false;
            }
          }
          else if (strcmp(key, "planet") == 0) {
            if (expectBoolean(cbor, &(myData->planet))) {
              // do nothing -> data already read!
            } else {
              Serial.println("planet was not a boolean!" );
              return false;
            }
          }
          else if (strcmp(key, "number") == 0) {
            if (expectInt(cbor, &(myData->number))) {
              // do nothing -> data already read!
            } else {
              Serial.println("number was not a integer!" );
              return false;
            }
          }
          else if (strcmp(key, "gravity") == 0) {
            if (expectDouble(cbor, &(myData->gravity))) {
              // do nothing -> data already read!
            } else {
              Serial.println("gravity was not a double!" );
              return false;
            }
          } else {
            // TODO: print what we have found, to improve debug message
            Serial.println("Found a strange key....." );
            return false;
          }



        } else { // not a text key!
          // TODO: print what we have found, to improve debug message
          Serial.println("Expected a text key. Found something else..." );
          return false;
        }

    }
  } else { // not a map!
    Serial.printf("Expected a fixed size map. Found %d\n", cbor.getDataType());
    return false;
  }

  return true;
}


void setup() {
  Serial.begin(115200);
  Serial.println("SETUP STARTED");                        //Serial connection
  WiFi.begin(SSID, SSID_PASSWORD);   //WiFi connection

  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion

    delay(500);
    Serial.println("Waiting for connection");

  }
  Serial.println("SETUP FINISHED");
}



void loop() {

  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
    HTTPClient http;

    //RECEIVE A CBOR MESSAGE

    Serial.print("[HTTP] begin RECEIVING CBOR..\n");
    // configure server and url
    http.begin("http://" +  (String)SERVER_IP_ADDRESS + ":3000/CBOR");

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/json");

    int httpCode =http.GET();
    if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {

        // get tcp stream
        WiFiClient * stream = http.getStreamPtr();

        // read all data from server
        while(http.connected()) {
          // get available data size
          size_t size = stream->available();

          if(size) {
            MyData dt;
            //loadMyData(&dt,stream);
            loadDataClassic(&dt, stream);

            Serial.printf("Received> name: %s, planet: %s, number: %u, gravity: ", dt.name, dt.planet ? "true" : "false", dt.number);
            //Known issue: Arduino ide does not support %f
            Serial.println(dt.gravity,3);
          }
        }

        Serial.print("[HTTP] connection closed or file end.\n");
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();


    Serial.println("...");
    Serial.println("...");
    Serial.println("...");
    delay(5000);  //Delay 5 seconds



    //POST A CBOR MESSAGE

    Serial.print("[HTTP] begin posting a CBOR...\n");
    // configure server and url
    http.begin("http://" + (String)SERVER_IP_ADDRESS + ":3000/CBOR/unpack");

    Serial.print("[HTTP] POST...\n");
    char CBORmessage[]={0xA4, 0x64, 0x6E, 0x61, 0x6D, 0x65, 0x65, 0x65, 0x61,
      0x72, 0x74, 0x68, 0x66, 0x70, 0x6C, 0x61, 0x6E, 0x65, 0x74, 0xF5, 0x66,
      0x6E, 0x75, 0x6D, 0x62, 0x65, 0x72, 0x03, 0x67, 0x67, 0x72, 0x61, 0x76,
      0x69, 0x74, 0x79, 0xFB, 0x40, 0x23, 0x9D, 0x2F, 0x1A, 0x9F, 0xBE, 0x77};

    // start connection and send HTTP header
    http.POST(CBORmessage);
    if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {

        String payload = http.getString();
        Serial.println("Response:");
        Serial.println(payload);
        Serial.println("");

        Serial.print("[HTTP] connection closed or file end.\n");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();








  }
  delay(30000);  //Send a request every 30 seconds

}
