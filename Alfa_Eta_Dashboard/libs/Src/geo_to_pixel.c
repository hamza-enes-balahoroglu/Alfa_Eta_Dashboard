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

static GPS_Data _gpsData ={
		.speed		= 0.00f
};

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

	Calculate_Icon_Angle();
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

        char status 	= 0;
        char *latStr 	= NULL;
        char *latDir 	= NULL;
        char *lonStr 	= NULL;
        char *lonDir 	= NULL;
        char *speedStr	= NULL;

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
                case 7:
                    speedStr = token;
                    break;

            }

            fieldIndex++;
            token = strtok(NULL, ",");
        }

        if (status == 'A' && latStr && lonStr && latDir && lonDir) {
            latitude = NMEA_To_Decimal(latStr);
            if (latDir[0] == 'S') latitude = -latitude;

            longitude = NMEA_To_Decimal(lonStr);
            if (lonDir[0] == 'W') longitude = -longitude;

            if (speedStr) {
            	// ayrı bir fonksiyonda hesaplanabilir.
                float speedKnots = atof(speedStr);
                float speedKmph = speedKnots * 1.852f;
                if (_gpsData.speed) _gpsData.speed = speedKmph;
            }

            _gpsData.raw_lat = latitude;
            _gpsData.raw_lon = longitude;

            GPS_Filter(&_gpsData);

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

void GPS_Filter(GPS_Data *gps)
{
    float dist = GPS_CalcDistance(gps->raw_lat, gps->raw_lon, gps->last_lat, gps->last_lon);

    if (gps->speed < 1.0f || dist < 3.0f) {
        // sabitsin, eski değerleri kullan
        gps->filtered_lat = gps->last_lat;
        gps->filtered_lon = gps->last_lon;
    } else {
        // hareket var, güncelle
        gps->filtered_lat = gps->raw_lat;
        gps->filtered_lon = gps->raw_lon;
        gps->last_lat = gps->raw_lat;
        gps->last_lon = gps->raw_lon;
    }
}

float GPS_CalcDistance(float lat1, float lon1, float lat2, float lon2)
{
    const float R = 6371000.0f; // Dünya yarıçapı metre
    float dLat = (lat2 - lat1) * (M_PI / 180.0f);
    float dLon = (lon2 - lon1) * (M_PI / 180.0f);

    float a = sinf(dLat/2) * sinf(dLat/2) +
              cosf(lat1 * M_PI / 180.0f) * cosf(lat2 * M_PI / 180.0f) *
              sinf(dLon/2) * sinf(dLon/2);

    float c = 2 * atan2f(sqrtf(a), sqrtf(1-a));
    return R * c; // metre cinsinden mesafe
}

void Calculate_Geo_To_Pixel(void){
	float mappedXf = Map_Float(_gpsData.filtered_lon, NW_lon, SE_lon, 0.00f , MAP_X_SIZE);
	int mappedX = (int)mappedXf;

	float mappedYf = Map_Float(_gpsData.filtered_lat, NW_lat, SE_lat, 0.00f, MAP_Y_SIZE);
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


float Map_Float(float input, float in_min, float in_max, float out_min, float out_max)
{
    float input_range = in_max - in_min;
    float ref_range = out_max - out_min;

    return ((ref_range * (input - in_min)) / input_range) + out_min;
}
