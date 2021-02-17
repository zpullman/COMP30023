/*
	server.h
	COMP30023 Computer Systems, Semester 1 2019
*/

#ifndef _SERVER_H_
#define _SERVER_H_

#include "keywords.h"

/* Differentiate between players */
typedef enum { PLAYER_1 = 0, PLAYER_2 = 1 } PLAYER;

/* Stages of game */
typedef enum {
	STAGE_1_WELCOME_MAIN,
	STAGE_2_PLAYING,
	STAGE_3_GAME_COMPLETE,
	STAGE_4_GAME_OVER
} STAGE;

/* Player stage */
typedef enum {
	PLAYER_BEGIN,
	PLAYER_NAME_ENTERED,
	PLAYER_START
} PLAYER_WELCOME_STAGE;

/* State of server */
typedef struct {
	/* Stage of game that server is in */
	STAGE stage;

	/* Welcome stage that player is in */
	PLAYER_WELCOME_STAGE player_welcome_stage[2];

	/* Player socket fds */
	int player_fd[2];

	/* Player names */
	char* player_name[2];

	/* Player keywords */
	Keywords* player_keywords[2];

	/* Game round */
	unsigned long round;
} State;

/* Handles the running of the image_tagger server */
void run_server(const char* addr, const int port, const int quitfd);

#endif
