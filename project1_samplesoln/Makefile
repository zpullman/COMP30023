CC          = gcc
OBJ         = response.o file.o main.o server.o keywords.o request.o
CFLAGS      = -ansi -Wall -Wpedantic -O3 -D_POSIX_C_SOURCE
EXE         = image_tagger

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(EXE)
	rm -f $(OBJ)
	rm -f *.gch

format:
	clang-format -style=file -i *.c *.h

archive:
	git archive --format zip -9 master > comp30023_2019_proj1_demo.zip
