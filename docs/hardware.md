# Documentation of Layblar hardware

The hardware is based on a ESP32-C3 Chip and a M-Bus Transreceiver (TSS721A or NCN5150).

# M-Bus Frames

The Smart Meter sends all 5 seconds informations about the current values. The data is too much for one frame, so there are two m-bus frames in a row. Each frame has a M-Bus Start and Stop part, which the data in between.

## Frame 1

| M-Bus Start | Message 1      | M-Bus Stop |
| ----------- | -------------- | ---------- |
| 4 Byte      | 250 Byte       | 2 Byte     |
| 68FAFA68    | 53FF ... F8BF5 | BD16       |

## Frame 2

| M-Bus Start | Message 2      | M-Bus Stop |
| ----------- | -------------- | ---------- |
| 4 Byte      | 114 Byte       | 2 Byte     |
| 68727268    | 53FF .... 57EB | B616       |

## Message 1

| Metadata   | Cipher Service | Title Length | Title            | Unknown  | Frame Counter | Data 1        |
| ---------- | -------------- | ------------ | ---------------- | -------- | ------------- | ------------- |
| 5 Byte     | 1 Byte         | 1 Byte       | 8 Byte           | 4 Byte   | 4 Byte        | 227 Byte      |
| 53FF000167 | DB             | 08           | 4B464D1020031D00 | 82015521 | 0001C91E      | 8BEE ... 8BF5 |

## Message 2

| Metadata   | Data 2        |
| ---------- | ------------- |
| 5 Byte     | 109 Byte      |
| 53FF110167 | FAF4 ... 57EB |

## Decrypt the messages

The sensible informations are encrypted with AES GMC. A encryption key from VKW is needed. Encryption also needs a Start Vector whis is defined as `Title` + `Frame Counter`.



The code uses `#include "mbedtls/gcm.h"` for decrypting.







# Changelog


## 2023-10-07 ([André Maurer](https://github.com/bouncecom))

- Initial release based on [Espressif example project](https://github.com/espressif/esp-idf/tree/master/examples/protocols/mqtt/tcp).

## 2023-10-05 ([André Maurer](https://github.com/bouncecom))

- Selected hardware parts for prototyp
- Sended BoM for ordering
- <a href="https://layblar.github.io/layblar_esp32/img/order_05102023.png" target="_blank">Shopping List</a>