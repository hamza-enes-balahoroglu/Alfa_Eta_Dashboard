#ifndef GEO_TO_PIXEL
#define GEO_TO_PIXEL

#include "stm32f4xx_hal.h"

/*--------------------- Map Location ---------------------*/
#define MAP_X_MIN_VAL 0 // Minimum horizontal offset for the background image (fully left)
#define MAP_X_MAX_VAL 450 // Maximum horizontal offset for the background image (fully right)
#define MAP_Y_MIN_VAL -270 // Minimum vertical offset for the background image (fully up)
#define MAP_Y_MAX_VAL 0 // Maximum vertical offset for the background image (fully down)

/*--------------------- Geo Boundaries ---------------------*/
#define NW_lat 40.824772493 // Latitude of the top-left (northwest) corner of the map
#define NW_lon 29.417859918 // Longitude of the top-left (northwest) corner of the map
#define SE_lat 40.822593887 // Latitude of the bottom-right (southeast) corner of the map
#define SE_lon 29.420935394 // Longitude of the bottom-right (southeast) corner of the map

typedef struct {
    int PixelX;
    int PixelY;
    int IconDirection;
    int Lap;
} MapOffset;


#endif // GEO_TO_PIXEL
