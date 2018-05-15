#include <stdio.h>
#define PORT "8080"
#define INITIAL_SESSIONS_LEN 2

#define stradd(buf, bufend, buflen, addition, addlen) \
    _stradd(&buf, &bufend, &buflen, addition, addlen)
//concatenates the string addition (with length addlen) into buf which has
//text length bufend and capacity buflen
void _stradd(char **buf, size_t *bufend, size_t *buflen,
        const char *addition, size_t addlen);

char *generate(char *query, char *path, int *final_length);

struct session {
    int wpieces[7]; /*list of positions of pieces (0 is unplayed, 1 is the first
                      spot a piece can go on the board, 14 is the end, 15 is
                      past the end, etc.)*/
    int bpieces[7];
    char set_up; //2 for set up, 1 for one player on, 0 for not set up
    int roll; //roll, between 0 and 4
    char turn; //'w' or 'b'
};

struct connection {
    struct session *session;
    char side; //'w' or 'b' for which player it is
};
