#include "printer.h"
#define JSMN_HEADER
#include "anycubic_i3_mega_dgus.h"
#include "curl_utils.h"
#include "jsmn.h"
#include "logging.h"
#include "strbuf.h"

#define DEFAULT_LIST_SIZE 40

extern int serial_port;
static int compare_files(const void *a, const void *b);

int
initPrinter(printer_t *printer)
{
	string_buffer_initialize(&printer->printing_filename);
	string_buffer_initialize(&printer->state);
	return 0;
}

int
initFileList(printer_t *printer)
{
	printer->files.list = (file_t *)calloc(sizeof(file_t), DEFAULT_LIST_SIZE);
	printer->files.cap	= DEFAULT_LIST_SIZE;
	return 0;
}

int
expandFileList(printer_t *printer)
{
	uint32_t new_cap = 0;
	if (printer->files.cap == 0)
		new_cap = DEFAULT_LIST_SIZE;
	else
		new_cap = printer->files.cap * 2;

	printer->files.list =
		(file_t *)realloc(printer->files.list, sizeof(file_t) * new_cap);
	printer->files.cap = new_cap;
	return 0;
}

int
clearFileList(printer_t *printer)
{
	file_t *filearr = printer->files.list;
	for (size_t i = printer->files.qty; i > 0; i--)
	{
		file_t file = filearr[i - 1];
		free(file.name);
		file.modified = 0;
	}
	free(printer->files.list);
	printer->files.qty = 0;
	printer->files.cap = 0;
	return 0;
}

int
addFile2List(file_t *file, printer_t *printer)
{
	if (printer->files.qty == printer->files.cap)
		expandFileList(printer);
	printer->files.list[printer->files.qty].name	   = file->name;
	printer->files.list[printer->files.qty++].modified = file->modified;

	return 0;
}

int
compare_files(const void *a, const void *b)
{
	return (((file_t *)b)->modified - ((file_t *)a)->modified);
}

int
ParsePrinterRespond(string_buffer_t *statusRespond, printer_t *printer)
{
	int			i;
	size_t		paramsTokensQty = 100;
	jsmn_parser jp;

	int r = 0;
	jsmntok_t *t = calloc(paramsTokensQty, sizeof(*t));
	if (t == NULL)
	{
		LOG_ERR("malloc(): errno=%d\n", errno);
		exit(EXIT_FAILURE);
	}
again:
	jsmn_init(&jp);
	r = jsmn_parse(&jp, statusRespond->ptr, statusRespond->len, t, paramsTokensQty);

	if (r < 0)
	{
		if (r == JSMN_ERROR_NOMEM)
		{
			paramsTokensQty = paramsTokensQty * 2;
			t				= realloc_it(t, sizeof(*t) * paramsTokensQty);
			if (t == NULL)
				return 3;
			goto again;
		}
	}
	else
	{
		for (i = 1; i < r; i++)
		{
			if (jsoneq(statusRespond->ptr, &t[i], "progress") == 0)
			{
				/* We may use strndup() to fetch string value */
				sscanf(statusRespond->ptr + t[i + 1].start, "%f", &printer->progress);
				i++;
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "speed_factor") == 0)
			{
				/* We may use strndup() to fetch string value */
				sscanf(statusRespond->ptr + t[i + 1].start, "%f", &printer->feed_rate);
				i++;
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "print_duration") == 0)
			{
				sscanf(statusRespond->ptr + t[i + 1].start, "%f", &printer->print_time);
				i++;
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "filename") == 0)
			{
				char *name = strndup(statusRespond->ptr + t[i + 1].start, t[i + 1].end - t[i + 1].start);
				if (strcmp(name, printer->printing_filename.ptr))
				{
					if (printer->printing_filename.len)
					{
						string_buffer_finish(&printer->printing_filename);
						string_buffer_initialize(&printer->printing_filename);
					}
					string_buffer_append(name, &printer->printing_filename);
				}
				free(name);
				i++;
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "state") == 0)
			{
				char *state = strndup(statusRespond->ptr + t[i + 1].start, t[i + 1].end - t[i + 1].start);
				if (strcmp(state, printer->state.ptr))
				{
					if (printer->state.len)
					{
						string_buffer_finish(&printer->state);
						string_buffer_initialize(&printer->state);
					}
					string_buffer_append(state, &printer->state);

					/* So, if we came here - printer status was updated.
					 * We need to notify display about this.
					 */
					if (strcmp("standby", printer->state.ptr) == 0)
						UART_Print("%s", "J12\r\n");
					else if (strcmp("printing", printer->state.ptr) == 0)
						UART_Print("%s", "J04\r\n");
					else if (strcmp("paused", printer->state.ptr) == 0)
						UART_Print("%s", "J18\r\n");
					else if (strcmp("complete", printer->state.ptr) == 0)
						UART_Print("%s", "J14\r\n");
					else if (strcmp("error", printer->state.ptr) == 0)
						UART_Print("%s", "J16\r\n");
					else if (strcmp("cancelled", printer->state.ptr) == 0)
						UART_Print("%s", "J16\r\n");
				}
				free(state);
				i++;
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "fan") == 0)
			{
				sscanf(statusRespond->ptr + t[i + 3].start, "%f", &printer->fan_speed);
				i++;
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "position") == 0)
			{
				sscanf(statusRespond->ptr + t[i + 2].start, "%f,%f,%f,%f", &printer->position.X, &printer->position.Y,
					   &printer->position.Z, &printer->position.E);
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "extruder") == 0)
			{
				static bool heat_done = false; // heat done notification sent
				static bool heating	  = false; // heating notification sent
				sscanf(statusRespond->ptr + t[i + 1].start, "{\"temperature\": %f, \"target\": %f}",
					   &printer->extruder_temp, &printer->extruder_target);

				if (printer->extruder_target > 0)
				{
					if (!heating)
					{
						UART_Print("J%02d\r\n", NOZZLE_HEATING);
						heating = true;
					}
					if (!heat_done && printer->extruder_temp >= printer->extruder_target)
					{
						UART_Print("J%02d\r\n", NOZZLE_HEATING_DONE);
						heat_done = true;
					}
				}
				else
				{
					heat_done = false;
					heating	  = false;
				}
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "heater_bed") == 0)
			{
				static bool heat_done = false; // heat done notification sent
				static bool heating	  = false; // heating notification sent
				sscanf(statusRespond->ptr + t[i + 1].start, "{\"temperature\": %f, \"target\": %f}",
					   &printer->heatbed_temp, &printer->heatbed_target);

				if (printer->heatbed_target > 0)
				{
					if (!heating)
					{
						UART_Print("J%02d\r\n", BED_HEATING);
						heating = true;
					}
					if (!heat_done && printer->heatbed_temp >= printer->heatbed_target)
					{
						UART_Print("J%02d\r\n", BED_HEATING_DONE);
						heat_done = true;
					}
				}
				else
				{
					heat_done = false;
					heating	  = false;
				}
			}
			else if (jsoneq(statusRespond->ptr, &t[i], "filament_detected") == 0)
			{
				if (strchr("tT", *(statusRespond->ptr + t[i + 1].start)))
					printer->filament_detected = true;
				else
					printer->filament_detected = false;
			}
		}
	}
	free(t);
	return 0;
}

int
getFileListFromServer(printer_t *printer)
{
	static int filesBufSize = 8;
	// jsmntok_t	   *curtok		 = NULL;
	jsmn_parser		jp;
	string_buffer_t filesRespond;
	jsmntok_t	   *t = calloc(filesBufSize, sizeof(jsmntok_t));
	if (t == NULL)
	{
		LOG_ERR("malloc(): errno=%d\n", errno);
		exit(EXIT_FAILURE);
	}

	if (curl_GET(printer->cfg.host, "/server/files/list", "", &filesRespond) == NULL)
	{
		UART_Print("J02\r\n");
		string_buffer_finish(&filesRespond);
		return -1;
	}
	UART_Print("J00\r\n");
	/* JASMINE JSON PARSE */
	int r = 0;

again:
	jsmn_init(&jp);
	r = jsmn_parse(&jp, filesRespond.ptr, filesRespond.len, t, filesBufSize);
	if (r < 0)
	{
		if (r == JSMN_ERROR_NOMEM)
		{
			filesBufSize *= 2;
			t = reallocarray(t, filesBufSize, sizeof(jsmntok_t));
			memset(t, '\0', filesBufSize * sizeof(jsmntok_t));
			if (t == NULL)
				return 3;
			goto again;
		}
	}
	else
	{
		DEBUG_LOG("%s\n", "Files Parsed");
		if (printer->files.qty)
		{
			clearFileList(printer);
			initFileList(printer);
		}

		for (size_t i = 0; i < r; i++)
		{
			if (jsoneq(filesRespond.ptr, &t[i], "path") == 0)
			{
				file_t new_file = {NULL, 0};
				new_file.name	= strndup(filesRespond.ptr + t[i + 1].start, t[i + 1].end - t[i + 1].start);
				sscanf(filesRespond.ptr + t[i + 3].start, "%lf", &new_file.modified);
				addFile2List(&new_file, printer);
				DEBUG_LOG("File: #%3ld: |%40s| Date: %lf\n", printer->files.qty - 1, new_file.name, new_file.modified);
				i++;
			}
		}
		qsort(printer->files.list, printer->files.qty, sizeof(file_t), compare_files);

		DEBUG_LOG("%s\n", "Sorted list of files");
		for (size_t i = 0; i < printer->files.qty; i++)
		{
			DEBUG_LOG("File: #%3ld: |%40s| Date: %lf\n", i, printer->files.list[i].name,
					  printer->files.list[i].modified);
		}
	}

	string_buffer_finish(&filesRespond);
	free(t);
	return EXIT_SUCCESS;
}
