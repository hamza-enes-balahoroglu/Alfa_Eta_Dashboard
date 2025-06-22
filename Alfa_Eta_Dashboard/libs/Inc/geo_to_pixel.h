#ifndef GEO_TO_PIXEL
#define GEO_TO_PIXEL

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MAP_X_SIZE 800
#define MAP_Y_SIZE 480

#define ICON_WIDTH  67
#define ICON_HEIGHT 67
#define ICON_X 640
#define ICON_Y 201

/*--------------------- Map Location ---------------------*/
#define MAP_X_MIN_VAL 0 // Minimum horizontal offset for the background image (fully left)
#define MAP_X_MAX_VAL 450 // Maximum horizontal offset for the background image (fully right)
#define MAP_Y_MIN_VAL -270 // Minimum vertical offset for the background image (fully up)
#define MAP_Y_MAX_VAL 0 // Maximum vertical offset for the background image (fully down)

/*--------------------- Geo Boundaries ---------------------*/
#define NW_lat 40.824772493 // Latitude (vertical) of the top-left (northwest) corner of the map
#define NW_lon 29.417859918 // Longitude (horizontal) of the top-left (northwest) corner of the map
#define SE_lat 40.822593887 // Latitude (vertical) of the bottom-right (southeast) corner of the map
#define SE_lon 29.420935394 // Longitude (horizontal) of the bottom-right (southeast) corner of the map

#define GPS_BUFFER_SIZE 100

typedef struct {
    int PixelX;
    int PixelY;
    int IconAngle;
    int Lap;
} MapOffset;


void Test(char *buf);



void Run_GeoPipeline(void);

void Geo_To_Pixel_Init(UART_HandleTypeDef *uart, MapOffset *mapData);

void Read_GPS_Location(void);

float NMEA_To_Decimal(char *nmea, char hemisphere);

void Calculate_Geo_To_Pixel(void);

void Get_Map_Draw_Position(int gpsPixelX, int gpsPixelY);

void Calculate_Icon_Angle(void);

float Map_Float(float input, float in_min, float in_max, float out_min, float out_max);

#endif // GEO_TO_PIXEL
