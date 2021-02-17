/*
	file.h
	COMP30023 Computer Systems, Semester 1 2019
*/

#ifndef _FILE_H_
#define _FILE_H_

/* Represents a file which has been read */
typedef struct {
	/* Content of file */
	char* content;
	/* Length of file contents */
	unsigned long length;
} File;

/* Reads a file and returns a pointer to a File struct */
File* get_file(const char* path);

/* Frees a File struct */
void free_file(File* file);

/* Returns the extension section of a path */
const char* get_extension(const char* path);

/* Returns value indicating whether the specified file exists */
const int file_exists(const char* path);

/* Returns value indicating whether the specified directory exists */
const int directory_exists(const char* path);

#endif
