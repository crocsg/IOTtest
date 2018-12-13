#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <PubSubClient.h>
//#include <fs.h>

const char* ssid = "maisonliffre";
const char* password = "02101967";
const char* mqtt_server = "coyote.sgodin.com";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[80];
char client_id[64];
int value = 0;
char topic[64];

int led_state = 1;
int change = 0;

void(* resetFunc) (void) = 0;//declare reset function at address 0

void setup_wifi() {

  delay(250);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int t = 0;

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    delay(125);
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(125);
    Serial.print(".");
    t++;

    if (t > 4 * 15)
      resetFunc ();
  }
  
  // initialize topic root
  snprintf (topic, sizeof(topic), "/dev/%s", WiFi.macAddress().c_str());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < (int) length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  change++;
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    led_state = 1;
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    snprintf (topic, sizeof(topic), "/dev/%s", WiFi.macAddress().c_str());
     Serial.print("Publish message: ");
    Serial.println("led on");
    client.publish(topic, "led on");

    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    led_state = 0;
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    snprintf (topic, sizeof(topic), "/dev/%s", WiFi.macAddress().c_str());
     Serial.print("Publish message: ");
    Serial.println("led off");
    client.publish(topic, "led off");
  }

}

void reconnect() {
  // Loop until we're reconnected
  char willtopic[64];
  char willmsg[64];
  while (!client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    client.setServer(mqtt_server, 1883);
    snprintf (willtopic, sizeof(willtopic), "%s/status", topic);
    
    if (client.connect(client_id, willtopic, 2, true, "DISCONNECTED"))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
      client.publish(willtopic, "CONNECTED", true);
      Serial.println("CONNECTED to mqtt");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
      delay(5000);
    }
  }
  digitalWrite(LED_BUILTIN, led_state == 1 ? LOW :  HIGH);   // Turn the LED acordingly
}
void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  
  Serial.begin(115200);
  setup_wifi();
  
  snprintf (client_id, sizeof(client_id), "ESP32Client_%s", WiFi.macAddress().c_str());
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  
  digitalWrite(LED_BUILTIN, led_state == 1 ? LOW :  HIGH);   // Turn the LED acordingly

}

void loop() {
  // put your main code here, to run repeatedly:
   if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  digitalWrite(LED_BUILTIN, led_state == 1 ? LOW :  HIGH);   // Turn the LED acordingly

  long now = millis();
  int vbat = -1;
  
  //#ifdef _VBAT
  vbat = analogRead (35);
  //plpo;
  //#endif

  //Serial.print("vbat:");
  //Serial.println(vbat);

  if (now - lastMsg > 1000 * 15) {
    IPAddress ip = WiFi.localIP();
    lastMsg = now;
    ++value;

    snprintf (msg, sizeof(msg), "Update #%d %d %ld %s", value, vbat, millis(), ip.toString().c_str());
    snprintf (topic, sizeof(topic), "/dev/%s", WiFi.macAddress().c_str());
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(topic, msg);
    
    

  }
  if (change != 0)
  {
    snprintf (msg, sizeof(msg), "led state #%d", value);
    snprintf (topic, sizeof(topic), "/dev/%s", WiFi.macAddress().c_str());
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(topic, msg);
    change =0;
  }
}