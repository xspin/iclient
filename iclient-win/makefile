TARGET = iclient.exe
BIN_DIR = 
DDK = ../lbs/inc/ddk/
INCLUDES = -I../lbs/inc/ #-I../lbs/inc/mfc42 -I../lbs/inc/atl21
# -I../lbs/Include/ -I$(DDK)/wnet -I$(DDK)/w2k
LIBS = ../WpdPack/Lib/wpcap.lib\
		 	 ../lbs/lib/wnet/i386/ws2_32.lib\
			 ../lbs/lib/wnet/i386/iphlpapi.lib
#LINKFLAGS =  ../lbs/lib/wnet/i386/iphlpapi.lib
CC = i586-mingw32msvc-g++
CCFLAGS =

CPP_SOURCES = iclient.cpp iclient_main.cpp
C_SOURCES = md5.c

CPP_OBJS = iclient.o iclient_main.o
C_OBJS = md5.o

# ------------  generate the names of the object files  ------------------------
OBJECTS = $(addsuffix .o,$(BASENAMES))

# ------------  generate the names of the hidden prerequisite files  -----------PREREQUISITES   = $(addprefix .,$(addsuffix .d,$(BASENAMES)))

# ------------  make the executable (the default goal)  ------------------------

$(C_OBJS): $(C_SOURCES)
	$(CC) -c $<  $(CCFLAGS) $(INCLUDES)

iclient.o: iclient.cpp
	$(CC) -c $<  $(CCFLAGS) $(INCLUDES)
iclient_main.o: iclient_main.cpp
	$(CC) -c $<  $(CCFLAGS) $(INCLUDES)

$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CC) -o $(TARGET) $^ $(LIBS) $(LINKFLAGS)

all:$(TARGET)

install:
	cp $(TARGET) ./release/iClient.exe
	#cp client.exe ./release/Info.exe
	#cp run.bat ./release/run.bat

clean:
	rm -f $(CPP_OBJS) $(C_OBJS)
	rm -f $(TARGET)
#install: $(TARGET)
	#cp $(TARGET) $(BIN_DIR)
#uninstall:
	#rm -f $(BIN_DIR)/$(TARGET)
rebuild: clean all

# ==============================================================================
# vim: set tabstop=2: set shiftwidth=2:
