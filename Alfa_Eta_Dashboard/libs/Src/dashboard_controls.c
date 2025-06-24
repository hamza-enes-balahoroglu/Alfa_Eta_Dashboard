/**
 ******************************************************************************
 * @file           : Dashboard_controls.c
 * @brief          : Sending Nextion commands via UART
 ******************************************************************************
 * @details
 * This file is designed to send string and numeric data to the Nextion display
 * using STM32 HAL libraries via UART.
 *
 * It includes helper functions for UART communication with the Nextion display.
 *
 ******************************************************************************
 */

#include "dashboard_controls.h"

const char *NEX_Command[] = {
	/*--------------------- Static Nextion Commands ---------------------*/
	"con=1",  // Handshake command to confirm connection
	/*--------------------- Gear Display ---------------------*/
	"pGr.pic=19",     // Drive gear icon
	"pGr.pic=20",   // Neutral gear icon
	"pGr.pic=21",   // Reverse gear icon

	/*--------------------- Handbrake ---------------------*/
	"pHb.aph=127",   // Handbrake engaged
	"pHb.aph=0",    // Handbrake released

	/*--------------------- Signal Lights ---------------------*/
	"pSL.aph=127",
	"pSL.aph=0",
	"pSR.aph=127",
	"pSR.aph=0",

	/*--------------------- Warning Indicators ---------------------*/
	"pCW.aph=127",
	"pCW.aph=0",
	"pBW.aph=127",
	"pBW.aph=0",

	/*--------------------- Lights ---------------------*/
	"pLt.aph=127",
	"pLt.aph=0",
};


const char *NEX_Int_Command[] = {
	/*--------------------- Speed Display ---------------------*/
	"nSd.val=%d",  // Speed number (e.g., RPM, km/h)

	/*--------------------- Battery Display ---------------------*/
	"nBt.val=%d",     // Battery value (number)
	"jBt.val=%d", // Battery bar (percentage)

	/*--------------------- Power Display ---------------------*/
	"nKW.val=%d",          // Power in kW
	"jKW.val=%d",    // Power as progress bar

	/*--------------------- Voltage Display ---------------------*/
	"xBV.val=%d",   // Total battery voltage
	"xBMa.val=%d",      // Maximum battery voltage
	"xBMi.val=%d",      // Minimum battery voltage

	/*--------------------- Temperature ---------------------*/
	"xBtT.val=%d",  // Battery temperature


	/*--------------------- Map ---------------------*/
	"pMap.x=%d",  // Map x value
	"pMap.y=%d",  // Map y value
	"zIc.val=%d", // Icon direction
	"nLap.val=%d" // Lap counter
};


/* Global pointer to UART handle used for communication */
static UART_HandleTypeDef *_uart;

/* Global pointer to binded data */
static NEX_Data *_dashboard = NULL;

/* Holds previous values to compare and avoid redundant updates */
static NEX_CachedData _previousValues = {0};

/* 3-byte terminator required for every Nextion command */
static const uint8_t COMMAND_END[3] = {0xFF, 0xFF, 0xFF};

/**
 * @brief Initializes the connection with the Nextion display (includes handshake)
 *
 * @note This function must be called *after* initializing the UART
 *       (e.g., after MX_USARTx_UART_Init()). Otherwise, the `_uart` pointer
 *       will remain NULL and communication errors may occur.
 *
 * @param uart: Pointer to UART interface to use
 * @retval HAL_OK or HAL_ERROR depending on handshake result
 */
HAL_StatusTypeDef Dashboard_Init(UART_HandleTypeDef *uart, NEX_Data *data){

    Dashboard_Bind(uart, data);
    return Nextion_Handshake(2000); // 2 seconds timeout for handshake
}

/**
  * @brief  Binds the dashboard data structure containing pointers to all runtime values.
  *         This function must be called once before using Dashboard_Refresh().
  * @param  data: Pointer to a user-defined DashboardData structure with field addresses set.
  * @retval None
  */
void Dashboard_Bind(UART_HandleTypeDef *uart, NEX_Data *data){
	_uart = uart;
    _dashboard = data;
}

/**
  * @brief  Refreshes the Nextion display with updated system data only.
  *         Only changed values are sent to reduce UART traffic.
  * @note   This function checks if the screen is connected first using handshake.
  * @retval HAL_OK if update sent successfully, HAL_ERROR otherwise.
  */
HAL_StatusTypeDef Dashboard_Refresh(void) {



    /* Numeric values */
    if (*_dashboard->speed != _previousValues.speed) {
        Send_Nextion_Int(SET_SPEED_COMMAND, *_dashboard->speed);
        _previousValues.speed = *_dashboard->speed;
    }

    if (*_dashboard->batteryValue != _previousValues.batteryValue) {
        Send_Nextion_Int(SET_BATTERY_NUMBER_COMMAND, *_dashboard->batteryValue);
        HAL_StatusTypeDef a = Send_Nextion_Progress_Bar(SET_BATTERY_PROGRESS_BAR_COMMAND, *_dashboard->batteryValue, 100, 0, PROGRESS_BAR_NO_REVERSE);
        if (a == HAL_ERROR)
             return HAL_ERROR;
        _previousValues.batteryValue = *_dashboard->batteryValue;
    }


    if (*_dashboard->powerKW != _previousValues.powerKW) {
        Send_Nextion_Int(SET_KW_NUMBER_COMMAND, *_dashboard->powerKW);
        if (Send_Nextion_Progress_Bar(SET_KW_PROGRESS_BAR_COMMAND, *_dashboard->powerKW, 6, 0, PROGRESS_BAR_REVERSE) == HAL_ERROR)
            return HAL_ERROR;
        _previousValues.powerKW = *_dashboard->powerKW;
    }


    if (*_dashboard->packVoltage != _previousValues.packVoltage) {
        Send_Nextion_Int(SET_PACK_VOLTAGE, *_dashboard->packVoltage);
        _previousValues.packVoltage = *_dashboard->packVoltage;
    }

    if (*_dashboard->maxVoltage != _previousValues.maxVoltage) {
        Send_Nextion_Int(SET_MAX_VOLTAGE, *_dashboard->maxVoltage);
        _previousValues.maxVoltage = *_dashboard->maxVoltage;
    }

    if (*_dashboard->minVoltage != _previousValues.minVoltage) {
        Send_Nextion_Int(SET_MIN_VOLTAGE, *_dashboard->minVoltage);
        _previousValues.minVoltage = *_dashboard->minVoltage;
    }

    if (*_dashboard->batteryTemp != _previousValues.batteryTemp) {
        Send_Nextion_Int(SET_BATTERY_TEMPERATURE, *_dashboard->batteryTemp);
        _previousValues.batteryTemp = *_dashboard->batteryTemp;
    }

    if (_dashboard->mapData->PixelX != _previousValues.mapData.PixelX) {
        Send_Nextion_Int(SET_MAP_X, _dashboard->mapData->PixelX);
        _previousValues.mapData.PixelX = _dashboard->mapData->PixelX;
    }

    if (_dashboard->mapData->PixelY != _previousValues.mapData.PixelY) {
        Send_Nextion_Int(SET_MAP_Y, _dashboard->mapData->PixelY);
        _previousValues.mapData.PixelY = _dashboard->mapData->PixelY;
    }

    if (_dashboard->mapData->IconAngle != _previousValues.mapData.IconAngle) {
        Send_Nextion_Int(SET_MAP_ICON, _dashboard->mapData->IconAngle);
        _previousValues.mapData.IconAngle = _dashboard->mapData->IconAngle;
    }

    if (_dashboard->mapData->Lap != _previousValues.mapData.Lap) {
        Send_Nextion_Int(SET_MAP_LAP, _dashboard->mapData->Lap);
        _previousValues.mapData.Lap = _dashboard->mapData->Lap;
    }

    /* Gear */
    if (*_dashboard->gear != _previousValues.gear) {
        switch (*_dashboard->gear) {
            case 0: Send_Nextion_Command(SET_GEAR_NEUTRAL); break;
            case 1: Send_Nextion_Command(SET_GEAR_DRIVE); break;
            case 2: Send_Nextion_Command(SET_GEAR_REVERSE); break;
        }
        _previousValues.gear = *_dashboard->gear;
    }

    /* Warnings */
    if (*_dashboard->handbrake != _previousValues.handbrake) {
        Send_Nextion_Command(*_dashboard->handbrake ? SET_HANDBREAK_ON : SET_HANDBREAK_OFF);
        _previousValues.handbrake = *_dashboard->handbrake;
    }

    if (*_dashboard->signalLeft != _previousValues.signalLeft) {
        Send_Nextion_Command(*_dashboard->signalLeft ? SET_SIGNAL_LEFT_ON : SET_SIGNAL_LEFT_OFF);
        _previousValues.signalLeft = *_dashboard->signalLeft;
    }

    if (*_dashboard->signalRight != _previousValues.signalRight) {
        Send_Nextion_Command(*_dashboard->signalRight ? SET_SIGNAL_RIGHT_ON : SET_SIGNAL_RIGHT_OFF);
        _previousValues.signalRight = *_dashboard->signalRight;
    }

    if (*_dashboard->connWarn != _previousValues.connWarn) {
        Send_Nextion_Command(*_dashboard->connWarn ? SET_CONNECTION_WARNING_ON : SET_CONNECTION_WARNING_OFF);
        _previousValues.connWarn = *_dashboard->connWarn;
    }

    if (*_dashboard->battWarn != _previousValues.battWarn) {
        Send_Nextion_Command(*_dashboard->battWarn ? SET_BATTERY_WARNING_ON : SET_BATTERY_WARNING_OFF);
        _previousValues.battWarn = *_dashboard->battWarn;
    }

    if (*_dashboard->lights != _previousValues.lights) {
        Send_Nextion_Command(*_dashboard->lights ? SET_LIGHTS_ON : SET_LIGHTS_OFF);
        _previousValues.lights = *_dashboard->lights;
    }

    return HAL_OK;
}

/**
 * @brief Sends a plain string command to the Nextion display
 *
 * @param cmd: Command string to send
 */
void Send_Nextion_Command(NEX_CommandID cmdID)
{
    Send_String_To_Nextion((char *)NEX_Command[cmdID]);
}

/**
 * @brief Sends a plain string command to the Nextion display
 *
 * @param cmd: Command string to send
 */
void Send_String_To_Nextion(char *str)
{
    HAL_UART_Transmit(_uart, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
    Command_Terminator(); // Send 3-byte terminator at the end of the command
}
/**
 * @brief Formats a command with an integer value and sends it
 *
 * @param cmd: Command string containing %d format
 * @param val: Integer value to insert
 */
void Send_Nextion_Int(NEX_Int_Command_ID cmdID, int val){
    char command[20]; // 20-character command buffer
    const char *cmd = NEX_Int_Command[cmdID];
    sprintf(command, cmd, val);  // Create formatted string
    Send_String_To_Nextion(command);  // Send via UART
}


/**
 * @brief Maps a given value within a specified range and sends it
 *        to a Nextion display progress bar. Supports normal or reversed direction.
 *
 * @param cmd: Command string containing %d placeholder (e.g., "jBt.val=%d")
 * @param val: The actual data value to be visualized
 * @param maxVal: The expected maximum value of the input range
 * @param minVal: The expected minimum value of the input range
 * @param reverseProgressBar: If 1, the progress bar will be reversed (100 to 0);
 *                             if 0, it will increase normally (0 to 100)
 *s
 * @retval HAL_OK: The value was within the range, successfully mapped and sent
 *         HAL_ERROR: The value was outside the defined range
 */
HAL_StatusTypeDef Send_Nextion_Progress_Bar(NEX_Int_Command_ID cmdID, int val, int maxVal, int minVal, NEX_ProgressBar_Rotation reverseProgressBar){
    if (minVal <= val && val <=maxVal){
    	int mapVal;
    	if(reverseProgressBar == PROGRESS_BAR_REVERSE)
    		mapVal = 100 - Map_Int(val, minVal, maxVal, 0, 100);  // Map to 0-100 range
    	else if(reverseProgressBar == PROGRESS_BAR_NO_REVERSE)
    		mapVal = Map_Int(val, minVal, maxVal, 0, 100);

        Send_Nextion_Int(cmdID, (uint8_t)mapVal);
        return HAL_OK;
    } else {
        return HAL_ERROR;  // Value out of range
    }
}

/**
 * @brief Maps an input value from one range to another
 *
 * @param input: Value to map
 * @param maxValue: Input maximum
 * @param minValue: Input minimum
 * @param refMaxValue: Target maximum
 * @param refMinValue: Target minimum
 * @retval Mapped output value
 */
int Map_Int(int input, int in_min, int in_max, int out_min, int out_max)
{
    //uint32_t input_range = in_max - in_min + 1;
    //uint32_t ref_range = out_max - out_min + 1;

    //return ((ref_range * (input - in_min)) / input_range) + out_min;


    int input_range = in_max - in_min;
    int output_range = out_max - out_min;

    return ((input - in_min) * output_range) / input_range + out_min;
}

/**
 * @brief Sends the standard 3-byte command terminator to Nextion
 */
void Command_Terminator(void){
    HAL_UART_Transmit(_uart, (uint8_t*)COMMAND_END, sizeof(COMMAND_END), 100);
}

/**
 * @brief Performs handshake to validate communication with Nextion screen.
 *
 * @param timeout: Timeout value for UART receive (in ms)
 * @retval HAL_OK: "OK" message received successfully
 *         HAL_ERROR: Valid response not received
 */
HAL_StatusTypeDef Nextion_Handshake(uint32_t timeout){
    uint8_t rx_buffer[10]; // Buffer for receiving data

    for (int i = 0; i < 5; i++) {
        HAL_UART_Receive(_uart, rx_buffer, 2, timeout);  // Wait for 2 bytes
        Send_Nextion_Command(CONNECTION_OK);  // Send "con=1" command

        if (rx_buffer[0] == 'O' && rx_buffer[1] == 'K') {
            return HAL_OK;  // "OK" received from screen
        }
    }
    return HAL_ERROR;  // No valid response received
}
