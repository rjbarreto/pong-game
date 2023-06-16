/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Intermediate Project
 * 
 * Pedro -------- 
 * Ricardo Barreto 
 * 
 * server.c-Relay pong server
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

#define SOCK_PORT 5000  //port number

int sock_fd;
node *player_info_list; //list of all current player
node *current_player;   //player with the ball


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
	
	if(player_info_list!=NULL){
		
		new_player->next=player_info_list;
	}
	player_info_list=new_player;
}

/**
 * Determines the next player who receives the ball
 * 
 **/
void next_player(){
	
	if(current_player->next==NULL){
		
		current_player=player_info_list;
	}
	else{
		current_player=current_player->next;
	}
	
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
 * Print in the terminal information about a message
 * mode status=true: message received
 * mode status=false: message sent
 * 
 **/
void print_communication_status(struct sockaddr_in client_addr, int n_bytes, message m, bool status){

	char remote_addr_str[100];
	int remote_port = ntohs(client_addr.sin_port);
	if (inet_ntop(AF_INET, &client_addr.sin_addr, remote_addr_str, 100) == NULL){
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
	node *aux=player_info_list;

	while(aux!=NULL){
		if(!compare_clients(aux->player_info.addr,addr)){

			n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
                	(const struct sockaddr *) &(aux->player_info.addr), client_addr_size);    
			print_communication_status(aux->player_info.addr, n_bytes, m, false); 
		}
		aux=aux->next;
	}
}

int main(){

    int n_bytes;
    message m;
	int player_total=0, number_players=0;
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
    
    while(1){
		//waits to receive information from socket sock_fd
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
			number_players++;

			//if new player is first player
			if(number_players==1){
				
				//initialize players' list
				current_player=player_info_list;

				//send message send_ball
				m.msg_type=2;
				m.num_player=number_players;
				n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
                		(const struct sockaddr *) &client_addr, client_addr_size);
				//prints in the terminal information about the message sent
				print_communication_status(client_addr, n_bytes, m, false);
			}
        }
		else if(m.msg_type == 1){//release ball
			
			//finds next player
			next_player();

			//sends ball to next player
			m.msg_type=2;
			m.num_player=number_players;
			n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
					(const struct sockaddr *) &(current_player->player_info.addr), client_addr_size);  
			//prints in the terminal information about the message sent
			print_communication_status(current_player->player_info.addr, n_bytes, m, false);   
        }
		else if(m.msg_type == 3){//move ball
			
			//broadcast move ball message
			m.num_player=number_players;
			broadcast(client_addr, m);	
        }
		else if(m.msg_type == 4){//disconnect

			number_players--;
			next_player(); //calculates next player who receives the ball

			if(number_players>0){
				//sends ball to next player
				m.msg_type=2;
				m.num_player=number_players;			
				n_bytes=sendto(sock_fd, &m, sizeof(m), 0, 
						(const struct sockaddr *) &(current_player->player_info.addr), client_addr_size); 
				//prints in the terminal information about the message sent
				print_communication_status(current_player->player_info.addr, n_bytes, m, false);    
			}  	

			//disconnect client
			disconnect_player(client_addr);		
        }
		else{
			printf("Message type: %d\nMessage ignored.\n", m.msg_type);
		}
    }
	close(sock_fd);
}
