//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 4. prints anything it receives to Serial.print
//
//
//************************************************************
#include <Arduino.h>
#include <global.h>
#include <tools.cpp>
#include <painlessMesh.h>

//#define CENTRAL   //ON

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
//#define   LED             2       // GPIO number of connected LED, ON ESP-12 IS GPIO2
#define BLINK_PERIOD 3000  // milliseconds until cycle repeat
#define BLINK_DURATION 100 // milliseconds LED is on for

#define MESH_SSID "myMesh"
#define MESH_PASSWORD "12345678"
#define MESH_PORT 5555

// Replace the next variables with your SSID/Password combination
// const char *STATION_SSID = "MIWIFI_2G_RXMu";
// const char *STATION_PASSWORD = "sAbubrRY";

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

// #define BME280_ON //ON
// #ifdef BME280_ON
// #include <sensors/bme280.cpp>
// void sendBme280();
// #endif

//********************* LED DETAILS *********************************************
uint8_t led_R = 14;
uint8_t led_G = 12;
uint8_t led_B = 13;
//*********************************************************************************

// Prototype so PlatformIO doesn't complain
void sendMessage(); // Prototype
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

void ledColorRGB(int led_red, int led_green, int led_blue);

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

//bool onFlagBME = false;

// *** Comunicaciones
//#define MQTT_ON
#ifdef MQTT_ON
#define HOSTNAME "MQTT_Bridge"
//#include <Comunications/mqtt.cpp>
#include <WiFiClient.h>
#include <PubSubClient.h>
IPAddress getlocalIP();
IPAddress myIP(0,0,0,0);

#endif

// *** PERIFERICOS

#define DISPLAY_ON //ON
#ifdef DISPLAY_ON
#include <Display/lcd.cpp>
#endif

//#define BME280_ON //ON
#ifdef BME280_ON

#include <sensors/bme280.cpp>
void sendBme280();

Task taskReadBme280(TASK_SECOND * 10, TASK_FOREVER, &sendBme280);
void sendBme280()
{
  _APP_DEBUG_("Send BME280", "");
  ledColorRGB(4, 0, 255);
  mesh.sendBroadcast(readBme280());
  wait(500);
  //taskReadBme280.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));
}

#endif //BME280_ON

// *** PERIFERICOS

void setupLedRGB()
{
  ledcAttachPin(led_R, 1); // assign RGB led pins to channels
  ledcAttachPin(led_G, 2); // assign RGB led pins to channels
  ledcAttachPin(led_B, 3); // assign RGB led pins to channels

  ledcSetup(1, 5000, 8); // 12 kHz PWM, 12-bit resolution
  ledcSetup(2, 5000, 8); // 12 kHz PWM, 12-bit resolution
  ledcSetup(3, 5000, 8); // 12 kHz PWM, 12-bit resolution

  ledcWrite(1, 255);
  ledcWrite(2, 255);
  ledcWrite(3, 255);
}

void ledColorRGB(int led_red, int led_green, int led_blue)
{
  ledcWrite(1, led_red);
  ledcWrite(2, led_green);
  ledcWrite(3, led_blue);
}

void showNodeID()
{
  Serial.print("Hola yo soy NodeID : ");
  Serial.println(mesh.getNodeId());
}

void sendMessage()
{
  String msg = "+ ";
  msg += mesh.getNodeId();
  msg += " RAM: " + String(ESP.getFreeHeap());
  mesh.sendBroadcast(msg);

  if (calc_delay)
  {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end())
    {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  Serial.printf("Enviando mensaje : %s\n", msg.c_str());

  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5)); // between 1 and 5 seconds
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Recivido desde %u msg=%s\n", from, msg.c_str());

#ifdef DISPLAY_ON

  StaticJsonDocument<250> doc;
  DeserializationError err = deserializeJson(doc, msg);  

    if (fila > 3)
  {
    fila = 0;
    lcd.clear();
  }

  //TOdO
  //if(err )

  //Todo Analizar mensajes
  if (String(msg).indexOf("BME280") != -1)
  {
    String nodeId = doc["nodeId"];
    String temp = doc["data"]["temp"];
    String hum  = doc["data"]["hum"];

    nodeId = nodeId.substring(nodeId.length() - 3 , nodeId.length());

    _APP_DEBUG_VALUE_("BME280", temp, hum);
    lcdText(0, fila, nodeId + " " + temp + "C " + hum + "%" );
    fila++;
    return;
    //value == 1 ? digitalWrite(gpio, HIGH) : digitalWrite(gpio, LOW);
  }



  //lcd.setCursor(0,fila);
  lcdText(0, fila, msg.substring(9, 25));
  fila++;
#endif
}

void newConnectionCallback(uint32_t nodeId)
{
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end())
  {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}

void setup()
{
  Serial.begin(115200);

#ifdef MQTT_ON



  // if (setup_wifi())
  // {
  //   _APP_DEBUG_("WIFI STATUS ", "ON");
  // }
  // else
  // {
  //   _APP_DEBUG_("WIFI STATUS ", "OFF");
  // }

  // if (mqtt_init())
  // {
  //   _APP_DEBUG_("MQTT STATUS ", "ON");
  // }
  // else
  // {
  //   _APP_DEBUG_("MQTT STATUS ", "OFF");
  // }
#endif

  setupLedRGB();

  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages
  //mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );


  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  // Channel set to 6. Make sure to use the same channel for your mesh and for you other
  // network (STATION_SSID)
  //mesh.init( MESH_SSID, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );


  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  // mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  // mesh.setHostname(HOSTNAME);

    // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
  //mesh.setRoot(true);
  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  //mesh.setContainsRoot(true);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  showNodeID();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
    // If on, switch off, else switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;

    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration())
    {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                 (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  // initialize LCD

#ifdef DISPLAY_ON
  lcdStart();
#endif

#ifdef BME280_ON
  bme280Init();
  _APP_DEBUG_(F("TEST BME280"), readBme280());
  userScheduler.addTask(taskReadBme280);
  taskReadBme280.enable();
#endif

  randomSeed(analogRead(A0));
}

void loop()
{
  mesh.update();

  if (onFlag)
    ledColorRGB(125, 255, 51);
  else
    ledColorRGB(255, 51, 252);

    // if(myIP != getlocalIP()){
    //   myIP = getlocalIP();
    //   Serial.println("My IP is " + myIP.toString());
    // }
}