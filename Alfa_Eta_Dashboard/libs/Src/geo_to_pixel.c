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
static float *x;
static float *y;

void Test(float *lon, float *lat, char *test, float *testx, float *testy){
	_lon=lon;
	_lat=lat;
	gps_buffer=test;
	x=testx;
	y=testy;
}

void Read_GPS_Location(void) {
    float latitude = 0.0f, longitude = 0.0f;

    // buffer'ı temizle
    memset(gps_buffer, 0, GPS_BUFFER_SIZE);
    HAL_UART_Receive(_uart, (uint8_t *)gps_buffer, GPS_BUFFER_SIZE, 1000);

    // $GNRMC geçen satırı bul
    char *start = gps_buffer;
    while ((start = strstr(start, "$GNRMC")) != NULL) {

        char temp_buf[GPS_BUFFER_SIZE];
        strncpy(temp_buf, start, GPS_BUFFER_SIZE - 1);
        temp_buf[GPS_BUFFER_SIZE - 1] = '\0';

        char *token = strtok(temp_buf, ",");
        int fieldIndex = 0;

        char *latStr = NULL;
        char *latDir = NULL;
        char *lonStr = NULL;
        char *lonDir = NULL;
        //	char *dirStr = NULL; // yön bilgisi burada geçici tutulacak
        char status = 0;

        while (token != NULL) {
            switch (fieldIndex) {
                case 2:
                    status = token[0]; // 'A' ya da 'V'
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
               /*
                case 8:
                	dirStr = token; // yön bilgisi (course over ground)
                	break;
                */

            }

            fieldIndex++;
            token = strtok(NULL, ",");
        }

        if (status == 'A' && latStr && lonStr && latDir && lonDir) {
            latitude = NMEA_To_Decimal(latStr);
            if (latDir[0] == 'S') latitude = -latitude;

            longitude = NMEA_To_Decimal(lonStr);
            if (lonDir[0] == 'W') longitude = -longitude;

            actualLat = latitude;
            actualLon = longitude;
/*
            if (dirStr) {

                _mapCachedData.IconAngle = _mapData->IconAngle;
                _mapData->IconAngle = (int)atof(dirStr);
            }

            */

            if (_lat) *_lat = latitude;
            if (_lon) *_lon = longitude;

            break; // başarılı satırı işledik, çık
        }

        // bir sonrakine bak, olur da başka GNRMC varsa
        start += 6; // "$GNRMC" uzunluğu kadar atla
    }
}

float NMEA_To_Decimal(char *nmea) {
    if (nmea == NULL) return 0.0f;

    int degrees = 0;
    float minutes = 0.0f;
    char deg_buff[4] = {0}; // max 3 digit + null

    int len = strchr(nmea, '.') - nmea; // noktaya kadar olan uzunluk
    int deg_digits = (len > 4) ? 3 : 2; // 3se boylam, 2se enlem

    strncpy(deg_buff, nmea, deg_digits);
    degrees = atoi(deg_buff);
    minutes = atof(nmea + deg_digits);

    return degrees + (minutes / 60.0f);
}

void Calculate_Geo_To_Pixel(void){
	float mappedXf = Map_Float(actualLon, NW_lon, SE_lon, 0.00f , MAP_X_SIZE);
	int mappedX = (int)mappedXf;

	float mappedYf = Map_Float(actualLat, NW_lat, SE_lat, 0.00f, MAP_Y_SIZE);
	int mappedY = (int)mappedYf;
	*x=mappedXf;
	*y=mappedYf;
	Get_Map_Draw_Position(mappedX, mappedY);
}

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
    float input_range = in_max - in_min;
    float ref_range = out_max - out_min;

    return ((ref_range * (input - in_min)) / input_range) + out_min;
}
