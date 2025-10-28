#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "LVFV_system.h"

static const char *TAG = "RTC";

typedef struct {
    esp_timer_handle_t start_timer;
    esp_timer_handle_t stop_timer;
} rtc_alarms_t;

static rtc_alarms_t rtc_alarms;

static esp_err_t program_alarm(esp_timer_handle_t *timer_handle, void (*callback)(void *), int hour, int minute, const char *name);
void alarm_start_cb(void *arg);
void alarm_stop_cb(void *arg);

esp_err_t initRTC() {
    
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL); // Cargar en RTC interno
    
    return ESP_OK;
}

esp_err_t setTime(struct tm *setting_time) {
    
    time_t now = mktime(setting_time);  // Convierte a timestamp UNIX

    struct timeval tv = {
        .tv_sec = now,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL); // Cargar en RTC interno
    
    return ESP_OK;
}

void getTime() {
    time_t now;

    time(&now);
    localtime_r(&now, &timeinfo);
    // ESP_LOGI(TAG, "Actualizo hora: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

static esp_err_t program_alarm(esp_timer_handle_t *timer_handle, void (*callback)(void *), int hour, int minute, const char *name) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    struct tm alarm_tm = timeinfo;
    alarm_tm.tm_hour = hour;
    alarm_tm.tm_min  = minute;
    alarm_tm.tm_sec  = 0;

    time_t alarm_epoch = mktime(&alarm_tm);
    if (alarm_epoch <= now) alarm_epoch += 24 * 3600;

    int64_t usec_until_alarm = (alarm_epoch - now) * 1000000LL;

    esp_timer_create_args_t timer_args = {
        .callback = callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = name
    };

    esp_err_t err = esp_timer_create(&timer_args, timer_handle);
    if (err != ESP_OK) return err;

    err = esp_timer_start_once(*timer_handle, usec_until_alarm);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Programada %s para %02d:%02d (en %lld segundos)", 
             name, hour, minute, (alarm_epoch - now));

    return ESP_OK;
}

esp_err_t rtc_schedule_alarms() {
    ESP_LOGI(TAG, "Configurando alarmas: INICIO %02d:%02d, FIN %02d:%02d", system_time_settings.time_start->tm_hour, system_time_settings.time_start->tm_min, system_time_settings.time_stop->tm_hour, system_time_settings.time_stop->tm_min);

    // Cancelo las anteriores si existían
    if (rtc_alarms.start_timer) {
        esp_timer_stop(rtc_alarms.start_timer);
    }

    if (rtc_alarms.stop_timer) {
        esp_timer_stop(rtc_alarms.stop_timer);
    }
    
    // Programo las nuevas
    esp_err_t err2 = program_alarm(&rtc_alarms.stop_timer, alarm_stop_cb, system_time_settings.time_stop->tm_hour, system_time_settings.time_stop->tm_min, "stop_alarm");
    esp_err_t err1 = program_alarm(&rtc_alarms.start_timer, alarm_start_cb, system_time_settings.time_start->tm_hour, system_time_settings.time_start->tm_min, "start_alarm");

    return (err1 == ESP_OK && err2 == ESP_OK) ? ESP_OK : ESP_FAIL;
}

// Callback para inicio de tarea
void alarm_start_cb(void *arg) {
    ESP_LOGI(TAG, "⏰ Alarma de INICIO disparada");
    // Acá podés arrancar tu tarea (ej: motor, display, etc.)
}

// Callback para fin de tarea
void alarm_stop_cb(void *arg) {
    ESP_LOGI(TAG, "⏰ Alarma de FIN disparada");
    // Acá podés detener tu tarea
}
