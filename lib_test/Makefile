CC=gcc
INCS=-L../bin/lib -I../bin/headers
LIBS=-lexip

# ----------------------------------------------------------------------
# Uncomment below to build the main file
CFLAGS=-g -Wall
OBJS= main.o
ifeq ($(OS), Windows_NT)
	OUT_NAME = main.exe
else
	OUT_NAME = main
endif

main:	$(OBJS)
	$(CC) $(INCS) $(OBJS) -o $(OUT_NAME) $(LIBS)

main.o: main.c
	$(CC) $(INCS) $(CFLAGS) -c $< -o $@

clean:
	-rm -rf *.o $(OUT_NAME)