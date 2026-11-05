# NFC Door Lock — PTX105R

A ready-to-run, hardware-independent NFC card reader for Renesas RA microcontrollers with servo functionality to simulate a doorlock, built on the
PTX105R NFC front-end. The NFC workflow — discovery, activation, NDEF read, callback dispatch and
deactivation — is fully encapsulated in a reusable module with a two-function public API, so the
application never touches the PTX SDK.

## Quickstart

![NFC Doorlock System Block Diagram](./images/NFC_Doorlock_System_BlockDiagram.jpg)

Connect the SG90 servomotor FPB-RA2E3: 

* Servo red wire to +5V at J2-5

* Servo brown wire to GND at J2-6

* Servo Yellow Wire to P301 at PMOD2, pin 10

Connect the PTX105R PMOD to PMOD1.

Connect the USB micro to CN6.

See section [Build and Run](#build-and-run).

---

## Table of Contents

1. [Overview](#overview)
2. [Requirements](#requirements)
3. [Build and Run](#build-and-run)
4. [Usage](#usage)
5. [Architecture](#architecture)
7. [Error Handling](#error-handling)
8. [Troubleshooting](#troubleshooting)
9. [References](#references)

---

## Overview

Once running, the firmware continuously polls for NFC cards. On each detection it identifies the card
UID, performs a checksum on the UID, then determines of the checksum is ODD or EVEN. Depending on the
NFC_AUTH_POLARITY_ODD flag, the software will either grant access via the servo or deny access by doing nothing.


### Supported Tags

| NFC Standard | Tag Type | Example Cards |
|---|---|---|
| ISO 14443-A | Type 2 (T2T) | MIFARE Ultralight, NTAG21x |
| ISO 14443-A | Type 4A (T4A) | MIFARE DESFire, ISO-DEP |
| ISO 14443-B | Type 4B (T4B) | ISO 14443-4 Type B cards |
| NFC-F / FeliCa | Type 3 (T3T) | FeliCa Lite, FeliCa Standard |
| ISO 15693 | Type 5 (T5T) | NFC-V tags (ICODE SLIX, …) |

NDEF read is supported for all five tag types.


---

## Requirements

### Tools

| Tool | Notes |
|---|---|
| VSCode | The Code IDE and extensions |
| Renesas FSP **6.5.0** | Supplies the BSP, drivers and FreeRTOS port |
| RA Smart Configurator (RASC) 6.5.0 | Only for command-line (non-e² studio) generation of `ra/`, `ra_gen/`, `ra_cfg/` |
| Arm Toolchain for Embedded (LLVM) 21.1.1 | `clang` / `ld.lld` with picolibc — `cmake/llvm.cmake` |
| Serial terminal | Optional, for the demo console output |


### MCU Peripherals

The PTX105R integration is MCU-agnostic, but the target device must be able to provide:

| Resource | Purpose |
|---|---|
| SPI-capable channel (SCI in SPI mode) | PTX105R host interface — master, 1 MHz, CPOL=0, CPHA=0, MSB first |
| GPIO output | Chip select (active-low, software driven) |
| External IRQ-capable pin | Rising-edge interrupt from the PTX105R |
| DTC/DMAC channels (TX + RX) | Zero-CPU SPI transfers |
| 2 × GPT timer channels | PTX timing services |
| Regulated 3.3 V rail | PTX105R supply |
| UART channel *(optional)* | Console/log output only |
| 16-bit timer | Servo operation |

> [!IMPORTANT]
> The NFC module itself has **no UART dependency** and runs headless. UART is required only by the
> demo application's logging.

### Reference Configuration

The `configuration.xml` shipped with this project is generated for the following setup. It is a
starting point, not a constraint — re-target it to your own device and pin map as described in
[Porting to Your Own Hardware](#porting-to-your-own-hardware).

| Item | Value |
|---|---|
| Board | FPB-RA2E3 |
| Device / core | `R7FA2E3073CFL` · Arm Cortex-M23 (`CM23`) |
| FSP | 6.5.0 |
| Toolchain | LLVM 21.1.1, picolibc |
| RTOS | FreeRTOS 11.1.0 (FSP `rm_freertos_port`), static + dynamic allocation, `heap_4` |
| SPI | SCI0 SPI master (`ptx_pmod_spi`) · 1 MHz · CPOL=0 · CPHA=0 · MSB first |
| PTX IRQ | External IRQ7 on P015, rising edge |
| Console UART | `g_uart_jlob_vcom` (SCI9) · 230400 8N1 |
| Linker script | `script/fsp.lld` (LLVM `ld.lld`) |

---

## Build and Run

### 1. Open the project

The project is an **VSCode CMake project**. Install the following Renesas extensions in VSCode:

* Renesas Platform

* Renesas Debug

* Renesas Built Utilities

* (optional) Renesas Memory Usage View

* (optional) Renesas Smart Browser

* (optional) Renesas Smart Manual

Download the project and use VSCode to open the project's code-workspace file.

If you have not already done so, you will need to navigate to the Renesas extensions 
toolbar > Manage Built Tools to install any
relevant build tools, such as FSP, ninja, the LLVM compiler, and CMake.

You will need to run 'CMake: Configure' to select the LLVM compiler for the project.

In the CMake toolbar, run the 'configure' job, followed by the 'build' job.


### 4. Flash and debug

Debug with the on-board J-Link (FPB-RA2E3) using the e² studio debug launch configuration, or the
bundled VS Code / Theia launch configurations (`.vscode/launch.json`, `.theia/launch.json`, target
device `R7FA2E307`). Point the launch configuration's program/image at
`build/<config>/NFC_Reader_PTX105R.elf`. Execution reaches the application through:

```text
main()                       ← FSP generated
└─ new_thread0_entry()       ← FreeRTOS thread
   └─ app_nfc_reader_entry()
      └─ rs_nfc_reader_Read()  ← spawns the "RS_NFC" worker task
   └─ app_thread_entry()       ← runs the applicatioh task for this demo
```

### 5. Observe

If the console UART is enabled, open a serial terminal at the configured baud (230400 8N1 by
default). On startup:

```text
System Initialization (RS NFC Reader) ... starting
RS NFC Reader launched (non-blocking)
```

Present a card to the antenna:

```text
CARD DETECTED! type=NFC-T4A UID=02840017810196 NDEF=15B
RF Technology  : TYPE A
Tag Type       : ISO-DEP (Type 4 Tag / ISO 14443-4)
Serial Number  : 02:84:00:17:81:01:96
Size           : 8192 bytes
Writeable      : Yes
NDEF           : 15 bytes
  NDEF raw (15 bytes): D1 01 0B 54 02 65 6E 59 4F 20 45 4C 49 4E 41
```

Optional LED feedback is compiled in via `APP_NFC_READER_LED_EN` in `src/app/app_nfc_reader.h`; both
configured LEDs are driven high while a card is being processed — comment the macro out to remove the
LED code entirely.

Upon reading an NFC card with the configured ODDNESS or EVENNESS (as determined by the 
NFC_AUTH_POLARITY_ODD flag), the servo motor will move 180° briefly, 
and then return to its original position.

---

## Usage

### Modifying the project

The following code is from app_thread_entry.c, which is responsible for the main application, including
NFC card authorization, servo positions, duration of the door lock open state, and timeout period after
door lock closing.

```c
#include "app_thread.h"
#include "r_gpt.h"
#include "rs_servo_motor/rs_servo.h"

#define ERR_CHECK(value)    if (value)  { __BKPT(0);  }

#define NFC_AUTH_POLARITY_ODD   false  //set to "true" if ODD NFC cards will open the door, else EVEN will
#define SERVO_OPEN_POSITION     -90         //ensure this is within the physical parameters in servo_device_cfg_t
#define SERVO_LOCKED_POSITION   90        //ensure this is within the physical parameters in servo_device_cfg_t
#define SERVO_OPEN_TIME_MS      2500u     //desired time for door to remain unlocked upon successful scan
#define SERVO_CLOSED_RESPONSE_MS 1u

extern uint8_t uid_is_odd;

//Servo specific physical parameters, this is set to the SG90
const servo_device_cfg_t g_sg90_motor_cfg =
{
    .minimum_angle = -90,
    .maximum_angle = 90,
    .minimum_microseconds = 500,
    .maximum_microseconds = 2500,
    .direction = SERVO_DIRECTION_DEFAULT
};
servo_ctrl_t g_sg90_servo_ctrl = {0};


/* App Thread entry function */
/* pvParameters contains TaskHandle_t */
void app_thread_entry(void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    volatile fsp_err_t err = FSP_SUCCESS;  //initialize error flag


    //open the servo and initialize the servo timer
    err = rs_servo_Open(&g_sg90_servo_ctrl, &g_sg90_motor_cfg);
    ERR_CHECK(err);


    //Move servo to each extreme of the user set range
    err = rs_servo_WriteAngle(&g_sg90_servo_ctrl, SERVO_OPEN_POSITION); //check open position
    ERR_CHECK(err);
    vTaskDelay(SERVO_OPEN_TIME_MS);

    err = rs_servo_WriteAngle(&g_sg90_servo_ctrl, SERVO_LOCKED_POSITION); //check closed position
    ERR_CHECK(err);
    vTaskDelay(SERVO_OPEN_TIME_MS);


    while(1)
    {
        xSemaphoreTake(g_uid_detection_semaphore, portMAX_DELAY); //Wait for NFC uid detection semaphore
        
        #if( true == NFC_AUTH_POLARITY_ODD ) //If the doorlock is set to open on ODD nubmered UID's
        if( uid_is_odd == true)
        {
            err = rs_servo_WriteAngle(&g_sg90_servo_ctrl, SERVO_OPEN_POSITION);
            ERR_CHECK(err);
            vTaskDelay(SERVO_OPEN_TIME_MS);
        }
        #else //If the doorlock is set to open on EVEN nubmered UID's
        if( uid_is_odd == false)
        {
            err = rs_servo_WriteAngle(&g_sg90_servo_ctrl, SERVO_OPEN_POSITION);
            ERR_CHECK(err);
            vTaskDelay(SERVO_OPEN_TIME_MS);
        }
        #endif

        //return to locked position
        err = rs_servo_WriteAngle(&g_sg90_servo_ctrl, SERVO_LOCKED_POSITION);
        ERR_CHECK(err);
        vTaskDelay(SERVO_OPEN_TIME_MS);

        xQueueReset(g_uid_detection_semaphore); //clear any duplicate NFC scans
    }

}
```

## Architecture

### Layers

| Layer | Responsibility | Key files |
|---|---|---|
| Application | Product behaviour — logging, LEDs, when to read | `src/app/`, `src/new_thread0_entry.c` |
| NFC reader module | NFC state machine: discover → activate → read → callback → deactivate | `src/rs-nfc-card-reader/rs_nfc_reader.c`, `rs_ndef_read.c` |
| PTX105R adapter | Translates module calls into PTX SDK operations | `src/rs-nfc-card-reader/ptx105r/` |
| PTX SDK + FSP drivers | NFC protocol engine and MCU peripheral access | `ptxIoTRd_*`, `ptxPLAT_*`, SCI SPI, external IRQ, GPT, DTC |
| Hardware | RF front-end and MCU | PTX105R + RA MCU |


One runtime cycle: the application starts the reader, the worker task configures and starts
discovery, blocks on the reader IRQ, activates the detected card, reads card info and NDEF, fires
`on_card_event`, deactivates the card and returns to discovery.


> [!CAUTION]
> Never hand-edit `ra/`, `ra_gen/` or `ra_cfg/`. They are regenerated whenever `configuration.xml`
> is saved or `cmake/prebuild.cmake` runs.

### FreeRTOS Tasks

FreeRTOS is required for non-blocking operation, because `rs_nfc_reader_Read()` creates and runs the
`RS_NFC` worker task.

| Task | Created by | Stack | Priority | Role |
|---|---|---|---|---|
| `new_thread0` | FSP (static) | 1 KB | 1 | Calls `app_nfc_reader_entry()`, then suspends |
| `app_thread` | FSP (static) | 1 KB | 2 | Initializes servo lib, activates door lock upon NFC |
| `RS_NFC` | `rs_nfc_reader_Read()` | 4 KB (static) | 1 | Runs the full NFC event loop; self-deletes on stop |
| `IDLE` | FreeRTOS kernel | 512 B | 0 | Kernel idle task |

> [!NOTE]
> All task stacks are statically allocated — `configSUPPORT_STATIC_ALLOCATION` must be `1`. Do not
> reduce the `RS_NFC` stack below 4096 bytes; protocol and NDEF paths are deep.

---

## Error Handling

The module reports status through the `rs_status_t` enum in `rs_nfc_reader.h`.

| Status | Cause | Recovery |
|---|---|---|
| `RS_OK` | Success | — |
| `RS_ERR_TIMEOUT` | Card removed, or the polling window expired | Continue polling / retry |
| `RS_ERR_INVALID_CFG` | Malformed `rs_nfc_reader_cfg_t` | Fix the offending fields; keep `cfg_valid_check_en = true` |
| `RS_ERR_DEPENDENCY` | Lower PTX105R stack not ready, or `Stop()` called with no read in progress | Verify the FSP stack configuration |
| `RS_ERR_NOT_FOUND` | Card is not NDEF-formatted | Log and continue polling |
| `RS_ERR_BUFFER_OVERFLOW` | NDEF message larger than `max_ndef_bytes` | Increase `max_ndef_bytes` (≤ `RS_NFC_NDEF_MAX_BYTES`) |
| `RS_ERR_INTERNAL` | Second non-blocking `Read()` while one is active, duplicate `Stop()`, or internal fault | Stop the running read first; reset the PTX105R if it persists |

---

## Troubleshooting

**No cards detected**
- Verify the PTX105R 3.3 V supply and antenna connection.
- Confirm the SPI pins, chip select and IRQ pin match `configuration.xml` and your schematic.
- Check the return code of `rs_nfc_ptx_start_discovery()` in the debugger.

**Console output missing**
- Confirm the terminal baud matches the FSP UART setting (230400 8N1 by default).
- Confirm the UART instance exists in the FSP stack and the correct port is open.

**Build fails with a flash overflow**
- Confirm the `-Oz` override and the `vfprintf=__l_vfprintf` link option are still in `CMakeLists.txt`; RASC overwrites flags placed in `cmake/GeneratedCfg.cmake`.

**Intermittent read failures**
- Increase `cfg.timeout_ms` for slower cards, and check the antenna tuning and card distance.

**Stack overflow or hard fault under load**
- Do not reduce the `RS_NFC` task stack below 4096 bytes, and leave headroom in the FreeRTOS heap.

---

## References

| Document | Description |
|---|---|
| [PTX105R product page](https://www.renesas.com/en/products/ptx105r) | Mid-power, multi-protocol NFC Forum compliant reader |
| [PTX105R datasheet](https://www.renesas.com/en/document/dst/ptx105r-datasheet) | Electrical and RF specifications |
| [Renesas FSP documentation](https://renesas.github.io/fsp/) | API reference for all FSP modules used |
| [RA Smart Configurator](https://www.renesas.com/en/software-tool/ra-smart-configurator) | FSP configuration and code generation tool |
| [e² studio](https://www.renesas.com/en/software-tool/e2-studio) | Renesas IDE user guide and downloads |
| [J-Link software](https://www.segger.com/products/debug-probes/j-link/tools/j-link-software/) | SEGGER debug probe tools |

**Support**

- [Renesas Wireless Connectivity Forum](https://community.renesas.com/wireles-connectivity)
- [Technical Support](https://www.renesas.com/support)
