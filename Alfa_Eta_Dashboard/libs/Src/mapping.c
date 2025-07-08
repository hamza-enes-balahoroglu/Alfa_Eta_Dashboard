/**
 ******************************************************************************
 * @file           : mapping.c
 * @brief          : Implementation of mapping functions for STM32 applications
 ******************************************************************************
 * @author         : Hamza Enes Balahoroglu
 * @version        : v1.0
 * @date           : 08.07.2025
 *
 * @details
 * This source file provides the implementation of integer and float mapping
 * functions using linear interpolation formulas.
 *
 * It includes:
 *  - Map_Int()    : Scales an integer from one range to another (truncates decimals)
 *  - Map_Float()  : Scales a float value from one range to another with precision
 *
 * Typical use cases:
 *  - Sensor input scaling
 *  - Display value conversion
 *  - Control range normalization
 *
 * Designed for use with STM32CubeIDE and STM32 HAL-based applications.
 *
 * @note
 * Ensure input range (in_max - in_min) is not zero to prevent division by zero.
 *
 ******************************************************************************
 */

#include "mapping.h"

/**
  * @brief  Maps an integer value from one range to another using linear interpolation.
  *
  *         Formula used:
  *         y = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
  *
  *         Useful when scaling sensor data, UI inputs or any raw value from hardware.
  *
  * @param  input:       Integer value to be scaled.
  * @param  in_min:      Lower bound of input range.
  * @param  in_max:      Upper bound of input range.
  * @param  out_min:     Lower bound of target range.
  * @param  out_max:     Upper bound of target range.
  *
  * @retval int:         Scaled result within the specified output range.
  *
  * @note   Integer division is used; fractional precision is discarded.
  *         Make sure (in_max - in_min) ≠ 0 to avoid division by zero.
  */
int Map_Int(int input, int in_min, int in_max, int out_min, int out_max)
{
    int input_range = in_max - in_min;
    int output_range = out_max - out_min;

    return ((input - in_min) * output_range) / input_range + out_min;
}

/**
  * @brief  Performs linear mapping of a float value from one range to another.
  *
  *         This function scales the input from the input range [in_min, in_max]
  *         to the output range [out_min, out_max] using the formula:
  *
  *         out = ((out_max - out_min) * (input - in_min)) / (in_max - in_min) + out_min
  *
  *         Useful for normalizing or converting sensor values to UI units, percentages, etc.
  *
  * @param  input: Input value to be mapped.
  * @param  in_min: Minimum bound of the input range.
  * @param  in_max: Maximum bound of the input range.
  * @param  out_min: Minimum bound of the output range.
  * @param  out_max: Maximum bound of the output range.
  * @retval float: Mapped output value.
  */
float Map_Float(float input, float in_min, float in_max, float out_min, float out_max)
{
    float input_range = in_max - in_min;
    float ref_range = out_max - out_min;

    return ((ref_range * (input - in_min)) / input_range) + out_min;
}
