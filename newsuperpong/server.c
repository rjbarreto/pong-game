/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Final Project
 * 
 * Group - 16
 * Pedro -------- 
 * Ricardo Barreto
 * 
 * server.c-New super pong server
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
#include <pthread.h>

#define SOCK_PORT 5000    //port number

node *player_info_list;   //list of all current player
ball_position_t ball;     //ball
int player_total=0, number_players=0;

pthread_mutex_t mux_player_info_list, mux_player_total, mux_number_players, mux_ball;

/**
 * Initializes the variable "ball" at a random position
 * 
 **/
void place_ball_random(ball_position_t * ball){

    ball->x = rand() % (WINDOW_SIZE-2)+1 ;
    ball->y = rand() % (WINDOW_SIZE-2)+1 ;
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

	node *aux;
	int aux_start_x, aux_end_x;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);

	int start_x = x - length;
	int end_x = x + length;
	
	pthread_mutex_lock(&mux_ball);
	if((ball.y==y)&&(ball.x>=start_x)&&(ball.x<=end_x)){//check sobreposition with ball

		pthread_mutex_unlock(&mux_ball);
		
		while(aux!=NULL){

			if(aux->player_info.player_paddle.player_id==my_id){//increments score of current player

				(aux->player_info.player_paddle.score)++;
				break;
			}
			aux=aux->next;
		}
		return false;
	}
	else{pthread_mutex_unlock(&mux_ball);}


	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);

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

	node *aux;
	int start_x, end_x;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);

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
 * Creates a node of a new player with "address" addr and id "player_id"
 * 
 **/
node *create_player(int c_sock_fd, int player_id){
	
	node *new_player=(node *)malloc(sizeof(node));
	
	new_player->player_info.sock_fd=c_sock_fd;
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
	pthread_mutex_lock(&mux_player_info_list);
	if(player_info_list!=NULL){
		
		new_player->next=player_info_list;
	}
	player_info_list=new_player;
	pthread_mutex_unlock(&mux_player_info_list);
}

/**
 * Removes from players' list, player with address "addr"
 * 
 **/
void disconnect_player(int c_sock_fd){
	
	node *aux, *aux1;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);
	
	//finds element to be removed
	while(aux!=NULL){
		
		if(aux->player_info.sock_fd==c_sock_fd){break;}
		aux1=aux;
		aux=aux->next;
	}
	
	pthread_mutex_lock(&mux_player_info_list);
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
	pthread_mutex_unlock(&mux_player_info_list);
}

/**
 * Print in the terminal information about a status message
 * 
 **/
void print_communication_status(int n_bytes,int c_sock_fd){

	printf("Received message with %d bytes from %d \n", n_bytes, c_sock_fd);
}

/**
 * Print in the terminal information about a board message
 * 
 **/
void print_board_communication(int n_bytes, int c_sock_fd, bool valid){

	
	if(valid){
		printf("Sent board message with %d bytes to %d\n", n_bytes, c_sock_fd);
	}
	else{
		printf("[INVALID] Sent board message with %d bytes to %d\n", n_bytes, c_sock_fd);
	}
}

/**
 * Writes to "m" the current state of the board
 * 
 **/
void fill_board_message(board_message *m, int number_players, int player_total){

	node *aux;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);

	m->num_player=number_players;
	m->player_id=player_total;
	pthread_mutex_lock(&mux_ball);
	m->ball=ball;
	pthread_mutex_unlock(&mux_ball);
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
paddle *find_paddle(int c_sock_fd){

	node *aux;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);

	while(aux!=NULL){
		if(aux->player_info.sock_fd==c_sock_fd){
			return &(aux->player_info.player_paddle);
		}
		aux=aux->next;
	}
	printf("Error: Player not found in list of players.");
	exit(-1);
}



/**
 * Bradcasts message "m" to all clients
 * 
 **/
void broadcast(board_message m_sent){

	int n_bytes;
	node *aux;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);

	while(aux!=NULL){
		n_bytes=write(aux->player_info.sock_fd, &m_sent, sizeof(m_sent));

		//prints in the terminal information about the message sent
		print_board_communication(n_bytes, aux->player_info.sock_fd, true); 
		aux=aux->next;
	}
}

/**
 * Thread that calculates ball position each second
 * 
 **/
void *ball_position(){

    board_message m;

    while(1){
        
		sleep(1);
		
		//calculate ball position
		pthread_mutex_lock(&mux_ball);
		moove_ball(&ball);
		pthread_mutex_unlock(&mux_ball);
		
		//send board message
		pthread_mutex_lock(&mux_number_players);
		fill_board_message(&m, number_players, player_total);
		broadcast(m);
		pthread_mutex_unlock(&mux_number_players);
    }
}

/**
 * Thread that receives message from a specific client
 * 
 **/
void *client_thread(void *arg){

	long c_sock_fd = *(int *) arg;
	int n_bytes;
    board_message m_sent;
	paddle_message m_received;

    while(1){

		//waits to receive information from socket 
        n_bytes = read(c_sock_fd, &m_received, sizeof(paddle_message));
        if (n_bytes!= sizeof(paddle_message)){//disconnect
			pthread_mutex_lock(&mux_number_players);
			number_players--;
			pthread_mutex_unlock(&mux_number_players);
			//disconnect client
			disconnect_player(c_sock_fd);		

			printf("Client with fd=%ld disconnected\n", c_sock_fd);
    
			return (void *)c_sock_fd;
		}

		//prints in the terminal information about the received message
		print_communication_status(n_bytes, c_sock_fd);
		
		//find paddle
		paddle *aux=find_paddle(c_sock_fd);

		//move paddle
		moove_paddle (&(aux->position), m_received.direction, m_received.player_id);

		//send board message
		pthread_mutex_lock(&mux_number_players);
		fill_board_message(&m_sent, number_players, player_total);
		broadcast(m_sent); 
		pthread_mutex_unlock(&mux_number_players);
    }
}


int main(){

	int sock_fd;
    int n_bytes;
    board_message m_sent;
	pthread_t thread;

    // create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
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
	pthread_mutex_lock(&mux_ball);
	place_ball_random(&ball);
	pthread_mutex_unlock(&mux_ball);

	//listen socket
	if(listen(sock_fd, 2) == -1) {
		perror("listen");
		exit(-1);
	}

	//mutex initializa
	pthread_mutex_init(&mux_player_info_list, NULL);
	pthread_mutex_init(&mux_player_total, NULL);
	pthread_mutex_init(&mux_number_players, NULL);
	pthread_mutex_init(&mux_ball, NULL);

	//create ball position thread
	pthread_create(&thread, NULL, ball_position, NULL);

    while(1){

		//accept clients
		int client_fd= accept(sock_fd, NULL, NULL);
		if(client_fd == -1) {
			perror("accept");
			exit(-1);
		}

		printf("Received message from new client with fd=%d\n", client_fd);

		pthread_mutex_lock(&mux_number_players);
		if(number_players<MAX_PLAYERS){//connect
			
			pthread_mutex_unlock(&mux_number_players);
            
			//create new player
			pthread_mutex_lock(&mux_player_total);
			player_total++;
			node *new_player=create_player(client_fd, player_total);
			pthread_mutex_unlock(&mux_player_total);

			//create client thread
			pthread_create(&(new_player->player_info.thread), NULL, client_thread, &client_fd);

			//insert new player in list
			insert_player(new_player);
			
			pthread_mutex_lock(&mux_number_players);
			number_players++;
			pthread_mutex_unlock(&mux_number_players);
			
			
			//send board message
			pthread_mutex_lock(&mux_number_players);
			fill_board_message(&m_sent, number_players, player_total);
			pthread_mutex_unlock(&mux_number_players);
			n_bytes=write(client_fd, &m_sent, sizeof(m_sent));

			//prints in the terminal information about the message sent
			print_board_communication(n_bytes, client_fd,true);
        }
		else{//max number of clients reached

			pthread_mutex_unlock(&mux_number_players);

			//send error message
			m_sent.status=false;
			n_bytes=write(client_fd, &m_sent, sizeof(m_sent));
			
			//prints in the terminal information about the message sent
			print_board_communication(n_bytes, client_fd, false);
		}
    }

	//destroy mutex
	pthread_mutex_destroy(&mux_player_info_list);
	pthread_mutex_destroy(&mux_player_total);
	pthread_mutex_destroy(&mux_number_players);
	pthread_mutex_destroy(&mux_ball);

	close(sock_fd);
}



		
