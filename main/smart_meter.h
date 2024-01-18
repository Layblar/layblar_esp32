#ifndef smart_meter__h
#define smart_meter__h

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

#define SIZE_MBUS_START     4
#define SIZE_METADATA       5
#define SIZE_SERVICE        1
#define SIZE_TITLE_LEN      1
#define SIZE_TITLE          8
#define SIZE_APDU_LEN       3
#define SIZE_SECURITY       1
#define SIZE_FRAME_CNT      4
#define SIZE_DATA_1         227
#define SIZE_DATA_2         109
#define SIZE_MBUS_STOP      2

#define LEN_FRAME_1         250
#define LEN_FRAME_2         114
#define LEN_DLSM_FRAME_1    LEN_FRAME_1 - SIZE_METADATA - SIZE_SERVICE - SIZE_TITLE_LEN - SIZE_TITLE - SIZE_APDU_LEN - SIZE_SECURITY - SIZE_FRAME_CNT
#define LEN_DLSM_FRAME_2    LEN_FRAME_2 - SIZE_METADATA
#define LEN_ENCRY           LEN_DLSM_FRAME_1 + LEN_DLSM_FRAME_2
#define LEN_IV              SIZE_TITLE + SIZE_FRAME_CNT

#define OFFSET_DATA_1       SIZE_MBUS_START + SIZE_METADATA + SIZE_SERVICE + SIZE_TITLE_LEN + SIZE_TITLE + SIZE_APDU_LEN + SIZE_SECURITY + SIZE_FRAME_CNT
#define OFFSET_DATA_2       SIZE_MBUS_START + LEN_FRAME_1 + SIZE_MBUS_STOP + SIZE_MBUS_START + SIZE_METADATA

#define POS_1_8 0x09060100010800FF
#define LEN_POS 6

#define BUF_SIZE (1024)

void smart_meter_init();
char* smart_meter_get_json();



#endif