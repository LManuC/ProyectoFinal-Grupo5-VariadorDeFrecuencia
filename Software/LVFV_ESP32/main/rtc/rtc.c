/**
 * @file RTC.c
 * @author Andrenacci - Carra
 * @brief Funciones de lectura y escritura del RTC y alarmas
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include <sys/time.h>

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "../system/sysControl.h"
#include "RTC.h"

/**
 * @typedef rtc_alarms_t
 * 
 * @brief Contenedor de handlers de alarma para inicio y fin de marcha de motor
 */
typedef struct {
    esp_timer_handle_t start_timer;
    esp_timer_handle_t stop_timer;
} rtc_alarms_t;

static const char *TAG = "RTC";                             // Variable dedicada a la impresión de mensajes de logs
static rtc_alarms_t rtc_alarms;                             // Declaración de contenedor de handlers de alarma para inicio y fin de marcha
static time_settings_t alarm_settings;                      // Settings de horarios de inicio y fin de marcha de motor

static struct tm time_start;
static struct tm time_stop;

/**
 * @fn static esp_err_t program_alarm(esp_timer_handle_t *timer_handle, void (*callback)(void *), int hour, int minute, const char *name);
 *
 * @brief Función dedicada a programar la alarma según hour y minute.
 *
 * @details No distingue de inicio o fin de marcha, para ello se le pasa el puntero a función 'callback' para la función de inicio o fin de marcha, el handler 'timer_handeler' y un nombre descriptivo. Los handlers de los timers son creados como tareas y no como interrupciones para poder utilizar las funciones .
 *
 * @param[out] timer_handle
 *      Puntero de salida donde se devuelve el handle del timer creado.
 *      Debe ser no-NULL. El caller es responsable de detener y destruir el timer cuando ya no se utilice (esp_timer_stop / esp_timer_delete).
 *
 * @param[in] callback
 *      Puntero a función de tipo `void (*)(void *)` que se invocará cuando la alarma dispare. Se ejecuta en el contexto de la tarea interna de esp_timer (no en ISR). Puede usar APIs no-ISR (colas, logs, etc.).
 *
 * @param[in] hour
 *      Hora en formato 24 h. Rango válido: 0–23.
 *
 * @param[in] minute
 *      Minutos. Rango válido: 0–59.
 *
 * @param[in] name
 *      Nombre descriptivo del timer (aparece en logs/diagnóstico). Debe
 *      apuntar a memoria de duración estática (p. ej., literal o `static const`).
 *
 * @return
 *      - ESP_OK en caso de éxito.
 *      - ESP_ERR_INVALID_ARG si @p timer_handle es NULL, @p callback es NULL o @p hour/@p minute están fuera de rango.
 *      - ESP_ERR_INVALID_STATE Si el RTC aún no fue inicializado
 *      - ESP_ERR_NO_MEM si la allocación de memoria para los handlers falla
 */
static esp_err_t program_alarm(esp_timer_handle_t *timer_handle, void (*callback)(void *), int hour, int minute, const char *name);

/**
 * @fn static void alarm_start_cb(void *arg);
 *
 * @brief Función programada como alarma para iniciar el funcionamiento del motor.
 *
 * @details Envía la señal al sistema para poder iniciar el funcionamiento del motor según la frecuencia de régimen, aceleración configuradas.
 *
 * @param[in] arg
 *      Sin uso.
 */
static void alarm_start_cb(void *arg);

/**
 * @fn static void alarm_stop_cb(void *arg);
 *
 * @brief Función programada como alarma para finalizar el funcionamiento del motor.
 *
 * @details Envía la señal al sistema para poder finalizar el funcionamiento del motor según la desaceleración configurada.
 *
 * @param[in] arg
 *      Sin uso.
 */
static void alarm_stop_cb(void *arg);

esp_err_t initRTC() {
    
    struct timeval tv = {
        .tv_sec = 0,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL); // Cargar en RTC interno

    alarm_settings.time_start = &time_start;
    alarm_settings.time_stop = &time_stop;
    
    return ESP_OK;
}

esp_err_t setTime(struct tm *setting_time) {

    if ( setting_time == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    time_t now = mktime(setting_time);  // Convierte a timestamp UNIX

    struct timeval tv = {
        .tv_sec = now,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL); // Cargar en RTC interno
    return ESP_OK;
}

esp_err_t getTime(struct tm *current_timeinfo) {
    time_t now;

    if ( current_timeinfo == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    time(&now);
    localtime_r(&now, current_timeinfo);

    return ESP_OK;
}

esp_err_t rtc_schedule_alarms(time_settings_t *time_settings) {

    if ( time_settings != NULL ) {
        alarm_settings.time_start->tm_hour = time_settings->time_start->tm_hour;
        alarm_settings.time_start->tm_min = time_settings->time_start->tm_min;
        alarm_settings.time_stop->tm_hour = time_settings->time_stop->tm_hour;
        alarm_settings.time_stop->tm_min = time_settings->time_stop->tm_min;
    }

    // Cancelo las anteriores si existían
    if (rtc_alarms.start_timer) {
        esp_timer_stop(rtc_alarms.start_timer);
    }
    // Programo las nuevas
    esp_err_t err1 = program_alarm(&rtc_alarms.start_timer, alarm_start_cb, alarm_settings.time_start->tm_hour, alarm_settings.time_start->tm_min, "start_alarm");

    // Cancelo las anteriores si existían
    if (rtc_alarms.stop_timer) {
        esp_timer_stop(rtc_alarms.stop_timer);
    }
    // Programo las nuevas
    esp_err_t err2 = program_alarm(&rtc_alarms.stop_timer, alarm_stop_cb, alarm_settings.time_stop->tm_hour, alarm_settings.time_stop->tm_min, "stop_alarm");

    
    if (err1 == ESP_OK && err2 == ESP_OK){
        ESP_LOGI(TAG, "Configurando alarmas: INICIO %02d:%02d", alarm_settings.time_start->tm_hour, alarm_settings.time_start->tm_min);
        ESP_LOGI(TAG, "                    :  FIN   %02d:%02d", alarm_settings.time_stop->tm_hour, alarm_settings.time_stop->tm_min);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "ERROR AL CONFIGURAR LAS ALARMAS");
        return ESP_FAIL;
    }
}

static esp_err_t program_alarm(esp_timer_handle_t *timer_handle, void (*callback)(void *), int hour, int minute, const char *name) {
    time_t now;
    struct tm timeinfo_alarm;
    time(&now);
    localtime_r(&now, &timeinfo_alarm);

    struct tm alarm_tm = timeinfo_alarm;
    alarm_tm.tm_hour = hour;
    alarm_tm.tm_min  = minute;
    alarm_tm.tm_sec  = 0;

    time_t alarm_epoch = mktime(&alarm_tm);
    if (alarm_epoch <= now) {
        alarm_epoch += 24 * 3600;
    }

    int64_t usec_until_alarm = (alarm_epoch - now) * 1000000LL;

    esp_timer_create_args_t timer_args = {
        .callback = callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = name
    };

    esp_err_t err = esp_timer_create(&timer_args, timer_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_timer_start_once(*timer_handle, usec_until_alarm);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Programada %s para %02d:%02d (en %lld segundos)", name, hour, minute, (alarm_epoch - now));

    return ESP_OK;
}

// Callback para inicio de tarea
static void alarm_start_cb(void *arg) {
    ESP_LOGI(TAG, "⏰ Alarma de INICIO disparada");

    // Cancelo las anteriores si existían
    if (rtc_alarms.start_timer) {
        esp_timer_stop(rtc_alarms.start_timer);
    }
    // Programo las nuevas
    ESP_ERROR_CHECK(program_alarm(&rtc_alarms.start_timer, alarm_start_cb, alarm_settings.time_start->tm_hour, alarm_settings.time_start->tm_min, "start_alarm"));

    // Cancelo las anteriores si existían
    if (rtc_alarms.stop_timer) {
        esp_timer_stop(rtc_alarms.stop_timer);
    }
    // Programo las nuevas
    ESP_ERROR_CHECK(program_alarm(&rtc_alarms.stop_timer, alarm_stop_cb, alarm_settings.time_stop->tm_hour, alarm_settings.time_stop->tm_min, "stop_alarm"));

    SystemEventPost(START_PRESSED);
}

// Callback para fin de tarea
static void alarm_stop_cb(void *arg) {
    ESP_LOGI(TAG, "⏰ Alarma de FIN disparada");

    // Cancelo las anteriores si existían
    if (rtc_alarms.start_timer) {
        esp_timer_stop(rtc_alarms.start_timer);
    }
    // Programo las nuevas
    ESP_ERROR_CHECK(program_alarm(&rtc_alarms.start_timer, alarm_start_cb, alarm_settings.time_start->tm_hour, alarm_settings.time_start->tm_min, "start_alarm"));

    // Cancelo las anteriores si existían
    if (rtc_alarms.stop_timer) {
        esp_timer_stop(rtc_alarms.stop_timer);
    }
    // Programo las nuevas
    ESP_ERROR_CHECK(program_alarm(&rtc_alarms.stop_timer, alarm_stop_cb, alarm_settings.time_stop->tm_hour, alarm_settings.time_stop->tm_min, "stop_alarm"));

    SystemEventPost(STOP_PRESSED);
}
