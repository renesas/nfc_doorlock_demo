#include "new_thread0.h"
#include "app_nfc_reader.h"

#define NFC_DISABLED true

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);


#if(true == NFC_DISABLED)
    /* Start the NFC card reader application.
     * This configures the RS NFC reader and launches the non-blocking NFC
     * event loop (spawns the "RS_NFC" worker task) before returning. */
    app_nfc_reader_entry();
#else
    while(1)
    {
        vTaskDelay(1);
    }
#endif

    /* Should not reach here; suspend if it does. */
    vTaskSuspend(NULL);
}
