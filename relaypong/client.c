/**************************************************************
 * IST-MEEC PSIS                   
 * 2021/22 1S P2
 * Intermediate Project
 * 
 * Pedro -------- 
 * Ricardo Barreto
 * 
 * client.c-Relay pong client
 * 
 ***************************************************************/

#include "pong.h"
#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>  
#include <ctype.h> 
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

WINDOW * message_win;     // message window
WINDOW * my_win;          // game board
paddle_position_t paddle; //paddle
ball_position_t ball;     //ball

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
 * Draws paddle on window "win"
 * mode draw=true: draws paddle
 * mode draw=false: cleans paddle
 * 
 **/
void draw_paddle(WINDOW *win, paddle_position_t * paddle, int draw){

    int ch;
    if(draw){
        ch = '_';
    }else{
        ch = ' ';
    }

    int start_x = paddle->x - paddle->length;
    int end_x = paddle->x + paddle->length;
    for (int x = start_x; x <= end_x; x++){
        wmove(win, paddle->y, x);
        waddch(win,ch);
    }
    wrefresh(win);
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
 * Checks if paddle move to coordinates "x" and "y" is valid
 * Only takes into account the ball
 * 
 **/
bool check_availability(int y, int x, int length){

	int start_x = x - length;
	int end_x = x + length;
	
	if((ball.y==y)&&(ball.x>=start_x)&&(ball.x<=end_x)){//check sobreposition with ball
		
		return false;
	}
	return true;
}

/**
 * Calculates the new coordinates of the paddle
 * 
 **/
void moove_paddle (paddle_position_t * paddle, int direction){

    if (direction == KEY_UP){
        if ((paddle->y  != 1)&&(check_availability(paddle->y-1, paddle->x, paddle->length))){
            paddle->y --;
        }
    }
    if (direction == KEY_DOWN){
        if ((paddle->y  != WINDOW_SIZE-2)&&(check_availability(paddle->y+1, paddle->x, paddle->length))){
            paddle->y ++;
        }
    }
    if (direction == KEY_LEFT){
        if ((paddle->x - paddle->length != 1)&&(check_availability(paddle->y, paddle->x-1, paddle->length))){
            paddle->x --;
        }
    }
    if (direction == KEY_RIGHT){
        if ((paddle->x + paddle->length != WINDOW_SIZE-2)&&(check_availability(paddle->y, paddle->x+1, paddle->length))){
            paddle->x ++;
        }
    }
}

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
 * Calculates the ball bouncing in the paddle
 * 
 **/
void paddle_bounce(int current_y, int current_x, ball_position_t *ball){

	int start_x, end_x;

    if(ball->y!=paddle.y){return;}//ball not at the same height as paddle 

    start_x = paddle.x - paddle.length;		
    end_x = paddle.x + paddle.length;

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
    }
}

/**
 * Calculates the new coordinates of the ball
 * 
 **/
void moove_ball(ball_position_t * ball){

    int current_x=ball->x, current_y=ball->y;
    
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
 * Prints in message window a message stating that the player is in the play state
 * 
 **/
void print_message_window(WINDOW *message_win){

    mvwprintw(message_win, 1,2,"PLAY STATE");
    mvwprintw(message_win, 2,2,"use arrow keys to control");
    mvwprintw(message_win, 3,2,"\t\tpaddle");
    waddch(message_win,' ');
    wrefresh(message_win);
}

/**
 * Returns current time in milliseconds
 * 
 * */
long long timeInMilliseconds() {
    
    struct timeval t;

    gettimeofday(&t,NULL);

    return (((long long)t.tv_sec)*1000)+(t.tv_usec/1000);
}

/**
 * Sends to server disconnect message and closes all descriptors
 * 
 **/
void send_disconnect_message(message m, int sock_fd, struct sockaddr_in server_addr){
    
    //send disconnect message
    m.msg_type=4;
    m.ball=ball;
    m.paddle=paddle;
    if(-1==sendto(sock_fd, &m, sizeof(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr))){exit(-1);}

    close(sock_fd);
    endwin();
}


int main(int argc, char *argv []){

    message m;
    int key = -1;
    bool play_state=false;
    long long start_time;

    //check if correct number of command line arguments
    if(argc!=3){
		printf("COMMAND: ./pong [ip] [port]\n");
		exit(-1);
	}

    //create socket
    int sock_fd;
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
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
    m.msg_type = 0;
    if(-1==sendto(sock_fd, &m, sizeof(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr))){
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

    //initialize ball and paddle
    place_ball_random(&ball);
    new_paddle(&paddle, PADLE_SIZE);

    while(1){
        //socket in blocking mode
        if(-1==recv(sock_fd, &m, sizeof(message), 0)){
            
            send_disconnect_message(m, sock_fd, server_addr); 
            printf("%s\n", strerror(errno)); 
            exit(-1);
        }

        if(m.msg_type== 2){ // send ball
            
            draw_ball(my_win, &ball, false);
            wrefresh(my_win);
            if(m.num_player>1){paddle=m.paddle; ball=m.ball;} //if not only player, get ball and paddle coordinates
            break;
        }
        //receive move ball message-update paddle and ball variables, draw ball
        draw_ball(my_win, &ball, false);
        paddle=m.paddle;
        ball=m.ball;
        draw_ball(my_win, &ball, true);
        wrefresh(my_win);
    }

    //client has the ball, draw paddle and ball
    start_time=timeInMilliseconds(); //reset counter
    play_state=true;
    draw_paddle(my_win, &paddle, true);
    draw_ball(my_win, &ball, true);
    wrefresh(my_win);

    //print in the message window information about the play state
    print_message_window(message_win);

    //while player doesn t press 'q' when has the ball
    while((key != 'q')||(play_state!=true)){

        key = wgetch(my_win); //read from keyboar
        if ((key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN)&&(play_state==true)){// arrow keys

            //draw ball and paddle at new coordinates
            draw_paddle(my_win, &paddle, false);
            draw_ball(my_win, &ball, false);
            moove_paddle (&paddle, key);
            draw_paddle(my_win, &paddle, true);
            moove_ball(&ball);
            draw_ball(my_win, &ball, true);
            wrefresh(my_win);
            
            //send move ball to server
            m.msg_type=3;
            m.ball=ball;
            m.paddle=paddle;
            if(-1==sendto(sock_fd, &m, sizeof(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr))){
                send_disconnect_message(m, sock_fd, server_addr); 
                printf("%s\n", strerror(errno)); 
                exit(-1);
            }
        }
        else if((key=='r')&&(play_state==true)&&(timeInMilliseconds()-start_time>10000)){// 'r' key

            //player no longer has the ball
            play_state=false;

            //send release ball message to server 
            m.msg_type=1;
            m.ball=ball;
            m.paddle=paddle;
            if(-1==sendto(sock_fd, &m, sizeof(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr))){
                send_disconnect_message(m, sock_fd, server_addr); 
                printf("%s\n", strerror(errno)); 
                exit(-1);
            }

            //cleans paddle from game board
            draw_paddle(my_win, &paddle, false);
            draw_ball(my_win, &ball, true);
            wrefresh(my_win);

            //cleans message window
            clear_window(message_win, MESSAGE_WINDOW_Y, MESSAGE_WINDOW_X);
        }
        
        while(play_state==false){
            
            m.msg_type =-1; 
            //socket in blocking mode
            if(-1==recv(sock_fd, &m, sizeof(message), 0)){
                send_disconnect_message(m, sock_fd, server_addr); 
                printf("%s\n", strerror(errno)); 
                exit(-1);
            }

            if(m.msg_type == 2){ //send ball

                //player has the ball
                start_time=timeInMilliseconds(); //reset counter
                play_state=true;

                //get ball and paddle positions and draw on the game board
                ball=m.ball;
                paddle=m.paddle;
                draw_paddle(my_win, &paddle, true);
                draw_ball(my_win, &ball, true);
                wrefresh(my_win);

                //print in the message window information about the play state
                print_message_window(message_win);
            }
            else if(m.msg_type==3){ //move ball

                //update ball position
                draw_ball(my_win, &ball, false);
                ball=m.ball;
                draw_ball(my_win, &ball, true);
                wrefresh(my_win);
            }
        }
    }

   //send disconnect message
    send_disconnect_message(m, sock_fd, server_addr);
    
    return 0;
}
