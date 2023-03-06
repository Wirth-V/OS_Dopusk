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
pthread_t drova, mods1, mods2, shot[3], Dakka1, Dakka2;
static char *player1;
static char *player2;
static int work_flag = 1;
static int game_flag1 = 0, game_flag2 = 0;
struct tank
{
	int left_tr[6];	
	int right_tr[6]; 
	int turret[2];
	int gun[2];	
	int f_gun;
	struct bullet
	{
		int x;
		int y;
		int numb;
		int act;
		int inc[2];
	} bl[3];
	int ch;
	int status[3];
	int yyyy;
};
struct tank *first, *second;
void *udp_server();
void *bullet_maker(void *args);
void *draw();
void *bullet_control(void *args);
void *tank_maker(void *args);

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
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
	if (sock < 0)
	{
		perror("sock");
		return -1;
	}
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof(yes));
	if (ret == -1)
	{
		perror("setsockopt");
		return 0;
	}

	addr_len = sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, addr_len);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	addr.sin_port = htons(PORT);

	initscr();
	first = (struct tank *)malloc(sizeof(struct tank)), second = (struct tank *)malloc(sizeof(struct tank));
	first->gun[0] = 2, first->gun[1] = 1, first->turret[0] = first->gun[0] - 1, first->turret[1] = first->gun[1], first->f_gun = 1;
	second->gun[0] = LINES - 3, second->gun[1] = COLS - 2, second->turret[0] = second->gun[0] + 1, second->turret[1] = second->gun[1], second->f_gun = 1;
	for (int i = 0; i < 3; i++)
	{
		first->bl[i].numb = 0;
		first->bl[i].act = 0;
		first->bl[i].x = first->gun[1];
		first->bl[i].y = first->gun[0] + 1;
		first->left_tr[i + 3] = first->turret[1] - 1;
		first->left_tr[i] = i;
		first->right_tr[i + 3] = first->turret[1] + 1;
		first->right_tr[i] = first->left_tr[i];
		first->status[i] = 0;

		second->bl[i].numb = 0;
		second->bl[i].act = 0;
		second->bl[i].x = second->gun[1];
		second->bl[i].y = second->gun[0] - 1;
		second->left_tr[i + 3] = second->turret[1] - 1;
		second->left_tr[i] = second->turret[0] - 1 + i;
		second->right_tr[i + 3] = second->turret[1] + 1;
		second->right_tr[i] = second->left_tr[i];
		second->status[i] = 0;
	}
	pthread_create(&drova, NULL, draw, NULL);
	pthread_create(&Dakka1, NULL, bullet_control, first);
	pthread_create(&Dakka2, NULL, bullet_control, second);
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	int ch;
	while (work_flag)
	{
		ch = getch();
		sendto(sock, &ch, sizeof(ch), 0, (struct sockaddr *)&addr, addr_len);
		if (game_flag1 == -1 || game_flag2 == -1)
			break;
	}
	close(sock);
	endwin();
	return 0;
}

void *udp_server()
{
	int sock, addr_len, ret;
	int buffer;
	fd_set readfd;
	struct sockaddr_in addr;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (0 > sock)
	{
		perror("socket");
		return NULL;
	}
	addr_len = sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, addr_len);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(PORT);
	addr_len = sizeof(addr);
	if (bind(sock, (struct sockaddr *)&addr, addr_len) < 0)
	{
		perror("bind");
		close(sock);
		return NULL;
	}
	while (work_flag)
	{
		FD_ZERO(&readfd);
		FD_SET(sock, &readfd);
		if (game_flag1 == -1 || game_flag2 == -1)
			break;
		ret = select(sock + 1, &readfd, NULL, NULL, 0);
		if (ret > 0)
		{
			if (FD_ISSET(sock, &readfd))
			{
				recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_len);
				if (inet_addr(player1) == addr.sin_addr.s_addr)
				{
					first->ch = buffer;
					if (first->ch == 's')
						game_flag1 = 1;
					pthread_create(&mods1, NULL, tank_maker, first);
				}
				if (inet_addr(player2) == addr.sin_addr.s_addr)
				{
					second->ch = buffer;
					if (second->ch == 's')
						game_flag2 = 1;
					pthread_create(&mods2, NULL, tank_maker, second);
				}
				buffer = 0;
			}
		}
	}
	close(sock);
	return (void *)NULL;
}

void *draw()
{
	while (1)
	{
		clear();
		mvaddch(first->turret[0], first->turret[1], 'o');
		mvaddch(second->turret[0], second->turret[1], 'o');
		for (int i = 0; i < 3; i++)
		{
			mvaddch(first->left_tr[i], first->left_tr[i + 3], '#');
			mvaddch(first->right_tr[i], first->right_tr[i + 3], '#');
			mvaddch(second->left_tr[i], second->left_tr[i + 3], '#');
			mvaddch(second->right_tr[i], second->right_tr[i + 3], '#');
		}
		switch (first->f_gun)
		{
		case 1:
			mvaddch(first->gun[0], first->gun[1], '|');
			break;
		case 2:
			mvaddch(first->gun[0], first->gun[1], '-');
			break;
		case 3:
			mvaddch(first->gun[0], first->gun[1], '\\');
			break;
		case 4:
			mvaddch(first->gun[0], first->gun[1], '/');
			break;
		}
		for (int j = 0; j < 3; j++)
		{
			if (first->status[j] == 1 || first->status[j] == 3)
			{
				mvaddch(first->bl[j].y, first->bl[j].x, '*');
				mvaddch(first->bl[j].y - first->bl[j].inc[0], first->bl[j].x - first->bl[j].inc[1], ' ');
				for (int k = 0; k < 3; k++)
				{
					if ((second->right_tr[k] == first->bl[j].y && second->right_tr[k + 3] == first->bl[j].x) || (second->left_tr[k] == first->bl[j].y && second->left_tr[k + 3] == first->bl[j].x) || (second->turret[0] == first->bl[j].y && second->turret[1] == first->bl[j].x) || (second->gun[0] == first->bl[j].y && second->gun[1] == first->bl[j].x))
					{
						game_flag2 = -1;
						mvaddstr(LINES / 2, COLS / 2 - 19, "Player 1 wins! Press any key to quit!");
						getch();
						pthread_exit(0);
					}
				}
			}
		}
		switch (second->f_gun)
		{
		case 1:
			mvaddch(second->gun[0], second->gun[1], '|');
			break;
		case 2:
			mvaddch(second->gun[0], second->gun[1], '-');
			break;
		case 3:
			mvaddch(second->gun[0], second->gun[1], '\\');
			break;
		case 4:
			mvaddch(second->gun[0], second->gun[1], '/');
			break;
		}
		for (int j = 0; j < 3; j++)
		{
			if (second->status[j] == 1 || second->status[j] == 3)
			{
				mvaddch(second->bl[j].y, second->bl[j].x, '*');
				mvaddch(second->bl[j].y - second->bl[j].inc[0], second->bl[j].x - second->bl[j].inc[1], ' ');
				for (int k = 0; k < 3; k++)
				{
					if ((first->right_tr[k] == second->bl[j].y && first->right_tr[k + 3] == second->bl[j].x) || (first->left_tr[k] == second->bl[j].y && first->left_tr[k + 3] == second->bl[j].x) || (first->turret[0] == second->bl[j].y && first->turret[1] == second->bl[j].x) || (first->gun[0] == second->bl[j].y && first->gun[1] == second->bl[j].x))
					{
						game_flag1 = -1;
						mvaddstr(LINES / 2, COLS / 2 - 19, "Player 2 wins! Press any key to quit!");
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

void *tank_maker(void *args)
{
	struct tank *tankinfo = (struct tank *)args;
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);
	if (tankinfo->ch == 'w')
	{
		tankinfo->gun[1] = tankinfo->turret[1];
		tankinfo->gun[0] = tankinfo->turret[0] - 1;
		tankinfo->f_gun = 1;
	}
	if (tankinfo->ch == 'x')
	{
		tankinfo->gun[1] = tankinfo->turret[1];
		tankinfo->gun[0] = tankinfo->turret[0] + 1;
		tankinfo->f_gun = 1; /* position | */
	}
	if (tankinfo->ch == 'a')
	{
		tankinfo->gun[1] = tankinfo->turret[1] - 1;
		tankinfo->gun[0] = tankinfo->turret[0];
		tankinfo->f_gun = 2;
	}
	if (tankinfo->ch == 'd')
	{
		tankinfo->gun[1] = tankinfo->turret[1] + 1;
		tankinfo->gun[0] = tankinfo->turret[0];
		tankinfo->f_gun = 2; /* position - */
	}
	if (tankinfo->ch == 'q')
	{
		tankinfo->gun[1] = tankinfo->turret[1] - 1;
		tankinfo->gun[0] = tankinfo->turret[0] - 1;
		tankinfo->f_gun = 3;
	}
	if (tankinfo->ch == 'e')
	{
		tankinfo->gun[1] = tankinfo->turret[1] + 1;
		tankinfo->gun[0] = tankinfo->turret[0] - 1;
		tankinfo->f_gun = 4;
	}
	if (tankinfo->ch == 'z')
	{
		tankinfo->gun[1] = tankinfo->turret[1] - 1;
		tankinfo->gun[0] = tankinfo->turret[0] + 1;
		tankinfo->f_gun = 4; /* position / */
	}
	if (tankinfo->ch == 'c')
	{
		tankinfo->gun[1] = tankinfo->turret[1] + 1;
		tankinfo->gun[0] = tankinfo->turret[0] + 1;
		tankinfo->f_gun = 3; /* position \ */
	}
	if (tankinfo->turret[0] > 1)
	{
		if (tankinfo->ch == KEY_UP || tankinfo->ch == 3)
		{
			tankinfo->turret[0]--;
			tankinfo->gun[0]--;
			for (int i = 0; i < 3; i++)
			{
				tankinfo->left_tr[i + 3] = tankinfo->turret[1] - 1;
				tankinfo->left_tr[i] = tankinfo->turret[0] - 1 + i;
				tankinfo->right_tr[i + 3] = tankinfo->turret[1] + 1;
				tankinfo->right_tr[i] = tankinfo->left_tr[i];
			}
		}
	}
	if (tankinfo->turret[0] < LINES - 2)
	{
		if (tankinfo->ch == KEY_DOWN || tankinfo->ch == 2)
		{
			tankinfo->turret[0]++;
			tankinfo->gun[0]++;
			for (int i = 0; i < 3; i++)
			{
				tankinfo->left_tr[i + 3] = tankinfo->turret[1] - 1;
				tankinfo->left_tr[i] = tankinfo->turret[0] - 1 + i;
				tankinfo->right_tr[i + 3] = tankinfo->turret[1] + 1;
				tankinfo->right_tr[i] = tankinfo->left_tr[i];
			}
		}
	}
	if (tankinfo->turret[1] > 1)
	{
		if (tankinfo->ch == KEY_LEFT || tankinfo->ch == 4)
		{
			tankinfo->turret[1]--;
			tankinfo->gun[1]--;
			for (int i = 0; i < 3; i++)
			{
				tankinfo->left_tr[i + 3] = tankinfo->turret[1] - 1 + i;
				tankinfo->left_tr[i] = tankinfo->turret[0] + 1;
				tankinfo->right_tr[i + 3] = tankinfo->left_tr[i + 3];
				tankinfo->right_tr[i] = tankinfo->turret[0] - 1;
			}
		}
	}
	if (tankinfo->turret[1] < COLS - 2)
	{
		if (tankinfo->ch == KEY_RIGHT || tankinfo->ch == 5)
		{
			tankinfo->turret[1]++;
			tankinfo->gun[1]++;
			for (int i = 0; i < 3; i++)
			{
				tankinfo->left_tr[i + 3] = tankinfo->turret[1] - 1 + i;
				tankinfo->left_tr[i] = tankinfo->turret[0] + 1;
				tankinfo->right_tr[i + 3] = tankinfo->left_tr[i + 3];
				tankinfo->right_tr[i] = tankinfo->turret[0] - 1;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	pthread_exit(0);
}
void *bullet_maker(void *args)
{
	struct tank *tankinfo = (struct tank *)args;
	int i, flag = 0;
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);
	for (i = 0; i < 3; i++)
	{
		if (tankinfo->status[i] == 1)
		{
			tankinfo->status[i] = 3; //mine
			tankinfo->bl[i].act = 1;
			tankinfo->bl[i].inc[1] = tankinfo->gun[1] - tankinfo->turret[1], tankinfo->bl[i].inc[0] = tankinfo->gun[0] - tankinfo->turret[0];
			tankinfo->bl[i].x = tankinfo->gun[1];
			tankinfo->bl[i].y = tankinfo->gun[0];
			tankinfo->bl[i].x += tankinfo->bl[i].inc[1];
			tankinfo->bl[i].y += tankinfo->bl[i].inc[0];
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	while (flag != 1)
	{
		tankinfo->bl[i].x += tankinfo->bl[i].inc[1];
		tankinfo->bl[i].y += tankinfo->bl[i].inc[0];
		if (game_flag1 == -1 || game_flag2 == -1)
			pthread_exit(0);
		if (tankinfo->bl[i].x == 0 || tankinfo->bl[i].x == COLS || tankinfo->bl[i].y == 0 || tankinfo->bl[i].y == LINES)
		{
			tankinfo->bl[i].act = 0;
			tankinfo->status[tankinfo->bl[i].numb] = 2;
			flag = 1;
		}
		usleep(10000);
	}
}
void *bullet_control(void *args)
{
	struct tank *tankinfo = (struct tank *)args;
	while (1)
	{
		if (game_flag1 == -1 || game_flag2 == -1)
			pthread_exit(0);
		for (int i = 0; i < 3; i++)
		{
			if (tankinfo->status[i] == 2)
			{
				pthread_join(shot[i], NULL);
				tankinfo->bl[i].numb = 0;
				tankinfo->bl[i].act = 0;
				tankinfo->bl[i].x = tankinfo->gun[1];
				tankinfo->bl[i].y = tankinfo->gun[0] - 1;
				tankinfo->status[i] = 0;
			}
		}
		if (tankinfo->ch == ' ' && game_flag1 == 1 && game_flag2 == 1)
		{
			tankinfo->ch = 'v';
			for (int i = 0; i < 3; i++)
			{
				if (tankinfo->status[i] == 0)
				{
					tankinfo->status[i] = 1;
					tankinfo->yyyy = i;
					tankinfo->bl[i].numb = i;
					pthread_create(&shot[tankinfo->yyyy], NULL, bullet_maker, tankinfo);
					break;
				}
			}
		}
	}
}