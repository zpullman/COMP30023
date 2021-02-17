#ifndef SERVER
#define SERVER

static char const * const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char const * const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_400_LENGTH = 47;
static char const * const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;

#define INTRO    1
#define START    2
#define TURN     3
#define ACCEPT   4
#define DISCARD  5
#define ENDGAME  6
#define GAMEOVER 7
#define WINNER   8

struct player;

char ** strsplit(const char * src, const char * delim);
void strsplitfree(char ** str);
bool http_post(char * buff, int sockfd, struct player ** players, int state, char* page, int n);
bool http_get(char * buff, int sockfd, char* page, int n);
bool player_ready(struct player ** players, int sockfd);
bool player_quit(struct player ** players, int sockfd);
bool guess_match(int sockfd, char * guess, struct player ** players);
void reset_game(struct player ** players);
void freeplayers(struct player ** players);
void generate_html(char * text, int rnd, int n);
bool player_win(struct player ** players, int sockfd);
void change_round(struct player ** players);

#endif
