/**
 ******************************************************************************
 * @file           : geo_to_pixel.c
 * @brief          : Implementation of GPS to pixel coordinate conversion - STM32 HAL compatible
 ******************************************************************************
 * @author         : Hamza Enes Balahoroğlu
 * @version        : v1.4
 * @date           : 25.06.2025
 *
 * @details
 * This file contains the implementation of functions for handling GPS data and
 * converting geographical coordinates to pixel positions on a predefined map layout.
 *
 * It includes:
 *  - Parsing NMEA-formatted GPS strings
 *  - Filtering GPS position data
 *  - Mapping latitude and longitude to x/y pixel coordinates
 *  - Computing directional angle of movement
 *
 * Designed for use with STM32CubeIDE and STM32 HAL libraries.
 *
 * @note
 * GPS-related functions should be called after UART and GPS module initialization.
 * Ensure the coordinate mapping limits (MAP_X/Y_MIN/MAX) match the map's resolution.
 *
 ******************************************************************************
 */

#include "geo_to_pixel.h"

/* Private variables ---------------------------------------------------------*/

/**
 * @brief UART handle used for GPS communication.
 */
static UART_HandleTypeDef *_uart = NULL;

/**
 * @brief Buffer to store raw NMEA sentences received from the GPS module.
 */
char gps_buffer[GPS_BUFFER_SIZE];

/**
 * @brief  Pointer to the external map data structure used to update display elements such as position,
 * 		   direction, and lap count.
 */
static MapOffset *_mapData = NULL;

/**
 * @brief Cached version of the map data used for detecting visual updates (e.g. angle, position).
 */
static MapOffset _mapCachedData = {
    .PixelX     = 0,
    .PixelY     = 0,
    .IconAngle  = 0,
    .Lap        = 0,
};

/**
 * @brief Internal GPS data structure containing raw and filtered coordinates, as well as speed.
 */
static GPS_Data _gpsData = {
    .speed      = 0.00f,
    .last_lat   = 0.00f,
    .last_lon   = 0.00f,
    .raw_lat    = 0.00f,
    .raw_lon    = 0.00f
};

/**
 * @brief Array of GPS checkpoints representing key locations on the track.
 *
 * Each checkpoint holds a status flag (0 = not reached, 1 = reached),
 * along with latitude and longitude coordinates.
 */
static GPS_Checkpoint Checkpoints[3] = {
    {.lat = 40.12345f, .lon = 29.12345f},  // Start point
	{.lat = 40.12345f, .lon = 29.12345f},
	{.lat = 40.12345f, .lon = 29.12345f},
};


/**
 * @brief Flag indicating whether a lap has started (1) or not (0).
 */
static uint8_t Is_Lap_Started = 0;


/* Private Constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/**
 * @brief Number of checkpoints in the Checkpoints array.
 */
#define NUM_CHECKPOINTS (sizeof(Checkpoints)/sizeof(Checkpoints[0]))

/* Private functions ---------------------------------------------------------*/
/** @addtogroup DMAEx_Private_Functions
  * @{
  */
static HAL_StatusTypeDef Read_GPS_Location(void);
static float NMEA_To_Decimal(char *nmea);
static void GPS_Filter(GPS_Data *gps);
static float GPS_CalcDistance(float lat1, float lon1, float lat2, float lon2);
static void Calculate_Geo_To_Pixel(void);
static void Get_Map_Draw_Position(int gpsPixelX, int gpsPixelY);
static void Calculate_Icon_Angle(void);
static void Count_Lap(void);
static uint8_t Is_Lap_Complete(void);
static void Clear_Checkpoints(void);
/**
  * @}
  */



/**
  * @brief  Initializes internal bindings for geolocation processing.
  *         Currently calls Geo_To_Pixel_Bind with given parameters.
  * @param  uart: UART interface handler for GPS data.
  * @param  mapData: Pointer to map data structure.
  * @retval None
  */
void Geo_To_Pixel_Init(UART_HandleTypeDef *uart, MapOffset *mapData)
{
    Geo_To_Pixel_Bind(uart, mapData);
    Clear_Checkpoints();
}

/**
  * @brief  Assigns UART interface and map data pointers to internal static variables.
  *         These pointers are used in subsequent geolocation processing functions.
  * @param  uart: UART handler used for GPS data reception.
  * @param  mapData: Pointer to map data structure to update pixel and icon angle values.
  * @retval None
  */
void Geo_To_Pixel_Bind(UART_HandleTypeDef *uart, MapOffset *mapData)
{
    _uart = uart;
    _mapData = mapData;
}

/**
  * @brief  Executes the full geolocation update sequence:
  *         - Reads raw GPS data via UART
  *         - Applies GPS filtering to reduce noise
  *         - Converts filtered GPS coordinates to pixel positions on the map
  *         - Calculates the orientation angle of the map icon based on movement
  *         - Checks if a full lap is completed and updates lap count accordingly
  *
  * @retval None
  */
void Run_GeoPipeline(void)
{

	if(Read_GPS_Location() == HAL_OK){

		GPS_Filter(&_gpsData);

		Calculate_Geo_To_Pixel();

		Calculate_Icon_Angle();

		Count_Lap();
	}
}

/**
  * @brief  Reads raw GPS data from UART via UART, searches for $GNRMC NMEA sentence,
  *         extracts status, latitude, longitude, and speed fields.
  *         Converts latitude and longitude to decimal degrees with NMEA_To_Decimal(),
  *         applies hemisphere corrections, and updates global _gpsData struct.
  *
  * @note   Uses internal global buffer 'gps_buffer' to store received UART data.
  *         Updates raw_lat, raw_lon, and speed fields in _gpsData.
  *
  * @retval HAL_OK if a valid $GNRMC sentence is found and parsed, otherwise HAL_ERROR.
  */
static HAL_StatusTypeDef Read_GPS_Location(void)
{
    float latitude = 0.0f, longitude = 0.0f;

    // Clear GPS UART buffer
    memset(gps_buffer, 0, GPS_BUFFER_SIZE);
    HAL_UART_Receive(_uart, (uint8_t *)gps_buffer, GPS_BUFFER_SIZE, 1000);

    // Search for $GNRMC sentence start in buffer
    char *start = gps_buffer;
    while ((start = strstr(start, "$GNRMC")) != NULL) {

        char temp_buf[GPS_BUFFER_SIZE];
        strncpy(temp_buf, start, GPS_BUFFER_SIZE - 1);
        temp_buf[GPS_BUFFER_SIZE - 1] = '\0';

        char *token = strtok(temp_buf, ",");
        int fieldIndex = 0;

        char status 	= 0;
        char *latStr 	= NULL;
        char *latDir 	= NULL;
        char *lonStr 	= NULL;
        char *lonDir 	= NULL;
        char *speedStr	= NULL;

        while (token != NULL) {
            switch (fieldIndex) {
                case 2:
                    status = token[0]; // 'A' = valid fix, 'V' = invalid
                    break;
                case 3:
                    latStr = token;
                    break;
                case 4:
                    latDir = token;
                    break;
                case 5:
                    lonStr = token;
                    break;
                case 6:
                    lonDir = token;
                    break;
                case 7:
                    speedStr = token;
                    break;

            }

            fieldIndex++;
            token = strtok(NULL, ",");
        }

        // Validate fix and presence of required fields
        if (status == 'A' && latStr && lonStr && latDir && lonDir) {
            latitude = NMEA_To_Decimal(latStr);
            if (latDir[0] == 'S') latitude = -latitude;

            longitude = NMEA_To_Decimal(lonStr);
            if (lonDir[0] == 'W') longitude = -longitude;

            // Convert speed from knots to km/h
            if (speedStr) {
                float speedKnots = atof(speedStr);
                float speedKmph = speedKnots * 1.852f;
                _gpsData.speed = speedKmph;
            }

            // Update GPS raw coordinates
            _gpsData.raw_lat = latitude;
            _gpsData.raw_lon = longitude;

            return HAL_OK; // Valid sentence processed, exit loop
        }

        // Move pointer to search for next $GNRMC sentence
        start += 6; // length of "$GNRMC"
    }
    return HAL_ERROR;
}

/**
  * @brief  Parses NMEA coordinate string and converts it to decimal degrees.
  *
  *         Extracts degree and minute parts from the string, then returns
  *         decimal representation (degrees + minutes/60).
  *
  * @param  nmea: NMEA coordinate string (e.g. "4916.45" or "12311.12").
  * @retval Decimal degrees as float.
  */
static float NMEA_To_Decimal(char *nmea)
{
    if (nmea == NULL) return 0.0f;

    int degrees = 0;
    float minutes = 0.0f;
    char deg_buff[4] = {0};

    int len = strchr(nmea, '.') - nmea;
    int deg_digits = (len > 4) ? 3 : 2;

    strncpy(deg_buff, nmea, deg_digits);
    degrees = atoi(deg_buff);
    minutes = atof(nmea + deg_digits);

    return degrees + (minutes / 60.0f);
}

/**
  * @brief  Filters GPS latitude and longitude to reduce noise by ignoring small movements.
  *
  *         Calculates distance between current raw and last known positions.
  *         If distance < 3 meters, keeps previous filtered coordinates to avoid jitter.
  *         Otherwise, updates filtered and last coordinates with new raw data.
  *
  * @param  gps: Pointer to GPS_Data struct containing raw, filtered, and last coordinates.
  * @retval None
  */
static void GPS_Filter(GPS_Data *gps)
{
    float dist = GPS_CalcDistance(gps->raw_lat, gps->raw_lon, gps->last_lat, gps->last_lon);

    if ( dist < 3.0f) { //gps->speed < 1.0f ||
    	// No significant movement, keep previous filtered values
        gps->filtered_lat = gps->last_lat;
        gps->filtered_lon = gps->last_lon;

    }
    else {
    	// Significant movement detected, update filtered and last values
        gps->filtered_lat = gps->raw_lat;
        gps->filtered_lon = gps->raw_lon;
        gps->last_lat = gps->raw_lat;
        gps->last_lon = gps->raw_lon;
    }
}

/**
  * @brief  Computes distance in meters between two geographic coordinates.
  *         Uses the Haversine formula accounting for earth's curvature.
  *
  * @param  lat1: Latitude of start point (degrees).
  * @param  lon1: Longitude of start point (degrees).
  * @param  lat2: Latitude of end point (degrees).
  * @param  lon2: Longitude of end point (degrees).
  *
  * @retval Distance in meters.
  */
static float GPS_CalcDistance(float lat1, float lon1, float lat2, float lon2)
{
    const float R = 6371000.0f; // earth radius in meters
    float dLat = (lat2 - lat1) * (M_PI / 180.0f);
    float dLon = (lon2 - lon1) * (M_PI / 180.0f);

    float a = sinf(dLat/2) * sinf(dLat/2) +
              cosf(lat1 * M_PI / 180.0f) * cosf(lat2 * M_PI / 180.0f) *
              sinf(dLon/2) * sinf(dLon/2);

    float c = 2 * atan2f(sqrtf(a), sqrtf(1-a));
    return R * c; // distance in meters
}

/**
  * @brief  Maps filtered GPS latitude and longitude into pixel coordinates.
  *         Updates drawable position based on mapped pixel values.
  * @retval None
  */
static void Calculate_Geo_To_Pixel(void)
{
	float mappedXf = Map_Float(_gpsData.filtered_lon, NW_lon, SE_lon, 0.00f , MAP_X_SIZE);
	int mappedX = (int)mappedXf;

	float mappedYf = Map_Float(_gpsData.filtered_lat, NW_lat, SE_lat, 0.00f, MAP_Y_SIZE);
	int mappedY = (int)mappedYf;
	Get_Map_Draw_Position(mappedX, mappedY);
}

/**
  * @brief  Calculates and clamps the pixel position for drawing the icon on the map.
  *
  *         Converts GPS pixel coordinates to screen pixel coordinates considering
  *         icon offset and clamps the result within allowed map boundaries.
  *
  * @param  gpsPixelX: X position derived from GPS mapping (in pixels).
  * @param  gpsPixelY: Y position derived from GPS mapping (in pixels).
  *
  * @note   ICON_X and ICON_Y define the base drawing position of the icon on screen.
  *         ICON_WIDTH/HEIGHT offsets center the icon correctly.
  *         Resulting pixel coordinates are clamped between MAP_X_MIN_VAL/MAX and MAP_Y_MIN_VAL/MAX.
  *
  * @retval None
  */
static void Get_Map_Draw_Position(int gpsPixelX, int gpsPixelY)
{
	int pixelX, pixelY;

	pixelX = ICON_X + (int)(ICON_WIDTH/2) - gpsPixelX;

	if(pixelX < MAP_X_MIN_VAL)
		pixelX = MAP_X_MIN_VAL;
	else if(pixelX > MAP_X_MAX_VAL)
		pixelX = MAP_X_MAX_VAL;

	pixelY = ICON_Y + (int)(ICON_HEIGHT/2) - gpsPixelY;

	if(pixelY < MAP_Y_MIN_VAL)
		pixelY = MAP_Y_MIN_VAL;
	else if(pixelY > MAP_Y_MAX_VAL)
		pixelY = MAP_Y_MAX_VAL;
	_mapData->PixelX = pixelX;
	_mapData->PixelY = pixelY;
}

/**
  * @brief  Calculates the direction (angle) of movement based on pixel coordinate changes.
  *
  *         This function compares the current and previous pixel positions on the map
  *         to compute the direction the icon should be rotated toward.
  *         The angle is calculated in degrees where:
  *             - 0° points to the **west**
  *             - 90° is **south**
  *             - 180° is **east**
  *             - 270° is **north**
  *
  *         Angle is normalized to the [0, 360) range.
  *
  *         The cached position is updated after calculation to track changes in the next call.
  *
  * @note   No angle is calculated if the position has not changed.
  *
  * @retval None
  */
static void Calculate_Icon_Angle(void)
{

	if (_mapCachedData.PixelX != _mapData->PixelX || _mapCachedData.PixelY != _mapData->PixelY){

	    double angle_rad = atan2(_mapCachedData.PixelY - _mapData->PixelY, _mapCachedData.PixelX - _mapData->PixelX);  // -π ile +π arası
	    double angle_deg = angle_rad * (180.0 / M_PI);             // -180 ile +180 arası

	    angle_deg += 180;                                          // 0 derece batı olacak şekilde kaydırıyoruz

	    // 0-360 arası normalize et
	    if (angle_deg < 0) angle_deg += 360;
	    if (angle_deg >= 360) angle_deg -= 360;

	    _mapData->IconAngle = angle_deg;  // artık 0 (batı) ile 360 arası

		_mapCachedData.PixelX = _mapData->PixelX;
		_mapCachedData.PixelY = _mapData->PixelY;
	}
}

/**
 ******************************************************************************
 * @brief  Checkpoint kontrolü yaparak tur sayısını günceller
 * @retval None
 *
 * This function loops through all predefined GPS checkpoints and calculates
 * the distance between each checkpoint and the current filtered GPS position.
 *
 * - If the current position is within 5 meters of any checkpoint, it sets
 *   the checkpoint status as passed.
 *
 * - If the first checkpoint is reached and a lap has already started, it
 *   checks whether all other checkpoints have also been passed. If so, it
 *   increments the lap counter and resets all checkpoint statuses.
 *
 * - If the current position is close to any other checkpoint (not the first),
 *   it marks the lap as started.
 *
 * The function uses:
 *  - `GPS_CalcDistance()` to calculate proximity
 *  - `Is_Lap_Complete()` to verify lap completion
 *  - `Clear_Checkpoints()` to reset states
 *
 * Typical usage:
 *  - Call this after filtering GPS data, ideally inside a geo update pipeline.
 ******************************************************************************
 */
static void Count_Lap(void)
{
    for (int index = 0; index < NUM_CHECKPOINTS; index++) {
        GPS_Checkpoint *point = &Checkpoints[index];
        float distance = GPS_CalcDistance(point->lat, point->lon,
                                          _gpsData.filtered_lat, _gpsData.filtered_lon);

        if (distance < 5.0f) { // close enough to a checkpoint

            if (index == 0) { // you're at the starting point

                if (Is_Lap_Started) {
                    // a lap had already started and you've returned to the starting point
                    point->status = 1;

                    if (Is_Lap_Complete()) {
                        _mapData->Lap++;  // lap completed
                    }

                    Clear_Checkpoints();
                    Is_Lap_Started = 0;
                }

            } else {
                // not at the starting point yet, but you've reached a checkpoint
                Is_Lap_Started = 1;
                point->status = 1;
            }

            break; // already passed one checkpoint, no need to check the rest
        }
    }
}

/**
 ******************************************************************************
 * @brief  Verifies if all GPS checkpoints have been passed
 * @retval uint8_t
 *         - 1: All checkpoints are marked as passed
 *         - 0: At least one checkpoint has not been passed
 *
 * This function checks the status of each checkpoint in the `Checkpoints` array.
 * If any checkpoint has a `status` of 0 (not passed), the function returns 0.
 * If all checkpoints have `status == 1`, the function returns 1 indicating
 * lap completion.
 *
 * Should be called in lap tracking logic, typically before incrementing lap count.
 ******************************************************************************
 */
static uint8_t Is_Lap_Complete(void)
{
	for (int index=0; index<sizeof(Checkpoints)/sizeof(Checkpoints[0]); index++){
		if(Checkpoints[index].status == 0) return 0;
	}
	return 1;
}

/**
 ******************************************************************************
 * @brief  Clears all GPS checkpoint statuses
 * @retval None
 *
 * This function resets the `status` field of each element in the `Checkpoints`
 * array to 0, indicating that no checkpoints have been passed.
 *
 * Typical usage:
 * - Call this after a completed lap to prepare for the next one.
 * - Should be used in coordination with `Is_Lap_Complete()` and `Count_Lap()`.
 ******************************************************************************
 */
static void Clear_Checkpoints(void)
{
	for (int index=0; index<sizeof(Checkpoints)/sizeof(Checkpoints[0]); index++){
		Checkpoints[index].status = 0;
	}
}
