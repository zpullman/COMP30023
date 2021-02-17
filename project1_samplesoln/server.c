/*
	server.c: Implementation of image tagger server
	COMP30023 Computer Systems, Semester 1 2019

	References:
	Lab 5/6 sample code
	man pages for accept, getaddrinfo
	https://stackoverflow.com/questions/24194961
	https://www.gnu.org/software/libc/manual/html_node/Inet-Example.html
	https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
*/

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include "request.h"
#include "server.h"

/* Maximum number of queued requests */
static const int MAX_QUEUED_REQUESTS = 10;

/* Function prototypes */
int create_socket(const char* addr, const int port);
void init_state(State* state, const int first);
void perform_cleanup(State* state, fd_set* fd_set);

/* Handles the running of the image_tagger server */
void run_server(const char* addr, const int port, const int quitfd) {
	int i, r;
	State state;
	int sockfd, new_sockfd;
	fd_set active_fd_set, read_fd_set;
	struct signalfd_siginfo fdsi;
	ssize_t s;

	/* Init state */
	init_state(&state, 1);

	/* Create server socket */
	sockfd = create_socket(addr, port);

	/* Set up socket to accept connections, allow maximum of
	MAX_QUEUED_REQUESTS queued client connection requests */
	if (listen(sockfd, MAX_QUEUED_REQUESTS) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* Initialise set of active sockets with master and signal sockets */
	FD_ZERO(&active_fd_set);
	FD_SET(sockfd, &active_fd_set);
	FD_SET(quitfd, &active_fd_set);

	while (1) {
		/* Block until connection on active socket(s)*/
		/* Check fds in read_fd_set and indicate which are ready to be read */
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			perror("select");
			perform_cleanup(&state, &active_fd_set);
			exit(EXIT_FAILURE);
		}

		/* Loop up to maximum set size */
		for (i = 0; i < FD_SETSIZE; ++i) {
			/* if fd is part of read set (those ready to be read) */
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == quitfd) {
					/* Read signal */
					s = read(quitfd, &fdsi, sizeof(struct signalfd_siginfo));
					if (s != sizeof(struct signalfd_siginfo)) {
						perror("read");
						exit(EXIT_FAILURE);
					}

					if (fdsi.ssi_signo != SIGINT &&
						fdsi.ssi_signo != SIGQUIT &&
						fdsi.ssi_signo != SIGTERM) {
						fprintf(stderr, "Unexpected signal\n");
						continue;
					}

					perform_cleanup(&state, &active_fd_set);
					exit(EXIT_SUCCESS);
				} else if (i == sockfd) {
					/* Connection request on original socket */
					/* Accept connection */
					new_sockfd = accept(sockfd, NULL, NULL);
					if (new_sockfd < 0) {
						perror("accept");
						perform_cleanup(&state, &active_fd_set);
						exit(EXIT_FAILURE);
					}

					/* If player is not player 1 or 2 */
					if (state.player_fd[PLAYER_1] != new_sockfd ||
						state.player_fd[PLAYER_2] != new_sockfd) {
						/* If player 1 & player 2 are already connected */
						if (state.player_fd[PLAYER_1] &&
							state.player_fd[PLAYER_2]) {
							/* Server full */
							close(new_sockfd);
							continue;
						}

						/* If we're in game over state */
						if (state.stage == STAGE_4_GAME_OVER) {
							/* Do not allow more clients to join */
							close(new_sockfd);
							continue;
						}

						/* Assign connection to player */
						if (!state.player_fd[PLAYER_1]) {
							state.player_fd[PLAYER_1] = new_sockfd;
						} else if (!state.player_fd[PLAYER_2]) {
							state.player_fd[PLAYER_2] = new_sockfd;
						}
					}

					/* Add new socket to set of sockets */
					FD_SET(new_sockfd, &active_fd_set);
				} else {
					fprintf(stderr, "Request from socket fd %d\n", i);

					/* Process request */
					r = process_request(&state, i);

					/* Close connection */
					if (r == CLOSE_CONNECTION || r == RESET_GAME) {
						close(i);
						FD_CLR(i, &active_fd_set);
						fprintf(stderr, "Closing socket fd %d\n", i);
					}

					/* Also reset state under RESET_GAME */
					if (r == RESET_GAME) {
						init_state(&state, 0);
					}
				}
			}
		}
	}
}

/* Initialises server state to default values */
void init_state(State* state, const int first) {
	state->stage = STAGE_1_WELCOME_MAIN;
	state->player_welcome_stage[PLAYER_1] = PLAYER_BEGIN;
	state->player_welcome_stage[PLAYER_2] = PLAYER_BEGIN;
	state->player_fd[PLAYER_1] = 0;
	state->player_fd[PLAYER_2] = 0;
	state->round = 1;

	if (first) {
		/* Initialising for first time */
		state->player_keywords[PLAYER_1] = keywords_new();
		state->player_keywords[PLAYER_2] = keywords_new();
	} else {
		/* Both player quit, prepare for new players */
		keywords_clear(state->player_keywords[PLAYER_1]);
		keywords_clear(state->player_keywords[PLAYER_2]);
		if (state->player_name[PLAYER_1]) {
			free(state->player_name[PLAYER_1]);
		}
		if (state->player_name[PLAYER_2]) {
			free(state->player_name[PLAYER_2]);
		}
	}

	state->player_name[PLAYER_1] = NULL;
	state->player_name[PLAYER_2] = NULL;
}

/* Performs server cleanup */
void perform_cleanup(State* state, fd_set* fd_set) {
	int i;

	/* Close sockets */
	for (i = 0; i < FD_SETSIZE; ++i) {
		if (FD_ISSET(i, fd_set)) {
			close(i);
		}
	}

	/* Clear names */
	if (state->player_name[PLAYER_1]) {
		free(state->player_name[PLAYER_1]);
	}
	if (state->player_name[PLAYER_2]) {
		free(state->player_name[PLAYER_2]);
	}

	/* Free state */
	keywords_free(state->player_keywords[PLAYER_1]);
	keywords_free(state->player_keywords[PLAYER_2]);
}

/* Create and return a socket bound to the given port */
int create_socket(const char* addr, const int port) {
	/* Socket file descriptor and address of server */
	int sockfd;
	struct sockaddr_in serv_addr;
	int re;

	/* Create SOCK_STREAM (connection oriented) socket for
	PF_INET address family */
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket open");
		exit(EXIT_FAILURE);
	}

	/* Create listen address for given port number (in network byte order)
	and addr */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(addr);
	serv_addr.sin_port = htons(port);

	/* Reuse port if possible */
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/* Bind address to socket */
	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	return sockfd;
}
