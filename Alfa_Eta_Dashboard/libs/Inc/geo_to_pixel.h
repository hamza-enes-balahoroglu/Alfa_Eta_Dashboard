/**
 ******************************************************************************
 * @file           : geo_to_pixel.h
 * @brief          : GPS coordinate to pixel conversion module - STM32 HAL compatible
 ******************************************************************************
 * @author         : Hamza Enes Balahoroğlu
 * @version        : v1.3
 * @date           : 25.06.2025
 *
 * @note
 * This header file provides functions and definitions for converting
 * GPS latitude and longitude data into pixel positions on a fixed map.
 * It includes GPS data filtering and icon angle calculation.
 *
 * - Designed for use with STM32CubeIDE and STM32 HAL library.
 * - The GPS module is expected to communicate at **9600 baudrate** via UART.
 *   Ensure this matches your UART peripheral configuration.
 *
 * Mismatched baudrates can result in corrupted data or no GPS lock at all.
 *
 * Designed for use with STM32CubeIDE and STM32 HAL library.
 *
 ******************************************************************************
 */

#ifndef GEO_TO_PIXEL
#define GEO_TO_PIXEL

#include "stm32f4xx_hal.h"
#include "mapping.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/*--------------------- Map Dimensions in Pixels ---------------------*/
#define MAP_X_SIZE 800.00f    /*!< Map width in pixels */
#define MAP_Y_SIZE 750.00f    /*!< Map height in pixels */

/*--------------------- Icon Size and Position Constants ---------------------*/
#define ICON_WIDTH  67        /*!< Width of the icon in pixels */
#define ICON_HEIGHT 67        /*!< Height of the icon in pixels */
#define ICON_X 640            /*!< Icon base x-coordinate on map */
#define ICON_Y 201            /*!< Icon base y-coordinate on map */

/*--------------------- Map Location ---------------------*/
#define MAP_X_MIN_VAL 0         /*!< Minimum horizontal offset (fully left) */
#define MAP_X_MAX_VAL 450       /*!< Maximum horizontal offset (fully right) */
#define MAP_Y_MIN_VAL -270      /*!< Minimum vertical offset (fully up) */
#define MAP_Y_MAX_VAL 0         /*!< Maximum vertical offset (fully down) */



/*--------------------- Geo Boundaries ---------------------*/
#define NW_lat 40.809190303f     /*!< Latitude of the top-left (NW) corner of the map */
#define NW_lon 29.353690785f     /*!< Longitude of the top-left (NW) corner of the map */
#define SE_lat 40.80401074f      /*!< Latitude of the bottom-right (SE) corner of the map */
#define SE_lon 29.36103314f      /*!< Longitude of the bottom-right (SE) corner of the map */

#define GPS_BUFFER_SIZE 100      /*!< Size of UART GPS data buffer */

typedef struct {
    int PixelX;                 /*!< Pixel X coordinate on the map */
    int PixelY;                 /*!< Pixel Y coordinate on the map */
    int IconAngle;              /*!< Orientation angle of the icon in degrees */
    int Lap;                    /*!< Current lap or iteration count */
} MapOffset;

typedef struct {
    float raw_lat;              /*!< Raw latitude value from GPS */
    float raw_lon;              /*!< Raw longitude value from GPS */
    float last_lat;             /*!< Last filtered latitude */
    float last_lon;             /*!< Last filtered longitude */
    float filtered_lat;         /*!< Filtered latitude for smoothing */
    float filtered_lon;         /*!< Filtered longitude for smoothing */
    float speed;                /*!< Speed in km/h */
} GPS_Data;


/**
 * @brief Structure to represent a GPS checkpoint with status and coordinates.
 */
typedef struct {
    uint8_t status;             /*!< Checkpoint status: 0 = not reached, 1 = reached */
    float lat;                  /*!< Latitude of the checkpoint */
    float lon;                  /*!< Longitude of the checkpoint */
} GPS_Checkpoint;


/**
  * @brief  Initializes the Geo to Pixel conversion module.
  *         Sets up UART handler and map data pointers.
  * @param  uart: Pointer to UART_HandleTypeDef for GPS communication.
  * @param  mapData: Pointer to MapOffset structure to hold map coordinates.
  * @retval None
  */
void Geo_To_Pixel_Init(UART_HandleTypeDef *uart, MapOffset *mapData);

/**
  * @brief  Binds UART handle and map data structure pointers for geolocation processing.
  *         Must be called before starting geo-to-pixel calculations.
  * @param  uart: Pointer to initialized UART_HandleTypeDef for GPS communication.
  * @param  mapData: Pointer to MapOffset structure to store computed pixel positions and icon angle.
  * @retval None
  */
void Geo_To_Pixel_Bind(UART_HandleTypeDef *uart, MapOffset *mapData);

/**
  * @brief  Runs the complete geolocation processing pipeline.
  *         Reads GPS data, filters it, converts coordinates to pixel values,
  *         calculates icon angle for display, and checks lap completion.
  *
  * @retval None
  */
void Run_GeoPipeline(void);


#endif // GEO_TO_PIXEL
