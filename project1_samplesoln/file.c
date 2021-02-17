/*
	file.c: Code for file handling
	COMP30023 Computer Systems, Semester 1 2019

	References:
	man pages for strrchr, access, opendir
	http://en.cppreference.com/w/c/io/fopen
*/

#include "file.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* The default buffer size */
static unsigned long DEFAULT_BUFFER_SIZE = 1024;

/* Reads a file and returns a pointer to a File struct */
File* get_file(const char* path) {
	FILE* fp;
	int c;
	unsigned long i;
	unsigned long size;
	char* content;
	File* file;

	/* Open file */
	fp = fopen(path, "r");
	if (!fp) {
		perror("Cannot open file");
		exit(EXIT_FAILURE);
	}

	/* Read the file contents into 'content' buffer */
	i = 0;
	size = DEFAULT_BUFFER_SIZE;
	content = malloc(sizeof(char) * size);
	if (!content) {
		fprintf(stderr, "Malloc failure");
		exit(EXIT_FAILURE);
	}
	while ((c = fgetc(fp)) != EOF) {
		/* Expand buffer - if necessary */
		if (i >= size) {
			size *= 2;
			content = realloc(content, size + 1);
			if (!content) {
				exit(EXIT_FAILURE);
			}
		}

		/* Set character */
		content[i++] = c;
	}
	content[i] = '\0';
	fclose(fp);

	/* Create and return File struct */
	file = malloc(sizeof(*file));
	if (!file) {
		fprintf(stderr, "Malloc failure");
		exit(EXIT_FAILURE);
	}
	file->length = i;
	file->content = content;
	return file;
}

/* Frees a File struct */
void free_file(File* file) {
	free(file->content);
	free(file);
}

/* Returns the extension section of a path */
const char* get_extension(const char* path) {
	char* extension = strrchr(path, '.');
	if (!extension) {
		return "\0";
	}
	return extension + 1;
}

/* Returns value indicating whether the specified file exists */
const int file_exists(const char* path) {
	/* Return 0 if path is directory */
	if (directory_exists(path)) {
		return 0;
	}
	return access(path, F_OK) == 0;
}

/* Returns value indicating whether the specified directory exists */
const int directory_exists(const char* path) {
	DIR* dir = opendir(path);
	int status = dir != NULL;
	closedir(dir);
	return status;
}
