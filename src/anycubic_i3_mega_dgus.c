#include "anycubic_i3_mega_dgus.h"
#include "logging.h"
#include "curl_utils.h"

extern int serial_port;
char	selectedFile[BUFFER_SIZE];
char	http_command[BUFFER_SIZE];
char	respond[RESPOND_BUF_SIZE];

static void makeRespond(reactions_uart_respond_t respond);

int
react(printer_t *printer, char *command, string_buffer_t *uart_respond)
{
	string_buffer_t strbuf;
	string_buffer_initialize(&strbuf);

	char	cmd;
	int32_t val;

	if (sscanf(command, "%c%d", &cmd, &val) != 2)
		cmd = 0;

	if (cmd == 'A')
	{
		static bool extrude_sent = false;
		static bool remove_sent	 = false;
		switch (val)
		{
			case 0: /* Current hotend temp */
				snprintf(respond, RESPOND_BUF_SIZE, "A0V %.0f\r\n", printer->extruder_temp);
				break;
			case 1: /* Target hotend temp*/
				snprintf(respond, RESPOND_BUF_SIZE, "A1V %.0f\r\n", printer->extruder_target);
				break;
			case 2: /* Current bed temp */
				snprintf(respond, RESPOND_BUF_SIZE, "A2V %.0f\r\n", printer->heatbed_temp);
				break;
			case 3: /* Target bed temp */
				snprintf(respond, RESPOND_BUF_SIZE, "A3V %.0f\r\n", printer->heatbed_target);
				break;
			case 4: /* Fan speed */
				snprintf(respond, RESPOND_BUF_SIZE, "A4V %.0f\r\n", printer->fan_speed * 100);
				break;
			case 5: /* Printhead coordinates */
				snprintf(respond, RESPOND_BUF_SIZE, "A5V X: %.2f Y: %.2f Z: %.2f\r\n", printer->position.X,
						 printer->position.Y, printer->position.Z);
				break;
			case 6: /* Printing progress */
				snprintf(respond, RESPOND_BUF_SIZE, "A6V %.0f\r\n", printer->progress * 100);
				break;
			case 7: /* Print time */
				do
				{
					float print_time = time_calc(printer);
					snprintf(respond, RESPOND_BUF_SIZE, "A7V %dH %dM\r\n", ((int)print_time) / 3600,
							 (((int)print_time) % 3600) / 60);
				} while (0);

				break;
			case 8: /* Files list select */
				size_t filenum = 0;
				if (printer->files.qty == 0)
					makeRespond(NO_SD_CARD);
				else
				{
					int written_bytes = 0;
					sscanf(command, "A8 S%ld\r", &filenum);
					DEBUG_LOG("File #: %ld\n", filenum);
					written_bytes =
						snprintf(respond, RESPOND_BUF_SIZE, "FN \r\n");
					for (size_t i = filenum; i < (filenum + 4) && i < printer->files.qty; i++)
					{
						written_bytes +=
							snprintf(respond + written_bytes, RESPOND_BUF_SIZE - written_bytes, "%ld\r\n", i);
						written_bytes +=
							snprintf(respond + written_bytes,
									 RESPOND_BUF_SIZE - written_bytes, "%s\r\n",
									 printer->files.list[i].name);
					}
					written_bytes += snprintf(respond + written_bytes, RESPOND_BUF_SIZE - written_bytes, "END\r\n");
				}
				break;
			case 9: /* Pause print */
				if (curl_POST(printer->cfg.host, "/printer/gcode/script", "script=PAUSE", &strbuf) != NULL)
					makeRespond(PAUSE_SUCCESS);
				break;
			case 10: /* Resume print */
				if (curl_POST(printer->cfg.host, "/printer/gcode/script", "script=RESUME", &strbuf) != NULL)
					makeRespond(PRINT_FROM_SD);
				break;
			case 11: /* Stop print */
				if (curl_POST(printer->cfg.host, "/printer/gcode/script", "script=STOP", &strbuf) != NULL)
					makeRespond(STOP_PRINT);
				break;
			case 12: /* Kill. Emergency stop */
				if (curl_POST(printer->cfg.host, "/printer/emergency_stop", "", &strbuf) != NULL)
					makeRespond(MAINBOARD_RESET);
				break;
			case 13: /* Select file */
				do
				{
					int fileNum = 0;
					if (sscanf(command, "A13 %d\r", &fileNum) != 1)
					{
						makeRespond(FILE_OPEN_FAIL);
						break;
					}
					printer->selected_file = &printer->files.list[fileNum];
					DEBUG_LOG("Selected file: #%4d %s\n", fileNum, printer->selected_file->name);
					if (fillFileData(printer->selected_file, printer) < 0)
						makeRespond(FILE_OPEN_FAIL);
					else
						makeRespond(FILE_OPEN_SUCCESS);
				} while (0);
				break;
			case 14: /* Start print selected file */
				do
				{
					char *urlfilename = NULL;
					if (printer->state == PRINTING)
					{
						/* If file already in print - do noting */
						if (printer->selected_file == printer->printing_file)
							break;
						/* If Printing in process and selected file is a new one - send an error */
						if (printer->printing_file != NULL && printer->selected_file != printer->printing_file)
						{
							makeRespond(MAINBOARD_RESET);
							break;
						}
					}
					else
					{
						DEBUG_LOG("Start printing file: \"%s\"\n", printer->selected_file->name);
						urlfilename = urlEncode(printer->selected_file->name, strlen(printer->selected_file->name));
						snprintf(http_command, BUFFER_SIZE, "filename=%s", urlfilename);
						free(urlfilename);

						if (curl_POST(printer->cfg.host, "/printer/print/start", http_command, &strbuf) == NULL)
							makeRespond(MAINBOARD_RESET);
						else
							printer->printing_file = printer->selected_file;
					}
				} while (0);
				break;
			case 16: /* Set hotend temp */
				do
				{
					int value = 0;
					sscanf(command, "A16 %*c%d", &value);
					sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=%d", value);
					if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
						LOG_ERR("Set bed temp error: '%s'", command);
					makeRespond(NOZZLE_HEATING);
					/* code */
				} while (0);
				break;
			case 17: /* Set bed temp */
				do
				{
					int value = 0;
					sscanf(command, "A17 S%d", &value);
					sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=%d", value);
					if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
						LOG_ERR("Set bed temp error: '%s'", command);
					makeRespond(BED_HEATING);
					/* code */
				} while (0);
				break;
			case 18: /* Set fan speed */
				do
				{
					int value = 0;
					sscanf(command, "A18 S%d", &value);
					sprintf(http_command, "script=M106%%20S%d", (value * 255) / 100);
					if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
						LOG_ERR("Fan speed error: '%s'", command);
				} while (0);
				break;
			case 19: /* Turn off motors */
				sprintf(http_command, "script=M84");
				if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
					LOG_ERR("Motors stop error: %s", command);
				extrude_sent = false;
				remove_sent	 = false;
				break;
			case 20: /* Set printing rate speed */
				do
				{
					int value = 0;
					if (sscanf(command, "A20 S%d", &value) == 1)
					{
						sprintf(http_command, "script=M220%%20S%d", value);
						if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
							LOG_ERR("Print speed error: '%s'", command);
					}
					else
					{
						snprintf(respond, RESPOND_BUF_SIZE, "A20V %.0f\r\n", printer->feed_rate * 100);
					}
				} while (0);
				break;
			case 21: /* Home */
				do
				{
					char axis = '\0';
					sscanf(command, "A21 %c", &axis);
					sprintf(http_command, "script=G28%c", axis == 'C' ? '\0' : axis);
					if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
						LOG_ERR("Home error: '%s'", command);
				} while (0);
				break;
			case 22: /* Move axis e.g: A22' 'X' '+10F1000\r\n */
				do
				{
					char  axis	 = '\0';
					float length = 0;
					int	  speed	 = 0;
					sscanf(command, "A22 %c %fF%d", &axis, &length, &speed);
					if (axis != 'E')
					{
						sprintf(http_command, "script=G91%%0AG1%%20%c%f%%20F%d%%0AG90", axis, length, speed);
						if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
							LOG_ERR("%s", "Axis move error");
					}
					else
					{
						if (length < 0 && !remove_sent)
						{
							sprintf(http_command, "script=FILAMENT_UNLOAD");
							if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
								LOG_ERR("Filament load error: %s", command);
							remove_sent	 = true;
							extrude_sent = false;
						}
						else if (length > 0 && !extrude_sent)
						{
							sprintf(http_command, "script=FILAMENT_LOAD");
							if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
								LOG_ERR("Filament load error: %s", command);
							remove_sent	 = false;
							extrude_sent = true;
						}
					}
				} while (0);
				break;
			case 23: /* Preheat PLA */
				sprintf(http_command,
						"script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=%d%%0A"
						"SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=%d",
						printer->cfg.PLA_E, printer->cfg.PLA_B);
				if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
					LOG_ERR("%s", "Extruder preheat error");
				break;
			case 24: /* Preheat ABS */
				sprintf(http_command,
						"script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=%d%%0A"
						"SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=%d",
						printer->cfg.ABS_E, printer->cfg.ABS_B);
				if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
					LOG_ERR("%s", "Extruder preheat error");
				break;
			case 25: /* Cooldown */
				sprintf(http_command,
						"script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=0%%0A"
						"SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=0");
				if (curl_POST(printer->cfg.host, "/printer/gcode/script", http_command, &strbuf) == NULL)
					LOG_ERR("%s", "Extruder preheat error");
				break;
			case 26: /* Refresh SD Card */
				if (getFileListFromServer(printer))
					makeRespond(NO_SD_CARD);
				break;
			default:
				cmd = 0;
				break;
		}
		string_buffer_append(respond, uart_respond);
		DEBUG_LOG("Respond: %s", uart_respond->ptr);
		string_buffer_finish(&strbuf);
		return EXIT_SUCCESS;
	}

	LOG_ERR("Unknown request: %s\n", command);
	string_buffer_finish(&strbuf);
	return EXIT_SUCCESS;
}


static void
makeRespond(reactions_uart_respond_t rsp)
{
	snprintf(respond, RESPOND_BUF_SIZE, "J%02d\r\n", rsp);
}