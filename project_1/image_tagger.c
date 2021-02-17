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
#include "server.h"

//player struct to handle the players
struct player
{
  char * username;
  char * guess;
  int state;
  int numguesses;
  int round;
};

// represents the types of method
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;

/* function gets the buffer and checks what state is should go into, then handles
** the request accordingly. typically generate a new html and send as get request.
*/
static bool handle_http_request(int sockfd, struct player ** players)
{
    // try to read the request
    char buff[2049];
    int n = read(sockfd, buff, 2049);
    if (n <= 0)
    {
      if(n < 0)
        perror("read");
      else {
        return false;
      }
    }

    // terminate the string
    buff[n] = 0;
    char * curr = buff;
    int state = 0;
    // parse the method
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
        ++curr;
    // assume the only valid request URI is "/" but it can be modified to accept more files
    if (*curr == ' ') {
      if (method == GET)
      {
        page = "1_intro.html";
        state = INTRO;
      } else if (method == POST)
      {
        page = "2_start.html";
        state = START;
        if (strstr(curr, "quit") != NULL)
        {
            page = "7_gameover.html";
            state = GAMEOVER;
            method = GET;
        }
      }
    } else if (*curr == 's'){
        if (method == GET)
         {
          generate_html(players[sockfd]->guess, players[sockfd]->round, TURN);
          page = "turn.html";
          state = TURN;
        } else if (method == POST)
        {
            if (strstr(curr, "quit") != NULL)
            {
              page = "7_gameover.html";
              state = GAMEOVER;
              method = GET;
            }
            if (strstr(curr, "guess") != NULL && player_win(players, sockfd))
            {
              page = "6_endgame.html";
              state = ENDGAME;
              method = GET;

          } else if (strstr(curr, "guess") != NULL && !player_ready(players, sockfd))
          {
              page = "5_discarded.html";
              state = DISCARD;
              method = GET;
          } else if (strstr(curr, "guess") != NULL && player_ready(players, sockfd))
          {
            page = "4_accepted.html";
            state = ACCEPT;
          }
        }
        if(player_quit(players, sockfd))
        {
          page = "7_gameover.html";
          state = GAMEOVER;
          method = GET;
        }
      }

    players[sockfd]->state = state;

    if (*curr == ' ' || *curr == 's') {
      if (method == GET)
        {
          if(!http_get(buff, sockfd, page, n))
            return false;
        }
        else if (method == POST)
        {
          if(!http_post(buff, sockfd, players, state, page, n))
            return false;

        } else
          // never used, just for completeness
          fprintf(stderr, "no other methods supported");
          // send 404, used
    } else if (write(sockfd, HTTP_404, HTTP_404_LENGTH) < 0)
        {
            perror("write");
            return false;
        }
    return true;
}

//checks for player ready
bool player_ready(struct player ** players, int sockfd)
{
  int i = 0;
  while(i < 8) {
    if(i == sockfd) {
      i++;
    } else {
        if(players[i]->state == TURN || players[i]->state == DISCARD || players[i]->state == ACCEPT) {
          return true;
        }
      i++;
    }
  }
  return false;
}

//checks for player Quit
bool player_quit(struct player ** players, int sockfd)
{
  int i = 0;
  while(i < 8) {
        if(players[i]->state == GAMEOVER)
          return true;
      i++;
  }
  return false;
}

//checks for if a player has won
bool player_win(struct player ** players, int sockfd)
{
  int i = 0;
  while(i < 8) {
        if(players[i]->state == WINNER)
          return true;
      i++;
  }
  return false;
}

struct player * initialise_player()
{
  struct player * p = malloc(sizeof(struct player));
  p->username = malloc(sizeof(char) * 50);
  p->guess = malloc(sizeof(char) * 50);
  p->username[0] = '\0';
  p->guess[0] = '\0';
  p->state = 0;
  p->numguesses = 0;
  p->round = 1;

  return p;
}

//splits string and gets ready for parsing
char ** strsplit(const char * src, const char * delim)
{
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

void strsplitfree(char ** str)
{
    int i = 0;
    while(str[i])
        free(str[i++]);

    free(str);
}
//handle post request
bool http_post(char * buff, int sockfd, struct player ** players, int state, char* page, int n)
{
  char * guess;
  char * username;
  long added_length;

  if (strstr(buff,"user=") != NULL)
  {
    username = strstr(buff, "user=") + 5;
    added_length = strlen(username);
    players[sockfd]->username = strdup(username);
    generate_html(players[sockfd]->username, players[sockfd]->round, START);
    page = "turn.html";
    return http_get(buff,sockfd,page,n);
  }
  //copy in the guesses if accepted or discard them
  if (strstr(buff, "keyword") != NULL && players[sockfd]->state == ACCEPT)
  {
    guess = strstr(buff, "keyword=") + 8;
    guess = strtok(guess, "&");
    if(guess_match(sockfd, guess, players)) {
      players[sockfd]->state = WINNER;
      page = "6_endgame.html";
      reset_game(players);
      change_round(players);
      return http_get(buff, sockfd, page, n);
    }

    char * copy;
    if( (copy = malloc(strlen(players[sockfd]->guess)+strlen(guess)+2)) != NULL)
    {
      copy[0] = '\0';
      strcat(copy,players[sockfd]->guess);
      if(players[sockfd]->guess[0] != '\0')
        strcat(copy,",");
      strcat(copy,guess);
      players[sockfd]->guess = realloc(players[sockfd]->guess, strlen(copy)+1);
      players[sockfd]->guess = strdup(copy);
      players[sockfd]->numguesses++;
      free(copy);
      added_length = strlen(players[sockfd]->guess);
      generate_html(players[sockfd]->guess, players[sockfd]->round, ACCEPT);
      page = "turn.html";

      return http_get(buff, sockfd, page, n);
    }
  } else if (strstr(buff, "keyword") != NULL && players[sockfd]->state != ACCEPT) {
      guess = strstr(buff, "keyword=") + 8;
      guess = strtok(guess, "&");
      added_length = strlen(guess);
  }
  // get the size of the file
  struct stat st;
  stat(page, &st);

  long size = st.st_size + added_length;
  n = sprintf(buff, HTTP_200_FORMAT, size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
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

  if (write(sockfd, buff, size) < 0)
  {
      perror("write");
      return false;
  }
  return true;
}
//llllll

void reset_game(struct player ** players)
{
  for (int i = 0; i < 8; i++) {
    /*
    free(players[i]->guess);
    if((players[i]->guess = malloc(sizeof(char) * 50)) != NULL)
      players[i]->guess[0] = '\0';
      */
      memset(players[i]->guess,0,strlen(players[i]->guess));
  }
}

void freeplayers(struct player ** players)
{
  for (int i = 0; i < 8; i++) {
    free(players[i]->guess);
    free(players[i]->username);
  }
  free(players);
}
//handle get Request
bool http_get(char * buff, int sockfd, char* page, int n)
{
    // get the size of the file
    struct stat st;
    stat(page, &st);
    n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
    // send the header first
    if (write(sockfd, buff, n) < 0)
    {
        perror("write");
        return false;
    }
    // send the file
    int filefd = open(page, O_RDONLY);
    do
    {
        n = sendfile(sockfd, filefd, NULL, 2048);
    }
    while (n > 0);
    if (n < 0)
    {
        perror("sendfile");
        close(filefd);
        return false;
    }
    close(filefd);
    return true;
}

// checks to see if theres a match in GUESSES
bool guess_match(int sockfd, char * guess, struct player ** players)
{
  int i = 0;
  while(i < 8) {
    if(i == sockfd)
      i++;
    else {
      char ** pp;
      pp = strsplit(players[i]->guess,",");

    for (int n = 0; n < players[i]->numguesses; n++) {
      if(strcmp(guess,pp[n]) == 0) {
        strsplitfree(pp);
        return true;
      }
    }
    i++;
    }
  }
  return false;
}

//makes html depending on state of the Game
void generate_html(char * text, int rnd, int n)
{
  char* img;
  char* header;
  if(n == START)
  {
    img = "https://swift.rc.nectar.org.au/v1/AUTH_eab314456b624071ac5aecd721b977f0/comp30023-project/image-3.jpg";
    header = "Image Tagger Game";
  } else if (n == ACCEPT || n == TURN)
  {
    if(n == ACCEPT)
      header = "Keyword Accepted! Keep trying more.";
    else
      header = "You are ready now!";
    if (rnd%2 == 1) {
      img = "https://swift.rc.nectar.org.au/v1/AUTH_eab314456b624071ac5aecd721b977f0/comp30023-project/image-2.jpg";
    } else {
      img = "https://swift.rc.nectar.org.au/v1/AUTH_eab314456b624071ac5aecd721b977f0/comp30023-project/image-1.jpg";
    }
  }
  FILE      *ptrFile = fopen( "turn.html", "w");

  fprintf( ptrFile, "<!DOCTYPE html>\n");
  fprintf( ptrFile, "<html>\n");
  fprintf( ptrFile, "<head>\n");
  fprintf( ptrFile, "</head>\n");
  fprintf( ptrFile, "<body>\n\n");
  fprintf( ptrFile, "<h2>%s</h2>\n\n", header);
  fprintf( ptrFile, "<img src=\"%s\" alt=\"HTML5 Icon\" style=\"width:700px;height:400px;\">\n\n", img);

  if (n == TURN || n == ACCEPT)
    fprintf( ptrFile, "<p>Rule: Try to guess the above image by typing a keyword which describes it:</p>\n\n");

  fprintf( ptrFile, "<p> %s </p>\n\n", text);

  if (n == TURN || n == ACCEPT)
  {
    fprintf( ptrFile, "<form method=\"POST\">\n");
    fprintf( ptrFile, "\tKeyword: <input type=\"text\" name=\"keyword\" />\n");
    fprintf( ptrFile, "\t<input type=\"submit\" class=\"button\" name=\"guess\" value=\"Guess\" />\n");
    fprintf( ptrFile, "</form>\n\n");
  } else if (n == START)
  {
    fprintf( ptrFile, "<form method=\"GET\">\n");
    fprintf( ptrFile, "\t<input type=\"submit\" class=\"button\" name=\"start\" value=\"Start\" />\n");
    fprintf( ptrFile, "</form>\n\n");
  }

  fprintf( ptrFile, "<form method=\"POST\">\n");
  fprintf( ptrFile, "\t<input type=\"submit\" class=\"button\" name=\"quit\" value=\"Quit\" />\n");
  fprintf( ptrFile, "</form>\n\n");

  fprintf( ptrFile, "</body>\n");
  fprintf( ptrFile, "</html>");

  fclose( ptrFile );
}

void change_round(struct player ** players)
{
  int i = 0;
  while(i < 8) {
      players[i]->round++;
      i++;
  }
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
    printf("image_tagger server is now running at IP: %s on port %s\n", argv[1], argv[2]);

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
                    }
                }
                // a request is sent from the client
                else if (!handle_http_request(i, players))
                {
                    close(i);
                    FD_CLR(i, &masterfds);
                }
            }
    }

    return 0;
}
