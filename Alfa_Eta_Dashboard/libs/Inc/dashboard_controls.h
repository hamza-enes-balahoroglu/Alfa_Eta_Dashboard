/**
 ******************************************************************************
 * @file           : dashboard_controls.h
 * @brief          : Nextion display control library - STM32 HAL compatible
 ******************************************************************************
 * @author         : Hamza Enes Balahoroglu
 * @version        : v1.0
 * @date           : 09.06.2025
 *
 * @note
 * This header file is intended to be used in STM32CubeIDE projects.
 * It provides macros and function declarations to interface a Nextion
 * display via UART using STM32 HAL libraries.
 *
 * IMPORTANT:
 * Dashboard_Init() must be called AFTER MX_USARTx_UART_Init() in main.c
 * to ensure the UART handle is correctly initialized. Calling it before
 * initialization may cause communication failures with the Nextion display.
 *
 ******************************************************************************
 */

#ifndef DASHBOARD_CONTROLS
#define DASHBOARD_CONTROLS

#include "stm32f4xx_hal.h"
#include "geo_to_pixel.h"
#include <string.h>
#include <stdio.h>

#define NEX_SCREEN_SIZE_X 800
#define NEX_SCREEN_SIZE_Y 480

/* Structure to hold pointers to all dashboard data values */
typedef struct {
    int *speed;
    int *batteryValue;
    int *powerKW; 		// 0-6
    int *packVoltage;
    int *maxVoltage;
    int *minVoltage;
    int *batteryTemp;
    MapOffset *mapData;
    int *gear;         // 0: N, 1: D, 2: R
    int *handbrake;    // 0: Off, 1: On
    int *signalLeft;   // 0/1
    int *signalRight;  // 0/1
    int *connWarn;     // 0/1
    int *battWarn;     // 0/1
    int *lights;       // 0/1
} NEX_Data;


/* Structure to cache previous values to all dashboard data values */
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

typedef enum {
	/*--------------------- Static Nextion Commands ---------------------*/
	CONNECTION_OK 					= 0x00U, 	// Handshake command to confirm connection

	/*--------------------- Gear Display ---------------------*/
	SET_GEAR_DRIVE 					= 0x01U,	// Drive gear icon
	SET_GEAR_NEUTRAL 				= 0x02U,	// Neutral gear icon
	SET_GEAR_REVERSE 				= 0x03U,	// Reverse gear icon

	/*--------------------- Handbrake ---------------------*/
	SET_HANDBREAK_ON				= 0x04U,	// Handbrake engaged
	SET_HANDBREAK_OFF				= 0x05U,    // Handbrake released

	/*--------------------- Signal Lights ---------------------*/
	SET_SIGNAL_LEFT_ON				= 0x06U,
	SET_SIGNAL_LEFT_OFF				= 0x07U,
	SET_SIGNAL_RIGHT_ON				= 0x08U,
	SET_SIGNAL_RIGHT_OFF			= 0x09U,

	/*--------------------- Warning Indicators ---------------------*/
	SET_CONNECTION_WARNING_ON		= 0x0AU,
	SET_CONNECTION_WARNING_OFF		= 0x0BU,
	SET_BATTERY_WARNING_ON			= 0x0CU,
	SET_BATTERY_WARNING_OFF			= 0x0DU,

	/*--------------------- Lights ---------------------*/
	SET_LIGHTS_ON					= 0x0EU,
	SET_LIGHTS_OFF					= 0x0FU

} NEX_CommandID;

extern const char *NEX_Command[];

typedef enum {
	/*--------------------- Speed Display ---------------------*/
	SET_SPEED_COMMAND				= 0x00U,  	// Speed number (e.g., RPM, km/h)

	/*--------------------- Battery Display ---------------------*/
	SET_BATTERY_NUMBER_COMMAND		= 0x01U,   	// Battery value (number)
	SET_BATTERY_PROGRESS_BAR_COMMAND= 0x02U, 	// Battery bar (percentage)

	/*--------------------- Power Display ---------------------*/
	SET_KW_NUMBER_COMMAND			= 0x03U,   	// Power in kW
	SET_KW_PROGRESS_BAR_COMMAND		= 0x04U,   	// Power as progress bar

	/*--------------------- Voltage Display ---------------------*/
	SET_PACK_VOLTAGE				= 0x05U,	// Total battery voltage
	SET_MAX_VOLTAGE					= 0x06U,	// Maximum battery voltage
	SET_MIN_VOLTAGE					= 0x07U,    // Minimum battery voltage

	/*--------------------- Temperature ---------------------*/
	SET_BATTERY_TEMPERATURE			= 0x08U,   	// Battery temperature

	/*--------------------- Temperature ---------------------*/
	SET_MAP_X						= 0x09U,   	// Map x value
	SET_MAP_Y						= 0x0AU,   	// Map y value
	SET_MAP_ICON					= 0x0BU,
	SET_MAP_LAP						= 0x0CU
} NEX_Int_Command_ID;
extern const char *NEX_Int_Command[];


/*--------------------- Progress Bar ---------------------*/
typedef enum
{
   PROGRESS_BAR_REVERSE     		= 0x01U,  	// Used to reverse the progress bar direction (100 to 0)
   PROGRESS_BAR_NO_REVERSE			= 0x02U  	// Used for normal progress bar direction (0 to 100)
}NEX_ProgressBar_Rotation;


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
  *         Sends only changed values to reduce UART traffic.
  * @retval HAL_OK on success, HAL_ERROR on transmission failure.
  */
HAL_StatusTypeDef Dashboard_Refresh(void);

/**
  * @brief  Sends a pre-defined command to the Nextion display using command ID.
  * @param  cmdID: Enum key for the Nextion command array (e.g., SET_GEAR_DRIVE).
  */
void Send_Nextion_Command(NEX_CommandID cmd);

/**
  * @brief  Sends a null-terminated command string to the Nextion display.
  *         Automatically appends the 3-byte command terminator.
  * @param  str: Command string (e.g., "page main").
  */
void Send_String_To_Nextion(char *str);

/**
  * @brief  Sends a formatted string with an integer value to the Nextion display.
  *         Useful for numeric values like speed, voltage, etc.
  * @param  cmdID: Enum key for integer-based commands with %d format.
  * @param  val: Integer value to inject into command string.
  */
void Send_Nextion_Int(NEX_Int_Command_ID cmdID, int val);

/**
  * @brief  Sends a scaled progress bar value to the Nextion display.
  *
  *         Maps the input value from its actual range to 0-100 scale and sends it to the progress bar.
  *         Supports normal and reversed progress bar directions.
  *
  * @param  cmdID:             Enum for the integer command string with a %d placeholder.
  * @param  val:               Current input value.
  * @param  maxVal:            Maximum expected input value.
  * @param  minVal:            Minimum expected input value.
  * @param  reverseProgressBar: Specifies progress direction:
  *                             - PROGRESS_BAR_REVERSE (1): Progress bar decreases from 100 to 0.
  *                             - PROGRESS_BAR_NO_REVERSE (0): Progress bar increases from 0 to 100.
  *
  * @retval HAL_OK if the value was within the expected range and sent successfully.
  * @retval HAL_ERROR if the value was out of range.
  */
HAL_StatusTypeDef Send_Nextion_Progress_Bar(NEX_Int_Command_ID cmd, int val, int maxVal, int minVal, NEX_ProgressBar_Rotation reverseProgressBar);

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

/**
  * @brief  Sends the standard 3-byte command terminator to the Nextion display.
  *
  * @note   Nextion protocol requires every command to end with 0xFF 0xFF 0xFF.
  *         This function transmits those bytes over the selected UART.
  *
  * @retval None
  */
void Command_Terminator(void);

/**
  * @brief  Maps an integer input value from one range to another.
  *
  * @param  input:       Value to be mapped.
  * @param  in_min:      Minimum of the input range.
  * @param  in_max:      Maximum of the input range.
  * @param  out_min:     Minimum of the output range.
  * @param  out_max:     Maximum of the output range.
  *
  * @retval int:         Mapped output value scaled to target range.
  *
  * @note   This function is specific to integer values. For floating-point precision,
  *         use a dedicated float version.
  */
int Map_Int(int input, int in_min, int in_max, int out_min, int out_max);

#endif // DASHBOARD_CONTROLS
