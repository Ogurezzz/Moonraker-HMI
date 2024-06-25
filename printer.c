
#include "printer.h"

int GetPrintFeedrate(char *json, printer_t *printer){
	json = strstr(json, "gcode_move");
	if (!json)
		return -1;
	json = strstr(json, "speed_factor");
	if (!json)
		return -1;
	sscanf(json, "speed_factor\": %f", &printer->feed_rate);
	return 0;
}

int GetPrintPercentage(char *json, printer_t *printer)
{
	json = strstr(json, "virtual_sdcard");
	if (!json)
		return -1;
	json = strstr(json, "progress");
	if (!json)
		return -1;
	sscanf(json, "progress\": %f", &printer->progress);
	return 0;
}

int GetPrintDuration(char *json, printer_t *printer)
{

	json = strstr(json, "print_stats");
	if (!json)
		return -1;
	json = strstr(json, "print_duration");
	if (!json)
		return -1;
	sscanf(json, "print_duration\": %f", &printer->print_time);
	return 0;
}
int GetCurrentPosition(char *json, printer_t *printer)
{
	json = strstr(json, "gcode_move");
	if (!json)
		return -1;
	json = strstr(json, "position");
	if (!json)
		return -1;
	sscanf(json, "position\": [%f,%f,%f,%f", &printer->position.X_POS, &printer->position.Y_POS, &printer->position.Z_POS,&printer->position.E_POS);
	return 0;
}

int GetExtruderTemperature(char *json, printer_t *printer)
{
	json = strstr(json, "extruder");
	if (!json)
		return -1;
	json = strstr(json, "temperature");
	if (!json)
		return -1;
	sscanf(json, "temperature\": %f", &printer->extruder_temp);
	return 0;
}
int GetExtruderTargetTemperature(char *json, printer_t *printer)
{
	json = strstr(json, "extruder");
	if (!json)
		return -1;
	json = strstr(json, "target");
	if (!json)
		return -1;
	sscanf(json, "target\": %f", &printer->extruder_target);
	return 0;
}
int GetBedTemperature(char *json, printer_t *printer)
{
	json = strstr(json, "heater_bed");
	if (!json)
		return -1;
	json = strstr(json, "temperature");
	if (!json)
		return -1;
	sscanf(json, "temperature\": %f", &printer->heatbed_temp);
	return 0;
}
int GetBedTargetTemperature(char *json, printer_t *printer)
{
	json = strstr(json, "heater_bed");
	if (!json)
		return -1;
	json = strstr(json, "target");
	if (!json)
		return -1;
	sscanf(json, "target\": %f", &printer->heatbed_target);
	return 0;
}
int GetFanSpeed(char *json, printer_t *printer)
{

	json = strstr(json, "fan");
	if (!json)
	{
		return -1;
	}

	json = strstr(json, "speed");
	if (!json)
	{
		return -1;
	}

	sscanf(json, "speed\": %f", &printer->fan_speed);

	return 0;
}
int GetFilamenSensorStatus(char *json, printer_t *printer){
	json = strstr(json, "filament_switch_sensor");
	if (!json)
	{
		return -1;
	}

	json = strstr(json, "filament_detected");
	if (!json)
	{
		return -1;
	}
	char *tmp = "filament_detected: true";
	if (strncmp(json,tmp,strlen(tmp))){
		printer->filamet_detected = 1;
	}else {
		printer->filamet_detected = 0;
	}

	return 0;
}

int GetCurFileName(char *json, printer_t *printer){
	json = strstr(json, "print_stats");
	if (!json)
		return -1;
	json = strstr(json, "filename");
	if (!json) return -1;
	json = strstr(json, ":");
	if (!json) return -1;
	json = strstr(json, "\"");
	if (!json) return -1;
	json++;
	char *name = printer->printing_filename;
	do
	{
		*name++ = *json++;
	} while (*json !='\"' && *json !=',');
	*name='\0';
	return 0;
}