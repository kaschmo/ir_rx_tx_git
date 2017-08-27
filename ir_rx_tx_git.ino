//IR Receiver and Transmitter for NodeMCU
#include "wifi_credentials.h"

 #include <IRrecv.h>
 #include <IRutils.h>
 #include <IRsend.h>
 //required for MQTT
 #include <ESP8266WiFi.h>
 #include <PubSubClient.h>
 
 
 // An IR detector/demodulator is connected to GPIO pin 14(D5 on a NodeMCU
 uint16_t RECV_PIN = 14; //=D5
 uint16_t SEND_PIN = 12; //=D6
 
 IRrecv irrecv(RECV_PIN);
 decode_results results;  // Somewhere to store the results
 irparams_t save;         // A place to copy the interrupt state while decoding.
 
 
 
 //MQTT
 WiFiClient espClient;
 PubSubClient client(espClient);
 
 
 const char* inTopic = "cmnd/test_ir/#";
 const char* outTopic = "stat/test_ir/";
 
 
 void setup_wifi() {
   delay(10);
   // We start by connecting to a WiFi network
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);
   WiFi.persistent(false);
   WiFi.mode(WIFI_OFF);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
     
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
 }
 
 
 //callback function for MQTT client
 void callback(char* topic, byte* payload, unsigned int length) {
   payload[length]='\0'; // Null terminator used to terminate the char array
   String message = (char*)payload;
 
   Serial.print("Message arrived on topic: [");
   Serial.print(topic);
   Serial.print("]: ");
   Serial.println(message);
   
   //get last part of topic 
   char* cmnd = "test";
   char* cmnd_tmp=strtok(topic, "/");
 
   while(cmnd_tmp !=NULL) {
     cmnd=cmnd_tmp; //take over last not NULL string
     cmnd_tmp=strtok(NULL, "/"); //passing Null continues on string
     //Serial.println(cmnd_tmp);    
   }
 
   if (!strcmp(cmnd, "tset")) {
     //dummy
   }
   
 }
 
 void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("IR_RX_TX")) {
        Serial.println("connected");
        
        client.publish(outTopic, "IR RX, TX Hub booted");
        
        // ... and resubscribe
        client.subscribe(inTopic);
  
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");      
        delay(5000);
      }
    }
  }
 
 void setup() {
   // Status message will be sent to the PC at 115200 baud
   Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
   irrecv.enableIRIn();  // Start the receiver
   
   //WIFI and MQTT
   setup_wifi();                   // Connect to wifi 
   client.setServer(mqtt_server, 1883);
   client.setCallback(callback);
 
  
 }
 
 // Dump out the decode_results structure.
 void dumpRaw(decode_results *results) {
   // Print Raw data
   Serial.print("Timing[");
   Serial.print(results->rawlen - 1, DEC);
   Serial.println("]: ");
 
   for (uint16_t i = 1;  i < results->rawlen;  i++) {
     if (i % 100 == 0)
       yield();  // Preemptive yield every 100th entry to feed the WDT.
     uint32_t x = results->rawbuf[i] * USECPERTICK;
     if (!(i & 1)) {  // even
       Serial.print("-");
       if (x < 1000) Serial.print(" ");
       if (x < 100) Serial.print(" ");
       Serial.print(x, DEC);
     } else {  // odd
       Serial.print("     ");
       Serial.print("+");
       if (x < 1000) Serial.print(" ");
       if (x < 100) Serial.print(" ");
       Serial.print(x, DEC);
       if (i < results->rawlen - 1)
         Serial.print(", ");  // ',' not needed for last one
     }
     if (!(i % 8)) Serial.println("");
   }
   Serial.println("");  // Newline
 }
 
 // Dump out the decode_results structure.
 //
 void dumpCode(decode_results *results) {
   // Start declaration
   Serial.print("uint16_t  ");              // variable type
   Serial.print("rawData[");                // array name
   Serial.print(results->rawlen - 1, DEC);  // array size
   Serial.print("] = {");                   // Start declaration
 
   // Dump data
   for (uint16_t i = 1; i < results->rawlen; i++) {
     Serial.print(results->rawbuf[i] * USECPERTICK, DEC);
     if (i < results->rawlen - 1)
       Serial.print(",");  // ',' not needed on last one
     if (!(i & 1)) Serial.print(" ");
   }
 
   // End declaration
   Serial.print("};");  //
 
   // Comment
   
   Serial.print(" ");
   serialPrintUint64(results->value, 16);
 
   // Newline
   Serial.println("");
 
   // Now dump "known" codes
   if (results->decode_type != UNKNOWN) {
     // Some protocols have an address &/or command.
     // NOTE: It will ignore the atypical case when a message has been decoded
     // but the address & the command are both 0.
     if (results->address > 0 || results->command > 0) {
       Serial.print("uint32_t  address = 0x");
       Serial.print(results->address, HEX);
       Serial.println(";");
       Serial.print("uint32_t  command = 0x");
       Serial.print(results->command, HEX);
       Serial.println(";");
     }
 
     // All protocols have data
     Serial.print("uint64_t  data = 0x");
     serialPrintUint64(results->value, 16);
     Serial.println(";");
   }
 }
 
 void loop() {
   if (!client.connected()) {
     reconnect();
   }
   client.loop();
   // Check if the IR code has been received.
   if (irrecv.decode(&results, &save)) {
     Serial.println("Receiving IR data");
     //dumpRaw(&results);            // Output the results in RAW format
     dumpCode(&results);           // Output the results as source code
     Serial.println("");           // Blank line between entries
   }
 
 }
 