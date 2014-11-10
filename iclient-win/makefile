TARGET = iclient.exe
PATH_LIBS = /home/i/VirtualBox\ VMs/share/pgm
BIN_DIR = 
#DDK = $(PATH_LIBS)/lbs/inc/ddk/
INCLUDES = -I $(PATH_LIBS)/lbs/inc/ #-I../lbs/inc/mfc42 -I../lbs/inc/atl21
# -I../lbs/Include/ -I$(DDK)/wnet -I$(DDK)/w2k
LIBS = $(PATH_LIBS)/WpdPack/Lib/wpcap.lib\
		 	 $(PATH_LIBS)/lbs/lib/wnet/i386/ws2_32.lib\
			 $(PATH_LIBS)/lbs/lib/wnet/i386/iphlpapi.lib
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
	mv $(TARGET) ./release/iClient.exe

clean:
	rm -f $(CPP_OBJS) $(C_OBJS)
	rm -f $(TARGET)

rebuild: clean all

