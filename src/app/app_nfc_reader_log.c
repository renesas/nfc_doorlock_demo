/*
 * app_nfc_reader_log.c
 *
 * Implementation of the application log helper. See app_nfc_reader_log.h.
 *
 * Notes:
 *  - Log output (printf / stdout) is handled by the rs-console-io stdio
 *    layer; this module only provides the card-info / buffer print helpers.
 */
#include <stdio.h>
#include "app_nfc_reader_log.h"
#include "hal_data.h"
#include "r_ioport.h"
#include "ptxCOMMON.h"

uint8_t uid_is_odd = false;

/*
 * ####################################################################################################################
 * API IMPLEMENTATION
 * ####################################################################################################################
 */

void app_nfc_reader_log_print_buffer (uint8_t  * buffer,
                                      uint32_t   bufferOffset,
                                      uint32_t   bufferLength,
                                      uint8_t    addNewLine,
                                      uint8_t    printASCII)
{
    uint32_t i;
    uint8_t character_to_print;

    if (NULL != buffer)
    {
        if (0 != bufferLength)
        {
            for (i = 0; (i < bufferLength) && (i < (uint32_t) TX_BUFFER_SIZE); i++)
            {
                if ((i > 0) && ((i % (LINE_LENGTH - 5) == 0)))
                {
                    printf("\r\n     ");
                }

                if (0 == printASCII)
                {
                    printf("%02X", (uint8_t) buffer[i + bufferOffset]);
                }
                else
                {
                    character_to_print = (uint8_t) buffer[i + bufferOffset];

                    if (character_to_print < 0x20)
                    {
                        printf(".");
                    }
                    else
                    {
                        printf("%c", character_to_print);
                    }
                }
            }

            if (0 != addNewLine)
            {
                printf("\r\n");
            }
        }
    }
}

/*
 * ####################################################################################################################
 * NDEF RECORD DECODER (human-readable record content)
 * ####################################################################################################################
 */

/* Human-readable NDEF TNF (Type Name Format) names. */
static const char * ndef_tnf_name (uint8_t tnf)
{
    switch (tnf)
    {
        case 0x00u: return "Empty";
        case 0x01u: return "NFC Forum well-known";
        case 0x02u: return "MIME media";
        case 0x03u: return "Absolute URI";
        case 0x04u: return "NFC Forum external";
        case 0x05u: return "Unknown";
        case 0x06u: return "Unchanged";
        default:    return "Reserved";
    }
}

/* NFC Forum URI-record abbreviation prefixes (indexed by the identifier byte). */
static const char * const NDEF_URI_PREFIX[] = {
    "", "http://www.", "https://www.", "http://", "https://", "tel:",
    "mailto:", "ftp://anonymous:anonymous@", "ftp://ftp.", "ftps://",
    "sftp://", "smb://", "nfs://", "ftp://", "dav://", "news:",
    "telnet://", "imap:", "rtsp://", "urn:", "pop:", "sip:", "sips:",
    "tftp:", "btspp://", "btl2cap://", "btgoep://", "tcpobex://",
    "irdaobex://", "file://", "urn:epc:id:", "urn:epc:tag:",
    "urn:epc:pat:", "urn:epc:raw:", "urn:epc:", "urn:nfc:"
};

static void print_printable (const uint8_t * p, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        uint8_t c = p[i];
        putchar(((c >= 0x20u) && (c < 0x7Fu)) ? (int) c : '.');
    }
}

/* Local single-char NDEF type check (e.g. "T" text, "U" URI). */
static bool ndef_type_is (const rs_ndef_record_t * rec, char c)
{
    return (1u == rec->type_len) && ((char) rec->type[0] == c);
}

/* Print the records already decoded by the module (result->decoded). */
static void print_ndef_records (const rs_nfc_card_result_t * result)
{
    const rs_ndef_decoded_t * decoded = &result->decoded;

    if (0u == decoded->record_count)
    {
        // printf("Records        : (none)\r\n");
        return;
    }

    // printf("Records        : %u%s\r\n",
    //        (unsigned) decoded->record_count,
    //        decoded->truncated ? " (truncated)" : "");

    // printf(APP_NFC_READER_LOG_COL_BRIGHT_CYAN);

    for (uint8_t i = 0; i < decoded->record_count; i++)
    {
        const rs_ndef_record_t * rec = &decoded->records[i];

        // printf("[%u] TNF=0x%02X (%s) Type='", (unsigned) i,
        //        (unsigned) rec->tnf, ndef_tnf_name(rec->tnf));
        // print_printable(rec->type, rec->type_len);
        // printf("'  Payload=%u bytes\r\n", (unsigned) rec->payload_len);

        /* Well-known Text record: [status][lang][UTF-8 text] */
        if ((0x01u == rec->tnf) &&
            ndef_type_is(rec, 'T') &&
            (rec->payload_len >= 1u))
        {
            uint8_t status   = rec->payload[0];
            uint8_t lang_len = (uint8_t) (status & 0x3Fu);

            if ((uint32_t) lang_len + 1u <= rec->payload_len)
            {
                const uint8_t * txt = &rec->payload[1u + lang_len];
                uint32_t txt_len = rec->payload_len - 1u - lang_len;

                // printf("        Text : \"");
                // print_printable(txt, txt_len);
                // printf("\"\r\n");
            }
        }
        /* Well-known URI record: [prefix-id][URI tail] */
        else if ((0x01u == rec->tnf) &&
                 ndef_type_is(rec, 'U') &&
                 (rec->payload_len >= 1u))
        {
            uint8_t id = rec->payload[0];

            // printf("        URI  : ");

            if (id < (uint8_t) (sizeof(NDEF_URI_PREFIX) / sizeof(NDEF_URI_PREFIX[0])))
            {
                // printf("%s", NDEF_URI_PREFIX[id]);
            }

            // print_printable(&rec->payload[1], rec->payload_len - 1u);
            // printf("\r\n");
        }
        /* Anything else: show a compact printable preview of the payload. */
        else if (rec->payload_len > 0u)
        {
            // printf("        Data : ");
            // print_printable(rec->payload, rec->payload_len);
            // printf("\r\n" APP_NFC_READER_LOG_COL_RESET);
        }
    }
}

/*
 * ####################################################################################################################
 * APPLICATION-LEVEL CARD-INFO PRINTER
 * ####################################################################################################################
 *
 * Reads fields from rs_nfc_card_result_t and formats a human-readable block
 * to the rs-console-io stdio output (printf).  Pure I/O - no LED or board
 * interaction; the caller is responsible for any visual feedback (blink, etc.).
 */
void app_nfc_reader_log_print_card_info (const rs_nfc_card_result_t * result)
{
    char rf_tech[16];

    if (NULL == result)
    {
        return;
    }

    switch (result->card_type)
    {
        case RS_NFC_CARD_TYPE_ISO14443A:
        case RS_NFC_CARD_TYPE_NFC_TAG_TYPE_2:
        case RS_NFC_CARD_TYPE_NFC_TAG_TYPE_4A:
            snprintf(rf_tech, sizeof(rf_tech), "TYPE A");
            break;

        case RS_NFC_CARD_TYPE_ISO14443B:
        case RS_NFC_CARD_TYPE_NFC_TAG_TYPE_4B:
            snprintf(rf_tech, sizeof(rf_tech), "TYPE B");
            break;

        case RS_NFC_CARD_TYPE_FELICA:
        case RS_NFC_CARD_TYPE_NFC_TAG_TYPE_3:
            snprintf(rf_tech, sizeof(rf_tech), "TYPE F");
            break;

        case RS_NFC_CARD_TYPE_ISO15693:
        case RS_NFC_CARD_TYPE_NFC_TAG_TYPE_5:
            snprintf(rf_tech, sizeof(rf_tech), "TYPE V");
            break;

        default:
            snprintf(rf_tech, sizeof(rf_tech), "UNKNOWN");
            break;
    }

    // printf("RF Technology  : "APP_NFC_READER_LOG_COL_BRIGHT_CYAN "%s\r\n" APP_NFC_READER_LOG_COL_RESET, rf_tech);

    // /* Tag Type */
    // printf("Tag Type       : %s\r\n",
    //        (NULL != result->tag_type_name)
    //            ? result->tag_type_name : "Unknown");

    // /* Serial Number */
    // printf("Serial Number  : ");


    uint8_t uid_checksum_8 = 0;
    if (0u == result->uid_len)
    {
        printf("N/A");
    }
    else
    {
        for (uint8_t i = 0; i < result->uid_len; i++)
        {
            if (i)
            {
                printf(":");
            }

            printf("%02X", result->uid[i]);
            uid_checksum_8 += result->uid[i];
        }
        uid_is_odd = 0x01u & uid_checksum_8;
        if( uid_is_odd )
        { 
            printf("\r\n ODD!");
        }
        else 
        {
            printf("\r\n EVEN!");
        }
        xSemaphoreGive(g_uid_detection_semaphore);
    }


    printf("\r\n");

    // /* Size / Writeable */
    // if (result->data_area_size > 0u)
    // {
    //     printf("Size           : %u bytes\r\n",
    //            (unsigned) result->data_area_size);
    //     printf("Writeable      : %s\r\n",
    //            result->writeable ? "Yes" : "No");
    // }
    // else
    // {
    //     printf("Size           : N/A\r\n");
    //     printf("Writeable      : N/A\r\n");
    // }

    /* NDEF records */
    if (result->ndef_present && (result->ndef_len > 0u))
    {
        printf("NDEF           : %u bytes\r\n", (unsigned) result->ndef_len);
        printf("NDEF raw (%u bytes):", (unsigned) result->ndef_len);

        for (uint32_t k = 0u; k < result->ndef_len; k++)
        {
            if ((k > 0u) && (0u == (k % 16u)))
            {
                printf("\r\n                       ");
            }
            printf(" %02X", result->ndef_data[k]);
        }
        printf("\r\n");

        /* Decode the NDEF message into individual records and print their
         * human-readable content (Text / URI / raw payload preview). */
        print_ndef_records(result);
    }
    else
    {
        printf("Records        : (none)\r\n");
    }
}
