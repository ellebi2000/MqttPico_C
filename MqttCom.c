#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"

// Credenziali WiFi
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWD"
// Credenziali MQTT
#define PORT  1883
#define MQTT_CLIENT_ID "clientID"
#define MQTT_BROKER_IP "YOUR_BROKER_IP" 
// LED da controllare
#define V_PIN 16
#define G_PIN 17
#define R_PIN 18
#define B_PIN 19
//BOTTONI
#define B_V_PIN 15

typedef struct {
    mqtt_client_t* mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    uint8_t data[MQTT_OUTPUT_RINGBUF_SIZE];
    uint8_t topic[100];
    uint32_t len;
} MQTT_CLIENT_DATA_T;

MQTT_CLIENT_DATA_T *mqtt;

void control_led(uint gpio_pin , bool on) {
    gpio_put(gpio_pin, on ? 1 : 0);
    // Pubblica lo stato del LED su un topic specifico
    const char* message = on ? "On" : "Off";
    char state_topic[128];
    snprintf(state_topic, sizeof(state_topic), "%s/stato", mqtt->topic);
    mqtt_publish(mqtt->mqtt_client_inst, state_topic, message, strlen(message), 0, 0, NULL, NULL);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(mqtt_client->data, data, len);
    mqtt_client->len = len;
    mqtt_client->data[len] = '\0';
    //Stampo i messaggi e  i topic 
    printf("Topic: %s, Messaggio: %s\n", mqtt_client->topic, mqtt_client->data);


    if (strcmp(mqtt->topic, "/verde") == 0)
    {
        if (strcmp((const char *)mqtt_client->data, "On") == 0)
            control_led(V_PIN,true);
        else if  (strcmp((const char *)mqtt_client->data, "Off") == 0)
            control_led(V_PIN, false);
    }
    else if (strcmp(mqtt->topic, "/giallo") == 0)
    {
        if (strcmp((const char *)mqtt_client->data, "On") == 0)
            control_led(G_PIN,true);
        else if  (strcmp((const char *)mqtt_client->data, "Off") == 0)
            control_led(G_PIN, false); 
    } 
    else if (strcmp(mqtt->topic, "/rosso") == 0)
    {
        if (strcmp((const char *)mqtt_client->data, "On") == 0)
            control_led(R_PIN,true);
        else if  (strcmp((const char *)mqtt_client->data, "Off") == 0)
            control_led(R_PIN, false); 
    } 
    else if (strcmp(mqtt->topic, "/blu") == 0)
    {
        if (strcmp((const char *)mqtt_client->data, "On") == 0)
            control_led(B_PIN,true);
        else if  (strcmp((const char *)mqtt_client->data, "Off") == 0)
            control_led(B_PIN, false); 
    } 
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
    strcpy(mqtt_client->topic, topic);
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        mqtt_sub_unsub(client, "/verde", 0, NULL, arg, 1);
        printf("Connesso al topic /verde con successo\n");
        mqtt_sub_unsub(client, "/giallo", 0, NULL, arg, 1);
        printf("Connesso al topic giallo con successo\n");
        mqtt_sub_unsub(client, "/rosso", 0, NULL, arg, 1);
        printf("Connesso al topic rosso con successo\n");
        mqtt_sub_unsub(client, "/blu", 0, NULL, arg, 1);
        printf("Connesso al topic blu con successo\n");
    }
}

int main() {

    stdio_init_all();
    gpio_init(V_PIN); gpio_set_dir(V_PIN, GPIO_OUT);
    gpio_init(G_PIN); gpio_set_dir(G_PIN, GPIO_OUT);
    gpio_init(R_PIN); gpio_set_dir(R_PIN, GPIO_OUT);
    gpio_init(B_PIN); gpio_set_dir(B_PIN, GPIO_OUT);

    mqtt = (MQTT_CLIENT_DATA_T*)calloc(1, sizeof(MQTT_CLIENT_DATA_T));
    if (!mqtt) {
        printf("Errore inizializzazione client MQTT\n");
        return 1;
    }
    // Imposta le informazioni del client MQTT
    mqtt->mqtt_client_info.client_id = MQTT_CLIENT_ID;
    mqtt->mqtt_client_info.keep_alive = 60; // Keep alive in secondi

    if (cyw43_arch_init()) {
        printf("Errore inizializzazione CYW43\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Errore connessione Wi-Fi\n");
        return 1;
    }
    printf("\nConnesso alla rete wifi\n");

    ip_addr_t addr;
    if (!ip4addr_aton(MQTT_BROKER_IP, &addr)) {
        printf("Indirizzo IP del server MQTT non valido\n");
        return 1;
    }

    mqtt->mqtt_client_inst = mqtt_client_new();
    if (!mqtt->mqtt_client_inst) {
        printf("Errore creazione istanza client MQTT\n");
        return 1;
    }
    printf("Connessione riuscita al broker MQTT\n");

    mqtt_set_inpub_callback(mqtt->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, mqtt);

    if (mqtt_client_connect(mqtt->mqtt_client_inst, &addr, PORT, mqtt_connection_cb, mqtt, &mqtt->mqtt_client_info) != ERR_OK) {
        printf("Errore connessione al broker MQTT\n");
        return 1;
    }

    while (1) {
        tight_loop_contents();
    }

    return 0;
}