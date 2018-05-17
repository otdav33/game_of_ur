#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "cwebserv.h"
#include "game_of_ur.h"

struct session *global_sessions;
struct connection *global_connections;
int global_sessions_len;

void _stradd(char **buf, size_t *bufend, size_t *buflen,
        const char *addition, size_t addlen) {
    // concatenates the string addition (with length addlen) into *buf
    // buf has text length *bufend and capacity *buflen
    if (addlen + *bufend > *buflen)
        *buf = realloc(*buf, *buflen += addlen + BUFSTEP);
    strlcpy(*buf + *bufend, addition, *buflen - *bufend);
    *bufend += addlen - 1;
}

struct connection *get_connection() {
    int i, j, k;
    struct connection *retval;
    for (i = 0; i < (global_sessions_len * 2) &&
            global_connections[i].side != 0; i++)
        ; //go to end of list of connections, or find an empty one
    if (i >= 2 * global_sessions_len) {
        //if you're at the end, make a fresh one
        j = global_sessions_len;
        global_sessions_len++;
        global_sessions = reallocarray(global_sessions,
            global_sessions_len, sizeof(*global_sessions));
        global_connections = reallocarray(global_connections,
            2 * global_sessions_len, sizeof(*global_connections));
        bzero(global_connections + (global_sessions_len * 2) - 1, 
                sizeof(global_connections[(global_sessions_len * 2) - 1]));
        bzero(global_connections + (global_sessions_len * 2) - 2, 
                sizeof(global_connections[(global_sessions_len * 2) - 1]));
        bzero(global_sessions + global_sessions_len - 1, 
                sizeof(global_sessions[global_sessions_len - 1]));
    } else {
        for (j = 0; j < global_sessions_len && global_sessions[j].set_up == 2;
                j++)
            ; //find an empty session
        assert(j < global_sessions_len);
    }
    retval = global_connections + i;
    retval->id = rand();
    retval->session = global_sessions + j;
    if (global_sessions[j].set_up == 0) { 
        retval->side = 'w';
        //simulate dice roll between 0 and 4, usually 2
        int r = rand();
        retval->session->roll = !!(r & 1) + !!(r & 2) + !!(r & 4) + !!(r & 8);
#ifdef DEBUG
        printf("First roll is %i\n", retval->session->roll);
#endif
        for (k = 0; k < 7; k++)
            global_sessions[j].wpieces[k] = global_sessions[j].bpieces[k] = 0;
        global_sessions[j].turn = 'w';
    } else {
        retval->side = 'b';
    }
    global_sessions[j].set_up++;
    return retval;
}

int how_many_at(int pieces[7], int position) {
    /*How many pieces are there at a position (defined in game_of_ur.h under
    struct session)*/
    int i, total = 0;
    for (i = 0; i < 7; i++)
        if (pieces[i] == position)
            total++;
    return total;
}

int gridpos_to_pos(struct session *session, int p) {
    /*convert position on the board to position as described in the struct 
     *session declaration. -1 for no position/error*/
    if (p == 0 || p == 5 || p == 10 || p == 15 || p == 20 || p == 23 || p == 26
            || p == 31) {
        //white on the left edge
        int slot; //position from top down
        switch (p) {
            case 23:
                slot = 5;
                break;
            case 26:
                slot = 6;
                break;
            case 31:
                slot = 7;
                break;
            default:
                slot = p / 5;
                break;
        }

        if (how_many_at(session->wpieces, 0) > slot) {
            return 0;
        } else if (how_many_at(session->wpieces, 15) > 7 - slot) {
            return 15;
        } else {
            return -1;
        }
    } else if (p == 4 || p == 9 || p == 14 || p == 19 || p == 22 || p == 25
            || p == 30 || p == 35) {
        //black on the right edge
        int slot; //position from top down
        switch (p) {
            case 22:
                slot = 4;
                break;
            case 25:
                slot = 5;
                break;
            case 30:
                slot = 6;
                break;
            case 35:
                slot = 7;
                break;
            default:
                slot = (p - 4) / 5;
                break;
        }

        if (how_many_at(session->bpieces, 0) > slot) {
            return 0;
        } else if (how_many_at(session->bpieces, 15) > 7 - slot) {
            return 15;
        } else {
            return -1;
        }
    } else if (p == 1 || p == 6 || p == 11 || p == 16) {
        //first 4 white spaces
        return 4 - ((p - 1) / 5);
    } else if (p == 3 || p == 8 || p == 13 || p == 18) {
        //first 4 black spaces
        return 4 - ((p - 3) / 5);
    } else if (p == 2 || p == 7 || p == 12 || p == 17 || p == 21 || p == 24
            || p == 28 || p == 33) {
        //middle stretch
        int slot; //position from top down
        if (p == 2) {
            slot = 5;
        } else if (p == 7) {
            slot = 6;
        } else if (p == 12) {
            slot = 7;
        } else if (p == 17) {
            slot = 8;
        } else if (p == 21) {
            slot = 9;
        } else if (p == 24) {
            slot = 10;
        } else if (p == 28) {
            slot = 11;
        } else {
            slot = 12;
        }

        return slot;
    } else if (p == 27 || p == 29 || p == 32 || p == 34) {
        return (p == 27 || p == 29) ? 14 : 13;
    } else {
        return -1;
    }
}

char *tile_for_gridpos(struct session *session, int p, char *w, char *b,
        char *floret) {
    /*return what is supposed to be in the tile at the position p (not the 0-15
     * one, but relative to the %s's in char* template) w and b are what are
     * supposed to be displayed, and floret for a floret*/
    if (p == 0 || p == 5 || p == 10 || p == 15 || p == 20 || p == 23 || p == 26
            || p == 31) {
        //white on the left edge
        if (gridpos_to_pos(session, p) == -1) //no piece
            return "";
        return w;
    } else if (p == 4 || p == 9 || p == 14 || p == 19 || p == 22 || p == 25
            || p == 30 || p == 35) {
        //black on the right edge
        if (gridpos_to_pos(session, p) == -1) //no piece
            return "";
        return b;
    } else if (p == 1 || p == 6 || p == 11 || p == 16) {
        //first 4 white spaces
        int slot = 4 - ((p - 1) / 5);
        if (how_many_at(session->wpieces, slot))
            return w;
        return (slot == 4) ? floret : "";
    } else if (p == 3 || p == 8 || p == 13 || p == 18) {
        //first 4 black spaces
        int slot = 4 - ((p - 3) / 5);
        if (how_many_at(session->bpieces, slot))
            return b;
        return (slot == 4) ? floret : "";
    } else if (p == 2 || p == 7 || p == 12 || p == 17 || p == 21 || p == 24
            || p == 28 || p == 33) {
        //middle stretch
        int slot = gridpos_to_pos(session, p); //position from top down

        if (how_many_at(session->wpieces, slot)) {
            return w;
        } else if (how_many_at(session->bpieces, slot)) {
            return b;
        } else {
            return (slot == 8) ? floret : "";
        }
    } else if (p == 27 || p == 29 || p == 32 || p == 34) {
        int slot = (p == 27 || p == 29) ? 14 : 13;
        if (p == 27 || p == 32) {
            //white side
            if (how_many_at(session->wpieces, slot))
                return w;
            else
                return (p == 27) ? floret : "";
        } else {
            //black side
            if (how_many_at(session->bpieces, slot))
                return b;
            else
                return (p == 29) ? floret : "";
        }
    } else {
        return "";
    }
}

char *generate(char *query, char *path, int *final_length) {
    /*will return a malloc'd string (that will need to be freed later) that
      has html generated for either the picross editor or player (depending on
      the path).
      final_length will be set to the length of the returned string.*/
    char template[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"<style>\n"
"body {\n"
"  background: #8b5a2b;\n"
"}\n"
"a {\n"
"  text-decoration: none;\n"
"  color: black;\n"
"}\n"
"table {\n"
"  border-collapse: collapse;\n"
"  margin: 0px auto;\n"
"}\n"
"td {\n"
"  width: 64px;\n"
"  height: 64px;\n"
"  background: #ffecb3;\n"
"  border: 1px solid #000000;\n"
"}\n"
".noborder {\n"
"  border-style: none;\n"
"  background: #8b5a2b;\n"
"}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"  <h3>You are playing %s</h3>\n"
"  <h3>It is%s your turn.</h3>\n"
"  <h3>Current dice roll is %i.</h3>\n"
"  <form method=\"post\" action=\"/\">\n"
"  <input type=\"hidden\" name=\"connection\" value=\"%i\">\n"
"  <h3>Click <input type=\"submit\" value=\"here\"> to check for "
"updates.</h3>\n"
"  </form>\n"
"  <table>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td class=\"noborder\">%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"      <td class=\"noborder\"></td>\n"
"      <td class=\"noborder\">%s</td>\n"
"    </tr>\n"
"  </table>\n"
"</body>\n"
"</html>\n";
    char end_bit[] = "</body></html>\n\n";
    char position_invalid[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>You may not move there.</h3>\n"
"</body>\n"
"</html>\n";
    char position_error[] = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>Error: position is inputted incorrectly.</h3>";
"</body>\n"
"</html>\n";
char too_low_error[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>Error: position is inputted incorrectly. Please set it to more than or "
"equal to 0, but less than 20.</h3>\n";
"</body>\n"
"</html>\n";
    char connection_incorrect[] = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>Error: connection is inputted incorrectly.</h3>";
"</body>\n"
"</html>\n";
    char internal_error[] = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>Error<h3>\n";
    char home_screen[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>If you haven't played before, you can watch "
"<a href=\"https://www.youtube.com/watch?v=WZskjLq040I\">this</a> youtube "
"video or read these instructions I wrote:</h3>\n"
"<p>At the beginning of your turn, you roll four dice that each have a "
"50/50 chance of rolling a one or a zero (essentially coins). You add up those "
"rolls, and that number is how far you get to move your piece. (This "
"implementation of the game rolls for you.)</p>\n"
"<p>You move one of your seven pieces that number of spaces by clicking on it. "
"If you land on a floret, you get to roll again and move again with any "
"piece.</p>\n"
"<p>You may not put one of your pieces in the same square as another of your "
"pieces. If a piece lands on the opponent's piece, the opponent's piece is "
"sent back to the beginning.</p>"
"<p>You start at the square three squares below the square on the upper corner "
"on your side of the board, move up, then to the middle column, then down to "
"the bottom, then back toward your side, then up. Once you get your piece "
"past the square above the square in the bottom corner, it has gone to the end."
"</p>\n"
"<p>The first person to get all seven of their pieces to the end wins.</p>\n"
"<br><br>\n"
"<h3>Click <a href=\"/make_match\">here</a> to start a game "
    "with a random person.</h3>\n"
"</body>\n"
"</html>\n";
    char waiting[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<meta http-equiv=\"refresh\" content=\"0; url=/?connection=%i\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>Waiting for a match...</h3>\n"
"<p>If the page does not automatically redirect you, click <a href=\""
"/?connection=%i\">here</a>.</p>\n"
"</body>\n"
"</html>\n";
    char black_won[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>Black won.</h3>\n"
"</body>\n"
"</html>\n";
    char white_won[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<style>\n"
"body {\n"
"  background: #ffecb3;\n"
"}\n"
"</style>\n"
"<meta charset=\"utf-8\" />\n"
"<title>Game of Ur</title>\n"
"</head>\n"
"<body>\n"
"<h3>White won.</h3>\n"
"</body>\n"
"</html>\n";
    char floret[] = "<img src=\"http://dread.life/pics/game_of_ur/floret.png\""
        " alt=\"floret\"/>";
    char white[] = "<form method=\"post\" action=\"/\">"
        "<input type=\"hidden\" name=\"connection\" value=\"%i\">"
        "<input type=\"hidden\" name=\"p\" value=\"%i\">"
        "<input type=\"image\" alt=\"white\""
        " src=\"http://dread.life/pics/game_of_ur/white.png\">"
        "</form>";
    char black[] = "<form method=\"post\" action=\"/\">"
        "<input type=\"hidden\" name=\"connection\" value=\"%i\">"
        "<input type=\"hidden\" name=\"p\" value=\"%i\">"
        "<input type=\"image\" alt=\"black\""
        " src=\"http://dread.life/pics/game_of_ur/black.png\">"
        "</form>";
    char white_nolink[] = "<img"
        " src=\"http://dread.life/pics/game_of_ur/white.png\" alt=\"white\"/>";
    char black_nolink[] = "<img"
        " src=\"http://dread.life/pics/game_of_ur/black.png\" alt=\"black\"/>";
    char make_match[] = "make_match";
    size_t bufend = 0, buflen = BUFSTART;
    char *buf = malloc(buflen);
    int position, i, j;
    struct connection *conn;

    char *pth = path;
    while (*pth == '/')
        pth++;
#ifdef DEBUG
    printf("pth is %s\n", pth);
#endif

    if (!strcmp(make_match, pth)) {
        //we need to set up a new match
        conn = get_connection();
        position = -1;
        if (conn->side == 'w') {
            size_t tsize = sizeof(waiting) + 2 * 14;
            char *t = alloca(tsize);
            //we are the first to connect
            int errcode = snprintf(t, tsize, waiting, conn->id, conn->id);
            if (errcode < 0 || errcode > tsize) {
                stradd(buf, bufend, buflen, internal_error,
                        sizeof(internal_error));
                printf("error %i for snprintf #0\n", errcode);
            } else {
                stradd(buf, bufend, buflen, t, errcode + 1);
            }
            printf("session is %i\n", conn->session);
            *final_length = bufend;
            return realloc(buf, bufend + 1);
        }
    } else {
        //get info from existing parameters
 
        //parse query string
        char *tofree, *q, *tok, *posstr = NULL, *constr = NULL;
        if (query != 0) { //If there is a QUERY_STRING env. variable
            tofree = q = strdup(query);
            while ((tok = strsep(&q, "&"))) {
                if (!strncmp(tok, "p=", 2))
                    posstr = strdup(tok + 2);
                if (!strncmp(tok, "connection=", 11))
                    constr = strdup(tok + 11);
            }
            free(tofree);
        }

        //make sure there are no missing fields
        if (!constr) {
            stradd(buf, bufend, buflen, home_screen, sizeof(home_screen));
            *final_length = bufend;
            return realloc(buf, bufend + 1);
        } else {
            //Convert position to an int
            char *endptr = NULL;
            errno = 0;
            int conno = strtol(constr, &endptr, 10);
            for (i = 0; i < global_sessions_len * 2; i++) {
#ifdef DEBUG
                printf("gcid[%i] = %i, conno = %i\n", i,
                        global_connections[i].id, conno);
#endif
                if (global_connections[i].id == conno) {
                    conn = global_connections + i;
                    break;
                }
            }
#ifdef DEBUG
            printf("constr errno is %i\n", errno);
#endif
            if (errno || i == global_sessions_len * 2 ||
                    constr[0] == '\0' || *endptr != '\0'
                    || (conn->side != 'w' && conn->side != 'b')) {
                //exit gracefully on error
                if (errno) {
                    printf("position error: %i\n", errno);
                }
                stradd(buf, bufend, buflen, connection_incorrect,
                        sizeof(connection_incorrect));
                *final_length = bufend;
                return realloc(buf, bufend + 1);
            }
        }
        if (!posstr) {
            position = -1;
        } else {
            //Convert position to an int
            char *endptr = NULL;
            errno = 0;
            position = strtol(posstr, &endptr, 10);
#ifdef DEBUG
            printf("position errno is %i\n", errno);
#endif
            if (errno || position < 0 || position > 6 || posstr[0] == '\0' ||
                    *endptr != '\0') {
                //exit gracefully on error
                if (errno) {
                    printf("position error: %i\n", errno);
                    stradd(buf, bufend, buflen, position_error,
                            sizeof(position_error));
                } else {
                    stradd(buf, bufend, buflen, too_low_error,
                            sizeof(too_low_error));
                }
                *final_length = bufend;
                return realloc(buf, bufend + 1);
            }
            //actually make the move
            if (conn->side == conn->session->turn) {
                int i;
                int (*pieces)[7], (*otherpieces)[7];
                if (conn->side == 'w') {
                    pieces = &conn->session->wpieces;
                    otherpieces = &conn->session->bpieces;
                } else {
                    pieces = &conn->session->bpieces;
                    otherpieces = &conn->session->wpieces;
                }
                int s = (*pieces)[position] + conn->session->roll;
                /*You can't jump on top of your own piece unless it's on the
                 *edge. You can capture pieces and send them to the beginning,
                 *but only if they are in the middle strip (positions 5-12).*/
                for (i = 0; i < 7; i++) {
                    if ((s == (*pieces)[i] && conn->session->roll != 0
                                && s < 15) || (*pieces)[position] == 15) {
                        stradd(buf, bufend, buflen, position_invalid,
                                sizeof(position_invalid));
                        *final_length = bufend;
                        return realloc(buf, bufend + 1);
                    }
                    if (s == (*otherpieces)[i] && s > 4 && s < 13) {
                        (*otherpieces)[i] = 0;
                    }
                }
                (*pieces)[position] = s > 15 ? 15 : s;
                if (s != 4 && s != 8 && s != 14) {//not on a floret
                    conn->session->turn =
                        (conn->session->turn == 'w') ? 'b' : 'w';
                }
                //simulate dice roll between 0 and 4, usually 2
                int r = rand();
                conn->session->roll =
                    !!(r & 1) + !!(r & 2) + !!(r & 4) + !!(r & 8);
#ifdef DEBUG
                printf("Subsequent roll is %i", conn->session->roll);
#endif
            }
        }
    }

    int bwon = 1;
    for (i = 0; i < 7; i++)
        if (conn->session->bpieces[i] != 15)
            bwon = 0;
    if (bwon) {
        stradd(buf, bufend, buflen, black_won, sizeof(black_won));
        *final_length = bufend;
        return realloc(buf, bufend + 1);
    }
    int wwon = 1;
    for (i = 0; i < 7; i++)
        if (conn->session->wpieces[i] != 15)
            wwon = 0;
    if (wwon) {
        stradd(buf, bufend, buflen, white_won, sizeof(white_won));
        *final_length = bufend;
        return realloc(buf, bufend + 1);
    }

    //make board
    size_t bsize = sizeof(template) + 5 * sizeof(floret) + 7 * sizeof(black) +
        7 * sizeof(white) + 28 * 12 /* 28 * 12 = potential size of all %i's */;
    char *board_template = malloc(bsize);
    //we are the first to connect
    int errcode;
    char *color, *w, *b, *ourcolor, *is_turn;
    int (*ourpieces)[7];
    if (conn->side == 'w') {
        color = "white";
        if (conn->session->turn == 'w') {
            w = white;
            is_turn = "";
        } else {
            w = white_nolink;
            is_turn = " NOT";
        }
        b = black_nolink;
        ourcolor = w;
        ourpieces = &conn->session->wpieces;
    } else {
        color = "black";
        w = white_nolink;
        if (conn->session->turn == 'b') {
            b = black;
            is_turn = "";
        } else {
            b = black_nolink;
            is_turn = " NOT";
        }
        ourcolor = b;
        ourpieces = &conn->session->bpieces;
    }
    int pindexes[7]; /*numbers that will show up in the link for each piece the
                       player owns, corresponding to the index of wpieces or
                       bpieces*/
    int pindex = 0; //index for pindexes
    char *tfgp[36];
    for (i = 0; i < 36; i++) {
        tfgp[i] = tile_for_gridpos(conn->session, i, w, b, floret);
        if (tfgp[i] == ourcolor) {
            //current piece is our color, so make the index for it
            int curpos = gridpos_to_pos(conn->session, i);
            for (j = 0; j < 7; j++) {
                if ((*ourpieces)[j] == curpos) {
                    pindexes[pindex++] = j;
                    break;
                }
            }
        }
    }

    errcode = snprintf(board_template, bsize, template,
            color, is_turn, conn->session->roll, conn->id,
            tfgp[0 ], tfgp[1 ], tfgp[2 ], tfgp[3 ], tfgp[4 ], tfgp[5 ],
            tfgp[6 ], tfgp[7 ], tfgp[8 ], tfgp[9 ], tfgp[10], tfgp[11],
            tfgp[12], tfgp[13], tfgp[14], tfgp[15], tfgp[16], tfgp[17],
            tfgp[18], tfgp[19], tfgp[20], tfgp[21], tfgp[22], tfgp[23],
            tfgp[24], tfgp[25], tfgp[26], tfgp[27], tfgp[28], tfgp[29],
            tfgp[30], tfgp[31], tfgp[32], tfgp[33], tfgp[34], tfgp[35]);
    if (errcode < 0 || errcode > bsize) {
        stradd(buf, bufend, buflen, internal_error, sizeof(internal_error));
        printf("error %i for snprintf #3\n", errcode);
    } else if (conn->side == conn->session->turn) {
        char *board = malloc(bsize);
        int errcode = snprintf(board, bsize, board_template,
                conn->id, pindexes[0],
                conn->id, pindexes[1],
                conn->id, pindexes[2],
                conn->id, pindexes[3],
                conn->id, pindexes[4],
                conn->id, pindexes[5],
                conn->id, pindexes[6]);
        if (errcode < 0 || errcode > bsize) {
            stradd(buf, bufend, buflen, internal_error, sizeof(internal_error));
            printf("error %i for snprintf #4\n", errcode);
        } else {
            stradd(buf, bufend, buflen, board, errcode + 1);
        }
        free(board);
    } else {
        stradd(buf, bufend, buflen, board_template, errcode + 1);
    }
    free(board_template);
    *final_length = bufend;
    return realloc(buf, bufend + 1);
}

int main(int argc, char **argv) {
    global_sessions_len = INITIAL_SESSIONS_LEN;
    global_sessions = calloc(global_sessions_len, sizeof(*global_sessions));
    global_connections = calloc(2 * global_sessions_len,
            sizeof(*global_connections));
    srand(global_sessions);
    cwebserv(argc > 1 ? argv[1] : PORT, generate);
    free(global_sessions);
    free(global_connections);
}
