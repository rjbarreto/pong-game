/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Final Project
 * 
 * Group - 16
 * Pedro -------- 
 * Ricardo Barreto 
 * 
 * server.c-New relay pong server
 * 
 ***************************************************************/

#include "pong.h"
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

#define SOCK_PORT 5000  //port number

int sock_fd;
node *player_info_list; //list of all current player
node *current_player;   //player with the ball
int number_players=0;
struct sockaddr_in client_addr;
socklen_t client_addr_size = sizeof(struct sockaddr_un);
pthread_mutex_t mux_current_player, mux_player_info_list, mux_number_players, message_variables_mux;
paddle_position_t paddle; //paddle
ball_position_t ball;     //ball


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
	new_player->player_info.player_id=player_id+1;
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
 * Initializes the variable "ball" at a random position
 * 
 **/
void place_ball_random(ball_position_t * ball){

    ball->x = rand() %(WINDOW_SIZE -2)+1;
    ball->y = rand() %(WINDOW_SIZE -2)+1;
    ball->c = 'o';
    ball->up_hor_down = rand() % 3 -1;     // -1 up, 1 down
    ball->left_ver_right = rand() % 3 -1 ; // 0 vertical, -1 left, 1 right
}

/**
 * Initializes the variable "paddle" with length "length" at a specific position
 * 
 **/
void new_paddle (paddle_position_t * paddle, int legth){
    paddle->x = WINDOW_SIZE/2;
    paddle->y = WINDOW_SIZE-2;
    paddle->length = legth;
}

/**
 * Determines the next player who receives the ball
 * 
 **/
void next_player(){

	pthread_mutex_lock(&mux_current_player);
	if(current_player->next==NULL){
		pthread_mutex_lock(&mux_player_info_list);
		current_player=player_info_list;
		pthread_mutex_unlock(&mux_player_info_list);
	}
	else{
		current_player=current_player->next;
	}
	pthread_mutex_unlock(&mux_current_player);
}

/**
 * Removes from players' list, player with address "addr"
 * 
 **/
void disconnect_player(struct sockaddr_in addr){
	
	node *aux, *aux1;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);
	
	//finds element to be removed
	while(aux!=NULL){
		
		if(compare_clients(aux->player_info.addr,addr)){break;}
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
 * Print in the terminal information about a message
 * mode status=true: message received
 * mode status=false: message sent
 * 
 **/
void print_communication_status(struct sockaddr_in _client_addr, int n_bytes, message m, bool status){

	char remote_addr_str[100];
	int remote_port = ntohs(_client_addr.sin_port);
	if (inet_ntop(AF_INET, &_client_addr.sin_addr, remote_addr_str, 100) == NULL){
		perror("converting remote addr: ");
	}
	if(status){
		printf("Received message of type %d with %d bytes from %s:%d\n", m.msg_type, n_bytes, remote_addr_str, remote_port);
	}
	else{
		printf("Sent message of type %d with %d bytes to %s:%d\n", m.msg_type, n_bytes, remote_addr_str, remote_port);
	}
}

/**
 * Bradcasts message "m" to all clients with the exception of the one with address "addr"
 * 
 **/
void broadcast(struct sockaddr_in addr, message m){

	int n_bytes;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);
	node *aux;

	pthread_mutex_lock(&mux_player_info_list);
	aux=player_info_list;
	pthread_mutex_unlock(&mux_player_info_list);

	while(aux!=NULL){
		if(!compare_clients(aux->player_info.addr,addr)){

			n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
                	(const struct sockaddr *) &(aux->player_info.addr), client_addr_size);    
			print_communication_status(aux->player_info.addr, n_bytes, m, false); 
		}
		aux=aux->next;
	}
}

/**
 * Thread that releases ball from client every 10 seconds
 * 
 **/
void *change_client(){

	message m;
	int n_bytes;

	while(1){

		sleep(10);

		pthread_mutex_lock(&mux_number_players);
		if(number_players>0){

			pthread_mutex_unlock(&mux_number_players);

			//releases ball of current player
			m.msg_type=1;
			pthread_mutex_lock(&mux_number_players);
			m.num_player=number_players;
			pthread_mutex_unlock(&mux_number_players);

			pthread_mutex_lock(&mux_current_player);
			n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
					(const struct sockaddr *) &(current_player->player_info.addr), client_addr_size);  
			//prints in the terminal information about the message sent
			print_communication_status(current_player->player_info.addr, n_bytes, m, false);
			pthread_mutex_unlock(&mux_current_player);


			//finds next player
			next_player();

			//sends ball to next player
			m.msg_type=2;
			pthread_mutex_lock(&message_variables_mux);
			m.paddle=paddle;
			m.ball=ball;
			pthread_mutex_unlock(&message_variables_mux);

			pthread_mutex_lock(&mux_number_players);
			m.num_player=number_players;
			pthread_mutex_unlock(&mux_number_players);

			pthread_mutex_lock(&mux_current_player);
			n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
					(const struct sockaddr *) &(current_player->player_info.addr), client_addr_size);  
			//prints in the terminal information about the message sent
			print_communication_status(current_player->player_info.addr, n_bytes, m, false); 
			pthread_mutex_unlock(&mux_current_player);
		}
		else{pthread_mutex_unlock(&mux_number_players);}
	}
}

/**
 * Thread that reads messages sent from all clients
 * 
 **/
void *read_socket(){
	
	message m;
	int n_bytes;
	int player_total=0;
	
	while(1){

		//waits to receive information from socket
        n_bytes = recvfrom(sock_fd, &m, sizeof(message), 0, 
                        (struct sockaddr *)&client_addr, &client_addr_size);
        if (n_bytes!= sizeof(message)){continue;}

		//prints in the terminal information about the received message
		print_communication_status(client_addr, n_bytes, m, true);
			
        if(m.msg_type == 0){//connect
            
			//create and insert new player
			player_total++;
			node *new_player=create_player(client_addr, player_total);
			insert_player(new_player);

			pthread_mutex_lock(&mux_number_players);
			number_players++;
			pthread_mutex_unlock(&mux_number_players);

			//if new player is first player
			pthread_mutex_lock(&mux_number_players);
			if(number_players==1){

				pthread_mutex_unlock(&mux_number_players);

				//initialize players' list
				pthread_mutex_lock(&mux_current_player);
				current_player=player_info_list;
				pthread_mutex_unlock(&mux_current_player);

				//send message send_ball
				m.msg_type=2;

				//initialize ball and paddle
				place_ball_random(&m.ball);
				new_paddle(&m.paddle, PADLE_SIZE);

				pthread_mutex_lock(&mux_number_players);
				m.num_player=number_players;
				pthread_mutex_unlock(&mux_number_players);
				n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
                		(const struct sockaddr *) &client_addr, client_addr_size);
				//prints in the terminal information about the message sent
				print_communication_status(client_addr, n_bytes, m, false);
			}
			else{pthread_mutex_unlock(&mux_number_players);}
        }
		else if(m.msg_type == 3){//move ball
			
			//broadcast move ball message
			pthread_mutex_lock(&mux_number_players);
			m.num_player=number_players;
			pthread_mutex_unlock(&mux_number_players);
			pthread_mutex_lock(&message_variables_mux);
			ball=m.ball;
			paddle=m.paddle;
			pthread_mutex_unlock(&message_variables_mux);
			broadcast(client_addr, m);	
        }
		else if(m.msg_type == 4){//disconnect

			pthread_mutex_lock(&mux_number_players);
			number_players--;
			pthread_mutex_unlock(&mux_number_players);

			pthread_mutex_lock(&mux_current_player);
			if(compare_clients(client_addr, current_player->player_info.addr)){//compare if disconnected player has the ball
				
				pthread_mutex_unlock(&mux_current_player);

				//calculates next player who receives the ball
				next_player();
			
				pthread_mutex_lock(&mux_number_players);
				if(number_players>0){

					pthread_mutex_unlock(&mux_number_players);

					//sends ball to next player
					m.msg_type=2;

					pthread_mutex_lock(&mux_number_players);
					m.num_player=number_players;
					pthread_mutex_unlock(&mux_number_players);

					pthread_mutex_lock(&mux_current_player);			
					n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
							(const struct sockaddr *) &(current_player->player_info.addr), client_addr_size); 
					//prints in the terminal information about the message sent
					print_communication_status(current_player->player_info.addr, n_bytes, m, false);    
					pthread_mutex_unlock(&mux_current_player);
				}  	
				else{pthread_mutex_unlock(&mux_number_players);}
			}
			else{pthread_mutex_unlock(&mux_current_player);}

			//disconnect client
			disconnect_player(client_addr);		
        }
		else{
			printf("Message type: %d\nMessage ignored.\n", m.msg_type);
		}
    }
}


int main(){

	pthread_t threads[2];

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
    
	//Initialize mutex
    pthread_mutex_init(&mux_current_player, NULL);
	pthread_mutex_init(&mux_player_info_list, NULL);
	pthread_mutex_init(&mux_number_players, NULL);
	pthread_mutex_init(&message_variables_mux, NULL);

	//start threads
    pthread_create(&(threads[0]), NULL, change_client, NULL);
	pthread_create(&(threads[1]), NULL, read_socket, NULL);
    
    for(int i=1; i<2; i++){

        pthread_join(threads[i],NULL);
    }

	//destroy mutex
	pthread_mutex_destroy(&mux_current_player);
	pthread_mutex_destroy(&mux_player_info_list);
	pthread_mutex_destroy(&mux_number_players);
	pthread_mutex_destroy(&message_variables_mux);

	close(sock_fd);
}
