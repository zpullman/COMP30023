/*
	keywords.h
	COMP30023 Computer Systems, Semester 1 2019
*/

#ifndef _KEYWORDS_H_
#define _KEYWORDS_H_

/* Defines list of keywords */
typedef struct {
	char** keywords;
	unsigned long length;
	unsigned long size;
} Keywords;

/* Creates a new keywords list */
Keywords* keywords_new();

/* Frees a keywords list */
void keywords_free(Keywords* keywords);

/* Appends a keyword to the list of keywords */
void keywords_append(Keywords* keywords, char* keyword);

/* Looks for a match (in the specified keywords list) */
int keywords_match(Keywords* keywords, char* keyword);

/* Clears keywords list in preparation for a new game */
void keywords_clear(Keywords* keywords);

/* Generates printable keywords list */
char* print_keywords(Keywords* keywords);

#endif
