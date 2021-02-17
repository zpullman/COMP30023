/*
	response.c: Code for generating responses
	COMP30023 Computer Systems, Semester 1 2019
*/

#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"

/* Constants for HTTP */
static const char HTTP_VERSION[] = "HTTP/1.0";

static const char HTTP_STATUS_200_CODE[] = "200";
static const char HTTP_STATUS_200_PHRASE[] = "OK";
static const char HTTP_CONTENT_LENGTH[] = "Content-Length: ";

static const char HTTP_STATUS_404_CODE[] = "400";
static const char HTTP_STATUS_404_PHRASE[] = "Not Found";
static const char HTTP_STATUS_404_LENGTH[] = "Content-Length: 0";

static const char HTTP_STATUS_500_CODE[] = "500";
static const char HTTP_STATUS_500_PHRASE[] = "Internal Server Error";
static const char HTTP_STATUS_500_LENGTH[] = "Content-Length: 0";

static const char CRLF[] = "\r\n";
static const char HTTP_CONTENT_TYPE[] = "Content-Type: ";
static const char MIME_HTML[] = "text/html";
static const char HTTP_KEEP_ALIVE[] = "Connection: Keep-Alive";
static const char HTTP_SET_COOKIE[] = "Set-Cookie: name=";

/* Tag for name */
static const char PTAG_OPEN[] = "<p>";
static const char PTAG_END[] = "</p>";

/* Contants for file names */
static const char FILE_WELCOME[] = "project_1_html_files/1_intro.html";
static const char FILE_START[] = "project_1_html_files/2_start.html";
static const char FILE_FIRST_TURN[] = "project_1_html_files/3_first_turn.html";
static const char FILE_ACCEPTED[] = "project_1_html_files/4_accepted.html";
static const char FILE_DISCARDED[] = "project_1_html_files/5_discarded.html";
static const char FILE_ENDGAME[] = "project_1_html_files/6_endgame.html";
static const char FILE_GAMEOVER[] = "project_1_html_files/7_gameover.html";

/* Default response header size - 512 should be safe as the header (in this
implementation) is composed of relatively short elements */
static const int DEFAULT_RESPONSE_HEADER_SIZE = 512;

/* Function prototypes */
Response* create_response();
File* get_page(const enum RESPONSE_PAGE page_identifier);

/* Initialises a response object */
Response* create_response() {
	Response* response;
	response = malloc(sizeof(*response));
	response->content =
		malloc(sizeof(char) * (DEFAULT_RESPONSE_HEADER_SIZE + 1));
	if (!response || !response->content) {
		fprintf(stderr, "Malloc failure");
		exit(EXIT_FAILURE);
	}
	return response;
}

/* Generates 404 response */
Response* generate_response_404() {
	Response* response;
	response = create_response();
	response->length =
		sprintf(response->content, "%s %s %s%s%s%s%s%s%s", HTTP_VERSION,
				HTTP_STATUS_404_CODE, HTTP_STATUS_404_PHRASE, CRLF,
				HTTP_STATUS_404_LENGTH, CRLF, HTTP_KEEP_ALIVE, CRLF, CRLF);
	return response;
}

/* Generates 500 response */
Response* generate_response_500() {
	Response* response;
	response = create_response();
	response->length =
		sprintf(response->content, "%s %s %s%s%s%s%s%s%s", HTTP_VERSION,
				HTTP_STATUS_500_CODE, HTTP_STATUS_500_PHRASE, CRLF,
				HTTP_STATUS_500_LENGTH, CRLF, HTTP_KEEP_ALIVE, CRLF, CRLF);
	return response;
}

/* Generates response */
Response* generate_response(const enum RESPONSE_PAGE page_identifier,
							const void* content) {
	unsigned long header_length;
	Response* response;
	File* file;
	int additional_length;
	int location;

	/* Get the page as a file */
	file = get_page(page_identifier);

	/* Initialise response object */
	response = create_response();

	/* Generate header line */
	header_length =
		sprintf(response->content, "%s %s %s%s", HTTP_VERSION,
				HTTP_STATUS_200_CODE, HTTP_STATUS_200_PHRASE, CRLF);

	/* Determine additional length (for keywords/name) */
	additional_length = 0;
	if (content) {
		additional_length =
			strlen((char*)content) + strlen(PTAG_OPEN) + strlen(PTAG_END) - 1;
	}

	/* Add headers and an empty line to denote the end of the headers */
	if (content && page_identifier == PAGE_START) {
		/* Set cookie header */
		header_length += sprintf(
			response->content + header_length, "%s%lu%s%s%s%s%s%s%s%s%s%s",
			HTTP_CONTENT_LENGTH, file->length + additional_length, CRLF,
			HTTP_CONTENT_TYPE, MIME_HTML, CRLF, HTTP_KEEP_ALIVE, CRLF,
			HTTP_SET_COOKIE, (char*)content, CRLF, CRLF);
	} else {
		header_length += sprintf(
			response->content + header_length, "%s%lu%s%s%s%s%s%s%s",
			HTTP_CONTENT_LENGTH, file->length + additional_length, CRLF,
			HTTP_CONTENT_TYPE, MIME_HTML, CRLF, HTTP_KEEP_ALIVE, CRLF, CRLF);
	}

	/* Determine total response length and reallocate the buffer */
	response->length = header_length + file->length + additional_length;
	response->content = realloc(response->content, response->length + 1);
	if (!response->content) {
		fprintf(stderr, "Malloc failure");
		exit(EXIT_FAILURE);
	}

	/* Copy file contents onto response content (following header) */
	if (content &&
		(page_identifier == PAGE_ACCEPTED || page_identifier == PAGE_START)) {
		/* Add keywords/name */
		/* Find location */
		location = 0;
		if (page_identifier == PAGE_ACCEPTED) {
			location = strstr(file->content, "</p>") - file->content + 4;
		} else if (page_identifier == PAGE_START) {
			location = strstr(file->content, "<form") - file->content - 1;
		}

		/* First half of file, <p>, content, </p>, rest of file */
		strncpy(response->content + header_length, file->content, location);
		strcpy(response->content + header_length + location, PTAG_OPEN);
		strcpy(response->content + header_length + location +
				   strlen(PTAG_OPEN),
			   (char*)content);
		strcpy(response->content + header_length + location +
				   strlen(PTAG_OPEN) + strlen((char*)content),
			   PTAG_END);
		strcpy(response->content + header_length + location +
				   strlen(PTAG_OPEN) + strlen((char*)content) +
				   strlen(PTAG_END),
			   file->content + location + 1);
	} else {
		/* If no modification is needed */
		memmove(response->content + header_length, file->content,
				file->length);
	}
	response->content[response->length] = '\0';

	free_file(file);
	return response;
}

/* Gets the specified page */
File* get_page(const enum RESPONSE_PAGE page_identifier) {
	File* file = NULL;
	if (page_identifier == PAGE_WELCOME) {
		file = get_file(FILE_WELCOME);
	} else if (page_identifier == PAGE_START) {
		file = get_file(FILE_START);
	} else if (page_identifier == PAGE_FIRST_TURN) {
		file = get_file(FILE_FIRST_TURN);
	} else if (page_identifier == PAGE_ACCEPTED) {
		file = get_file(FILE_ACCEPTED);
	} else if (page_identifier == PAGE_DISCARDED) {
		file = get_file(FILE_DISCARDED);
	} else if (page_identifier == PAGE_END_GAME) {
		file = get_file(FILE_ENDGAME);
	} else if (page_identifier == PAGE_GAME_OVER) {
		file = get_file(FILE_GAMEOVER);
	}
	return file;
}

/* Frees allocated response */
void free_response(Response* response) {
	free(response->content);
	free(response);
}
