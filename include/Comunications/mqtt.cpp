//#include <WiFi.h>
#include <PubSubClient.h>

// Replace the next variables with your SSID/Password combination
const char *STATION_SSID = "MIWIFI_5G_RXMu";
const char *STATION_PASSWORD = "sAbubrRY";

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char *mqtt_server = "192.168.10.111";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

bool setup_wifi()
{
    wait(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(STATION_SSID);

    WiFi.begin(STATION_SSID, STATION_PASSWORD);
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 5)
    {
        wait(500);
        Serial.print(".");
        intentos++;
    }


    //WiFi.mode(WIFI_AP_STA); // [WIFI_STA | WIFI_AP | WIFI_AP_STA]


    _APP_DEBUG_("MAC: " , WiFi.macAddress() );
    _APP_DEBUG_("IP: " , WiFi.localIP() );

    return true;
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
//   if (String(topic) == "esp32/output") {
//     Serial.print("Changing output to ");
//     if(messageTemp == "on"){
//       Serial.println("on");
//       digitalWrite(ledPin, HIGH);
//     }
//     else if(messageTemp == "off"){
//       Serial.println("off");
//       digitalWrite(ledPin, LOW);
//     }
//   }
}

bool mqtt_init(){
   client.setServer(mqtt_server, 1883);
    
    client.setCallback(callback);

    return client.connected();
}