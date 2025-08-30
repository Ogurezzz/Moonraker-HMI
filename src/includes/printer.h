#ifndef _PRINTER_H
#define _PRINTER_H
#define JSMN_HEADER
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "jsmn.h"
#include "strbuf.h"

typedef struct
{
	char  *name;
	double modified;
} file_t;

typedef struct
{
	file_t *list;
	size_t	qty;
	size_t	cap;
} filesList_t;

typedef struct
{
	string_buffer_t state;
	string_buffer_t printing_filename;
	const char	   *selected_file;
	float			extruder_temp;
	float			extruder_target;
	float			heatbed_temp;
	float			heatbed_target;
	float			progress;
	float			print_time;
	float			fan_speed;
	struct
	{
		float X;
		float Y;
		float Z;
		float E;
	} position;
	float		feed_rate;
	bool		filament_detected;
	filesList_t files;
	struct
	{
		char host[HOST_NAME_MAX];
		char serial[PATH_MAX];
		int	 PLA_E;
		int	 PLA_B;
		int	 ABS_E;
		int	 ABS_B;
	} cfg;

} printer_t;

//*** PARSER FUNCTIONS PROTOTYPES START ***//

int initPrinter(printer_t *printer);
int initFileList(printer_t *printer);
int clearFileList(printer_t *printer);
int expandFileList(printer_t *printer);
int addFile2List(file_t *file, printer_t *printer);

int getFileListFromServer(printer_t *printer);
int ParsePrinterRespond(string_buffer_t *statusRespond, printer_t *printer);
int line_process(char *line, char *usart_tx_buf, char *http_command,
				 char *http_address);

//*** PARSER FUNCTIONS PROTOTYPES END ***//

#endif