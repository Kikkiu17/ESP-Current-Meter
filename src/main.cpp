#define BLYNK_TEMPLATE_ID "xxxxxxxxxxxxxxxxxxxxxxx"
#define BLYNK_DEVICE_NAME "xxxxxxxxxxxxxxxxxxxxxxx"
#define BLYNK_AUTH_TOKEN "xxxxxxxxxxxxxxxxxxxxxxx"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include "Screen.h"
#include <string_view>
#include <iomanip>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "Settings.h"
#include <array>

const char* res_turnoff = "/trigger/xxxxx/with/key/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char* res_turnon = "/trigger/xxxxx/with/key/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char* server = "maker.ifttt.com";
const char *mqtt_server = "192.168.1.6"; // IP raspberrypi
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "xxxxxxxxxxxxxxx";
char pass[] = "xxxxxxxxxxxxxxx";
char tx_topic[20];
char tx_message[5];

WiFiUDP ntpUDP;
WiFiClient client;
BlynkTimer timer;
PubSubClient PSClient(client);
Screen scr;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

void callback(char topic[19], byte *message, unsigned int length)
{
    for (uint32_t i = 0; i < 20; i++)
        tx_topic[i] = topic[i];

    for (uint32_t i = 0; i < length; i++)
        tx_message[i] = (char)message[i];
    tx_message[4] = '\0';
}

void reconnect()
{
    while (!PSClient.connected())
    {
        Serial.print("Attempting MQTT connection...");

        if (PSClient.connect("RX"))
        {
            Serial.println("connected");
            PSClient.subscribe("/CURRENT_TX/CURRENT");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(PSClient.state());
            Serial.println(" try again in 1 second");
            delay(1000);
        }
    }
}

void TIMER()
{
  // manda uno stato di READY al TX ogni secondo
  // quindi gli chiede di mandare l'informazione (corrente, x.xx)
  PSClient.publish("/CURRENT_RX/STATUS", "RDY");
}

void turnOnSocket() {
    Serial.print("Connecting to "); 
    Serial.print(server);

    WiFiClient client;
    if (!client.connect(server, 80)) {
        Serial.println("connection failed");
    }

    Serial.print("Request resource: "); 
    Serial.println(res_turnon);
    client.print(String("GET ") + res_turnon + " HTTP/1.1\r\n" +
                    "Host: " + server + "\r\n" + 
                    "Connection: close\r\n\r\n");
                    

    unsigned long timeout = millis();
    // Read all the lines of the reply from server and print them to Serial
    while (client.available() == 0)
    {
        if (millis() - timeout > 5000)
        {
            Serial.println(">>> Client Timeout !");
            client.stop(); return;
        }
    } 

    while(client.available())
        Serial.write(client.read());
    Serial.println("\nclosing connection");
    client.stop();
}

void turnOffSocket() {
    Serial.print("Connecting to "); 
    Serial.print(server);

    WiFiClient client;
    if (!client.connect(server, 80)) {
        Serial.println("connection failed");
    }

    Serial.print("Request resource: "); 
    Serial.println(res_turnoff);
    client.print(String("GET ") + res_turnoff + " HTTP/1.1\r\n" +
                    "Host: " + server + "\r\n" + 
                    "Connection: close\r\n\r\n");
                    

    unsigned long timeout = millis();
    // Read all the lines of the reply from server and print them to Serial
    while (client.available() == 0)
    {
        if (millis() - timeout > 5000)
        {
            Serial.println(">>> Client Timeout !");
            client.stop(); return;
        }
    } 

    while(client.available())
        Serial.write(client.read());
    Serial.println("\nclosing connection");
    client.stop();
}

int timeout = 0;
int hour = 0;
int prev_hour = 0;
uint32_t n_gathered_pwr = 0;
uint32_t total_consumption = 0;

float last_current = 0;
uint32_t last_watts = 0;

void loop()
{
    timeClient.update();

    Blynk.run();
    timer.run();

    if (!PSClient.connected())
        reconnect();
    PSClient.loop();

    std::string_view topic_str(tx_topic);
    if (topic_str == "/CURRENT_TX/CURRENT")
    {
        hour = timeClient.getHours();
        for (uint32_t i = 0; i < 20; i++)
            tx_topic[i] = '\n';

        if (scr.slp_time == scr.user_slp_time && scr.user_override == 0)
            scr.sleep();

        if (!scr.is_sleeping)
            scr.slp_time++;

        float received_current = atof(tx_message);

        uint32_t pwr = received_current * 220;
        scr.checkWake(pwr, last_watts);

        scr.writePower(pwr, received_current, last_watts, last_current);

        last_current = received_current;
        last_watts = pwr;

        Blynk.virtualWrite(V1, pwr);
        Blynk.virtualWrite(V0, received_current);

        if (scr.seconds_without_overload > 10 && !scr.car_turned_on)
        {
            turnOnSocket(); // riconnette macchina
            scr.car_turned_on = true;
        }

        // OVERLOAD OLTRE 10 SEC
        if (pwr >= 3100)
        {
            if (scr.overload_time == 10)
            {
                if (!scr.overload_warn)
                {
                    scr.overload_warn = true;
                    // eseguito solo una volta appena supera i 3100 W per 10 secondi
                    turnOffSocket();    // disconnette macchina
                    scr.car_turned_on = false;
                }
                scr.blynk_overload_time++;
            }
            else
                scr.overload_time++;
        }
        else if (scr.overload_time == 10)
        {
            scr.overload_warn = false;
            scr.overload_time = 0;
        }
        else
            scr.seconds_without_overload++;

        if (hour == prev_hour)
        {
            n_gathered_pwr++;
            total_consumption += pwr;
        }
        else
        {
            prev_hour = hour;
            int kwh = total_consumption / n_gathered_pwr;
            n_gathered_pwr = 0;
            total_consumption = 0;
            Blynk.virtualWrite(V7, kwh);
            static char kwh_char[4];
            dtostrf(kwh, 4, 0, kwh_char);
            PSClient.publish("/CURRENT_RX/KWH", kwh_char);
        }

        PSClient.publish("/CURRENT_TX/STATUS", "LOOP DONE");
    }

    scr.loop();
}

void setup()
{
    Serial.begin(115200);

    scr.begin();

    Serial.println("Connessione al WiFi in corso");
    // WiFi.config(LOCAL_IP, GATEWAY, SUBNET);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("");
    Serial.println("Connesso al WiFi");
    Serial.print("Stato: ");
    Serial.println(WiFi.status());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("Segnale: ");
    Serial.println(WiFi.RSSI());

    Blynk.config(auth);
    while (Blynk.connect() == false) {}
    Serial.println("Connesso a server Blynk");
    timer.setInterval(1000L, TIMER);

    pinMode(WORK_LED, OUTPUT);
    pinMode(WARNING_LED, OUTPUT);
    pinMode(SCR_LED, OUTPUT);

    PSClient.setServer(mqtt_server, 1883);
    PSClient.setCallback(callback);

    timeClient.update();
    hour = timeClient.getHours();
    prev_hour = hour;
}

BLYNK_WRITE(V2)
{
    scr.user_slp_time = param.asInt();
    scr.ledBlink(WORK_LED);
}

// a che potenza si deve accendere lo schermo
BLYNK_WRITE(V3)
{
    scr.user_wake_power = param.asInt();
    scr.ledBlink(WORK_LED);
}

// accendere lo schermo manualmente
BLYNK_WRITE(V4)
{
    scr.user_override = param.asInt();
    if (scr.user_override == 1)
    {
        scr.wake();
        scr.slp_time = 0;
    }
    else
        scr.slp_time = 0;
    scr.ledBlink(WORK_LED);
}

// reset dell'overshoot dei 3kW
BLYNK_WRITE(V6)
{
    int val = param.asInt();
    if (val == 1)
    {
        scr.blynk_overload_time = 0;
        Blynk.virtualWrite(V5, scr.blynk_overload_time);
    }
    scr.ledBlink(WORK_LED);
}
