#ifndef _PRINTER_H
#define _PRINTER_H
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#define JSMN_HEADER
#include "jsmn.h"
typedef struct
{
	float extruder_temp;
	float extruder_target;
	float heatbed_temp;
	float heatbed_target;
	float progress;
	float print_time;
	float fan_speed;
	struct {
		float X_POS;
		float Y_POS;
		float Z_POS;
		float E_POS;
	}position;
	float feed_rate;
	uint8_t filamet_detected;
	char printing_filename[256];

}printer_t;
//*** PARSER FUNCTIONS PROTOTYPES START ***//

int GetExtruderTemperature(char *json, printer_t *printer);
int GetExtruderTargetTemperature(char *json, printer_t *printer);
int GetBedTemperature(char *json, printer_t *printer);
int GetBedTargetTemperature(char *json, printer_t *printer);
int GetFanSpeed(char *json, printer_t *printer);
int GetCurrentPosition(char *json, printer_t *printer);
int GetPrintPercentage(char *json, printer_t *printer);
int GetPrintDuration(char *json, printer_t *printer);
int GetPrintFeedrate(char *json, printer_t *printer);
int GetFilamenSensorStatus(char *json, printer_t *printer);
int GetCurFileName(char *json, printer_t *printer);


//*** PARSER FUNCTIONS PROTOTYPES END ***//

#endif