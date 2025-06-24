#include "geo_to_pixel.h"

static UART_HandleTypeDef *_uart = NULL;

char *gps_buffer;

static MapOffset *_mapData = NULL;
static MapOffset _mapCachedData = {
		.PixelX		=	0,
		.PixelY		=	0,
		.IconAngle 	=	0,
		.Lap 		=	0,
};

float actualLon, actualLat;

void Geo_To_Pixel_Init(UART_HandleTypeDef *uart, MapOffset *mapData){
    Geo_To_Pixel_Bind(uart, mapData);
}

void Geo_To_Pixel_Bind(UART_HandleTypeDef *uart, MapOffset *mapData){
    _uart = uart;
    _mapData = mapData;
}

void Run_GeoPipeline(void){
	Read_GPS_Location();

	Calculate_Geo_To_Pixel();

	if (_mapCachedData.PixelX != _mapData->PixelX || _mapCachedData.PixelY != _mapData->PixelY){
		Calculate_Icon_Angle();
		_mapCachedData.PixelX = _mapData->PixelX;
		_mapCachedData.PixelY = _mapData->PixelY;
	}
	// _mapData->Lap = _mapData->Lap+1;

}

static float *_lon = NULL;
static float *_lat = NULL;
static int *x;
static int *y;

void Test(float *lon, float *lat, char *test, int *testx, int *testy){
	_lon=lon;
	_lat=lat;
	gps_buffer=test;
	x=testx;
	y=testy;
}

void Read_GPS_Location(void) {
	float latitude,longitude;
    memset(gps_buffer, 0, GPS_BUFFER_SIZE);
    HAL_UART_Receive(_uart, (uint8_t *)gps_buffer, GPS_BUFFER_SIZE, 1000);

    if (strstr(gps_buffer, "$GNRMC")) {
        char *token;

        token = strtok(gps_buffer, ",");
        while (token != NULL) {


            if (strstr(token, "$GNRMC")){

            	token = strtok(NULL, ",");
            	token = strtok(NULL, ",");
            	token = strtok(NULL, ",");

            	latitude = NMEA_To_Decimal(token); // default 'N'

            	token = strtok(NULL, ",");

            	if (token[0] == 'S')
            		latitude = -latitude;

            	token = strtok(NULL, ",");
            	longitude = NMEA_To_Decimal(token); // default 'E'

            	token = strtok(NULL, ",");
            		if (token[0] == 'W')
            			longitude = -longitude;
            	break;
            }

            token = strtok(NULL, ",");
        }
        actualLat = latitude;
        actualLon = longitude;

        if (_lat != NULL) *_lat = latitude;
        if (_lon != NULL) *_lon = longitude;

    }
}

float NMEA_To_Decimal(char *nmea) {
    float deg, min;
    float val = atof(nmea);
    deg = (int)(val / 100);
    min = val - (deg * 100);
    float decimal = deg + (min / 60.0);
    return decimal;
}

void Calculate_Geo_To_Pixel(void){
	float mappedXf = Map_Float(actualLon, NW_lon, SE_lon, 0.00f , MAP_X_SIZE);
	int mappedX = (int)mappedXf;

	float mappedYf = Map_Float(actualLat, SE_lat, NW_lat, 0.00f, MAP_Y_SIZE);
	int mappedY = (int)mappedYf;
	*x=mappedX;
	*y=mappedY;
	Get_Map_Draw_Position(mappedX, mappedY);
}

void Get_Map_Draw_Position(int gpsPixelX, int gpsPixelY) {
	int pixelX, pixelY;

	pixelX = ICON_X + (ICON_WIDTH/2) - gpsPixelX;
/*
	if(pixelX < MAP_X_MIN_VAL)
		pixelX = MAP_X_MIN_VAL;
	else if(pixelX > MAP_X_MAX_VAL)
		pixelX = MAP_X_MAX_VAL; */

	pixelY = ICON_Y + (ICON_HEIGHT/2) - gpsPixelY;

/*	if(pixelY < MAP_Y_MIN_VAL)
		pixelY = MAP_Y_MIN_VAL;
	else if(pixelY > MAP_Y_MAX_VAL)
		pixelY = MAP_Y_MAX_VAL; */
	_mapData->PixelX = pixelX;
	_mapData->PixelY = pixelY;
}

void Calculate_Icon_Angle(void) {

    double angle_rad = atan2(_mapCachedData.PixelY - _mapData->PixelY, _mapCachedData.PixelX - _mapData->PixelX);  // -π ile +π arası
    double angle_deg = angle_rad * (180.0 / M_PI);             // -180 ile +180 arası

    angle_deg += 180;                                          // 0 derece batı olacak şekilde kaydırıyoruz

    // 0-360 arası normalize et
    if (angle_deg < 0) angle_deg += 360;
    if (angle_deg >= 360) angle_deg -= 360;

    _mapData->IconAngle = angle_deg;  // artık 0 (batı) ile 360 arası
}

/*
float Map_Float(float input, float in_min, float in_max, float out_min, float out_max) {
    float ratio = (input - in_min) / (in_max - in_min);
    return out_min + ratio * (out_max - out_min);
}
*/
float Map_Float(float input, float in_min, float in_max, float out_min, float out_max)
{
    float input_range = in_max - in_min + 1;
    float ref_range = out_max - out_min + 1;

    return ((ref_range * (input - in_min)) / input_range) + out_min;
}
