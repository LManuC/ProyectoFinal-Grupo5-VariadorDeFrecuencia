#ifndef SPI_MODULE_H
#define SPI_MODULE_H


// ============================================================================
// Definiciones y configuraciones
// ============================================================================


// ============================================================================
// Tipos de datos
// ============================================================================

typedef enum {
    SPI_REQUEST_START       = 10,
    SPI_REQUEST_STOP        = 11,
    SPI_REQUEST_SET_FREC    = 12,
    SPI_REQUEST_SET_ACEL    = 13,
    SPI_REQUEST_SET_DESACEL = 14,
    SPI_REQUEST_SET_DIR     = 15,
    SPI_REQUEST_GET_FREC    = 16,
    SPI_REQUEST_GET_ACEL    = 17,
    SPI_REQUEST_GET_DESACEL = 18,
    SPI_REQUEST_GET_DIR     = 19,
    SPI_REQUEST_IS_STOP     = 20,
    SPI_REQUEST_EMERGENCY   = 21,
    SPI_REQUEST_LAST        = 22,
} SPI_Request;

typedef enum {
    SPI_RESPONSE_OK = 0xFF,
    SPI_RESPONSE_ERR = 0xA0,
    SPI_RESPONSE_ERR_CMD_UNKNOWN = 0xA1,        // Comando desconocido
    SPI_RESPONSE_ERR_NO_COMMAND = 0xA2,         // Llego mensaje pero sin comando
    SPI_RESPONSE_ERR_MOVING = 0xA3,             // Comando Start pero motor ya esta en movimiento
    SPI_RESPONSE_ERR_NOT_MOVING = 0xA4,         // Comando Stop pero motor no esta en movimiento    
    SPI_RESPONSE_ERR_DATA_MISSING = 0xA5,        // Comando que requiere datos pero no llegaron
    SPI_RESPONSE_ERR_DATA_INVALID = 0xA6,       // Comando que tiene datos invalidos
    SPI_RESPONSE_ERR_DATA_OUT_RANGE = 0xA7,     // Comando con datos fuera de rango permitido
    SPI_RESPONSE_ERR_EMERGENCY_ACTIVE = 0xA8,     // Emergencia activa
    SPI_LAST_VALUE,                             // Los mensajes deben tener un valor consecutivo uno de otro
} SPI_Response;


// ============================================================================
// Funciones públicas
// ============================================================================

/**
 * @brief Inicializa el módulo 
 *
 * @param  void
 * @return void
 */
void SPI_Init(void);

/**
 * @brief Ejecuta una consulta y devuelve la respuesta
 *
 * @param request En este se carga el tipo de consulta que se requiere
 * @param setValue Dependiendo del tipo de request se necesita un valor o no. Si es un setFrec
 * le debemo enviar en este campo la frecuencia deseada. Si se quiere retornar un valor es 
 * indistinto el valor que se ingrese a este campo. 
 * @param getValue Este campo se utiliza para retornar un valor en caso que coresponda, manejar
 * los punteros respectivamente, el programa espera espacio para un solo int en el que se guardara
 * informacion de retorno aparte de la respuesta return de la funcion.
 * @return En esta se devuelve una de la lista posible de respuestas SPI_Response
 */
SPI_Response SPI_SendRequest(SPI_Request request, int setValue, int* getValue);


#endif // SPI_MODULE_H
