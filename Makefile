CC=gcc

# optimization
OPT = -O0 -ggdb

TARGET = moonraker-hmi
BUILD_DIR = bin

C_SOURCES =  \
src/main.c \
src/printer.c \
src/jsmn.c \
src/configini.c \
src/curl_utils.c \
src/strbuf.c \
src/anycubic_i3_mega_dgus.c


#######################################
# binaries
#######################################
PREFIX = ""
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif

C_DEFS =

# C includes
C_INCLUDES =  \
-Isrc/includes

# compile gcc flags
CFLAGS += $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -std=gnu11 -fstack-usage -fdiagnostics-color=always

ifeq ($(DEBUG), 1)
CFLAGS += -g3
endif

LIBS = -lcurl
LIBDIR =
LDFLAGS = $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET)

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR):
	mkdir $@

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)