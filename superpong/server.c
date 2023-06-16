/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Intermediate Project
 * 
 * Pedro -------- 
 * Ricardo Barreto 
 * 
 * server.c-Super pong server
 * 
 ***************************************************************/

#include "pong.h"
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdbool.h>

#define SOCK_PORT 5000    //port number

int sock_fd;
node *player_info_list;   //list of all current player
ball_position_t ball;     //ball


/**
 * Initializes the variable "ball" at a random position
 * 
 **/
void place_ball_random(ball_position_t * ball){

    ball->x = rand() % WINDOW_SIZE ;
    ball->y = rand() % WINDOW_SIZE ;
    ball->c = 'o';
    ball->up_hor_down = rand() % 3 -1;     // -1 up, 1 down
    ball->left_ver_right = rand() % 3 -1 ; // 0 vertical, -1 left, 1 right
}

/**
 * Checks if paddle move to coordinates "x" and "y" is valid
 * Only takes into account the other paddles and the ball
 * 
 **/
bool check_availability(int y, int x, int length, int my_id){

	node *aux=player_info_list;
	int aux_start_x, aux_end_x;

	int start_x = x - length;
	int end_x = x + length;
	
	if((ball.y==y)&&(ball.x>=start_x)&&(ball.x<=end_x)){//check sobreposition with ball
		
		while(aux!=NULL){

			if(aux->player_info.player_paddle.player_id==my_id){//increments score of current player

				(aux->player_info.player_paddle.score)++;
				break;
			}
			aux=aux->next;
		}
		return false;
	}

	aux=player_info_list;
	while(aux!=NULL){

		if(aux->player_info.player_paddle.player_id==my_id){aux=aux->next; continue;}//checks if both paddles are the same
		if(y!=aux->player_info.player_paddle.position.y){aux=aux->next; continue;}   //checks if both paddles are not at the same height

		aux_start_x = aux->player_info.player_paddle.position.x - aux->player_info.player_paddle.position.length;		
		aux_end_x = aux->player_info.player_paddle.position.x + aux->player_info.player_paddle.position.length;

		if((aux_end_x>=start_x)&&(aux_start_x<=end_x)){//sobreposition with paddle
			return false;
		}
		aux=aux->next;
	}
	return true;
}



/**
 * Initializes the variable "paddle" with length "length" at a specific position
 * 
 **/
void new_paddle (paddle_position_t * paddle, int length, int my_id){

	for(int i=0; i<WINDOW_SIZE-4; i++){
		if(check_availability(WINDOW_SIZE-2-i, WINDOW_SIZE/2, length, my_id)){
			paddle->x = WINDOW_SIZE/2;
			paddle->y = WINDOW_SIZE-2-i;
			paddle->length = length;
			break;
		}
	}
}

/**
 * Calculates the new coordinates of the paddle
 * 
 **/
void moove_paddle (paddle_position_t * paddle, int direction, int my_id){

    if (direction == KEY_UP){
        if ((paddle->y  != 1)&&(check_availability(paddle->y-1, paddle->x, paddle->length, my_id))){
            paddle->y --;
        }
    }
    if (direction == KEY_DOWN){
        if ((paddle->y  != WINDOW_SIZE-2)&&(check_availability(paddle->y+1, paddle->x, paddle->length, my_id))){
            paddle->y ++;
        }
    }
    if (direction == KEY_LEFT){
        if ((paddle->x - paddle->length != 1)&&(check_availability(paddle->y, paddle->x-1, paddle->length, my_id))){
            paddle->x --;
        }
    }
    if (direction == KEY_RIGHT)
        if ((paddle->x + paddle->length != WINDOW_SIZE-2)&&(check_availability(paddle->y, paddle->x+1, paddle->length, my_id))){
            paddle->x ++;
    }
}

/**
 * Calculates the ball bouncing in the paddle
 * 
 **/
void paddle_bounce(int current_y, int current_x, ball_position_t *ball){

	node *aux=player_info_list;
	int start_x, end_x;

	while(aux!=NULL){
		
		if(ball->y!=aux->player_info.player_paddle.position.y){aux=aux->next; continue;}//ball not at the same height as current paddle 

		start_x = aux->player_info.player_paddle.position.x - aux->player_info.player_paddle.position.length;		
		end_x = aux->player_info.player_paddle.position.x + aux->player_info.player_paddle.position.length;

		if((ball->x>=start_x)&&(ball->x<=end_x)){//sobreposition with paddle (ball bounce occurs)
			
			ball->x=current_x;
			ball->y=current_y;

			if(ball->up_hor_down==0){
				ball->left_ver_right *= -1;
				ball->up_hor_down = rand() % 3 -1; 
			}
			else{
				ball->up_hor_down *= -1;
        		ball->left_ver_right = rand() % 3 -1;
			}
			aux->player_info.player_paddle.score++; //increment player score

			break;
		}
		aux=aux->next;
	}
}

/**
 * Calculates the new coordinates of the ball
 * 
 **/
void moove_ball(ball_position_t * ball){

	int current_x=ball->x, current_y=ball->y;
    
	//bouncing on walls
    int next_x = ball->x + ball->left_ver_right;
    if( next_x == 0 || next_x == WINDOW_SIZE-1){
        ball->up_hor_down = rand() % 3 -1 ;
        ball->left_ver_right *= -1;
    }else{
		ball->x = next_x;
    }
   
    int next_y = ball->y + ball->up_hor_down;
    if( next_y == 0 || next_y == WINDOW_SIZE-1){
        ball->up_hor_down *= -1;
        ball->left_ver_right = rand() % 3 -1;
    }else{
		ball -> y = next_y;
    }

	//bouncing on paddle
	paddle_bounce(current_y, current_x, ball);
}

/**
 * Compares if socket addresses "addr1" and "addr2" match
 * 
 **/
bool compare_clients(struct sockaddr_in addr1, struct sockaddr_in addr2){

	if((addr1.sin_port==addr2.sin_port)&&(addr1.sin_addr.s_addr==addr2.sin_addr.s_addr)){
		
		return true;
	}
	return false;
}

/**
 * Creates a node of a new player with "address" addr and id "player_id"
 * 
 **/
node *create_player(struct sockaddr_in addr, int player_id){
	
	node *new_player=(node *)malloc(sizeof(node));
	
	new_player->player_info.addr=addr;
	new_player->player_info.player_paddle.player_id=player_id;
	new_player->player_info.player_paddle.score=0;
	new_paddle(&(new_player->player_info.player_paddle.position), PADLE_SIZE, player_id);

	new_player->next=NULL;
	
	return new_player;
}

/**
 * Inserts new player "new_player" at the head of the players' list
 * 
 **/
void insert_player(node *new_player){
	
	if(player_info_list!=NULL){
		
		new_player->next=player_info_list;
	}
	player_info_list=new_player;
}

/**
 * Removes from players' list, player with address "addr"
 * 
 **/
void disconnect_player(struct sockaddr_in addr){
	
	node *aux=player_info_list, *aux1;
	
	//finds element to be removed
	while(aux!=NULL){
		
		if(compare_clients(aux->player_info.addr,addr)){break;}
		aux1=aux;
		aux=aux->next;
	}
	
	if(aux==NULL){
		printf("Error: player not found at disconnect.\n");
		exit(0);
	}
	else if((aux==player_info_list)&&(aux->next==NULL)){ //remove only element of the list
		free(aux);
		player_info_list=NULL;
	}
	else if(aux==player_info_list){ //remove head
		player_info_list=aux->next;
		free(aux);
	}
	else if(aux->next==NULL){ //remove tail
		aux1->next=NULL;
		free(aux);
	}
	else{ //remove middle element
		aux1->next=aux->next;
		free(aux);
	}
}

/**
 * Print in the terminal information about a status message
 * 
 **/
void print_communication_status(struct sockaddr_in client_addr, int n_bytes, status_message m){

	char remote_addr_str[100];
	int remote_port = ntohs(client_addr.sin_port);
	if (inet_ntop(AF_INET, &client_addr.sin_addr, remote_addr_str, 100) == NULL){
		perror("converting remote addr: ");
	}
	printf("Received message of type %d with %d bytes from %s:%d\n", m.msg_type, n_bytes, remote_addr_str, remote_port);
}

/**
 * Print in the terminal information about a board message
 * 
 **/
void print_board_communication(struct sockaddr_in client_addr, int n_bytes, bool valid){

	char remote_addr_str[100];
	int remote_port = ntohs(client_addr.sin_port);
	if (inet_ntop(AF_INET, &client_addr.sin_addr, remote_addr_str, 100) == NULL){
		perror("converting remote addr: ");
	}
	if(valid){
		printf("Sent board message with %d bytes to %s:%d\n", n_bytes, remote_addr_str, remote_port);
	}
	else{
		printf("[INVALID] Sent board message with %d bytes to %s:%d\n", n_bytes, remote_addr_str, remote_port);
	}
}

/**
 * Writes to "m" the current state of the board
 * 
 **/
void fill_board_message(board_message *m, int number_players, int player_total){

	node *aux=player_info_list;

	m->num_player=number_players;
	m->player_id=player_total;
	m->ball=ball;
	m->status=true;

	for(int i=0; i<number_players; i++){

		m->player_paddle[i]=aux->player_info.player_paddle;
		aux=aux->next;
	}
}

/**
 * Returns pointer to paddle with address "addr"
 * 
 **/
paddle *find_paddle(struct sockaddr_in addr){

	node *aux=player_info_list;

	while(aux!=NULL){
		if(compare_clients(aux->player_info.addr, addr)){
			return &(aux->player_info.player_paddle);
		}
		aux=aux->next;
	}
	printf("Error: Player not found in list of players.");
	exit(-1);
}


int main(){

    int n_bytes;
    board_message m_sent;
	status_message m_received;
	int player_total=0, number_players=0, paddle_moves=0;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    // create socket
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1){
        perror("socket: ");
        exit(-1);
    }

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(SOCK_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket server
    int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (err == -1){
        perror("bind");
        exit(-1);
    }

	//initialize ball
	place_ball_random(&ball);
    
    while(1){

		//waits to receive information from socket sock_fd
        n_bytes = recvfrom(sock_fd, &m_received, sizeof(status_message), 0, 
                        (struct sockaddr *)&client_addr, &client_addr_size);
        if (n_bytes!= sizeof(status_message)){continue;}

		//prints in the terminal information about the received message
		print_communication_status(client_addr, n_bytes, m_received);
			
        if((m_received.msg_type == 0)&&(number_players<MAX_PLAYERS)){//connect
            
			//create and insert new player
			player_total++;
			node *new_player=create_player(client_addr, player_total);
			insert_player(new_player);
			number_players++;
			
			//send board message
			fill_board_message(&m_sent, number_players, player_total);
			n_bytes=sendto(sock_fd, &m_sent, sizeof(m_sent), 0, 
					(const struct sockaddr *) &client_addr, client_addr_size);
			//prints in the terminal information about the message sent
			print_board_communication(client_addr, n_bytes, true);
        }
		else if(m_received.msg_type == 0){//connect but max number of clients reached

			//send error message
			m_sent.status=false;
			n_bytes=sendto(sock_fd, &m_sent, sizeof(m_sent), 0, 
					(const struct sockaddr *) &client_addr, client_addr_size);
			//prints in the terminal information about the message sent
			print_board_communication(client_addr, n_bytes, false);
		}
		else if(m_received.msg_type == 1){//paddle move
			
			//find paddle
			paddle *aux=find_paddle(client_addr);

			//move paddle
			moove_paddle (&(aux->position), m_received.direction, m_received.player_id);
			paddle_moves++;

			//move ball
			if(paddle_moves>=number_players){paddle_moves=0; moove_ball(&ball);}

			//send board message
			fill_board_message(&m_sent, number_players, player_total);
			n_bytes=sendto(sock_fd, &m_sent, sizeof(m_sent), 0, 
					(const struct sockaddr *) &client_addr, client_addr_size);
			//prints in the terminal information about the message sent
			print_board_communication(client_addr, n_bytes, true);			
        }
		else if(m_received.msg_type == 2){//disconnect

			number_players--;

			//disconnect client
			disconnect_player(client_addr);		
        }
		else{
			printf("Message type: %d\nMessage ignored.\n", m_received.msg_type);
		}
    }
	close(sock_fd);
}
