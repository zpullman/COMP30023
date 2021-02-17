/*
	request.h
	COMP30023 Computer Systems, Semester 1 2019
*/

#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "server.h"

/* Return values for process_request */
enum REQUEST_ACTION { DONT_CLOSE, CLOSE_CONNECTION, RESET_GAME };

/* Processes requests and writes responses */
enum REQUEST_ACTION process_request(State* state, const int fd);

#endif
