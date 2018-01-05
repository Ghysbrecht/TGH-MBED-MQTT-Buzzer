
#include "mbed.h"
#include "stdio.h"

#define logMessage pc.printf

#define MQTTCLIENT_QOS2 1

#include "easy-connect.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

int arrivedcount = 0;

Serial pc(USBTX, USBRX);
PwmOut speaker(p26);

DigitalOut led(LED1);

typedef struct {
    int frequency;
} buzzerfreq;


Mail<buzzerfreq, 16> mail_box;

void messageArrived(MQTT::MessageData& md)
{
    led = !led;
    MQTT::Message &message = md.message;
    logMessage("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    logMessage("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
    
    buzzerfreq *buzfreq = mail_box.alloc();
    
    int test;
    sscanf(message.payloadlen,(char*)message.payload, "%d", &test);
    logMessage("Payload INT %i \r\n", test);
    
    buzfreq->frequency = test;
    mail_box.put(buzfreq);    
}

void playFreqThread(){
    while (true) {
        osEvent evt = mail_box.get();
        if (evt.status == osEventMail) {
            buzzerfreq *buzfreq = (buzzerfreq*)evt.value.p;
            printf("\nPlaying frequency: %i\n\r" , buzfreq->frequency);
            speaker.period((float)(1.0/(float)buzfreq->frequency));  // 4 second period
            speaker.write(0.50f);  // 50% duty cycle
            Thread::wait(1000);
            speaker.write(0.0f);  // 50% duty cycle
            printf("Done playing...\n\r");
            mail_box.free(buzfreq);
        }
    }
}



int main(int argc, char* argv[])
{
    pc.baud(9600);
    char* topic = "iot/thomas/buzzer";
    pc.printf("Initing Program");

    NetworkInterface* network = easy_connect(true);
    if (!network) {
        pc.printf("Network failed\r\n");
        return -1;
    }

    MQTTNetwork mqttNetwork(network);

    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    const char* hostname = "141.105.126.62";
    int port = 1883;
    logMessage("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        logMessage("rc from TCP connect is %d\r\n", rc);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "thomasWoefBrecht565676";

    if ((rc = client.connect(data)) != 0)
        logMessage("rc from MQTT connect is %d\r\n", rc);

    if ((rc = client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
        logMessage("rc from MQTT subscribe is %d\r\n", rc);
    
    Thread thread(playFreqThread);
    
    while(1){
        client.yield(100);
    }
}
