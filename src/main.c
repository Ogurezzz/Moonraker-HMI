
#include "anycubic_i3_mega_dgus.h"
#include "strbuf.h"
#include "curl_utils.h"
#include "logging.h"

// Allocate memory for read buffer, set size according to your needs

char *config_path;
char  read_buf[BUFFER_SIZE];
char  usart_tx_buf[BUFFER_SIZE];
char  post_req[BUFFER_SIZE];
char  FileName[BUFFER_SIZE];

int	  serial_port;
char *get_req;
bool hello = false;

// Configuration parameters
#define DEFAULT_CONFIG_PATH "/etc/moonraker-hmi/moonraker-hmi.cfg"

printer_t my_printer;

#define CONFIGREADFILE "../etc/config.cnf"
#define CONFIGSAVEFILE "../etc/new-config.cnf"

#define ENTER_TEST_FUNC                                                      \
	do                                                                       \
	{                                                                        \
		LOG_INFO("%s", "\n-----------------------------------------------"); \
		LOG_INFO("<TEST: %s>\n", __FUNCTION__);                              \
	} while (0)

int msleep(long msec);



int
min(int a, int b)
{
	return a < b ? a : b;
}

int
main(int argc, char *argv[])
{
	clock_t http_time;
	sleep(1);

	//*** Read configuration file ***//
	Config *cfg			= NULL;
	char   *config_path = DEFAULT_CONFIG_PATH;

	LOG_INFO("Moonraker-HMI service v%s starting...", VERSION);
	if (argc > 2)
	{
		LOG_ERR("Too many arguments.%s", "");
		exit(EXIT_FAILURE);
	}
	else if (argc == 2)
	{
		config_path = argv[1];
	}
	else
	{
		LOG_ERR("No config file set. Abort...%s", "");
		exit(EXIT_FAILURE);
	}

	if (ConfigReadFile(config_path, &cfg) != CONFIG_OK)
	{
		LOG_ERR("ConfigOpenFile failed for %s", config_path);
		exit(EXIT_FAILURE);
	}
	ConfigReadString(cfg, "connection", "host", my_printer.cfg.host,
					 sizeof(my_printer.cfg.host), "http://127.0.0.1:7125");
	ConfigReadString(cfg, "connection", "serial", my_printer.cfg.serial,
					 sizeof(my_printer.cfg.serial), "");
	ConfigReadInt(cfg, "preheat", "pla_hotend", &my_printer.cfg.PLA_E, 220);
	ConfigReadInt(cfg, "preheat", "pla_heatbed", &my_printer.cfg.PLA_B, 60);
	ConfigReadInt(cfg, "preheat", "abs_hotend", &my_printer.cfg.ABS_E, 240);
	ConfigReadInt(cfg, "preheat", "abs_heatbed", &my_printer.cfg.ABS_B, 100);
	do
	{
		char time_option[32];
		ConfigReadString(cfg, "time", "time_stat", time_option, sizeof(time_option), "print-time");
		if (strcmp(time_option, "print-time") == 0)
			my_printer.cfg.time = PRINT_TIME;
		else if (strcmp(time_option, "print-time-full") == 0)
			my_printer.cfg.time = PRINT_TIME_FULL;
		else if (strcmp(time_option, "time-left-slicer") == 0)
			my_printer.cfg.time = TIME_LEFT_SLICER;
		else if (strcmp(time_option, "time-left-file") == 0)
			my_printer.cfg.time = TIME_LEFT_FILE;
		else if (strcmp(time_option, "time-left-avg") == 0)
			my_printer.cfg.time = TIME_LEFT_AVG;
		else if (strcmp(time_option, "time-eta") == 0)
			my_printer.cfg.time = TIME_ETA;
	} while (0);

	LOG_INFO("host: %s", my_printer.cfg.host);
	LOG_INFO("serial: %s", my_printer.cfg.serial);

	//*** CURL INIT START ***//

	char *status_query_address = "/printer/objects/query";
	char *status_query_command =
		"extruder=temperature,target&heater_bed=temperature,target&fan=speed&"
		"gcode_move=position,speed_factor&virtual_sdcard=progress&print_stats&"
		"filament_switch_sensor%20Runout_Sensor=filament_detected";

	//*** CURL INIT END ***//

	//*** SERIAL INIT START ***//
	// Open the serial port. Change device path as needed (currently set to an
	// standard FTDI USB-UART cable type device)
	serial_port = open(my_printer.cfg.serial, O_RDWR);

	// Create new termios struct, we call it 'tty' for convention
	struct termios tty;

	// Read in existing settings, and handle any error
	if (tcgetattr(serial_port, &tty) != 0)
	{
		LOG_ERR("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in
							// communication (most common)
	tty.c_cflag &= ~CSIZE;	// Clear all bits that set the data size
	tty.c_cflag |= CS8;		// 8 bits per byte (most common)
	// tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most
	// common)
	tty.c_cflag |=
		CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO;	// Disable echo
	tty.c_lflag &= ~ECHOE;	// Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG;	// Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
					 ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes
						   // (e.g. newline chars)
	tty.c_oflag &=
		~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT
	// PRESENT ON LINUX) tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars
	// (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as
						  // soon as any data is received.
	tty.c_cc[VMIN] = 1;

// Set in/out baud rate to be 9600
#ifdef ANYCUBIC_DGUS_TFT
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);
#endif
#ifdef MKS_TFT35
	//	cfsetispeed(&tty, B115200);
	// cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B9600);
	cfsetospeed(&tty, B9600);
#endif
	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
	{
		LOG_ERR("Error %i from tcsetattr: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
		;
	}

	//*** SERIAL INIT END ***//
	// initPrinter(&my_printer);
#ifdef DRY_RUN
	DEBUG_LOG("%s\n", "Dry run mode!");
#endif
	http_time = clock();
	getFileListFromServer(&my_printer);

	LOG_INFO("Moonraker-HMI service v%s started", VERSION);
	int counter = 10;
	while (counter > 0)
	{
		// Read bytes. The behaviour of read() (e.g. does it block?,
		// how long does it block for?) depends on the configuration
		// settings above, specifically VMIN and VTIME

		int bytes_read = read(serial_port, &read_buf, sizeof(read_buf));
		if (bytes_read < 0)
		{
			LOG_ERR("Error reading: %s\n", strerror(errno));
		}
		else
		{
			for (int i = 0; i < bytes_read; i++)
			{
				static int index = 0;
				char	   st[BUFFER_SIZE];
				st[index] = read_buf[i];
				switch (st[index])
				{
					case '\n':
						st[index] = '\0';
						index	  = 0;
						if (line_process(st, usart_tx_buf, post_req,
										 my_printer.cfg.host) == EXIT_FAILURE)
						{
							return EXIT_FAILURE;
						}
						break;
					case '\0':
						UART_Print("ok\r\n");
						break;
					default:
						index++;
						break;
				}
			}
		}

		if (clock() - http_time > 0.5 && bytes_read >= 0)
		{
			string_buffer_t sb;
			http_time = clock();
			if (curl_GET(my_printer.cfg.host, status_query_address,
						 status_query_command, &sb) != NULL)
				ParsePrinterRespond(&sb, &my_printer);
			string_buffer_finish(&sb);
			// counter--;
			//*** HTTP REQUEST END ***//
		}
	}
	ConfigFree(cfg);
	// string_buffer_finish(&strbuf);

	close(serial_port);
	return EXIT_SUCCESS;
}

int
line_process(char *line, char *usart_tx_buf, char *http_command,
			 char *http_address)
{
	memset(&post_req, '\0', sizeof(post_req));
	usart_tx_buf[0] = '\0';
	http_command[0] = '\0';
	DEBUG_LOG("Get:  %s\n", line);
	if (strnlen(line, (size_t)BUFFER_SIZE) == (size_t)BUFFER_SIZE)
	{
		return EXIT_FAILURE;
	}

#ifdef ANYCUBIC_DGUS_TFT
	string_buffer_t uart_respond;
	string_buffer_initialize(&uart_respond);
	if (react(&my_printer, line, &uart_respond) == EXIT_FAILURE)
		EXIT_FAILURE;
	if (uart_respond.len)
	{
		DEBUG_LOG("Sent: %s", uart_respond.ptr);
		UART_Print("%s", uart_respond.ptr);
	}
	string_buffer_finish(&uart_respond);
#endif
#ifdef MKS_TFT35
	if (!strncmp(line, "M105", 4))
	{
		UART_Print("ok\r\nB:%.1f /%.1f T0:%.1f /%.1f\r\n",
				   my_printer.heatbed_temp, my_printer.heatbed_target,
				   my_printer.extruder_temp, my_printer.extruder_target);
	}
	else if (!strncmp(line, "M114", 4))
	{
		UART_Print("M114 X:%.3f Y:%.3f Z:%.3f E:%3f\r\nok\r\n",
				   my_printer.position.X_POS, my_printer.position.Y_POS,
				   my_printer.position.Z_POS, my_printer.position.E_POS);
	}
	else
	{
		/*** Replace all space to %20 ***/
		sprintf(http_command, "/printer/gcode/script?script=");
		char *command	 = http_command + strlen(http_command);
		char *input_line = line;
		while (*input_line)
		{
			if (*input_line != ' ')
			{
				*command++ = *input_line++;
			}
			else
			{
				*command++ = '%';
				*command++ = '2';
				*command++ = '0';
				input_line++;
			}
		}
		command = '\0';
		// printf("2printer: %s\r\n", strstr(http_command,"=")+1);
		printf("2printer: %s\r\n", http_command);
		if (curl_execute(http_address, http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		char *result = strstr(strbuf.ptr, "\"result\": \"");
		if (!result)
		{
			result = strstr(strbuf.ptr, "\"error\":");
			if (!result)
			{
				return EXIT_FAILURE;
			}
			else
			{
				result = strstr(strbuf.ptr, "'message': ");
				if (!result)
				{
					return EXIT_FAILURE;
				}
				else
				{
					result += 12;
					char *errMsg = result;
					while (*result++ != '\'')
					{
						// LOG_ERR("%c",*result++);
					}
					*result = '\0';
					UART_Print("!! %s\r\nok\r\n", errMsg);
					// LOG_ERR("%s\r\n",errMsg);
				}
			}
		}
		else
		{
			result += 11;
			int i = 0;
			while (*result != '\"') usart_tx_buf[i++] = *result++;
			usart_tx_buf[i++] = '\r';
			usart_tx_buf[i++] = '\n';
			usart_tx_buf[i]	  = '\0';
			UART_Print("%s", usart_tx_buf);
			// LOG_INFO("Push: %s", usart_tx_buf);
			// write(serial_port, usart_tx_buf, strlen(usart_tx_buf));
		}
	}

#endif
	return EXIT_SUCCESS;
}