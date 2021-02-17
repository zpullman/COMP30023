/*
	request.c: Processes requests and writes responses
	COMP30023 Computer Systems, Semester 1 2019
*/

#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "response.h"

/* HTTP method/URIs */
enum METHOD_URI {
	GET_INDEX,
	POST_INDEX,
	FAVICON,
	GET_START,
	POST_START,
	UNKNOWN
};

/* Maximum request header size */
/* 8192 is used as it is the limit for some CDNs/Web servers */
static const int MAX_REQUEST_SIZE = 8192;

/* Image string */
static const char IMAGE_STR[] = "comp30023-project/image-";
/* Number of images that we have */
static const int NUM_IMAGES = 4;

/* Start strings */
static const char COOKIE_START[] = "Cookie: name=";
static const char NAME_START[] = "user=";
static const char KEYWORD_START[] = "keyword=";
static const char DELIMITER[] = "=";

/* Function prototypes */
Response* determine_response(State* state, const char* buffer,
							 const enum METHOD_URI request_method,
							 const int player, int* return_value);
void change_image_for_round(Response* response, const int round);
const enum METHOD_URI find_method_uri(const char* buffer);
char* extract_form_data(const char* buffer, const char* start_str,
						const char* delimiter);
const int quit_present(const char* buffer);

/* Processes requests */
enum REQUEST_ACTION process_request(State* state, const int fd) {
	int n, return_value;
	char* buffer;
	enum METHOD_URI request_method;
	PLAYER player;
	Response* response;

	/* Create buffer and read from connection */
	buffer = calloc(MAX_REQUEST_SIZE + 1, sizeof(char));
	if (!buffer) {
		fprintf(stderr, "Calloc failure\n");
		exit(EXIT_FAILURE);
	}
	n = read(fd, buffer, MAX_REQUEST_SIZE);
	if (n < 0) {
		perror("read");
		free(buffer);
		exit(EXIT_FAILURE);
	}
	/* EOF for stream connection, so close socket and remove from set */
	if (n == 0) {
		free(buffer);
		return CLOSE_CONNECTION;
	}
	buffer[n] = '\0';

	/* Get method from request */
	request_method = find_method_uri(buffer);
	/* Get player number (used to index later) */
	player = fd == state->player_fd[PLAYER_1] ? PLAYER_1 : PLAYER_2;

	/* Default return value - don't close socket */
	return_value = DONT_CLOSE;

	/* Determine the response */
	response = determine_response(state, buffer, request_method, player,
								  &return_value);

	/* Server implementation is faulty */
	if (!response) {
		response = generate_response_500();
	}

	/* Change image depending on round */
	change_image_for_round(response, state->round);

	/* Send the generated response */
	if (write(fd, response->content, response->length) < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
	free_response(response);
	free(buffer);
	return return_value;
}

/* Determines the response */
Response* determine_response(State* state, const char* buffer,
							 const enum METHOD_URI request_method,
							 const int player, int* return_value) {
	Response* response;
	char *keyword, *joined_keywords, *name;
	response = NULL;

	/* Favicon and quit */
	if (request_method == FAVICON) {
		/* Handle GET to /favicon.ico */
		response = generate_response_404();
		return response;
	} else if (state->stage == STAGE_4_GAME_OVER) {
		/* When on game over */
		/* First player has already received game over page at this point */
		response = generate_response(PAGE_GAME_OVER, NULL);
		*return_value = RESET_GAME;
		return response;
	} else if (quit_present(buffer)) {
		/* Handle quit on any page */
		*return_value = CLOSE_CONNECTION;
		state->player_fd[player] = 0;

		/* Switch to game over stage if not on stage 1 */
		if (state->stage != STAGE_1_WELCOME_MAIN) {
			state->stage = STAGE_4_GAME_OVER;
		}
		if (state->stage == STAGE_1_WELCOME_MAIN &&
			state->player_welcome_stage[player == PLAYER_1 ? PLAYER_2
														   : PLAYER_1] ==
				PLAYER_START) {
			state->stage = STAGE_4_GAME_OVER;
		}

		response = generate_response(PAGE_GAME_OVER, NULL);
		return response;
	}

	if (state->stage == STAGE_1_WELCOME_MAIN) {
		if (state->player_welcome_stage[player] == PLAYER_BEGIN &&
			request_method == GET_INDEX) {
			/* GET Welcome page */
			/* Try to extract name from cookie */
			name = extract_form_data(buffer, COOKIE_START, DELIMITER);
			if (!name) {
				response = generate_response(PAGE_WELCOME, NULL);
				free(name);
			} else {
				state->player_name[player] = name;
				state->player_welcome_stage[player] = PLAYER_NAME_ENTERED;
				response = generate_response(PAGE_START, name);
			}
		} else if (state->player_welcome_stage[player] == PLAYER_BEGIN &&
				   request_method == POST_INDEX) {
			/* POST name (after welcome) */
			/* Extract player name */
			name = extract_form_data(buffer, NAME_START, DELIMITER);
			if (strlen(name) == 0) {
				response = generate_response(PAGE_WELCOME, NULL);
				free(name);
			} else {
				/* Extract and save player name */
				state->player_name[player] = name;

				/* Save name */
				state->player_welcome_stage[player] = PLAYER_NAME_ENTERED;
				response =
					generate_response(PAGE_START, state->player_name[player]);
			}
		} else if (state->player_welcome_stage[player] ==
					   PLAYER_NAME_ENTERED &&
				   request_method == GET_INDEX) {
			response =
				generate_response(PAGE_START, state->player_name[player]);
		} else if (state->player_welcome_stage[player] ==
					   PLAYER_NAME_ENTERED &&
				   request_method == GET_START) {
			/* Click start on start page */
			/* Change player state */
			state->player_welcome_stage[player] = PLAYER_START;
			response = generate_response(PAGE_FIRST_TURN, NULL);

			/* Move onto the next game state if both players have started */
			if (state->player_welcome_stage[PLAYER_1] == PLAYER_START &&
				state->player_welcome_stage[PLAYER_2] == PLAYER_START) {
				state->stage = STAGE_2_PLAYING;
			}
		} else if (state->player_welcome_stage[player] == PLAYER_START &&
				   request_method == GET_START) {
			/* Get start page again */
			response = generate_response(PAGE_FIRST_TURN, NULL);
		} else if (state->player_welcome_stage[player] == PLAYER_START &&
				   request_method == POST_START) {
			/* Other player not ready yet */
			response = generate_response(PAGE_DISCARDED, NULL);
		}
	} else if (state->stage == STAGE_2_PLAYING) {
		if ((request_method == GET_INDEX || request_method == GET_START) &&
			state->player_keywords[player]->length == 0) {
			/* Get first turn keywords page, when no keywords entered */
			response = generate_response(PAGE_FIRST_TURN, NULL);
		} else if (request_method == GET_INDEX &&
				   state->player_keywords[player]->length > 0) {
			/* Get accepted keyword page */
			response = generate_response(PAGE_ACCEPTED, NULL);
		} else if (request_method == POST_START ||
				   request_method == POST_INDEX) {
			/* New keyword */
			/* Extract keyword */
			keyword = extract_form_data(buffer, KEYWORD_START, DELIMITER);

			if (!keyword) {
				/* Keyword empty */
				response = generate_response(PAGE_ACCEPTED, NULL);
			} else if (keywords_match(state->player_keywords[(player + 1) % 2],
									  keyword)) {
				/* If match */
				state->stage = STAGE_3_GAME_COMPLETE;
				keywords_clear(state->player_keywords[PLAYER_1]);
				keywords_clear(state->player_keywords[PLAYER_2]);
				state->player_welcome_stage[PLAYER_1] = PLAYER_BEGIN;
				state->player_welcome_stage[PLAYER_2] = PLAYER_BEGIN;
				state->player_welcome_stage[player] = PLAYER_NAME_ENTERED;
				state->round++;
				response = generate_response(PAGE_END_GAME, NULL);
			} else {
				/* If no match, add and get list for response */
				keywords_append(state->player_keywords[player], keyword);
				joined_keywords =
					print_keywords(state->player_keywords[player]);
				response = generate_response(PAGE_ACCEPTED, joined_keywords);
				free(joined_keywords);
			}
			free(keyword);
		}
	} else if (state->stage == STAGE_3_GAME_COMPLETE) {
		if (state->player_welcome_stage[player] == PLAYER_BEGIN) {
			/* Serve game complete page */
			state->player_welcome_stage[player] = PLAYER_NAME_ENTERED;
			response = generate_response(PAGE_END_GAME, NULL);
		} else if (request_method == GET_START) {
			/* If other player has started as well */
			if (state->player_welcome_stage[player == PLAYER_1 ? PLAYER_2
															   : PLAYER_1] ==
				PLAYER_START) {
				state->stage = STAGE_2_PLAYING;
			}
			state->player_welcome_stage[player] = PLAYER_START;
			response = generate_response(PAGE_FIRST_TURN, NULL);
		} else if (request_method == POST_INDEX ||
				   request_method == POST_START) {
			response = generate_response(PAGE_DISCARDED, NULL);
		}
	}

	return response;
}

/* Changes the image depending on the round */
void change_image_for_round(Response* response, const int round) {
	char* image_loc;
	int num;

	/* Calculate num */
	if (round % NUM_IMAGES == 0) {
		num = 4;
	} else {
		num = round % NUM_IMAGES;
	}

	/* Attempt to find image string in response */
	image_loc = strstr(response->content, IMAGE_STR);
	if (image_loc) {
		/* Change image */
		image_loc += strlen(IMAGE_STR);
		image_loc[0] = num + 48;
	}
}

/* Extracts some form data, caller is responsible for freeing */
/* e.g. foo=x&b=x, start_str is foo=, delimiter is =, and returned data is x */
char* extract_form_data(const char* buffer, const char* start_str,
						const char* delimiter) {
	char* start;
	char* content_start;
	char* content;

	/* Find start */
	start = strstr(buffer, start_str);
	if (!start) {
		return NULL;
	}

	/* Find start of content (after delimiter) */
	content_start = strstr(start, delimiter) + strlen(delimiter);
	if (!content_start || content_start[0] == '&') {
		return NULL;
	}
	/* Only want content up to & or end of line */
	strtok(content_start, "&");
	strtok(content_start, "\r");

	/* Allocate */
	content = malloc(sizeof(char) * strlen(content_start) + 1);
	if (!content) {
		fprintf(stderr, "Malloc failure\n");
		exit(EXIT_FAILURE);
	}

	/* Copy and return */
	strcpy(content, content_start);
	return content;
}

/* Determines whether quit is present */
const int quit_present(const char* buffer) {
	return strstr(buffer, "quit=Quit") == NULL ? 0 : 1;
}

/* Find method */
const enum METHOD_URI find_method_uri(const char* buffer) {
	if (strncmp(buffer, "GET / HTTP", 10) == 0) {
		return GET_INDEX;
	} else if (strncmp(buffer, "POST / HTTP", 11) == 0) {
		return POST_INDEX;
	} else if (strncmp(buffer, "GET /favicon.ico HTTP", 21) == 0) {
		return FAVICON;
	} else if (strncmp(buffer, "GET /?start=Start", 17) == 0) {
		return GET_START;
	} else if (strncmp(buffer, "POST /?start=Start", 18) == 0) {
		return POST_START;
	}
	return UNKNOWN;
}
