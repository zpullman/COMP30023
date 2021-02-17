/*
** image tagger project
** Zac Pullman
** 695145
** Adapted from code given for lab6
*/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// constants
static char const * const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char const * const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_400_LENGTH = 47;
static char const * const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;

const int INTRO = 1;
const int START = 2;
const int TURN = 3;
const int ACCEPT = 4;
const int DISCARD = 5;
const int ENDGAME = 6;
const int GAMEOVER = 7;

struct player {
  char * username;
  char * guess;
  //int sockID;
  int state;
  int numguesses;
};

// represents the types of method
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;
void move_trailing(char * str, char * copy, int length_added, long size);
char ** strsplit(const char * src, const char * delim);
void strsplitfree(char ** str);
bool http_post(char * buff, int sockfd, struct player ** players, int state, char* page, int n);
bool http_get(char * buff, int sockfd, char* page, int n);
void better2(char *s1, char *s2);
bool player_ready(struct player ** players, int sockfd);

static bool handle_http_request(int sockfd, struct player ** players)
{
    // try to read the request
    char buff[2049];
    int n = read(sockfd, buff, 2049);
    if (n <= 0) //line 71
    {
        if (n < 0)
            perror("read");
        else
            printf("socket %d close the connection\n", sockfd);
        return false;
    }

    // terminate the string
    buff[n] = 0;
    char * curr = buff;
    int state = 0;
    // parse the method
    printf("\n\n %s \n\n", curr);
    METHOD method = UNKNOWN;
    if (strncmp(curr, "GET ", 4) == 0)
    {
        curr += 4;
        method = GET;
    }
    else if (strncmp(curr, "POST ", 5) == 0)
    {
        curr += 5;
        method = POST;
    }
    else if (write(sockfd, HTTP_400, HTTP_400_LENGTH) < 0)
    {
        perror("write");
        return false;
    }

    // sanitise the URI
    char * page = "";
    while (*curr == '.' || *curr == '/' || *curr == '?')
        //printf("\n%s\n", curr);
        ++curr;
        //printf("\n%s\n", curr);
    // assume the only valid request URI is "/" but it can be modified to accept more files
    if (*curr == ' ') {
      if (method == GET) {
        page = "1_intro.html";
        state = 1;
      } else if (method == POST) {
        page = "2_start.html";
        state = 2;
          if (strstr(curr, "quit") != NULL) {
            page = "7_gameover.html";
            state = 7;
          }
      }
    } else if (*curr == 's'){
        if (method == GET) {
          page = "3_first_turn.html";
          if(player_ready(players, sockfd) || !player_ready(players, sockfd))
            printf("\n\nPLAYER READY WORKS...kinda\n\n");
          state = 3;
        } else if (method == POST){
            if (strstr(curr, "quit") != NULL)
            page = "7_gameover.html";
            state = 7;

            if (strstr(curr, "guess") != NULL && player_ready(players, sockfd)) {
            page = "4_accepted.html";
            state = 4;
          } else if (strstr(curr, "guess") != NULL && !player_ready(players, sockfd)) {
            page = "5_discarded.html";
            state = 5;
          }
        }
    } else {
      printf("NO MORE FUNCTIONS: LINE 137");
    }
    if (*curr == ' ' || *curr == 's')
      if (method == GET)
        {
          if(!http_get(buff, sockfd, page, n))
            return false;
        }
        else if (method == POST)
        {
          if(!http_post(buff, sockfd, players, state, page, n))
            return false;

        }
        else
            // never used, just for completeness
            fprintf(stderr, "no other methods supported");
    // send 404
    if (write(sockfd, HTTP_404, HTTP_404_LENGTH) < 0)
    {
        perror("write");
        return false;
    }
    return true;
}

void move_trailing(char * str, char * copy, int length_added, long size) {
  int p1, p2;
  for (p1 = size - 1, p2 = p1 - length_added; p1 >= size - 25; --p1, --p2)
      str[p1] = str[p2];
  ++p2;
  // put the separator
  str[p2++] = ' ';
  str[p2++] = ',';
  // copy the string
  int copy_length = strlen(copy);
  strncpy(str + p2, copy, copy_length);
}

bool player_ready(struct player ** players, int sockfd)
{
  long i = 0;
  while(i < 8) {
    if(i == sockfd)
      i++;
    else
        if(players[i]->state == 4 || players[i]->state == 5)
          return true;
      i++;
  }
  return false;
}

struct player * initialise_player() {
  struct player * p = malloc(sizeof(struct player));
  p->username = malloc(sizeof(char) * 50);
  p->guess = malloc(sizeof(char) * 1000);
  p->username[0] = '\0';
  p->guess[0] = '\0';
  p->numguesses = 0;

  return p;
}

//splits string and gets ready for parsing, from a project last year so its large
char ** strsplit(const char * src, const char * delim) {

    char * pbuf = NULL;
    char * ptok = NULL;
    int count = 0;
    int srclen = 0;
    char ** pparr = NULL;

    srclen = strlen( src );

    pbuf = (char*) malloc( srclen + 1 );

    if( !pbuf )
        return NULL;

    strcpy( pbuf, src );

    ptok = strtok( pbuf, delim );

    while( ptok )
    {
        pparr = (char**) realloc( pparr, (count+1) * sizeof(char*) );
        *(pparr + count) = strdup(ptok);

        count++;
        ptok = strtok( NULL, delim );
    }

    pparr = (char**) realloc( pparr, (count+1) * sizeof(char*) );
    *(pparr + count) = NULL;

    free(pbuf);

    return pparr;
}

void strsplitfree(char ** str) {

    int i = 0;
    while(str[i])
        free(str[i++]);

    free(str);
}

bool http_post(char * buff, int sockfd, struct player ** players, int state, char* page, int n)
{
  // locate the username,
  //TODO: ensure username not overwritten
  // copied to another buffer using strcpy or strncpy to ensure that it will not be overwritten.
  char * guess;
  char * username;
  int username_length;
  long added_length;

  if (strstr(buff,"user=") != NULL) {
    username = strstr(buff, "user=") + 5;
    username_length = strlen(username);
  // the length needs to include the ", " before the username
    added_length = username_length + 2;
//    if(strlen(currentPlayer->username) < 2)
    players[sockfd]->username = strdup(username);

  }
  if (strstr(buff, "keyword") != NULL) {
    guess = strstr(buff, "keyword=") + 8;
    guess = strtok(guess, "&");
    //better2(players[sockfd]->guess, guess);
    players[sockfd]->guess = strdup(guess);
    added_length = strlen(guess) + 2;
  }

  // get the size of the file
  struct stat st;
  stat(page, &st);
  // increase file size to accommodate the username
  long size = st.st_size + added_length;
  n = sprintf(buff, HTTP_200_FORMAT, size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
      printf("\n\nclosed at line 208\n\n");

  }
  // read the content of the HTML file
  int filefd = open(page, O_RDONLY);
  n = read(filefd, buff, 2048);
  if (n < 0)
  {
      perror("read");
      close(filefd);
      return false;
  }
  close(filefd);
  // move the trailing part backward & update player

  if(strstr(buff, "user=") != NULL) {
    move_trailing(buff, username, added_length, size);
    }

  if (strstr(buff,"keyword=") != NULL) {
    move_trailing(buff, players[sockfd]->guess, added_length, size);
  }
  players[sockfd]->state = state;
  printf("PLayer %i: user: %s, Guess: %s state: %i \n", sockfd, players[sockfd]->username, players[sockfd]->guess, players[sockfd]->state);

  if (write(sockfd, buff, size) < 0)
  {
      perror("write");
      printf("\n\nclosed at line 242\n\n");
      return false;
  }
  return true;
}

bool http_get(char * buff, int sockfd, char* page, int n)
{
  printf("\n IN GET REQUEST \n");
    // get the size of the file
  //  printf("\n%s\n", curr);
    struct stat st;
    stat(page, &st);
    n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
  //  printf("\nSPRINTF: n = %d\n", n);
  //  printf("\n%s\n", buff);
    // send the header first
    if (write(sockfd, buff, n) < 0)
    {
        perror("write");
        printf("\n\nclosed at line 148\n\n");

        return false;
    }
    // send the file

    int filefd = open(page, O_RDONLY);
    printf("THE VALUE OF N IS: %d", n);
    do
    {
        n = sendfile(sockfd, filefd, NULL, 2048);
        printf("n = %d\n", n);
    }
    while (n > 0);
    if (n < 0)
    {
        perror("sendfile");
        close(filefd);
        printf("\n\nclosed at line 164\n\n");
        return false;
    }
    close(filefd);
    return true;
}

void better2(char *s1, char *s2)
{
  printf("ABOUT TO MALLOC STR");
        char *str = malloc(strlen(s1) + strlen(s2) + 1 * sizeof(*s1));
        if (str == NULL)
                abort();
        str[0] = '\0';
        printf("ABOUT TO STRCAT");
        strcat(str, s1);
        strcat(str, s2);
        printf("\nbasic: %s\n", str);

        free(str);
}

int main(int argc, char * argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s ip port\n", argv[0]);
        return 0;
    }

    // create TCP socket which only accept IPv4
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // reuse the socket if possible
    int const reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
    {
        printf("\n socket error for reuse: %d\n", reuse);
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // create and initialise address we will listen on
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // if ip parameter is not specified
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // bind address to socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen on the socket
    listen(sockfd, 5);

    // initialise an active file descriptors set
    fd_set masterfds;
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    // record the maximum socket number
    int maxfd = sockfd;
    printf("max socket num: %i", maxfd);

    struct player ** players;
    players = (struct player **)malloc(8 * sizeof(struct player *));
    for (int i = 0; i < 8; i++) {
      players[i] = initialise_player();
    }

    while (1)
    {
        // monitor file descriptors
        fd_set readfds = masterfds;
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // loop all possible descriptor
        for (int i = 0; i <= maxfd; ++i)
            // determine if the current file descriptor is active
            if (FD_ISSET(i, &readfds))
            {
                // create new socket if there is new incoming connection request
                if (i == sockfd)
                {
                    struct sockaddr_in cliaddr;
                    socklen_t clilen = sizeof(cliaddr);
                    int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);

                    if (newsockfd < 0)
                        perror("accept");
                    else
                    {
                        // add the socket to the set
                        FD_SET(newsockfd, &masterfds);
                        // update the maximum tracker
                        if (newsockfd > maxfd)
                            maxfd = newsockfd;
                        // print out the IP and the socket number
                        char ip[INET_ADDRSTRLEN];
                        printf(
                            "new connection from %s on socket %d\n",
                            // convert to human readable string
                            inet_ntop(cliaddr.sin_family, &cliaddr.sin_addr, ip, INET_ADDRSTRLEN),
                            newsockfd
                        );
                    }
                }
                // a request is sent from the client
                else if (!handle_http_request(i, players))
                {
                    close(i);
                  //  printf("\nsocket %i has been closed\n", i);
                    FD_CLR(i, &masterfds);
                }
            }
    }

    return 0;
}
