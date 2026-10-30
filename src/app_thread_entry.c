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
