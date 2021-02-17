CC			= gcc
CFLAGS	= -Wall -std=c99
#modify flags above
EXE			= crack
OBJ			= crack.o sha256.o
# add new objects above

#top (default) target
all: $(EXE)

#how to link exectutable
$(EXE): $(OBJ)
		$(CC) $(CFLAGS) -o $(EXE) $(OBJ)

#other dependencies
crack.o: crack.h sha256.h
sha256.o: sha256.h

#phony targets dont represent files
.PHONY: clean cleanly all CLEAN

clean:
	rm -f $(OBJ)
CLEAN: clean
	rm -f $(EXE)
cleanly: all clean
