/**
 ******************************************************************************
 * @file           : mapping.h
 * @brief          : Integer and floating-point mapping functions for STM32
 ******************************************************************************
 * @author         : Hamza Enes Balahoroglu
 * @version        : v1.0
 * @date           : 08.07.2025
 *
 * @note
 * This header provides range mapping functions for both integer and float types,
 * designed to be lightweight and compatible with STM32 HAL-based projects.
 *
 * USAGE:
 * These functions help to scale sensor inputs or normalized values from one
 * numerical range to another. Useful in embedded control systems, sensor calibration,
 * or display value scaling.
 *
 ******************************************************************************
 */

#ifndef MAPPING
#define MAPPING

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

/**
  ******************************************************************************
  * @brief  Maps a float value from one range to another.
  *
  * @param  input: The input value to be mapped.
  * @param  in_min: The minimum value of the input range.
  * @param  in_max: The maximum value of the input range.
  * @param  out_min: The minimum value of the target range.
  * @param  out_max: The maximum value of the target range.
  * @retval float: The mapped output value within the target range.
  ******************************************************************************
  */
float Map_Float(float input, float in_min, float in_max, float out_min, float out_max);

#endif /* MAPPING */
