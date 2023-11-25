# Documentation of Layblar hardware

The hardware is based on a ESP32-C3 Chip and a M-Bus Transreceiver (TSS721A or NCN5150).

# M-Bus Frames

The Smart Meter sends all 5 seconds informations about the current values. The data is too much for one frame, so there are two m-bus frames in a row. Each frame has a M-Bus Start and Stop part, which the data in between.

## Frame 1

| M-Bus Start | Message 1 | M-Bus Stop |
| ----------- | --------- | ---------- |
| 4 Byte      | 250 Byte  | 2 Byte     |

## Frame 2

| M-Bus Start | Message 2 | M-Bus Stop |
| ----------- | --------- | ---------- |
| 4 Byte      | 114 Byte  | 2 Byte     |

## M-Bus Start

| Start     | Length                     | Length                     | Start     |
| --------- | -------------------------- | -------------------------- | --------- |
| 1 Byte    | 1 Byte                     | 1 Byte                     | 1 Byte    |
| <u>68</u> | FA (Frame 1), 72 (Frame 2) | FA (Frame 1), 72 (Frame 2) | <u>68</u> |

## M-Bus Stop

| Checksum | End       |
| -------- | --------- |
| 1 Byte   | 1 Byte    |
| B6       | <u>16</u> |

## Message 1

| Metadata          | Service   | Title Length | Title                   | APDU Length   | Security  | Frame Counter | Data 1        |
| ----------------- | --------- | ------------ | ----------------------- | ------------- | --------- | ------------- | ------------- |
| 5 Byte            | 1 Byte    | 1 Byte       | 8 Byte                  | 3 Byte        | 1 Byte    | 4 Byte        | 227 Byte      |
| <u>53FF000167</u> | <u>DB</u> | <u>08</u>    | <u>4B464D1020031D00</u> | <u>820155</u> | <u>21</u> | 0001C91E      | 8BEE ... 8BF5 |

## Message 2

| Metadata          | Data 2        |
| ----------------- | ------------- |
| 5 Byte            | 109 Byte      |
| <u>53FF110167</u> | FAF4 ... 57EB |

<u>Underlined values mean that these values are allways the same.</u>

## Decrypt the messages

The sensible informations are encrypted with AES GCM. A encryption key from Smart Meter provider VKW is needed. Encryption also needs a Start Vector whis is defined as `Title` + `Frame Counter`.



The code uses `#include "mbedtls/gcm.h"` for decrypting.







# Changelog

## 2023-11-24 ([André Maurer](https://github.com/bouncecom))

- Reverse engineered data frame format as there is no good documentation from vkw (other provider has some documentation as orientation)
- Added documentation.
- Refactored data frame structure using #define instead of size_t for better data handling.
- Error search of failing decryption. 

## 2023-11-11 ([André Maurer](https://github.com/bouncecom))

- Got evaluation board running (proof of concept).
- Got correct signals form evaluation board via UART.
- Started implementation of decryption.


## 2023-10-07 ([André Maurer](https://github.com/bouncecom))

- Initial release based on [Espressif example project](https://github.com/espressif/esp-idf/tree/master/examples/protocols/mqtt/tcp).

## 2023-10-05 ([André Maurer](https://github.com/bouncecom))

- Selected hardware parts for prototyp
- Sended BoM for ordering
- <a href="https://layblar.github.io/layblar_esp32/img/order_05102023.png" target="_blank">Shopping List</a>