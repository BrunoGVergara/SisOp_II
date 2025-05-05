# Directories
IDIR = include
SDIR = src
CDIR = client
SERVERDIR = server
UTILDIR = utils
COMMOM = commom

# Compiler and flags
CC = g++
CFLAGS = -I. -I$(IDIR) -std=c++20  # Use C++20 standard

# Dependencies
DEPS = $(IDIR)/Client.h $(IDIR)/Server.h $(IDIR)/Notify.h $(IDIR)/global_settings.h $(IDIR)/concurrent_dictionary.h $(IDIR)/Packet.h $(IDIR)/Packet.h  $(IDIR)/FileDispacher.h $(IDIR)/Util.h


# Object files for server and client
OBJSERVER = $(SDIR)/$(SERVERDIR)/main.o $(SDIR)/$(SERVERDIR)/Server.o $(SDIR)/$(UTILDIR)/global_settings.o $(SDIR)/$(COMMOM)/Packet.o $(SDIR)/commom/Service.o $(SDIR)/$(UTILDIR)/FileDispacher.o $(SDIR)/$(UTILDIR)/Util.o
OBJCLIENT = $(SDIR)/$(CDIR)/main.o $(SDIR)/$(CDIR)/Client.o $(SDIR)/$(CDIR)/Notify.o $(SDIR)/$(UTILDIR)/global_settings.o $(SDIR)/$(COMMOM)/Packet.o $(SDIR)/commom/Service.o $(SDIR)/$(UTILDIR)/FileDispacher.o

# Pattern rules
%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Default target
default: server client

# Build server
server: $(OBJSERVER)
	$(CC) -o $@ $^ $(CFLAGS)

# Build client
client: $(OBJCLIENT)
	$(CC) -o $@ $^ $(CFLAGS)


# Clean up build files
clean:
	rm -rf server client $(OBJSERVER) $(OBJCLIENT)
