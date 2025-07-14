/**
 ******************************************************************************
 * @file           : dashboard_controls.h
 * @brief          : Nextion display control library - STM32 HAL compatible
 ******************************************************************************
 * @author         : Hamza Enes Balahoroglu
 * @version        : v1.3
 * @date           : 14.07.2025
 *
 * @note
 * This header file is intended to be used in STM32CubeIDE projects.
 * It provides macros and function declarations to interface a Nextion
 * display via UART using STM32 HAL libraries.
 *
 * IMPORTANT:
 * - NEX_Init() must be called AFTER MX_USARTx_UART_Init() in main.c
 *   to ensure the UART handle is correctly initialized.
 * - The Nextion display must be configured to communicate at **115200 Baud Rate**.
 *   Make sure this setting matches both the UART peripheral and the Nextion editor config.
 *
 * Failing to match the baud rate or calling initialization before UART setup
 * will cause communication failures or random garbage characters on screen.
 *
 *           _  __        ______ _______
 *     /\   | |/ _|      |  ____|__   __|/\
 *    /  \  | | |_ __ _  | |__     | |  /  \
 *   / /\ \ | |  _/ _` | |  __|    | | / /\ \
 *  / ____ \| | || (_| | | |____   | |/ ____ \
 * /_/    \_\_|_| \__,_| |______|  |_/_/    \_\
 ******************************************************************************
 */

#ifndef DASHBOARD_CONTROLS
#define DASHBOARD_CONTROLS

#include "stm32f4xx_hal.h"
#include "geo_to_pixel.h"
#include "mapping.h"
#include <string.h>
#include <stdio.h>

#define NEX_SCREEN_SIZE_X 800  /*!< Width of the Nextion display in pixels */
#define NEX_SCREEN_SIZE_Y 480  /*!< Height of the Nextion display in pixels */

#define NEX_HANDSHAKE_ATTEMPTS 10  /*!< Number of times the handshake command will be sent to ensure reliable UART connection */

#define NEX_BATTERY_PROGRESS_BAR_MIN_VAL 0    /*!< Minimum value for the battery level progress bar */
#define NEX_BATTERY_PROGRESS_BAR_MAX_VAL 100  /*!< Maximum value for the battery level progress bar */

#define NEX_KW_PROGRESS_BAR_MIN_VAL 0         /*!< Minimum value for the power (kW) progress bar */
#define NEX_KW_PROGRESS_BAR_MAX_VAL 5         /*!< Maximum value for the power (kW) progress bar */


/**
 * @brief Enum for selecting gear states via dashboard.
 *
 * This enum maps each gear state to a specific value understood by
 * the Nextion display. Instead of sending raw values (e.g., 0, 1, 2),
 * using this enum improves readability and reduces confusion.
 *
 * Gear Values:
 *     - NEUTRAL : 0x00 → Idle gear (no torque)
 *     - DRIVE   : 0x01 → Forward gear
 *     - REVERSE : 0x02 → Backward gear
 */
typedef enum {
	NEX_GEAR_NEUTRAL   = 0x00U,  /*!< Neutral gear */
	NEX_GEAR_DRIVE 	   = 0x01U,  /*!< Forward gear */
    NEX_GEAR_REVERSE   = 0x02U   /*!< Reverse gear */
} NEX_Gears;

/**
 * @brief  Represents binary ON/OFF states for dashboard indicators and controls.
 *
 * This enum is used for flags and controls that have two possible states:
 * active (ON) or inactive (OFF). Commonly used for handbrake status,
 * signal indicators, lights, battery warnings, and similar dashboard elements.
 *
 * @note
 * Designed to improve code readability by replacing raw binary flags (1/0)
 * with self-explanatory enum values.
 *
 ******************************************************************************
 */
typedef enum {
    NEX_STATE_OFF = 0x00U,   /*!< Feature is inactive or turned off */
    NEX_STATE_ON  = 0x01U    /*!< Feature is active or turned on */
} NEX_State;

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
    NEX_Gears *gear;               /*!< Gear position: 0=N, 1=D, 2=R */
    NEX_State *handbrake;          /*!< Handbrake: 0=Off, 1=On */
    NEX_State *signalLeft;         /*!< Left signal: 0/1 */
    NEX_State *signalRight;        /*!< Right signal: 0/1 */
    NEX_State *connWarn;           /*!< Connection warning: 0/1 */
    NEX_State *battWarn;           /*!< Battery warning: 0/1 */
    NEX_State *lights;             /*!< Lights: 0=Off, 1=On */
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
    NEX_Gears gear;
    NEX_State handbrake;
    NEX_State signalLeft;
    NEX_State signalRight;
    NEX_State connWarn;
    NEX_State battWarn;
    NEX_State lights;
} NEX_CachedData;


/*--------------------- Function Prototypes ---------------------*/

/**
  * @brief  Initializes the dashboard UART communication.
  * @param  uart: Pointer to the UART handle used for Nextion communication (e.g., &huart2).
  * @param  data: Pointer to a NEX_Data structure that holds all runtime value pointers.
  * @retval HAL_StatusTypeDef
  *         - HAL_OK: Initialization and handshake successful.
  *         - HAL_ERROR: UART or data pointer is NULL, or handshake failed.
  * @note   This function must be called after MX_USARTx_UART_Init().
  *         It binds the provided pointers and performs an initial handshake with the display.
  */

HAL_StatusTypeDef NEX_Init(UART_HandleTypeDef *uart, NEX_Data *data);

/**
  * @brief  Binds internal references to UART and dashboard data structures.
  * @param  uart: Pointer to UART handle used for Nextion communication.
  * @param  data: Pointer to the runtime data structure for dashboard updates.
  * @retval HAL_StatusTypeDef
  *         - HAL_OK: Binding was successful.
  *         - HAL_ERROR: Provided pointers are NULL.
  */
HAL_StatusTypeDef NEX_Bind(UART_HandleTypeDef *uart, NEX_Data *data);

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
HAL_StatusTypeDef NEX_Refresh(void);

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
HAL_StatusTypeDef NEX_Handshake(uint32_t timeout);

#endif // DASHBOARD_CONTROLS
