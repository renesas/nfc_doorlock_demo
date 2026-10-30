/** \file
    ---------------------------------------------------------------
    SPDX-License-Identifier: BSD-3-Clause

    Copyright (c) 2024, Renesas Electronics Corporation and/or its affiliates


    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice, this list of
       conditions and the following disclaimer in the documentation and/or other
       materials provided with the distribution.

    3. Neither the name of Renesas nor the names of its
       contributors may be used to endorse or promote products derived from this
       software without specific prior written permission.



    THIS SOFTWARE IS PROVIDED BY Renesas "AS IS" AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL RENESAS OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    ---------------------------------------------------------------

    Project     : PTX1K
    Module      : IOT_READER Demo
    File        : app_nfc_reader.c

    Description : IoT Reader demo application for PTX1xxR NFC Platform.
                  Thin application layer — all NFC protocol logic lives in the
                  RS NFC Reader module (src/rs-nfc-card-reader/).
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "app_nfc_reader.h"
#include "app_nfc_reader_log.h"
#include "rs_nfc_reader.h"

/*
 * ####################################################################################################################
 * DEFINES / TYPES
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * LED UTILITIES
 * ####################################################################################################################
 */
#if defined(APP_NFC_READER_LED_EN)
/* LED pins mapped for RA2E3 FPB */
static const bsp_io_port_pin_t led_pins[] =
{
    APP_NFC_READER_LED_1,
    APP_NFC_READER_LED_2,
};

static const uint32_t LED_COUNT = (uint32_t)(sizeof(led_pins) / sizeof(led_pins[0]));

/*
 * Drive every LED known to this module to the requested level.
 *
 * NOTE: `g_bsp_pin_cfg` (used by R_IOPORT_Open at boot) does not include
 * the on-board LED pins (P02_13, P09_14), so the application must configure them as outputs
 */
static void app_nfc_reader_led_set_all (bsp_io_level_t level)
{
    for (uint32_t i = 0; i < LED_COUNT; i++)
    {
        (void)g_ioport.p_api->pinWrite(g_ioport.p_ctrl, led_pins[i], level);
    }
}
#endif /* APP_NFC_READER_LED_EN */

/*
 * ####################################################################################################################
 * DATA EXCHANGE (card info + raw demo exchange)
 * ####################################################################################################################
 */
static void app_nfc_reader_card_event (const rs_nfc_card_result_t *result)
{
    if (NULL == result)
    {
        return;
    }

    /* All extended fields (ndef_*, data_area_size, writeable, tag_type_name)
     * and the optional raw-exchange TX/RX frames are pre-populated by
     * rs_nfc_reader_Read() before firing the per-card event, so the app
     * only needs to consume `result` — no further RS calls required. */
    app_nfc_reader_log_print_card_info(result);

    /* Raw protocol exchange (populated by RS when cfg.run_raw_exchange = true
     * and the protocol is not ISO-DEP / UNDEFINED). */
    if (result->raw_exchange.valid)
    {
        printf(APP_NFC_READER_SEPARATOR_TOP);
        printf("TX = ");
        app_nfc_reader_log_print_buffer((uint8_t *)result->raw_exchange.tx, 0, result->raw_exchange.tx_len, 1, 0);

        if (RS_OK == result->raw_exchange.status)
        {
            printf("RX = ");
            app_nfc_reader_log_print_buffer((uint8_t *)result->raw_exchange.rx, 0, result->raw_exchange.rx_len, 1, 0);
        }
        else
        {
            printf("ERROR - RF-Exchange failed (status=%d)\r\n", (int)result->raw_exchange.status);
        }

        printf(APP_NFC_READER_SEPARATOR_BOT);
    }
}

/*
 * ####################################################################################################################
 * CALLBACKS
 * ####################################################################################################################
 */
static void on_nfc_read_done (rs_status_t                  status,
                              const rs_nfc_card_result_t * result,
                              const char                 * summary,
                              void                       * p_context)
{
    (void)p_context;

#if defined(APP_NFC_READER_LED_EN)
    app_nfc_reader_led_set_all(BSP_IO_LEVEL_HIGH);
#endif

    /* Informational / warning / fatal events (result == NULL) */
    if (NULL == result)
    {
        if (NULL != summary)
        {
            printf("%s\r\n", summary);
        }

#if defined(APP_NFC_READER_LED_EN)
        app_nfc_reader_led_set_all(BSP_IO_LEVEL_LOW);
#endif
        return;
    }

    printf(APP_NFC_READER_LOG_COL_BRIGHT_GREEN "\r\n\r\n%s" APP_NFC_READER_LOG_COL_RESET "\r\n",
           (NULL != summary) ? summary : "CARD DETECTED!");

    app_nfc_reader_card_event(result);
    (void)status;

#if defined(APP_NFC_READER_LED_EN)
    app_nfc_reader_led_set_all(BSP_IO_LEVEL_LOW);
#endif
}

static void on_nfc_operation_done (rs_status_t status, void *p_context)
{
    (void)p_context;
    printf("rs_nfc_reader_Read completed (status=%d)\r\n", (int)status);
}

/*
 * ####################################################################################################################
 * STOP EXAMPLE (placeholder)
 * ####################################################################################################################
 */
/*
 * Placeholder example showing how to request a graceful stop of a running
 * read (blocking or non-blocking). This is intentionally NOT called anywhere
 * in the default demo flow — it exists only to illustrate the API usage.
 *
 * Typical real-world triggers:
 *   - a button/GPIO ISR that signals the app to stop scanning
 *   - another FreeRTOS task deciding the read should end
 *   - a timeout/watchdog handler
 *
 * Behaviour: rs_nfc_reader_Stop() sets the stop flag and wakes the task
 * blocked in the discovery loop, so it exits cleanly. The operation-end
 * callback (on_nfc_operation_done) then fires with RS_OK.
 * Returns RS_ERR_DEPENDENCY when no operation is in flight (harmless).
 */
static void app_nfc_reader_stop (void)
{
    rs_status_t st = rs_nfc_reader_Stop();

    if (RS_OK != st)
    {
        printf("rs_nfc_reader_Stop FAILED (status=%d)\r\n", (int)st);
    }
    else
    {
        printf("RS NFC Reader stop requested\r\n");
    }
}

/*
 * ####################################################################################################################
 * APPLICATION ENTRY POINT
 * ####################################################################################################################
 */

static void app_nfc_reader_init (void)
{
    rs_status_t st = RS_OK;

    // printf("System Initialization (RS NFC Reader) ... starting\r\n");

    rs_nfc_reader_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.tech_mask             = RS_NFC_TECH_ALL;
    cfg.timeout_ms            = RS_NFC_TIMEOUT_INFINITE;
    cfg.retry_count           = RS_NFC_RETRY_DISABLED;
    cfg.read_ndef             = RS_NFC_NDEF_READ_ENABLED;
    cfg.max_ndef_bytes        = RS_NFC_NDEF_MAX_BYTES;
    cfg.run_raw_exchange      = RS_NFC_RAW_EXCHANGE_ENABLED;
    cfg.callback              = on_nfc_operation_done;
    cfg.p_context             = RS_NFC_CONTEXT_NONE;
    cfg.on_card_event         = on_nfc_read_done;
    cfg.p_card_event_context  = RS_NFC_CONTEXT_NONE;
    cfg.validate_dependencies = RS_NFC_DEP_VALIDATION_DISABLED;
    cfg.cfg_valid_check_en    = RS_NFC_CFG_VALIDATION_ENABLED;

    /* Non-blocking read: cfg.callback is set, so rs_nfc_reader_Read() spawns
     * a worker task and returns immediately. Pass NULL for result_out — the
     * worker then uses its own task-local buffer. Passing the address of a
     * local here would dangle the moment this function returns (the worker
     * keeps writing to it), corrupting whatever reuses that stack region. */
    st = rs_nfc_reader_Read(&cfg, NULL);

    if (RS_OK != st)
    {
        // printf("rs_nfc_reader_Read launch FAILED (status=%d)\r\n", (int) st);

        /* A launch failure (e.g. RS_ERR_INTERNAL when a non-blocking read is
         * already active) leaves the module in an undefined run state. Request
         * a graceful stop so any in-flight discovery loop exits cleanly before
         * we bail out. Safe to call even if nothing is running. */
        app_nfc_reader_stop();
    }
    else
    {
        // printf("RS NFC Reader launched (non-blocking)\r\n");
    }
}

void app_nfc_reader_entry (void)
{
    app_nfc_reader_init();
}
