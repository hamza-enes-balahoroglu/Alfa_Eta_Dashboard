/**
 ******************************************************************************
 * @file           : geo_to_pixel.c
 * @brief          : Implementation of GPS to pixel coordinate conversion - STM32 HAL compatible
 ******************************************************************************
 * @author         : Hamza Enes Balahoroğlu
 * @version        : v1.0
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
static UART_HandleTypeDef *_uart = NULL;

/* Buffer to store raw GPS data received via UART */
char gps_buffer[GPS_BUFFER_SIZE];

/* Pointer to external map data structure */
static MapOffset *_mapData = NULL;

/* Cached map data for angle calculation and change detection */
static MapOffset _mapCachedData = {
    .PixelX     = 0,
    .PixelY     = 0,
    .IconAngle  = 0,
    .Lap        = 0,
};

/* GPS data with raw, filtered coordinates and speed */
static GPS_Data _gpsData = {
    .speed      = 0.00f,
    .last_lat   = 0.00f,
    .last_lon   = 0.00f,
    .raw_lat    = 0.00f,
    .raw_lon    = 0.00f
};

/**
  * @brief  Initializes internal bindings for geolocation processing.
  *         Currently calls Geo_To_Pixel_Bind with given parameters.
  * @param  uart: UART interface handler for GPS data.
  * @param  mapData: Pointer to map data structure.
  * @retval None
  */
void Geo_To_Pixel_Init(UART_HandleTypeDef *uart, MapOffset *mapData){
    Geo_To_Pixel_Bind(uart, mapData);
}

/**
  * @brief  Assigns UART interface and map data pointers to internal static variables.
  *         These pointers are used in subsequent geolocation processing functions.
  * @param  uart: UART handler used for GPS data reception.
  * @param  mapData: Pointer to map data structure to update pixel and icon angle values.
  * @retval None
  */
void Geo_To_Pixel_Bind(UART_HandleTypeDef *uart, MapOffset *mapData){
    _uart = uart;
    _mapData = mapData;
}

/**
  * @brief  Executes the full geolocation update sequence:
  *         - Reads raw GPS data via UART
  *         - Applies GPS filtering to reduce noise
  *         - Converts filtered GPS coordinates to pixel positions on map
  *         - Calculates the orientation angle of the map icon based on movement
  *
  * @retval None
  */
void Run_GeoPipeline(void){
	Read_GPS_Location();

	GPS_Filter(&_gpsData);

	Calculate_Geo_To_Pixel();

	Calculate_Icon_Angle();

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
  * @retval None
  */
void Read_GPS_Location(void) {
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

            break; // Valid sentence processed, exit loop
        }

        // Move pointer to search for next $GNRMC sentence
        start += 6; // length of "$GNRMC"
    }
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
float NMEA_To_Decimal(char *nmea) {
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
void GPS_Filter(GPS_Data *gps)
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
float GPS_CalcDistance(float lat1, float lon1, float lat2, float lon2)
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
void Calculate_Geo_To_Pixel(void){
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
void Get_Map_Draw_Position(int gpsPixelX, int gpsPixelY) {
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
void Calculate_Icon_Angle(void) {

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
