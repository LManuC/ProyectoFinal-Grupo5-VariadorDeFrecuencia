/* FreeRTOS Real Time Stats Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp32.h"
#include "io_control.h"
#include "STM32_comm.h"

#define XTASK_CROE_0        taskVALID_CORE_ID( 0 )
#define XTASK_CROE_1        taskVALID_CORE_ID( 1 )
#define XTASK_CROE_x        tskNO_AFFINITY

#define NUM_OF_SPIN_TASKS 6
#define SPIN_ITER 500000 // Actual CPU cycles used will depend on compiler optimization
#define SPIN_TASK_PRIO 2
#define STATS_TASK_PRIO 3
#define STATS_TICKS pdMS_TO_TICKS(1000)
#define ARRAY_SIZE_OFFSET 5 // Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE



void app_main(void) {
    MCP23017_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    cmd_queue = xQueueCreate(10, sizeof(comando_t));

    xTaskCreate(MCP23017_interrupt_attendance, "MCP23017_interrupt_attendance", 2048, NULL, 10, NULL);
    xTaskCreate(spi_task, "spi_task", 4096, NULL, 10, NULL);
}