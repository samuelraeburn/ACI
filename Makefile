.PHONY: clean

CC:= gcc
OBJECTS:=acousticComplexityIndex.o kiss_fft.o kiss_fftr.o
CFLAGS:=-pedantic -Wall
KISS_FFT_DIR:=kiss_fft130
KISS_FFT_TOOLS_DIR:=$(KISS_FFT_DIR)/tools
INCLUDES:=-I$(KISS_FFT_DIR) -I$(KISS_FFT_TOOLS_DIR)
CFLAGS+=$(INCLUDES)

all: acousticComplexityIndex

acousticComplexityIndex.o: acousticComplexityIndex.c
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -c acousticComplexityIndex.c

kiss_fft.o: $(KISS_FFT_DIR)/kiss_fft.c
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -c $(KISS_FFT_DIR)/kiss_fft.c

kiss_fftr.o: $(KISS_FFT_TOOLS_DIR)/kiss_fftr.c
	@echo "Compiling $@"
	@$(CC) $(CFLAGS) -c $(KISS_FFT_TOOLS_DIR)/kiss_fftr.c

acousticComplexityIndex: $(OBJECTS)
	@echo "Creating $@"
	@$(CC) $(OBJECTS) -o acousticComplexityIndex

clean:
	@rm -f *.o acousticComplexityIndex
