/*
	response.h
	COMP30023 Computer Systems, Semester 1 2019
*/

#ifndef _RESPONSE_H_
#define _RESPONSE_H_

/* Defines a response */
typedef struct {
	char* content;
	unsigned long length;
} Response;

/* The various pages */
enum RESPONSE_PAGE {
	PAGE_WELCOME,
	PAGE_START,
	PAGE_FIRST_TURN,
	PAGE_ACCEPTED,
	PAGE_DISCARDED,
	PAGE_END_GAME,
	PAGE_GAME_OVER
};

/* Generates response */
Response* generate_response(const enum RESPONSE_PAGE page_identifier,
							const void* content);

/* Generates 404 response */
Response* generate_response_404();

/* Generates 500 response */
Response* generate_response_500();

/* Frees allocated response */
void free_response(Response* response);

#endif
