/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Final Project
 * 
 * Group - 16
 * Pedro -------- 
 * Ricardo Barreto 
 * 
 * client.c-New super pong client
 * 
 ***************************************************************/

#include "pong.h"
#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <ctype.h> 
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>


WINDOW * message_win;     // message window
WINDOW * my_win;          // game board


ball_position_t ball;     //ball
paddle paddles_list[MAX_PLAYERS];

int sock_fd;
int my_id;

/**
 * Draws paddle on window "win"
 * mode draw=true: draws paddle
 * mode draw=false: cleans paddle
 * 
 **/
void draw_paddle(WINDOW *win, paddle_position_t * paddle, char ch){

    int start_x = paddle->x - paddle->length;
    int end_x = paddle->x + paddle->length;
    for (int x = start_x; x <= end_x; x++){
        wmove(win, paddle->y, x);
        waddch(win,ch);
    }
    wrefresh(win);
}

/**
 * Draws all paddles on window "win"
 * mode draw=true: draws paddle
 * mode draw=false: cleans paddle
 * 
 **/
void draw_paddle_board(WINDOW *my_win, paddle paddles_list[MAX_PLAYERS], int draw, int my_id, int num_player){

    char ch;
    
    for(int i=0; i<num_player; i++){
        
        if(draw){
            if(paddles_list[i].player_id==my_id){
                ch = '=';
            }
            else{
                ch = '_';
            }
        }else{
            ch = ' ';
        }

        draw_paddle(my_win, &(paddles_list[i].position), ch);
    }
}

/**
 * Fills window "win" with blank spaces, that is, cleans the window
 * 
 **/
void clear_window(WINDOW *win, int window_y, int window_x){
    
    int ch= ' ';
    
    for (int i=1; i <window_y-1; i++){
        for(int j=1; j<window_x-1; j++){
            wmove(win, i, j);
            waddch(win,ch);
        }
    }
    wrefresh(win);
}

/**
 * Draws ball on window "win"
 * mode draw=true: draws ball
 * mode draw=false: cleans ball
 * 
 **/
void draw_ball(WINDOW *win, ball_position_t * ball, int draw){

    int ch;
    if(draw){
        ch = ball->c;
    }else{
        ch = ' ';
    }
    wmove(win, ball->y, ball->x);
    waddch(win,ch);
    wrefresh(win);
}

/**
 * Prints in message window the players' scores
 * 
 **/
void print_message_window(WINDOW *message_win, board_message m, int my_id){

    char buffer[STRING_SIZE];

    for(int i=0; i<m.num_player; i++){
        
        sprintf(buffer, "P%d - %d ", m.player_paddle[i].player_id, m.player_paddle[i].score);
        if(m.player_paddle[i].player_id==my_id){strcat(buffer,"<---");}
        mvwprintw(message_win, m.num_player-i,2,buffer);
    } 
    wrefresh(message_win);
}

/**
 * Copy to array all gameboard paddles
 * 
 **/
void copy_paddles(paddle *paddles_list, board_message m){

    for(int i=0; i<m.num_player; i++){
        paddles_list[i]=m.player_paddle[i];
    }
}

/**
 * Close client descriptors
 * 
 **/
void close_client(int sock_fd){
    
    endwin();  
    close(sock_fd);
}


/**
 * Thread that receives board message from the server and draws the information on the window
 * 
 **/
void *read_socket(){
    board_message m_received;

    while (1){        
        //socket in blocking mode
        if(0>=read(sock_fd, &m_received, sizeof(board_message))){
            close_client(sock_fd);  
            printf("%s\n", strerror(errno)); 
            exit(-1);
        }

        //clean ball and paddle at new coordinates
        clear_window(my_win, WINDOW_SIZE, WINDOW_SIZE);

        copy_paddles(paddles_list,m_received);
          
        //draw ball and paddle at new coordinates
        ball=m_received.ball;
        draw_paddle_board(my_win, paddles_list, true, my_id, m_received.num_player);
        
        draw_ball(my_win, &ball, true);
        clear_window(message_win, MESSAGE_WINDOW_Y, MESSAGE_WINDOW_X);
        print_message_window(message_win, m_received, my_id);
    }
}

int main(int argc, char *argv []){

    paddle_message m_sent;
    board_message m_received;
    pthread_t thread;
    int key = -1;
 
    //check if correct number of command line arguments
    if(argc!=3){
		printf("COMMAND: ./pong [ip] [port]\n");
		exit(-1);
	}

    //create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1){
	    perror("socket: ");
	    exit(-1);
    }
    
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	if( inet_pton(AF_INET, argv[1], &server_addr.sin_addr) < 1){
		printf("no valid address: \n");
		exit(-1);
	}

    // send to server connect message
    if(-1==connect(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr))){
        printf("%s\n", strerror(errno)); 
        close(sock_fd); 
        exit(-1);
    }

	initscr();		    	// Start curses mode 		
	cbreak();				// Line buffering disabled
    keypad(stdscr, TRUE);   // We get F1, F2 etc..		
	noecho();			    // Don't echo() while we do getch 

    // creates a window and draws a border 
    my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);
    keypad(my_win, true);
    
    // creates a window and draws a border 
    message_win = newwin(MESSAGE_WINDOW_Y, MESSAGE_WINDOW_X, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
	wrefresh(message_win);
    
    //socket in blocking mode
    if(0>=read(sock_fd, &m_received, sizeof(board_message))){
        endwin();
        close(sock_fd);
        exit(-1);
    }

    //checks for max number of clients reached
    if(m_received.status==false){
        endwin();
        close(sock_fd);
        printf("Max number of clients reached.\n"); 
        exit(-1);
    }

    //copies data from board message to relevant data structures
    ball=m_received.ball;
    my_id=m_received.player_id;   
    copy_paddles(paddles_list,m_received);
    

    //refreshes the screen 
    draw_paddle_board(my_win, paddles_list, true, my_id, m_received.num_player);    
    draw_ball(my_win, &ball, true);
    print_message_window(message_win, m_received, my_id);

    //create thread to read from socket
    pthread_create(&thread, NULL, read_socket, NULL);

    //while player doesn t press 'q'
    while(key != 'q'){

        key = wgetch(my_win); //read from keyboar
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){// arrow keys

            //send paddle move message
            m_sent.direction=key;
            m_sent.player_id=my_id;
            if(-1==write(sock_fd, &m_sent, sizeof(paddle_message))){
                close_client(sock_fd); 
                printf("%s\n", strerror(errno)); 
                exit(-1);
            }
        }
    }
    //close client descriptors
    close_client(sock_fd);

    return 0;
}
