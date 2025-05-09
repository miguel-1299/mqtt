#include <Arduino.h>
#include <PubSubClient.h>
#include <mqtt.h>
#include <button.h>

// use the button for forcing a message (for testing)
Button bootButton(0);

/**
 * subscriptions is passed to the connection method so that the client will
 * re-establish your subscriptions if the cxn is dropped.
 * 
 * By default, we subscribe to all of the messages for the team, but you may
 * want to edit the list to avoid overwhelming the cxn.
*/
String subscriptions[] = 
{
    String("/#")
};

/**
 * Check for input on Serial (and Serial2 in the next function). Ignores \r. Terminates on \n.
*/
String rxString;
bool checkSerial(void)
{
    while(Serial.available())
    {
        char c = Serial.read();
        if(c == '\n') return true;
        else if (c != '\r') rxString += c;
    }

    return false;
}

String rx2String;
bool checkSerial2(void)
{
    while(Serial2.available())
    {
        char c = Serial2.read();
        if(c == '\n') return true;
        else if (c != '\r') rx2String += c;
    }

    return false;
}

/**
 * publishMQTT parses a message (received as "topic:message" over Serial) 
 * and appends the topic/message to the team number and sends it to an MQTT broker.
 * The MQTT packet will have the format
 * 
 *      "teamN/topic/message"
 * 
 * */
bool publishMQTT(String& str)
{
    // for debugging; comment out if you don't need this
    Serial.println(str);

    /**
     * On the Serial, messages must be formatted topic:msg. If we don't find a colon, then 
     * ignore the message.
    */
    int iColon = str.indexOf(':');
    if(iColon == -1) 
    {
        Serial.println("Failed to find delimiter.");
        str = "";
        return false;
    }

    String topic = str.substring(0, iColon);
    String message = str.substring(iColon + 1);

    bool success = client.publish(topic.c_str(), message.c_str());
    str = "";

    return success;
}

/**
 * callback() gets called whenever we receive a message for this team  
 * (i.e., "teamN"). It strips off the team name and sends "topic:message"
 * over the UART.
 * */
void callback(char* topic, byte *payload, unsigned int length) 
{
    // These two lines can be commented out; they are used for testing and don't affect functionality
    Serial.print("Full topic: ");
    Serial.println(topic);

    String strTopic(topic);

    // This prints to the Serial monitor
    Serial.print(strTopic.substring(strTopic.indexOf('/') + 1));
    Serial.print(':');  
    Serial.write(payload, length);
    Serial.println();

    // This prints to Serial2, which can be connected to another uC
    Serial2.print(strTopic.substring(strTopic.indexOf('/') + 1));
    Serial2.print(':');  
    Serial2.write(payload, length);
    Serial2.println();
}

void setup() 
{
    Serial.begin(115200);
    delay(100);  //give us a moment to bring up the Serial

    Serial.println("setup()");

    Serial2.begin(115200);

    /**
     * Sets the callback function that gets called for every message received from the broker.
    */
    client.setCallback(callback);

    /**
     * Using button class, so must call init()
    */
    bootButton.init();

    Serial.println("/setup()");
}

void loop() 
{
    // mqtt_reconnect() tests for a cxn and reconnects, if needed
    if(!client.loop()) {mqtt_reconnect(subscriptions, sizeof(subscriptions)/sizeof(String));}
    
    /**
     * Receives input on both Serial and Serial2. 
    */
    if(checkSerial()) publishMQTT(rxString);
    if(checkSerial2()) publishMQTT(rx2String);

    // For testing connectivity:
    if(bootButton.checkButtonPress()) {String bStr("button0:pressed"); publishMQTT(bStr);}    
}