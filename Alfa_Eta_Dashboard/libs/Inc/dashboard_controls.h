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
#include <string.h>
#include <stdio.h>


/* Structure to hold pointers to all dashboard data values */
typedef struct {
    int *speed;
    int *batteryValue;
    int *powerKW; 		// 0-6
    int *packVoltage;
    int *maxVoltage;
    int *minVoltage;
    int *batteryTemp;
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
	SET_SPEED_COMMAND				= 0x00U,  // Speed number (e.g., RPM, km/h)

	/*--------------------- Battery Display ---------------------*/
	SET_BATTERY_NUMBER_COMMAND		= 0x01U,   // Battery value (number)
	SET_BATTERY_PROGRESS_BAR_COMMAND= 0x02U, // Battery bar (percentage)

	/*--------------------- Power Display ---------------------*/
	SET_KW_NUMBER_COMMAND			= 0x03U,           // Power in kW
	SET_KW_PROGRESS_BAR_COMMAND		= 0x04U,     // Power as progress bar

	/*--------------------- Voltage Display ---------------------*/
	SET_PACK_VOLTAGE				= 0x05U,	// Total battery voltage
	SET_MAX_VOLTAGE					= 0x06U,	// Maximum battery voltage
	SET_MIN_VOLTAGE					= 0x07U,    // Minimum battery voltage

	/*--------------------- Temperature ---------------------*/
	SET_BATTERY_TEMPERATURE			= 0x08U   // Battery temperature

} NEX_Int_Command_ID;
extern const char *NEX_Int_Command[];


/*--------------------- Progress Bar ---------------------*/
typedef enum
{
   PROGRESS_BAR_REVERSE     		= 0x01U,  // Used to reverse the progress bar direction (100 to 0)
   PROGRESS_BAR_NO_REVERSE			= 0x02U  // Used for normal progress bar direction (0 to 100)
}NEX_ProgressBar_Rotation;


/*--------------------- Function Prototypes ---------------------*/

/**
 * @brief Initializes the dashboard UART communication.
 * @param uart: Pointer to the UART handler (e.g., &huart2).
 * @retval HAL status.
 * @note Must be called after MX_USARTx_UART_Init().
 */
HAL_StatusTypeDef Dashboard_Init(UART_HandleTypeDef *uart);

/**
 * @brief Initializes the dashboard pointer map (must be called once after setting up values).
 * @param data: Pointer to a structure containing all relevant value addresses.
 */
void Dashboard_Bind(NEX_Data *data);

/**
 * @brief Refreshes the dashboard screen with the latest data.
 * @retval HAL status.
 */
HAL_StatusTypeDef Dashboard_Refresh(void);

/**
 * @brief Sends a NEX_Command to the Nextion display.
 * @param cmd: NEX_CommandID value to command
 */
void Send_Nextion_Command(NEX_CommandID cmd);

/**
 * @brief Sends a string to the Nextion display.
 * @param cmd: Null-terminated command string.
 */
void Send_String_To_Nextion(char *str);

/**
 * @brief Sends a formatted command with integer value to the display.
 * @param cmd: Command string with %d placeholder.
 * @param val: Integer value to insert.
 */
void Send_Nextion_Int(NEX_Int_Command_ID cmdID, int val);

/**
 * @brief Sends a scaled progress bar value based on a given range.
 *        Supports normal or reversed progress direction.
 *
 * @param cmd: Command string with %d placeholder (e.g., "jBt.val=%d").
 * @param val: Current input value to be mapped.
 * @param maxVal: Maximum expected value of the input.
 * @param minVal: Minimum expected value of the input.
 * @param reverseProgressBar: Set to PROGRESS_BAR_REVERSE or 1 for reversed progress (100 to 0),
 *                             or PROGRESS_BAR_NO_REVERSE or 0 for normal progress (0 to 100).
 *
 * @retval HAL_OK if the value is within the range and sent successfully.
 *         HAL_ERROR if the value is out of the defined range.
 */
HAL_StatusTypeDef Send_Nextion_Progress_Bar(NEX_Int_Command_ID cmd, int val, int maxVal, int minVal, NEX_ProgressBar_Rotation reverseProgressBar);

/**
 * @brief Performs a UART-based handshake with the Nextion screen.
 * @param timeout: Maximum wait time for response in milliseconds.
 * @retval HAL_OK on successful handshake, HAL_ERROR otherwise.
 */
HAL_StatusTypeDef Nextion_Handshake(uint32_t timeout);

/**
 * @brief Sends command termination bytes (0xFF 0xFF 0xFF).
 */
void Command_Terminator(void);

/**
 * @brief Maps a value from one range to another.
 * @param input: Input value.
 * @param maxValue: Source range maximum.
 * @param minValue: Source range minimum.
 * @param refMaxValue: Target range maximum.
 * @param refMinValue: Target range minimum.
 * @return Mapped integer result.
 */
int Map(uint32_t input, uint32_t maxValue, uint32_t minValue, uint32_t refMaxValue, uint32_t refMinValue);

#endif // DASHBOARD_CONTROLS
