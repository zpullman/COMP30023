/*
	COMP30023 Computer Systems, Semester 1 2019

	References:
	man page for signalfd
	https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
*/

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signalfd.h>
#include "server.h"

int main(int argc, char* argv[]) {
	sigset_t mask;
	int quitfd;
	char c;
	char* addr;
	int port;

	/* Signals */
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGQUIT);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}
	quitfd = signalfd(-1, &mask, 0);
	if (quitfd == -1) {
		perror("signalfd");
		exit(EXIT_FAILURE);
	}

	/* Set invalid values */
	addr = NULL;
	port = 0;

	/* Parse command line options */
	while ((c = getopt(argc, argv, "h")) != -1) {
		switch (c) {
		case 'h':
			/* Print help */
			printf("Usage: %s hostname port\n", argv[0]);
			exit(EXIT_FAILURE);
		default: exit(EXIT_FAILURE);
		}
	}

	/* Ensure that enough arguments are given */
	if (argc < 3) {
		fprintf(stderr, "%s: missing arguments\n", argv[0]);
		fprintf(stderr, "Try '%s -h' for more information\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Set addr and port if flags are not given */
	if (!addr) {
		addr = argv[optind];
	}
	if (!port) {
		port = atoi(argv[optind + 1]);
	}

	/* Print info */
	printf("image_tagger server is now running at IP: %s on port %d\n", addr,
		   port);
	run_server(addr, port, quitfd);

	return 0;
}
