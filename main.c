
#include "main.h"

// Allocate memory for read buffer, set size according to your needs
char read_buf[256];
char usart_tx_buf[256];
char post_req[256];
char FileName[256];
char selectedFile[256];
int serial_port;
char *get_req;
char *fileList;
char *host;

jsmn_parser p;
jsmntok_t *filesList = NULL;
size_t filesBufSize=128;

/* Function realloc_it() is a wrapper function for standard realloc()
 * with one difference - it frees old memory pointer in case of realloc
 * failure. Thus, DO NOT use old data pointer in anyway after call to
 * realloc_it(). If your code has some kind of fallback algorithm if
 * memory can't be re-allocated - use standard realloc() instead.
 */
static inline void *realloc_it(void *ptrmem, size_t size) {
  void *p = realloc(ptrmem, size);
  if (!p) {
    free(ptrmem);
    fprintf(stderr, "realloc(): errno=%d\n", errno);
  }
  return p;
}

typedef enum
{
	CURL_GET,
	CURL_POST
}CURL_TYPE;

//*** CURL FUNCTIONS START ***//
typedef struct string_buffer_s
{
	char *ptr;
	size_t len;
} string_buffer_t;

typedef struct file
{
		jsmntok_t *name;
		double modified;
} file_t;

typedef struct
{
	file_t *f;
	size_t len;
	string_buffer_t filesRespond;
} jsmntok_arr_t;

string_buffer_t strbuf;
jsmntok_arr_t files;

static void
string_buffer_initialize(string_buffer_t *sb)
{
	sb->len = 0;
	sb->ptr = malloc(sb->len + 1);
	sb->ptr[0] = '\0';
}

static void
string_buffer_finish(string_buffer_t *sb)
{
	free(sb->ptr);
	sb->len = 0;
	sb->ptr = NULL;
}

static size_t
string_buffer_callback(void *buf, size_t size, size_t nmemb, void *data)
{
	string_buffer_t *sb = data;
	size_t new_len = sb->len + size * nmemb;

	sb->ptr = realloc(sb->ptr, new_len + 1);

	memcpy(sb->ptr + sb->len, buf, size * nmemb);

	sb->ptr[new_len] = '\0';
	sb->len = new_len;

	return size * nmemb;
}

static size_t
header_callback(char *buf, size_t size, size_t nmemb, void *data)
{
	return string_buffer_callback(buf, size, nmemb, data);
}

static size_t
write_callback(void *buf, size_t size, size_t nmemb, void *data)
{
	return string_buffer_callback(buf, size, nmemb, data);
}
//*** CURL FUNCTIONS END ***//
printer_t printer;

int msleep(long msec);
int line_process(char *line, char *usart_tx_buf, char *http_command, char *http_address);
char *getFileNameByNum(char *fileList, int fileNum, char buf[]);
jsmntok_t * getFileNameBySmallLeters(jsmntok_arr_t *fileList, const char *small_letters_name);
char *curl_POST(char *address, char *command, string_buffer_t *strbuf);
char *curl_execute(char *address, char *command, string_buffer_t *strbuf, CURL_TYPE request_type);
static int getFileListFromServer(void);
int compare_files  (const void* a, const void* b);

int min(int a, int b)
{
	return a < b ? a : b;
}

int main(int argc, char *argv[])
{
	clock_t http_time;

	//*** jasmine init ***//
	  jsmn_init(&p);

	/* Allocate some tokens as a start */
	filesList = malloc(sizeof(*filesList) * filesBufSize);
	if (filesList == NULL) {
		fprintf(stderr, "malloc(): errno=%d\n", errno);
		return 3;
	}

	//*** CURL INIT START ***//

	host = argv[1];
	char *status_querry_address = "/printer/objects/query";
	char *status_querry_command = "?extruder=temperature,target&heater_bed=temperature,target&fan=speed&gcode_move=position,speed_factor&virtual_sdcard=progress&print_stats&filament_switch_sensor%20Runout_Sensor=filament_detected";

	//*** CURL INIT END ***//

	//*** SERIAL INIT START ***//
	char *portPath = argv[2];
	// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
	serial_port = open(portPath, O_RDWR);

	// Create new termios struct, we call it 'tty' for convention
	struct termios tty;

	// Read in existing settings, and handle any error
	if (tcgetattr(serial_port, &tty) != 0)
	{
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		return 1;
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE;	// Clear all bits that set the data size
	tty.c_cflag |= CS8;		// 8 bits per byte (most common)
	// tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO;														 // Disable echo
	tty.c_lflag &= ~ECHOE;														 // Disable erasure
	tty.c_lflag &= ~ECHONL;														 // Disable new-line echo
	tty.c_lflag &= ~ISIG;														 // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);										 // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 1;

	// Set in/out baud rate to be 9600
	#ifdef ANYCUBIC_DGUS_TFT
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);
	#endif
	#ifdef MKS_TFT35
	//	cfsetispeed(&tty, B115200);
	//cfsetospeed(&tty, B115200);
	 cfsetispeed(&tty, B9600);
	 cfsetospeed(&tty, B9600);
	#endif
	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
	{
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
		return 1;
	}

	//*** SERIAL INIT END ***//
#ifdef DRY_RUN
	DEBUG_LOG("Dry run mode!\n");
#endif
	http_time = clock();
	if (getFileListFromServer())
		return EXIT_FAILURE;


	while (1)
	{
		// Read bytes. The behaviour of read() (e.g. does it block?,
		// how long does it block for?) depends on the configuration
		// settings above, specifically VMIN and VTIME

		int bytes_read = read(serial_port, &read_buf, sizeof(read_buf));
		if (bytes_read < 0)
		{
			printf("Error reading: %s\n", strerror(errno));
		}
		else
		{
			for (int i = 0; i < bytes_read; i++)
			{
				static int index = 0;
				char st[256];
				st[index] = read_buf[i];
				if (st[index] == '\n')
				{
					st[index] = '\0';
					index = 0;
					if (line_process(st, usart_tx_buf, post_req, host) == EXIT_FAILURE)
					{
						return EXIT_FAILURE;
					}
				}
				else if(st[index] == '\0'){
					UART_Print("ok\r\n");
					//write(serial_port, "ok\r\n", strlen("ok\r\n"));
					printf ("Got Zero\n");
				}
				else
				{
					index++;
				}
			}
		}

		if (clock() - http_time > 0.5 && bytes_read >= 0)
		{
			http_time = clock();
			char *json = curl_execute(status_querry_address, status_querry_command, &strbuf, CURL_GET);
			if (json == NULL)
				return EXIT_FAILURE;

			//*** JSON PARSER ***//
			GetPrintPercentage(json, &printer);
			GetPrintDuration(json, &printer);
			GetFanSpeed(json, &printer);
			GetCurrentPosition(json, &printer);
			GetPrintFeedrate(json, &printer);
			GetFilamenSensorStatus(json, &printer);
			GetExtruderTemperature(json, &printer);
			GetExtruderTargetTemperature(json, &printer);
			GetBedTemperature(json, &printer);
			GetBedTargetTemperature(json, &printer);
			//*** JSON PARSER END ***//

			string_buffer_finish(&strbuf);
			//*** HTTP REQUEST END ***//
		}
	}
	free(fileList);

	close(serial_port);

	return EXIT_SUCCESS;
}

int line_process(char *line, char *usart_tx_buf, char *http_command, char *http_address)
{
	memset(&post_req, '\0', sizeof(post_req));
	usart_tx_buf[0] = '\0';
	http_command[0] = '\0';
	printf("Get: %s\n",line);
	if (strnlen(line, (size_t) 256) == (size_t) 256)
	{
		return EXIT_FAILURE;
	}


#ifdef ANYCUBIC_DGUS_TFT
	//*** Anycubic Protocol ***//
	if (!strncmp(line, "A0\r", 3))
	{
		UART_Print("A0V %.0f\r\n", printer.extruder_temp);
	}
	else if (!strncmp(line, "A1\r", 3))
	{
		UART_Print("A1V %.0f\r\n", printer.extruder_target);
	}
	else if (!strncmp(line, "A2\r", 3))
	{
		UART_Print("A2V %.0f\r\n", printer.heatbed_temp);
	}
	else if (!strncmp(line, "A3\r", 3))
	{
		UART_Print("A3V %.0f\r\n", printer.heatbed_target);
	}
	else if (!strncmp(line, "A4\r", 3))
	{
		UART_Print("A4V %.0f\r\n", printer.fan_speed * 100);
	}
	else if (!strncmp(line, "A5\r", 3))
	{
		UART_Print("A5V X: %.2f Y: %.2f Z: %.2f\r\n", printer.position.X_POS, printer.position.Y_POS, printer.position.Z_POS);
	}
	else if (!strncmp(line, "A6\r", 3))
	{
		UART_Print("A6V %.0f\r\n", printer.progress * 100);
	}
	else if (!strncmp(line, "A7\r", 3))
	{
		UART_Print("A7V %dH %dM\r\n", ((int)printer.print_time) / 3600, (((int)printer.print_time) % 3600) / 60);
	}
	else if (!strncmp(line, "A8 ", 3))
	{
		int filenum = 0;
		if (files.filesRespond.ptr == NULL)
		{
			UART_Print("J02\r\n");
		}
		else
		{
			sscanf(line, "A8 S%d\r", &filenum);
			DEBUG_LOG("File #: %d\n", filenum);
			UART_Print("FN \r\n");
			for (int i = filenum; i < (filenum+4) && i < files.len; i++)
			{
				UART_Print("%.*s\r\n", min(files.f[i].name->size,29), files.filesRespond.ptr+files.f[i].name->start);
				UART_Print("%.*s\r\n",  min(files.f[i].name->size,22), files.filesRespond.ptr+files.f[i].name->start);
			}
			UART_Print("END\r\n");
		}
	}
	else if (!strncmp(line, "A9\r", 3))
	{
		if (curl_POST("/printer/gcode/script", "script=PAUSE", &strbuf) == NULL)
			return EXIT_FAILURE;
		UART_Print("J18\r\n");
	}
	else if (!strncmp(line, "A10", 3))
	{
		if (curl_POST("/printer/gcode/script", "script=RESUME", &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A11", 3))
	{
		// sprintf(http_command, "/printer/gcode/script?script=STOP");
		if (curl_POST("/printer/gcode/script", "script=STOP", &strbuf) == NULL){
			return EXIT_FAILURE;
		}
		UART_Print("J16\r\n");
	}
	else if (!strncmp(line, "A12", 3))
	{
		if (curl_POST("/printer/emergency_stop", "", &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A13", 3))
	{
		char *str = line+4;
		int i = 0;
		while (*str != '\r'){
			selectedFile[i++] = *str++;
		}
		selectedFile[i] = '\0';
		printf("Selected: %s\n", selectedFile);
		jsmntok_t *real_name = getFileNameBySmallLeters(&files,selectedFile);
		if (real_name == NULL)
		{
			DEBUG_LOG("Filename \"%s\" not found\n", selectedFile);
			return 0;
		}
		printf("Real Name: %.*s\n", real_name->size , files.filesRespond.ptr + real_name->start);
		sprintf(selectedFile,"%.*s",real_name->size , files.filesRespond.ptr + real_name->start);
		sprintf(http_command, "filename=%s", selectedFile);
		DEBUG_LOG("request: %s\n", http_command);
		if (curl_POST("/server/files/metadata", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		if (strstr(strbuf.ptr,"\"code\": 404") == NULL)
			UART_Print("J20\r\n");
		else
			UART_Print("J21\r\n");
	}
	else if (!strncmp(line, "A14", 3))
	{
		DEBUG_LOG("Start printing file: \"%s\"\n",selectedFile);
		sprintf(http_command, "filename=%s",selectedFile);
		if (curl_POST("/printer/print/start", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		UART_Print("J04\r\n");
	}

	else if (!strncmp(line, "A16", 3))
	{
		int value = 0;
		char subCmd = '\0';
		sscanf(line, "A16 %c%d", &subCmd, &value);
		if (subCmd == 'C' || subCmd == 'S')
		{
			sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=%d", value);
			if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			{
				return EXIT_FAILURE;
			}
				UART_Print("J06\r\n");
		}
	}
	else if (!strncmp(line, "A17", 3))
	{
		int value = 0;
		sscanf(line, "A17 S%d", &value);
		sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=%d", value);
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
		{
			return EXIT_FAILURE;
		}
			UART_Print("J07\r\n");
	}
	else if (!strncmp(line, "A18", 3))
	{
		int value = 0;
		sscanf(line, "A18 S%d", &value);
		sprintf(http_command, "script=M106%%20S%d", value);
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A19", 3))
	{
		sprintf(http_command, "script=M84");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A20", 3))
	{
		UART_Print("A20V %.0f\r\n", printer.feed_rate * 100);
	}
	else if (!strncmp(line, "A21", 3))
	{
		char axis[5] = {'%', '2', '0', 'A', '\0'};
		axis[3] = line[4];
		if (axis[3] == 'C')
			axis[0] = '\0';
		sprintf(http_command, "script=G28%s",axis);
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A22", 3))
	{
		char axis = '\0';
		int length = 0;
		int speed = 0;
		sscanf(line, "A22 %c %dF%d", &axis, &length, &speed);
		sprintf(http_command, "script=G91");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		sprintf(http_command, "script=G1%%20%c%d%%20F%d", axis, length, speed);
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		sprintf(http_command, "script=G90");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A23", 3))
	{
		sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=230");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=80");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A24", 3))
	{
		sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=245");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=100");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A25", 3))
	{
		sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=extruder%%20TARGET=0");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		sprintf(http_command, "script=SET_HEATER_TEMPERATURE%%20HEATER=heater_bed%%20TARGET=0");
		if (curl_POST("/printer/gcode/script", http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
	}
	else if (!strncmp(line, "A26", 3))
	{
		if (getFileListFromServer())
			return EXIT_FAILURE;
	}
	else
		{
		printf("Unknown request: %s\n", line);
	}
#endif
#ifdef MKS_TFT35
	if (!strncmp(line, "M105", 4))
	{
		UART_Print("ok\r\nB:%.1f /%.1f T0:%.1f /%.1f\r\n", printer.heatbed_temp, printer.heatbed_target, printer.extruder_temp, printer.extruder_target);
	}
	else if (!strncmp(line, "M114", 4))
	{
		UART_Print("M114 X:%.3f Y:%.3f Z:%.3f E:%3f\r\nok\r\n", printer.position.X_POS, printer.position.Y_POS, printer.position.Z_POS,printer.position.E_POS);
	}
	else
	{
		/*** Replace all spacet to %20 ***/
		sprintf(http_command, "/printer/gcode/script?script=");
		char *command = http_command+strlen(http_command);
		char *input_line = line;
		while (*input_line)
		{
			if(*input_line != ' ')
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
		//printf("2printer: %s\r\n", strstr(http_command,"=")+1);
		printf("2printer: %s\r\n", http_command);
		if (curl_execute(http_address, http_command, &strbuf) == NULL)
			return EXIT_FAILURE;
		char *result = strstr(strbuf.ptr,"\"result\": \"");
		if (!result){
			result = strstr(strbuf.ptr,"\"error\":");
			if (!result)
			{
				return EXIT_FAILURE;
			}
			else
			{
				result = strstr(strbuf.ptr,"'message': ");
				if (!result)
				{
					return EXIT_FAILURE;
				}
				else
				{
					result +=12;
					char *errMsg = result;
					while (*result++ != '\'')
					{
						//fprintf(stderr,"%c",*result++);
					}
					*result = '\0';
					UART_Print("!! %s\r\nok\r\n",errMsg);
					//fprintf(stderr,"%s\r\n",errMsg);
				}
			}
		}
		else
		{
			result += 11;
			int i = 0;
			while (*result !='\"')
				usart_tx_buf[i++]=*result++;
			usart_tx_buf[i++]='\r';
			usart_tx_buf[i++]='\n';
			usart_tx_buf[i]='\0';
				UART_Print( "%s", usart_tx_buf);
				//fprintf(stdout, "Push: %s", usart_tx_buf);
				//write(serial_port, usart_tx_buf, strlen(usart_tx_buf));
		}
	}

#endif
	string_buffer_finish(&strbuf);
	return EXIT_SUCCESS;
}

char *curl_POST(char *address, char *command, string_buffer_t *strbuf)
{
#ifdef DRY_RUN
	DEBUG_LOG("POST: %s%s%s\n", host, address, command);
	return "ok";
#else
	return curl_execute(address, command, strbuf, CURL_POST);
#endif
}

char *curl_execute(char *address, char *command, string_buffer_t *strbuf, CURL_TYPE request_type)
{
	//*** HTTP REQUEST START ***//
	string_buffer_initialize(strbuf);
	char *url = malloc(strlen(host) + strlen(address) + strlen(command) + 1);
	strcpy(url, host);
	strcat(url, address);
	if (request_type == CURL_GET)
		strcat(url, command);
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();

	if (!curl)
	{
		fprintf(stderr, "Fatal: curl_easy_init() error.\n");
		string_buffer_finish(strbuf);
		free(url);
		return NULL;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	if (request_type == CURL_POST)
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, strbuf);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, strbuf);
	res = curl_easy_perform(curl);
	while (res == CURLE_COULDNT_CONNECT)
	{
		fprintf(stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(res));
		sleep(1);
		res = curl_easy_perform(curl);
	}

	if (res != CURLE_OK)
	{
		fprintf(stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(res));

		curl_easy_cleanup(curl);
		string_buffer_finish(strbuf);
		free(url);
		return NULL;
	}
	free(url);
	curl_easy_cleanup(curl);
	return (char *)strbuf->ptr;
}

char *getFileNameByNum(char *fileList, int fileNum, char buf[])
{
	int i = 0;
	while (i <= fileNum)
	{
		fileList = strstr(fileList, "path");
		if (fileList == NULL)
			return NULL;
		while (*fileList++ != '\"')
				;
		while (*fileList++ != '\"')
			;
		int j = 0;
		while (*fileList != '\"')
		{
			if (*fileList == '/' || j >= 30){
				j = 0;
				break;
			}else{
				buf[j++] = *fileList++;
			}
		}
		if (j >0 ){
			i++;
		}
		buf[j] = '\0';
	}
	return (&buf[0]);
}

jsmntok_t *getFileNameBySmallLeters(jsmntok_arr_t *fileLst, const char *small_letters_name)
{
	size_t pos = 0;

	if (fileLst == NULL || small_letters_name == NULL)
		return NULL;
	while (pos<fileLst->len)
	{
		// char *normalName = fileLst->filesRespond.ptr + (*fileLst).f[pos].name->start;
		//char *smallName = small_letters_name;
		char *normalName = malloc((*fileLst).f[pos].name->size + 1);
		strncpy(normalName, (fileLst->filesRespond.ptr + (*fileLst).f[pos].name->start),(*fileLst).f[pos].name->size);
		for (char *p = normalName; *p; p++)
		{
			*p=tolower(*p);
		}
		if (!strncmp(normalName,small_letters_name,(*fileLst).f[pos].name->size))
		{
			free(normalName);
			return (*fileLst).f[pos].name;
		}
		free(normalName);
		pos++;
	}
	return NULL;
}

static int getFileListFromServer(void)
{
		if (curl_execute("/server/files/list","", &files.filesRespond, CURL_GET) == NULL)
		{
			UART_Print("J02\r\n");
			return EXIT_FAILURE;
		}
		else
		{
			UART_Print("J00\r\n");
			/* JASMINE JSON PARSE */
			for (;;) {
				int r = 0;
				again:
				r = jsmn_parse(&p, files.filesRespond.ptr, files.filesRespond.len, filesList, filesBufSize);
				if (r < 0) {
				if (r == JSMN_ERROR_NOMEM) {
					filesBufSize = filesBufSize * 2;
					filesList = realloc_it(filesList, sizeof(*filesList) * filesBufSize);
					if (filesList == NULL) {
					return 3;
					}
					goto again;
				}
				} else {
				DEBUG_LOG("Files Parsed\n");
				jsmntok_t *curtok = filesList;
				files.len = 0;
				while (curtok->end <= files.filesRespond.len)
				{
					if (!strncmp(files.filesRespond.ptr+curtok->start, "path", 4))
					{
						curtok++;
						files.len++;
						DEBUG_LOG("File: %d %.*s\n",curtok->type, curtok->end - curtok->start, files.filesRespond.ptr+curtok->start);
					}
					curtok++;
				}
				files.f = malloc(sizeof(file_t)*files.len);
				curtok = filesList;
				size_t n = 0;
				while (curtok->end <= files.filesRespond.len)
				{
					if (!strncmp(files.filesRespond.ptr+curtok->start, "path", 4))
					{
						curtok++;
						files.f[n].name = curtok;
						files.f[n].name->size = curtok->end-curtok->start;
						curtok+=2;
						sscanf(files.filesRespond.ptr+curtok->start,"%lf", &files.f[n].modified);
						DEBUG_LOG("File: #%3ld: %40.*s Date: %lf\n",n, min(40,files.f[n].name->size), files.filesRespond.ptr+files.f[n].name->start,
																 files.f[n].modified);
						n++;
					}
					curtok++;
				}
/* Sort files by date */
				qsort(files.f, files.len, sizeof(file_t), compare_files);

				DEBUG_LOG("Sorted list of files\n");
				for (size_t i=0; i<files.len;i++)
				{
					DEBUG_LOG("File: #%3ld: %40.*s Date: %lf\n",i, min(40,files.f[i].name->size), files.filesRespond.ptr+files.f[i].name->start,
																 files.f[i].modified);
				}
				break;
				}
			}

		}
		return EXIT_SUCCESS;
}

int compare_files  (const void* a, const void* b)
{
    return (((file_t*)b)->modified - ((file_t*)a)->modified);
}