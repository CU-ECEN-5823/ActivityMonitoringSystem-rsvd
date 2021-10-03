/*
 * scheduler.c - Scheduler functions
 *
 *  Created on: Sep 12, 2021
 *      Author: vishn
 */

#include "scheduler.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

static event_t event_q[EVENT_QUEUE_SIZE];
static uint8_t rd_ptr = 0;
//static uint8_t wr_ptr = 0;
static uint8_t q_len = 0;


/*
 * Enqueues event onto event queue
 *
 * @param event - Event to be enqueued
 *
 * @return None
 */
//static void EventQ_EnqueueEvent(event_t event);


/*
 * Dequeues event from event queue
 *
 * @param None
 *
 * @return Next event
 */
static event_t EventQ_DequeueEvent(void);


/*static void EventQ_EnqueueEvent(event_t event) {
    if (q_len == EVENT_QUEUE_SIZE) {
        return;
    }
    event_q[(wr_ptr++) & EVENT_QUEUE_SIZE_MASK] = event;
    q_len++;
}*/


static event_t EventQ_DequeueEvent(void) {
    event_t current_event;
    if (q_len == 0) {
        current_event = ev_NONE;
    }
    else {
        current_event = event_q[(rd_ptr++) & EVENT_QUEUE_SIZE_MASK];
        q_len--;
    }
    return current_event;
}


void Scheduler_SetEvent_LETIMER0_UF(void) {
    CORE_DECLARE_IRQ_STATE;

    CORE_ENTER_CRITICAL();
    //EventQ_EnqueueEvent(ev_LETIMER0_UF);
    sl_bt_external_signal(ev_LETIMER0_UF);
    CORE_EXIT_CRITICAL();
}


void Scheduler_SetEvent_LETIMER0_COMP1(void) {
    CORE_DECLARE_IRQ_STATE;

    CORE_ENTER_CRITICAL();
    //EventQ_EnqueueEvent(ev_LETIMER0_COMP1);
    sl_bt_external_signal(ev_LETIMER0_COMP1);
    CORE_EXIT_CRITICAL();
}


void Scheduler_SetEvent_I2C0_TRANSFER_DONE(void) {
    CORE_DECLARE_IRQ_STATE;

    CORE_ENTER_CRITICAL();
    //EventQ_EnqueueEvent(ev_I2C0_TRANSFER_DONE);
    sl_bt_external_signal(ev_I2C0_TRANSFER_DONE);
    CORE_EXIT_CRITICAL();
}


event_t Scheduler_GetNextEvent(void) {
    event_t current_event;
    CORE_DECLARE_IRQ_STATE;

    CORE_ENTER_CRITICAL();
    current_event = EventQ_DequeueEvent();
    CORE_EXIT_CRITICAL();
    return current_event;
}


void PowerUp(void) {
    I2C0_Init();
    I2C0_Enable(true);
    timerWaitUs_irq(TEMP_SENSOR_POWER_ON_WAIT_US);
}


void SendReadTempCommand(void) {
    LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
    sl_power_manager_add_em_requirement(EM1);

    I2C0_SendCommand(CMD_READ_TEMP);
}


void WaitForTempSensorReading(void) {
    sl_power_manager_remove_em_requirement(EM1);
    I2C0_DisableIntForTransfer();

    timerWaitUs_irq(TEMP_SENSOR_READ_TEMP_WAIT_US);
}


void RequestTempSensorReading(void) {
    LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
    sl_power_manager_add_em_requirement(EM1);

    I2C0_RequestRead();
}


void ReadOutTempSensorReading(ble_data_struct_t* ble_data) {
    uint8_t read_buff[2];
    int16_t temperature_code;
    int16_t temperature_C;
    uint8_t temp_buff[5];
    uint8_t* p = temp_buff;
    uint32_t temp_C_float;
    sl_status_t ble_status;

    sl_power_manager_remove_em_requirement(EM1);
    I2C0_DisableIntForTransfer();

    I2C0_ReadBytes(read_buff, 2);
    I2C0_Enable(false);

    temperature_code = (read_buff[0] << 8) | read_buff[1];
    temperature_C = (175.25 * temperature_code) / 65536.0 - 46.85; //calculation taken from data sheet

    LOG_INFO("%d\r\n", temperature_C);

    temp_C_float = UINT32_TO_FLOAT(temperature_C * 1000, -3);

    UINT8_TO_BITSTREAM(p, 0); //Celcius
    UINT32_TO_BITSTREAM(p, temp_C_float);

    ble_status = sl_bt_gatt_server_send_indication(
        ble_data->connectionHandle,
        gattdb_temperature_measurement,
        sizeof(temp_buff),
        temp_buff);
    if (ble_status != SL_STATUS_OK) {
        LOG_ERROR("sl_bt_gatt_server_send_indication: %d\r\n", ble_status);
    }

    ble_data->indicationInFlight = true;
    I2C0_Teardown();
}

/*
void TemperatureStateMachine(event_t event) {
    temp_fsm_state_t current_state;
    static temp_fsm_state_t next_state = PERIOD_WAIT;

    current_state = next_state;

    switch (current_state) {
        case PERIOD_WAIT:
            if (event == ev_LETIMER0_UF) {
                PowerUp();
                next_state = POWERING_UP;
            }
            break;

        case POWERING_UP:
            if (event == ev_LETIMER0_COMP1) {
                SendReadTempCommand();
                next_state = REQUEST_TEMP;
            }
            break;

        case REQUEST_TEMP:
            if (event == ev_I2C0_TRANSFER_DONE) {
                WaitForTempSensorReading();
                next_state = READING_TEMP;
            }
            break;

        case READING_TEMP:
            if (event == ev_LETIMER0_COMP1) {
                RequestTempSensorReading();
                next_state = RECEIVED_TEMP;
            }
            break;

        case RECEIVED_TEMP:
            if (event == ev_I2C0_TRANSFER_DONE) {
                ReadOutTempSensorReading();
                next_state = PERIOD_WAIT;
            }
            break;
    }
}
*/

void state_machine(sl_bt_msg_t* event) {
    temp_fsm_state_t current_state;
    static temp_fsm_state_t next_state = PERIOD_WAIT;
    ble_data_struct_t* ble_data = BLE_GetDataStruct();
    event_t ev;

    if (!(ble_data->connected) || !(ble_data->indicating)) {
        if (ble_data->readingTemp) {
            ble_data->readingTemp = 0;
            LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
            NVIC_DisableIRQ(I2C0_IRQn);
            I2C0_Teardown();
            ev = ev_SHUTDOWN;
        }
        else {
            return;
        }
    }
    else {
      if (!(ble_data->readingTemp)) {
          ble_data->readingTemp = 1;
      }
      ev = event->data.evt_system_external_signal.extsignals;
    }

    current_state = next_state;

    switch (current_state) {
        case PERIOD_WAIT:
            if (ev == ev_LETIMER0_UF) {
                PowerUp();
                next_state = POWERING_UP;
            }
            break;

        case POWERING_UP:
            if (ev == ev_LETIMER0_COMP1) {
                SendReadTempCommand();
                next_state = REQUEST_TEMP;
            }
            else if (ev == ev_SHUTDOWN) {
                next_state = PERIOD_WAIT;
            }
            break;

        case REQUEST_TEMP:
            if (ev == ev_I2C0_TRANSFER_DONE) {
                WaitForTempSensorReading();
                next_state = READING_TEMP;
            }
            else if (ev == ev_SHUTDOWN) {
                next_state = PERIOD_WAIT;
            }
            break;

        case READING_TEMP:
            if (ev == ev_LETIMER0_COMP1) {
                RequestTempSensorReading();
                next_state = RECEIVED_TEMP;
            }
            else if (ev == ev_SHUTDOWN) {
                next_state = PERIOD_WAIT;
            }
            break;

        case RECEIVED_TEMP:
            if (ev == ev_I2C0_TRANSFER_DONE) {
                ReadOutTempSensorReading(ble_data);
                next_state = PERIOD_WAIT;
            }
            else if (ev == ev_SHUTDOWN) {
                next_state = PERIOD_WAIT;
            }
            break;
    }
}
