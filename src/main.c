#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "easy_board.h"
#include <MQTTClient.h>


#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;

static void msg_delivered(void *context, MQTTClient_deliveryToken dt)
{
    LOG_INFO("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

static int msg_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char *payloadptr;
    LOG_INFO("Message arrived\n");
    LOG_INFO("     topic: %s\n", topicName);
    LOG_INFO("   message: ");

    payloadptr = message->payload;
    for(i = 0; i < message->payloadlen; i++){
        putchar(*payloadptr++);
    }
    putchar('\n');

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

static void conn_lost(void *context, char *cause)
{
    LOG_INFO("\nConnection lost\n");
    LOG_INFO("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions connOpt = MQTTClient_connectOptions_initializer;
    int rc = 0;
    int ch;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    connOpt.keepAliveInterval = 20;
    connOpt.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, conn_lost, msg_arrived, msg_delivered);

    if ((rc = MQTTClient_connect(client, &connOpt)) != MQTTCLIENT_SUCCESS)
    {
        LOG_INFO("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    LOG_INFO("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);
    do
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
