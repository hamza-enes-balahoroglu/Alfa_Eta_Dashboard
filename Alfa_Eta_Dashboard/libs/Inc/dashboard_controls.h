/**
 ******************************************************************************
 * @file           : dashboard_controls.h
 * @brief          : Nextion display control library - STM32 HAL compatible
 ******************************************************************************
 * @author         : Hamza Enes Balahoroglu
 * @version        : v1.1
 * @date           : 09.06.2025
 *
 * @note
 * This header file is intended to be used in STM32CubeIDE projects.
 * It provides macros and function declarations to interface a Nextion
 * display via UART using STM32 HAL libraries.
 *
 * IMPORTANT:
 * - Dashboard_Init() must be called AFTER MX_USARTx_UART_Init() in main.c
 *   to ensure the UART handle is correctly initialized.
 * - The Nextion display must be configured to communicate at **115200 baudrate**.
 *   Make sure this setting matches both the UART peripheral and the Nextion editor config.
 *
 * Failing to match the baudrate or calling initialization before UART setup
 * will cause communication failures or random garbage characters on screen.
 *
 ******************************************************************************
 */

#ifndef DASHBOARD_CONTROLS
#define DASHBOARD_CONTROLS

#include "stm32f4xx_hal.h"
#include "geo_to_pixel.h"
#include "mapping.h"
#include <string.h>
#include <stdio.h>

#define NEX_SCREEN_SIZE_X 800
#define NEX_SCREEN_SIZE_Y 480

/**
  * @brief  Structure holding pointers to all dashboard variables.
  *         Used for accessing real-time data.
  */
typedef struct {
    int *speed;              /*!< Vehicle speed in km/h */
    int *batteryValue;       /*!< Battery charge percentage (0-100) */
    int *powerKW;            /*!< Power output in kW */
    int *packVoltage;        /*!< Total battery pack voltage */
    int *maxVoltage;         /*!< Maximum cell voltage */
    int *minVoltage;         /*!< Minimum cell voltage */
    int *batteryTemp;        /*!< Battery temperature in °C */
    MapOffset *mapData;      /*!< Map informations */
    int *gear;               /*!< Gear position: 0=N, 1=D, 2=R */
    int *handbrake;          /*!< Handbrake: 0=Off, 1=On */
    int *signalLeft;         /*!< Left signal: 0/1 */
    int *signalRight;        /*!< Right signal: 0/1 */
    int *connWarn;           /*!< Connection warning: 0/1 */
    int *battWarn;           /*!< Battery warning: 0/1 */
    int *lights;             /*!< Lights: 0=Off, 1=On */
} NEX_Data;


/**
  * @brief  Structure used to cache previous values of dashboard data.
  *         Useful for detecting changes.
  */
typedef struct {
    int speed;
    int batteryValue;
    int powerKW;
    int packVoltage;
    int maxVoltage;
    int minVoltage;
    int batteryTemp;
    MapOffset mapData;
    int gear;
    int handbrake;
    int signalLeft;
    int signalRight;
    int connWarn;
    int battWarn;
    int lights;
} NEX_CachedData;


/**
  * @brief  Enumeration of static commands for Nextion display control.
  */
typedef enum {
    CONNECTION_OK                 = 0x00U, /*!< Connection established */

    SET_GEAR_DRIVE                = 0x01U, /*!< Set gear to Drive */
    SET_GEAR_NEUTRAL              = 0x02U, /*!< Set gear to Neutral */
    SET_GEAR_REVERSE              = 0x03U, /*!< Set gear to Reverse */

    SET_HANDBREAK_ON              = 0x04U, /*!< Handbrake engaged */
    SET_HANDBREAK_OFF             = 0x05U, /*!< Handbrake released */

    SET_SIGNAL_LEFT_ON            = 0x06U,
    SET_SIGNAL_LEFT_OFF           = 0x07U,
    SET_SIGNAL_RIGHT_ON           = 0x08U,
    SET_SIGNAL_RIGHT_OFF          = 0x09U,

    SET_CONNECTION_WARNING_ON     = 0x0AU,
    SET_CONNECTION_WARNING_OFF    = 0x0BU,
    SET_BATTERY_WARNING_ON        = 0x0CU,
    SET_BATTERY_WARNING_OFF       = 0x0DU,

    SET_LIGHTS_ON                 = 0x0EU,
    SET_LIGHTS_OFF                = 0x0FU

} NEX_CommandID;


/**
  * @brief  Enumeration of numeric/value-based commands for Nextion display.
  */
typedef enum {
    SET_SPEED_COMMAND             = 0x00U, /*!< Update speed value */

    SET_BATTERY_NUMBER_COMMAND    = 0x01U, /*!< Update battery percentage */
    SET_BATTERY_PROGRESS_BAR_COMMAND = 0x02U, /*!< Update battery bar */

    SET_KW_NUMBER_COMMAND         = 0x03U, /*!< Update power kW value */
    SET_KW_PROGRESS_BAR_COMMAND   = 0x04U, /*!< Update kW bar */

    SET_PACK_VOLTAGE              = 0x05U, /*!< Update total voltage */
    SET_MAX_VOLTAGE               = 0x06U, /*!< Update max cell voltage */
    SET_MIN_VOLTAGE               = 0x07U, /*!< Update min cell voltage */

    SET_BATTERY_TEMPERATURE       = 0x08U, /*!< Update battery temperature */

    SET_MAP_X                     = 0x09U, /*!< Update map X position */
    SET_MAP_Y                     = 0x0AU, /*!< Update map Y position */
    SET_MAP_ICON                  = 0x0BU, /*!< Update vehicle icon rotation */
    SET_MAP_LAP                   = 0x0CU  /*!< Update lap counter */
} NEX_Int_Command_ID;


/**
  * @brief  Enumeration for progress bar direction on the display.
  */
typedef enum {
    PROGRESS_BAR_REVERSE          = 0x01U, /*!< Progress bar from full to empty */
    PROGRESS_BAR_NO_REVERSE       = 0x02U  /*!< Progress bar from empty to full */
} NEX_ProgressBar_Rotation;


/*--------------------- External Variables ---------------------*/
extern const char *NEX_Command[];
extern const char *NEX_Int_Command[];


/*--------------------- Function Prototypes ---------------------*/

/**
  * @brief  Initializes the dashboard UART communication.
  * @param  uart: Pointer to the UART handler (e.g., &huart2).
  * @param  data: Pointer to a NEX_Data structure that holds all runtime value pointers.
  * @retval HAL status.
  * @note   Must be called after MX_USARTx_UART_Init().
  */
HAL_StatusTypeDef Dashboard_Init(UART_HandleTypeDef *uart, NEX_Data *data);

/**
  * @brief  Initializes the dashboard pointer map.
  *         This function binds the internal pointers to user-defined runtime values.
  * @param  uart: Pointer to the UART handler used for Nextion communication.
  * @param  data: Pointer to a NEX_Data structure containing all relevant value addresses.
  * @retval None
  * @note   Must be called once after setting up values.
  */
void Dashboard_Bind(UART_HandleTypeDef *uart, NEX_Data *data);

/**
  * @brief  Refreshes the dashboard screen with the latest runtime data.
  * @retval HAL_StatusTypeDef
  *         - HAL_OK: Transmission successful
  *         - HAL_ERROR: UART transmission failed
  *
  * This function checks each field of the dashboard data and compares it with
  * previously sent values. Only changed values are transmitted to minimize
  * UART load and avoid redundant updates on the Nextion display.
  *
  * Should be called periodically in the main loop or task scheduler.
  */
HAL_StatusTypeDef Dashboard_Refresh(void);

/**
  * @brief  Performs a UART-based handshake with the Nextion display.
  *
  *         This function attempts to confirm communication with the Nextion screen
  *         by waiting for an "OK" response and replying with a standard connection command.
  *
  * @param  timeout: Maximum time to wait (in milliseconds) for a response per attempt.
  *
  * @retval HAL_OK    If handshake is successful.
  * @retval HAL_ERROR If no valid response is received within given retries.
  */
HAL_StatusTypeDef Nextion_Handshake(uint32_t timeout);

#endif // DASHBOARD_CONTROLS
