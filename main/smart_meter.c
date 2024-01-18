
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "mbedtls/gcm.h"
#include "cjson/cJSON.h"
#include "smart_meter.h"

static const char *TAG_SMART_METER = "SMART_METER";
bool flag_new_json = false;

char* json_str = NULL;
extern QueueHandle_t xQueue1;
extern char* serial_number;

struct uInteger16{  // len = 19 Byte
    uint8_t pos[2+6+1];
    uint8_t value[2];
    uint8_t prescale[3]; // 02020F
    uint8_t scale; // FF = *10^-1, FE = *10^-2, FD = *10^-3, 00 = noscale
    uint8_t u2[4];
};

struct uInteger32{  // len = 21 Byte
    uint8_t pos[2+6+1];
    uint8_t value[4];
    uint8_t prescale[3]; // 02020F
    uint8_t scale; // FF = *10^-1, FE = *10^-2, FD = *10^-3, 00 = noscale
    uint8_t u2[4];
};

struct uInteger32_last{  // len = 21 - 2 Byte (workaround: last two bytes are missing)
    uint8_t pos[2+6+1];
    uint8_t value[4];
    uint8_t prescale[3];
    uint8_t scale;
    uint8_t u2[2];
};

struct dlms_frame{
    uint8_t first[6];
    uint8_t time_stamp[12];
    uint8_t u1[2];
    uint8_t pos_1_0[2+6+2]; // 0906 + <!--0.0.1.0.0.255--> + cnt
    uint8_t value_1_0[12];
    uint8_t u2[2];
    uint8_t pos_96_1[2+6+2]; // 0906 + <!--0.0.96.1.0.255--> + cnt
    uint8_t value_96_1[14];
    uint8_t u3[2];
    uint8_t pos_42_0[2+6+2]; // 0906 + <!--0.0.42.0.0.255--> + cnt
    uint8_t value_42_0[16]; // bis hier 96 Byte
    uint8_t u4[2];   
    struct uInteger16 pos_32_7;
    struct uInteger16 pos_52_7;
    struct uInteger16 pos_72_7;
    struct uInteger16 pos_31_7;
    struct uInteger16 pos_51_7;
    struct uInteger16 pos_71_7;
    struct uInteger32 pos_1_7;
    struct uInteger32 pos_2_7;
    struct uInteger32 pos_1_8;
    struct uInteger32 pos_2_8;
    struct uInteger32 pos_3_8;
    struct uInteger32_last pos_4_8;

};

static uint16_t get_uint16(uint8_t* array){
    return (array[0] << 8) + array[1];
}

static uint32_t get_uint32(uint8_t* array){
    return (array[0] << 24) + (array[1] << 16) +  (array[2] << 8) + array[3];
}

static void add_value_to_json(cJSON* json, char* name, uint32_t value, uint16_t scale){
    int div = 1;
    switch(scale){
        case 0x00: div = 1; break;
        case 0xFF: div = 10; break;
        case 0xFE: div = 100; break;
        case 0xFD: div = 1000; break;
        default: ESP_LOGW(TAG_SMART_METER, "UNKNOWN SCALE"); break;
    }
    cJSON_AddNumberToObject(json, name, (double)value/div); 
}


static void smart_meter_task(void *arg)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    uint8_t *encry = (uint8_t *) malloc(BUF_SIZE / 2);
    uint8_t *decry = (uint8_t *) malloc(BUF_SIZE / 2);
    uint8_t *iv = (uint8_t *) malloc(BUF_SIZE / 2);
    // char *json_str = (char *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        size_t len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 2000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG_SMART_METER, "Readed bytes");
        if (len) {
            ESP_LOGI(TAG_SMART_METER, "length of bytes more than 0 with byte0: %d", data[0]);


            if (data[0] == 0x68){

                size_t len_frame1 = data[1];
                size_t len_frame2 = data[SIZE_MBUS_START + len_frame1 + SIZE_MBUS_STOP + 1];
                if(len_frame1 == LEN_FRAME_1 && len_frame2 == LEN_FRAME_2){
                    // for(int i = 0; i < (SIZE_MBUS_START + LEN_FRAME_1 + SIZE_MBUS_STOP + SIZE_MBUS_START + LEN_FRAME_2 + SIZE_MBUS_STOP); i++){
                    //     printf("%02hhX", data[i]);
                    // }
                    // printf("\n");
                    ESP_LOGI(TAG_SMART_METER, "CORRECT FRAME LENGTHS DETECTED !!!");
                    // printf("len dlsm frame 1: %d\n", LEN_DLSM_FRAME_1);
                    // printf("len dlsm frame 2: %d\n", LEN_DLSM_FRAME_2);

                    // ***** GET encry *****
                    memcpy((char *) encry,(char *) (data + OFFSET_DATA_1), LEN_DLSM_FRAME_1);
                    memcpy((char *) (encry + LEN_DLSM_FRAME_1),(char *) (data + OFFSET_DATA_2), LEN_DLSM_FRAME_2);
                    // for(int i = 0; i < LEN_ENCRY; i++){
                    //     printf("%02hhX", encry[i]);
                    // }
                    // printf("\n");

                    // ***** GET iv *****
                    size_t apdu_len_len = 1;
                    switch(data[SIZE_MBUS_START + SIZE_METADATA + SIZE_SERVICE + SIZE_TITLE_LEN + SIZE_TITLE]){
                        case 0x80: apdu_len_len = 1; break;
                        case 0x81: apdu_len_len = 2; break;
                        case 0x82: apdu_len_len = 3; break;

                    }
                    memcpy((char *) iv,(char *) (data + SIZE_MBUS_START + SIZE_METADATA + SIZE_SERVICE + SIZE_TITLE_LEN), SIZE_TITLE);
                    memcpy((char *) (iv + SIZE_TITLE),(char *) (data + SIZE_MBUS_START + SIZE_METADATA + SIZE_SERVICE + SIZE_TITLE_LEN + SIZE_TITLE + apdu_len_len + 1), SIZE_FRAME_CNT);
                    // for(int i = 0; i < LEN_IV; i++){
                    //     printf("%02hhX", iv[i]);
                    // }
                    // printf("\n");

                    uint8_t key[16] = {0x32, 0x69, 0x31, 0x63, 0x79, 0x79, 0x45, 0x6C, 0x59, 0x37, 0x34, 0x44, 0x73, 0x6D, 0x33, 0x75};

                    ESP_LOGI(TAG_SMART_METER, "len encry: %d", LEN_ENCRY);
                    ESP_LOGI(TAG_SMART_METER, "len iv: %d", LEN_IV);
                    

                    mbedtls_gcm_context aes;
                    // init the context...
                    mbedtls_gcm_init( &aes );
                    // Set the key. This next line could have CAMELLIA or ARIA as our GCM mode cipher...
                    int ret0 = mbedtls_gcm_setkey( &aes, MBEDTLS_CIPHER_ID_AES, (const unsigned char*) key, 128);
                    ESP_LOGI(TAG_SMART_METER, "ret0: %d", ret0);
                    // Initialise the GCM cipher...
                    mbedtls_gcm_starts(&aes, MBEDTLS_GCM_DECRYPT, (const unsigned char*)iv, LEN_IV);
                    size_t decry_len = 0;
                    int ret = mbedtls_gcm_update(&aes, (unsigned char*)encry, LEN_ENCRY, (unsigned char*)decry, 512, &decry_len);
                    // mbedtls_gcm_finish(&aes, (unsigned char*) auth_tag, 4, (unsigned char*)rest, 15, &rest_len);
                    ESP_LOGI(TAG_SMART_METER, "ret: %d", ret);
                    mbedtls_gcm_free( &aes );
                    // for(int i = 0; i < decry_len; i++){
                    //     printf("%02hhX", decry[i]);
                    // }
                    // printf("\n");
                    ESP_LOGI(TAG_SMART_METER, "decry_len: %d", decry_len);

                    // Search 
                    struct dlms_frame* frame1 = decry;
                    // for(int i = 0; i < sizeof(frame1->pos_42_0); i++){
                    //     printf("%02hhX", frame1->pos_42_0[i]);
                    // }
                    // printf("\n");
                    // for(int i = 0; i < sizeof(frame1->pos_32_7.value); i++){
                    //     printf("%02hhX\n", frame1->pos_32_7.value[i]);
                    // }
                    // for(int i = 0; i < sizeof(frame1->pos_31_7.value); i++){
                    //     printf("%02hhX\n", frame1->pos_31_7.value[i]);
                    // }
                    // printf("pos_32_7: %d\n", get_uint16(frame1->pos_32_7.value));
                    // printf("pos_52_7: %d\n", get_uint16(frame1->pos_52_7.value));
                    // printf("pos_72_7: %d\n", get_uint16(frame1->pos_72_7.value));
                    // printf("pos_31_7: %d\n", get_uint16(frame1->pos_31_7.value));

                    // copy serial number
                    if(serial_number == NULL){
                        // char serial[20];
                        char* serial = malloc(sizeof(char[20]));
                        uint8_t* array_ser = frame1->value_96_1;
                        sprintf(serial, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c", array_ser[0], array_ser[1], array_ser[2], array_ser[3], array_ser[4], array_ser[5], array_ser[6], array_ser[7], array_ser[8], array_ser[9], array_ser[10], array_ser[11], array_ser[12], array_ser[13]);
                        serial_number = serial;
                        printf("Device %s found\n", serial_number);
                    }

                    // create a cJSON object 
                    cJSON *json = cJSON_CreateObject(); 
                    add_value_to_json(json, "1.7.0", get_uint32(frame1->pos_1_7.value), frame1->pos_1_7.scale);
                    add_value_to_json(json, "1.8.0", get_uint32(frame1->pos_1_8.value), frame1->pos_1_8.scale);
                    add_value_to_json(json, "2.7.0", get_uint32(frame1->pos_2_7.value), frame1->pos_2_7.scale);
                    add_value_to_json(json, "2.8.0", get_uint32(frame1->pos_2_8.value), frame1->pos_2_8.scale);
                    add_value_to_json(json, "3.8.0", get_uint32(frame1->pos_3_8.value), frame1->pos_3_8.scale);
                    add_value_to_json(json, "4.8.0", get_uint32(frame1->pos_4_8.value), frame1->pos_4_8.scale);
                    add_value_to_json(json, "32.7.0", get_uint16(frame1->pos_32_7.value), frame1->pos_32_7.scale);
                    add_value_to_json(json, "52.7.0", get_uint16(frame1->pos_52_7.value), frame1->pos_52_7.scale);
                    add_value_to_json(json, "72.7.0", get_uint16(frame1->pos_72_7.value), frame1->pos_72_7.scale);
                    add_value_to_json(json, "31.7.0", get_uint16(frame1->pos_31_7.value), frame1->pos_31_7.scale);
                    add_value_to_json(json, "51.7.0", get_uint16(frame1->pos_51_7.value), frame1->pos_51_7.scale);
                    add_value_to_json(json, "71.7.0", get_uint16(frame1->pos_71_7.value), frame1->pos_71_7.scale);
                    uint8_t* array = frame1->time_stamp;
                    char timestamp[30];
                    sprintf(timestamp, "%d-%02d-%02dT%02d:%02d:%02d", (array[0] << 8) + array[1], array[2], array[3], array[5], array[6], array[7]);
                    cJSON_AddStringToObject(json, "timestamp", timestamp);
                    
                    // convert the cJSON object to a JSON string
                    if(json_str != NULL){
                        cJSON_free(json_str); 
                    }
                    json_str = cJSON_Print(json);
                    if(xQueueSend( xQueue1, ( void * ) &json_str, ( TickType_t ) 0 ) == errQUEUE_FULL){
                        ESP_LOGI(TAG_SMART_METER, "Failed to send to queue");
                    }
                    else{
                        ESP_LOGI(TAG_SMART_METER, "Sent to queue successfully");
                    }
                    flag_new_json = true;
                    // printf("%s\n", json_str); 
                    // free the JSON string and cJSON object 
                    cJSON_Delete(json); 

                }
            }
        }
    }
}

void smart_meter_init(){
        /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    xQueue1 = xQueueCreate( 5, sizeof(char*) );
    if( xQueue1 != NULL ){
        ESP_LOGI(TAG_SMART_METER, "Queue created");
    }

    BaseType_t xReturned;
    xReturned = xTaskCreate(smart_meter_task, "uart_echo_task", ECHO_TASK_STACK_SIZE*2, NULL, 10, NULL);
    if( xReturned == pdPASS ){
        ESP_LOGI(TAG_SMART_METER, "smart_meter_task created");
    }
}

char* smart_meter_get_json(){
    while(!flag_new_json){
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    flag_new_json = false;
    return json_str;
}
