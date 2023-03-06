#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#ifndef PORT
	#define PORT 9999
#endif
pthread_t bibi, mods1, mods2, piu[3], pidd1, pidd2;
static char *player1, *player2, *first, *second;
static int work_flag = 1, first_flag = 0;
static int game_flag1 = 0, game_flag2 = 0;
struct tank{
	int left_tr[6];	// [0-2] - y, [3-5] - x
	int right_tr[6];// [0-2] - y, [3-5] - x
	int turret[2]; 	// [0] - y, [1] - x
	int gun[2];	// [0] - y, [1], x
	int f_gun;
	struct bullet{
		int x;
		int y;
		int numb;
		int act;
		int inc[2];	// [0] - y, [1] - x
	}bl[3];
	int ch;
	int status[3];
	int yyyy;
};
struct tank *t1, *t2;
//int status[3] = {0, 0, 0};
void *udp_server(){
	int sock, addr_len, ret;
	int buffer;
	fd_set readfd;
	struct sockaddr_in addr;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(0 > sock){
		perror("socket");
		return NULL;
	}
	addr_len = sizeof(struct sockaddr_in);
	memset((void*)&addr, 0, addr_len);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(PORT);
	addr_len = sizeof(addr);
	if(bind(sock, (struct sockaddr*)&addr, addr_len)<0){
		perror("bind");
		close(sock);
		return NULL;
	}
	while(work_flag){
		FD_ZERO(&readfd);
		FD_SET(sock, &readfd);
		if(game_flag1 == -1 || game_flag2 == -1)
			break;
		ret = select(sock+1, &readfd, NULL, NULL, 0);
		if(ret>0){
			if(FD_ISSET(sock, &readfd)){
				recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addr_len);
				if((strcmp(player1, inet_ntoa(addr.sin_addr)) == 0 || strcmp(player2, inet_ntoa(addr.sin_addr)) == 0) && buffer == 's' && first_flag == 0){
					first = malloc(sizeof(inet_ntoa(addr.sin_addr)));
					strcpy(first, inet_ntoa(addr.sin_addr));
					first_flag = 1;
					if(strcmp(player1, first) == 0){
						second = malloc(sizeof(player2));
						strcpy(second, player2);
					}
					else{
						second = malloc(sizeof(player1));
						strcpy(second, player1);
					}
				}
				if(first_flag == 1){
					if(strcmp(first, inet_ntoa(addr.sin_addr)) == 0){
						t1->ch = buffer;
						if(t1->ch == 's')
							game_flag1 = 1;
					}
					if(strcmp(second, inet_ntoa(addr.sin_addr)) == 0 && first_flag == 1){
						t2->ch = buffer;
						if(t2->ch == 's')
							game_flag2 = 1;
					}
				}
				buffer = 0;
			}
		}
	}
	close(sock);
	return (void*)NULL;
}
void *bullet_maker(void *args)
{
	struct tank *data = (struct tank*)args;
	int i, flag = 0;
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);
	for( i = 0; i < 3; i++){
		if(data->status[i] == 1){
			data->status[i] = 3; //mine
			data->bl[i].act = 1;	
			data->bl[i].inc[1] = data->gun[1] - data->turret[1], data->bl[i].inc[0] = data->gun[0] - data->turret[0];
			data->bl[i].x = data->gun[1];
			data->bl[i].y = data->gun[0];
			data->bl[i].x += data->bl[i].inc[1];
			data->bl[i].y += data->bl[i].inc[0];
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	while(flag != 1){
		data->bl[i].x += data->bl[i].inc[1];
		data->bl[i].y += data->bl[i].inc[0];
		if(game_flag1 == -1 || game_flag2 == -1)
			pthread_exit(0);
		if(data->bl[i].x == 0 || data->bl[i].x == COLS || data->bl[i].y == 0 || data->bl[i].y == LINES){
			data->bl[i].act = 0;
			data->status[data->bl[i].numb] = 2;
			flag = 1;
		}
		usleep(10000);
	}
}
void *draw()
{	
	while(1){
		clear();
		mvaddch(t1->turret[0], t1->turret[1], 'o');
		mvaddch(t2->turret[0], t2->turret[1], 'o');
		for(int i=0;i<3;i++)
		{
			mvaddch(t1->left_tr[i], t1->left_tr[i+3], '#');
			mvaddch(t1->right_tr[i], t1->right_tr[i+3], '#');
			mvaddch(t2->left_tr[i], t2->left_tr[i+3], '#');
			mvaddch(t2->right_tr[i], t2->right_tr[i+3], '#');
		}
		switch(t1->f_gun)
		{
			case 1:
				mvaddch(t1->gun[0], t1->gun[1], '|');
				break;
			case 2:
				mvaddch(t1->gun[0], t1->gun[1], '-');
				break;
			case 3:
				mvaddch(t1->gun[0], t1->gun[1], '\\');
				break;
			case 4:
				mvaddch(t1->gun[0], t1->gun[1], '/');
				break;
		}
		for(int j = 0; j < 3; j++){
			if(t1->status[j] == 1 || t1->status[j] == 3)
			{
				mvaddch(t1->bl[j].y, t1->bl[j].x, '*');
				mvaddch(t1->bl[j].y - t1->bl[j].inc[0], t1->bl[j].x - t1->bl[j].inc[1], ' ');
				for(int k=0;k<3;k++)
				{
					if((t2->right_tr[k] == t1->bl[j].y && t2->right_tr[k+3] == t1->bl[j].x) || (t2->left_tr[k] == t1->bl[j].y && t2->left_tr[k+3] == t1->bl[j].x) || (t2->turret[0] == t1->bl[j].y  && t2->turret[1] == t1->bl[j].x) || (t2->gun[0] == t1->bl[j].y && t2->gun[1] == t1->bl[j].x)){
						game_flag2 = -1;
						mvaddstr(LINES/2, COLS/2 - 19, "Player 1 wins! Press any key to quit!");
						getch();
						pthread_exit(0);
					}
				}
			}
		}
		switch(t2->f_gun)
		{
			case 1:
				mvaddch(t2->gun[0], t2->gun[1], '|');
				break;
			case 2:
				mvaddch(t2->gun[0], t2->gun[1], '-');
				break;
			case 3:
				mvaddch(t2->gun[0], t2->gun[1], '\\');
				break;
			case 4:
				mvaddch(t2->gun[0], t2->gun[1], '/');
				break;
		}
		for(int j = 0; j < 3; j++){
			if(t2->status[j] == 1 || t2->status[j] == 3)
			{
				mvaddch(t2->bl[j].y, t2->bl[j].x, '*');
				mvaddch(t2->bl[j].y - t2->bl[j].inc[0], t2->bl[j].x - t2->bl[j].inc[1], ' ');
				for(int k=0;k<3;k++)
				{
					if((t1->right_tr[k] == t2->bl[j].y && t1->right_tr[k+3] == t2->bl[j].x) || (t1->left_tr[k] == t2->bl[j].y && t1->left_tr[k+3] == t2->bl[j].x) || (t1->turret[0] == t2->bl[j].y  && t1->turret[1] == t2->bl[j].x) || (t1->gun[0] == t2->bl[j].y && t1->gun[1] == t2->bl[j].x)){
						game_flag1 = -1;
						mvaddstr(LINES/2, COLS/2 - 19, "Player 2 wins! Press any key to quit!");
						getch();
						pthread_exit(0);
					}
				}
			}
		}
		usleep(10000);
		refresh();
	}
}
void *bullet_control(void *args){
	struct tank *data = (struct tank*)args;
	while(1){
		if(game_flag1 == -1 || game_flag2 == -1)
			pthread_exit(0);
		for(int i=0; i<3; i++){
			if(data->status[i] == 2){
					pthread_join(piu[i], NULL);
					data->bl[i].numb = 0; data->bl[i].act = 0;
					data->bl[i].x = data->gun[1]; data->bl[i].y = data->gun[0] - 1;
				data->status[i] = 0;
			}
		}
		if(data->ch == ' ' && game_flag1 == 1 && game_flag2 == 1){
			data->ch = 'v';
			for(int i = 0; i < 3; i++){
				if(data->status[i] == 0){
					data->status[i] = 1;
					data->yyyy = i;
					data->bl[i].numb = i;
					pthread_create(&piu[data->yyyy], NULL, bullet_maker, data);
					break;
				}
			}
		}
	}
}
void *tank_maker(void *args)
{
	struct tank *data = (struct tank*) args;
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);
	if(data->ch == 'w'){			
		data->gun[1] = data->turret[1];
		data->gun[0] = data->turret[0] - 1;
		data->f_gun = 1;
	}
	if(data->ch == 'x'){
		data->gun[1] = data->turret[1];
		data->gun[0] = data->turret[0] + 1;
		data->f_gun = 1; /* position | */
	}
	if(data->ch == 'a'){
		data->gun[1] = data->turret[1] - 1;
		data->gun[0] = data->turret[0];
		data->f_gun = 2;
	}
	if(data->ch == 'd'){
		data->gun[1] = data->turret[1] + 1;
		data->gun[0] = data->turret[0];
		data->f_gun = 2; /* position - */
	}
	if(data->ch == 'q'){					
		data->gun[1] = data->turret[1] - 1;
		data->gun[0] = data->turret[0] - 1;
		data->f_gun = 3;
	}
	if(data->ch == 'e'){
		data->gun[1] = data->turret[1] + 1;
		data->gun[0] = data->turret[0] - 1;
		data->f_gun = 4;
	}
	if(data->ch == 'z'){
		data->gun[1] = data->turret[1] - 1;
		data->gun[0] = data->turret[0] + 1;
		data->f_gun = 4; /* position / */
	}
	if(data->ch == 'c'){
		data->gun[1] = data->turret[1] + 1;
		data->gun[0] = data->turret[0] + 1;
		data->f_gun = 3; /* position \ */
	}
	if(data->turret[0] > 1){
		if(data->ch == KEY_UP){
			data->turret[0]--;
			data->gun[0]--;
			for(int i=0;i<3;i++)
			{
				data->left_tr[i+3] = data->turret[1] - 1;
				data->left_tr[i] = data->turret[0] - 1 + i;
				data->right_tr[i+3] = data->turret[1] + 1;
				data->right_tr[i] = data->left_tr[i];		
			}
		}
	}
	if(data->turret[0]<LINES-2){
		if(data->ch == KEY_DOWN){
			data->turret[0]++;
			data->gun[0]++;
			for(int i=0;i<3;i++)
			{
				data->left_tr[i+3] = data->turret[1] - 1;
				data->left_tr[i] = data->turret[0] - 1 + i;
				data->right_tr[i+3] = data->turret[1] + 1;
				data->right_tr[i] = data->left_tr[i];	
			}
		}
	}
	if(data->turret[1]>1){
		if(data->ch == KEY_LEFT){
			data->turret[1]--;
			data->gun[1]--;
			for(int i=0;i<3;i++)
			{
				data->left_tr[i+3] = data->turret[1] - 1 + i;
				data->left_tr[i] = data->turret[0] + 1;
				data->right_tr[i+3] = data->left_tr[i+3];
				data->right_tr[i] = data->turret[0] - 1;
			}
		}
	}
	if(data->turret[1]<COLS-2){
		if(data->ch == KEY_RIGHT){
			data->turret[1]++;
			data->gun[1]++;
			for(int i=0;i<3;i++)
			{
				data->left_tr[i+3] = data->turret[1] - 1 + i;
				data->left_tr[i] = data->turret[0] + 1;
				data->right_tr[i+3] = data->left_tr[i+3];
				data->right_tr[i] = data->turret[0] - 1;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	pthread_exit(0);
}
int main(int argc, char* argv[])
{
	if(argc!=3){
		perror("argc");
		exit(0);
	}
	player1 = malloc(sizeof(argv[1]));
	player2 = malloc(sizeof(argv[2]));
	strcpy(player1, argv[1]);
	strcpy(player2, argv[2]);

	int sock, yes = 1;
	pthread_t pid;
	struct sockaddr_in addr;
	int addr_len;
	int count;
	int ret;
	fd_set readfd;
	pthread_create(&pid, NULL, udp_server, NULL);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
		perror("sock");
		return -1;
	}
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
	if(ret == -1){
		perror("setsockopt");
		return 0;
	}

	addr_len = sizeof(struct sockaddr_in);
	memset((void*)&addr, 0, addr_len);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	addr.sin_port = htons(PORT);

	initscr();
	t1 = (struct tank*)malloc(sizeof(struct tank)), t2 = (struct tank*)malloc(sizeof(struct tank));
	t1->gun[0] = 2, t1->gun[1] = 1, t1->turret[0] = t1->gun[0] - 1, t1->turret[1] = t1->gun[1], t1->f_gun = 1;
	t2->gun[0] = LINES - 3, t2->gun[1] = COLS - 2, t2->turret[0] = t2->gun[0] + 1, t2->turret[1] = t2->gun[1], t2->f_gun = 1;
	for(int i=0;i<3;i++)
	{	
		t1->bl[i].numb = 0; t1->bl[i].act = 0;
		t1->bl[i].x = t1->gun[1]; t1->bl[i].y = t1->gun[0] + 1;
		t1->left_tr[i+3] = t1->turret[1] - 1;
		t1->left_tr[i] = i;
		t1->right_tr[i+3] = t1->turret[1] + 1;
		t1->right_tr[i] = t1->left_tr[i];
		t1->status[i] = 0;

		t2->bl[i].numb = 0; t2->bl[i].act = 0;
		t2->bl[i].x = t2->gun[1]; t2->bl[i].y = t2->gun[0] - 1;
		t2->left_tr[i+3] = t2->turret[1] - 1;
		t2->left_tr[i] = t2->turret[0] - 1 + i;
		t2->right_tr[i+3] = t2->turret[1] + 1;
		t2->right_tr[i] = t2->left_tr[i];
		t2->status[i] = 0;
	}
	pthread_create(&bibi, NULL, draw, NULL);
	pthread_create(&pidd1, NULL, bullet_control, t1);
	pthread_create(&pidd2, NULL, bullet_control, t2);
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	int ch;
	while(work_flag){
		ch = getch();
		sendto(sock, &ch, sizeof(ch), 0, (struct sockaddr *)&addr, addr_len);
		if(game_flag1 == 1 && game_flag2 == 1){
			pthread_create(&mods1, NULL, tank_maker, t1);
			pthread_create(&mods2, NULL, tank_maker, t2);
		}
		if(game_flag1 == -1 || game_flag2 == -1) break;
	}
	close(sock);
	free(t1);
	free(t2);
	free(player1);
	free(player2);
	free(first);
	free(second);
	endwin();
	return 0;
}
