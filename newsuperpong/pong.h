/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Final Project
 * 
 * Group - 16
 * Pedro -------- 
 * Ricardo Barreto 
 * 
 * pong.h-New super pong header file
 * 
 * 
 ***************************************************************/

#include<netinet/in.h>
#include <stdbool.h>

#define PADLE_SIZE 2
#define STRING_SIZE 1000
#define WINDOW_SIZE 20
#define MESSAGE_WINDOW_X WINDOW_SIZE+10
#define MESSAGE_WINDOW_Y 12
#define MAX_PLAYERS 10

typedef struct ball_position_t{

    int x, y;
    int up_hor_down; //  -1 up, 0 horizontal, 1 down
    int left_ver_right; //  -1 left, 0 vertical,1 right
    char c;
} ball_position_t;

typedef struct paddle_position_t{

    int x, y;
    int length;
} paddle_position_t;

typedef struct paddle_message{

	int direction;
    int player_id;
} paddle_message;

typedef struct paddle{

    int player_id;
    int score;
    paddle_position_t position;
} paddle;

typedef struct board_message{

	ball_position_t ball;
	paddle player_paddle[MAX_PLAYERS];
    int num_player;
    int player_id;
    bool status; //true if connect is valid, false otherwise
} board_message;

typedef struct player{  

    int sock_fd;
    pthread_t thread;
    paddle player_paddle;
} player;

typedef struct node{

	player player_info;
	struct node *next;
} node;

