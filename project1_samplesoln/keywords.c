/*
	keywords.c: Code for keyword management
	COMP30023 Computer Systems, Semester 1 2019
*/

#include "keywords.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Initial keywords array size */
static const int INITIAL_SIZE = 20;
/* Delimiter for output */
static const char DELIMITER[] = ", ";

/* Creates a new keywords list */
Keywords* keywords_new() {
	Keywords* keywords = malloc(sizeof(*keywords));
	if (!keywords) {
		fprintf(stderr, "Bad malloc\n");
		exit(EXIT_FAILURE);
	}

	keywords->length = 0;
	keywords->size = INITIAL_SIZE;

	keywords->keywords = malloc(sizeof(char**) * keywords->size);
	if (!keywords->keywords) {
		fprintf(stderr, "Bad malloc\n");
		exit(EXIT_FAILURE);
	}

	return keywords;
}

/* Frees a keywords list */
void keywords_free(Keywords* keywords) {
	int i;
	assert(keywords != NULL);

	/* Free each keyword */
	for (i = 0; i < keywords->length; i++) {
		free(keywords->keywords[i]);
	}

	/* Free container */
	free(keywords->keywords);
	free(keywords);
}

/* Appends a keyword to the list of keywords */
void keywords_append(Keywords* keywords, char* keyword) {
	int i;
	assert(keywords != NULL);

	/* Realloc keywords array if applicable */
	if (keywords->length == keywords->size) {
		keywords->size *= 2;
		keywords->keywords = realloc(
			keywords->keywords, sizeof(*keywords->keywords) * keywords->size);
		if (!keywords->keywords) {
			fprintf(stderr, "Realloc failure\n");
			exit(EXIT_FAILURE);
		}
	}

	/* Copy keyword into array */
	i = keywords->length;
	keywords->keywords[i] = malloc(sizeof(char) * strlen(keyword) + 1);
	if (!keywords->keywords[i]) {
		fprintf(stderr, "Malloc failure\n");
		exit(EXIT_FAILURE);
	}
	strcpy(keywords->keywords[i], keyword);

	/* Increment length */
	keywords->length++;
}

/* Looks for a match (in the specified keywords list) */
int keywords_match(Keywords* keywords, char* keyword) {
	int i;
	assert(keywords != NULL);
	for (i = 0; i < keywords->length; i++) {
		if (strcasecmp(keywords->keywords[i], keyword) == 0) {
			return 1;
		}
	}
	return 0;
}

/* Clears keywords list in preparation for a new game */
void keywords_clear(Keywords* keywords) {
	int i;
	assert(keywords != NULL);
	for (i = 0; i < keywords->length; i++) {
		free(keywords->keywords[i]);
	}
	keywords->length = 0;
}

/* Generates printable keywords list */
char* print_keywords(Keywords* keywords) {
	char *joined_keywords, *location;
	int length;
	int i;
	assert(keywords != NULL);

	/* First, get combined length, */
	length = (keywords->length - 1) * strlen(DELIMITER);
	for (i = 0; i < keywords->length; i++) {
		length += strlen(keywords->keywords[i]);
	}

	/* Allocate output string */
	joined_keywords = malloc(sizeof(char) * (length + 1));
	if (!joined_keywords) {
		fprintf(stderr, "Malloc\n");
		exit(EXIT_FAILURE);
	}

	/* Copy over */
	location = joined_keywords;
	for (i = 0; i < keywords->length; i++) {
		/* Keyword */
		strcpy(location, keywords->keywords[i]);
		location += strlen(keywords->keywords[i]);

		/* Delimiter if not last keyword */
		if (i < keywords->length - 1) {
			strcpy(location, DELIMITER);
			location += strlen(DELIMITER);
		}
	}

	location[0] = '\0';
	return joined_keywords;
}
