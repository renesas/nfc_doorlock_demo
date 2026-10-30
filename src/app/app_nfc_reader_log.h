/*
 * app_nfc_reader_log.h
 *
 * Application log helper. Debug output (printf) is emitted via the
 * rs-console-io stdio layer; this module no longer owns a UART instance.
 * Its remaining job is to provide the card-info / buffer print helpers.
 */

#ifndef APP_NFC_READER_LOG_H_
#define APP_NFC_READER_LOG_H_

#include <stddef.h>
#include <stdint.h>
#include "rs_nfc_reader.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_NFC_READER_LOG_COL_RESET        "\x1B[0m"
#define APP_NFC_READER_LOG_COL_BRIGHT_GREEN "\x1B[1;32m"
#define APP_NFC_READER_LOG_COL_BRIGHT_CYAN  "\x1B[1;36m"

/**
 * Print formatted card-info block (tag type, UID, size, NDEF) via printf
 * (rs-console-io stdio output).  Pure I/O — no LED or board interaction.
 */
void app_nfc_reader_log_print_card_info(const rs_nfc_card_result_t * result);

/**
 * Hex/ASCII dump of `bufferLength` bytes from `buffer` (starting at
 * `bufferOffset`) via printf.
 *   addNewLine != 0 : append a trailing newline.
 *   printASCII != 0 : print printable ASCII (non-printables as '.'),
 *                     otherwise print two-digit hex.
 */
void app_nfc_reader_log_print_buffer(uint8_t  * buffer,
                                     uint32_t   bufferOffset,
                                     uint32_t   bufferLength,
                                     uint8_t    addNewLine,
                                     uint8_t    printASCII);

#ifdef __cplusplus
}
#endif

#endif /* APP_NFC_READER_LOG_H_ */
