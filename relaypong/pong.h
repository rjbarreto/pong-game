/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Intermediate Project
 * 
 * Pedro -------- 
 * Ricardo Barreto 
 * 
 * pong.h-Relay pong header file
 * 
 ***************************************************************/

#include<netinet/in.h>

#define PADLE_SIZE 2
#define WINDOW_SIZE 20
#define MESSAGE_WINDOW_X WINDOW_SIZE+10
#define MESSAGE_WINDOW_Y 5

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

typedef struct message{

    int msg_type;  // 0->Connect, 1->Release_ball, 2->Send_ball 3->Move_ball 4->Disconnect
	ball_position_t ball;
	paddle_position_t paddle;
    int num_player;
} message;

typedef struct player{  

	int player_id;
    struct sockaddr_in addr; 
} player;

typedef struct node{

	player player_info;
	struct node *next;
} node;
