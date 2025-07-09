/**
 ******************************************************************************
 * @file           : geo_to_pixel.h
 * @brief          : GPS coordinate to pixel conversion module - STM32 HAL compatible
 ******************************************************************************
 * @author         : Hamza Enes Balahoroğlu
 * @version        : v1.2
 * @date           : 25.06.2025
 *
 * @note
 * This header file provides functions and definitions for converting
 * GPS latitude and longitude data into pixel positions on a fixed map.
 * It includes GPS data filtering and icon angle calculation.
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
  *         and calculates the icon angle for display.
  *
  * @retval None
  */
void Run_GeoPipeline(void);

/**
  * @brief  Reads GPS data from UART buffer and parses $GNRMC NMEA sentence.
  *         Converts latitude and longitude from NMEA format to decimal degrees,
  *         and updates global GPS data structure with raw coordinates and speed.
  *
  * @note   Parses only first valid $GNRMC sentence found in UART buffer.
  *
  * @retval None
  */
HAL_StatusTypeDef Read_GPS_Location(void);

/**
  * @brief  Converts NMEA latitude or longitude string to decimal degrees.
  *
  * @param  nmea: Pointer to NMEA coordinate string (format: ddmm.mmmm or dddmm.mmmm).
  * @retval Converted coordinate as float in decimal degrees.
  */
float NMEA_To_Decimal(char *nmea);

/**
  * @brief  Applies a simple distance-based filter to GPS coordinates.
  *         Updates filtered values only if movement exceeds threshold.
  *
  * @param  gps: Pointer to GPS_Data structure containing raw and filtered values.
  * @retval None
  */
void GPS_Filter(GPS_Data *gps);

/**
  * @brief  Calculates the great-circle distance between two GPS coordinates using the Haversine formula.
  *
  * @param  lat1: Latitude of the first point in degrees.
  * @param  lon1: Longitude of the first point in degrees.
  * @param  lat2: Latitude of the second point in degrees.
  * @param  lon2: Longitude of the second point in degrees.
  *
  * @retval Distance in meters between the two points.
  */
float GPS_CalcDistance(float lat1, float lon1, float lat2, float lon2);

/**
  * @brief  Converts filtered GPS coordinates (latitude, longitude) to pixel positions on the map.
  *
  *         Maps the geographic coordinates to corresponding pixel values using defined
  *         map boundaries and dimensions, then updates drawable positions accordingly.
  *
  * @retval None
  */
void Calculate_Geo_To_Pixel(void);

/**
  * @brief  Calculates and clamps the drawable pixel position of the map icon.
  *
  *         Translates GPS pixel coordinates into screen pixel positions relative to the icon,
  *         then clamps them within defined minimum and maximum bounds to prevent overflow.
  *
  * @param  gpsPixelX: X coordinate derived from GPS mapping.
  * @param  gpsPixelY: Y coordinate derived from GPS mapping.
  * @retval None
  */
void Get_Map_Draw_Position(int gpsPixelX, int gpsPixelY);

/**
  ******************************************************************************
  * @brief  Calculates the movement direction based on last known pixel position.
  *
  *         Computes and updates the icon's heading angle in degrees based on the
  *         difference between current and previous pixel coordinates.
  *
  *         Sets 0° as west and normalizes result to [0, 360).
  *
  * @retval None
  ******************************************************************************
  */
void Calculate_Icon_Angle(void);

/**
 ******************************************************************************
 * @brief Counts laps based on GPS checkpoint proximity
 * @retval None
 *
 * This function compares the current filtered GPS coordinates against
 * predefined checkpoints. If the distance to any checkpoint is below the
 * defined threshold (5 meters), it updates the checkpoint status.
 *
 * - If the first checkpoint is reached and the lap was already started,
 *   it checks for lap completion and increments the lap count.
 * - If another checkpoint is reached, it marks the lap as started.
 *
 * Should be called periodically after GPS data is filtered.
 ******************************************************************************
 */
static void Count_Lap(void);

/**
 ******************************************************************************
 * @brief  Checks if all GPS checkpoints have been passed
 * @retval uint8_t: 1 if all checkpoints are passed, 0 otherwise
 *
 * This function iterates through the global Checkpoints array to determine
 * whether all checkpoints have been marked as passed (`status == 1`).
 *
 * Typically used inside lap counting logic to verify lap completion.
 ******************************************************************************
 */
static uint8_t Is_Lap_Complete(void);

/**
 ******************************************************************************
 * @brief  Resets the status of all GPS checkpoints
 * @retval None
 *
 * Sets all checkpoint status values to 0 in the global `Checkpoints` array.
 * This is typically used after a lap is completed to prepare for the next one.
 ******************************************************************************
 */
static void Clear_Checkpoints(void);
#endif // GEO_TO_PIXEL
