#include <ncurses.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define PORT 9998


typedef enum { HOR, VERT } TANK_DIR;

typedef struct {
	int x;
	int y;
} Pos2;


typedef struct Node {
    struct Node* prev;
    struct Node* next;

    in_addr_t ip;
	Pos2 pos;
	TANK_DIR tank_dir;
	int gun_dir;
} Node;

typedef struct {
    struct Node* first;
} List;


typedef struct Bullet {
	struct Bullet* next;

	Pos2 pos;
	Pos2 vel;
} Bullet;

typedef struct {
	Bullet* first;
} BulletsList;

const char TANK_VERTICAL[9] = {'#',' ','#','#','*','#','#',' ','#'};
const char TANK_HORIZONTAL[9] = {'#','#','#',' ','*',' ','#','#','#'};
const int PLAYERS = 2;
const int BUFFER_SIZE = sizeof(Node)*PLAYERS;


int server_running = 1;
int current_players = 0;

List players = { NULL };
BulletsList bullets = { NULL };
char buffer[1024]; // should be BUFFER_SIZE


void make_tank(char* tank, TANK_DIR dir, int gun_dir);
void draw_tank(char* tank, Pos2 pos); 
void* udp_server(void* none);
void get_tank_pos_by_count(Pos2* pos, int c);


void get_tank_pos_by_count(Pos2* pos, int c)
{
	if (c == 1)
	{
		pos->x = 0;
		pos->y = 0;
	}
	else if (c == 2)
	{
		pos->x = COLS - 3;
		pos->y = LINES - 3;
	}
	else
	{
		pos->x = ((c+1)%2 + 1)*COLS/3;
		pos->y = ((c+1)/2    )*LINES/3;
	}
}

void shoot_bullet(int x_from, int y_from, int vel_x, int vel_y)
{
	Bullet* new_bullet = (Bullet*)malloc(sizeof(Bullet));
	new_bullet->next = NULL;
	new_bullet->pos.x = x_from;
	new_bullet->pos.y = y_from;
	new_bullet->vel.x = vel_x;
	new_bullet->vel.y = vel_y;


	if (bullets.first == NULL)
	{
		bullets.first = new_bullet;
		return;
	}
	Bullet* bullet = bullets.first;
	for (; bullet->next != NULL; bullet = bullet->next);
	bullet->next = new_bullet;
}

void remove_bullet(Bullet* cur)
{
	if (bullets.first == NULL)
		return;

	if (bullets.first == cur)
	{
		if (cur->next != NULL)
			bullets.first = cur->next; 
		else 
		{
			bullets.first = NULL;
		}
		return;
	}

	Bullet* prev = bullets.first;
	for (; prev->next != cur && prev->next != NULL; prev = prev->next);

	if (prev->next != NULL)
		prev->next = cur->next;
}

Node* add_new_back(List* list, in_addr_t ip)
{
	Node* last = list->first;
	if (last != NULL)
		for(; last->next != NULL; last = last->next);

    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->ip = ip;

	current_players += 1;
	get_tank_pos_by_count(&(new_node->pos), current_players);

	new_node->tank_dir = VERT;
	new_node->gun_dir = 1;

	if(list->first)
	{
		new_node->prev = last;
    	new_node->next = NULL;
		last->next = new_node;
	}
	else
	{
		list->first = new_node;
		new_node->prev = NULL;
    	new_node->next = NULL;
	}
	return new_node;
}

Node* get_in_list(List* list, in_addr_t ip)
{
	Node* ret = NULL;
    for (Node* node = list->first; node != NULL; node = node->next)
        if (node->ip == ip)
            return node;
    return ret;
}

void make_first(List* list, Node* node)
{
	if (list->first == NULL || list->first == node)
		return;

	Node* prev = list->first;
	for (; prev->next != node && prev->next != NULL; prev = prev->next);

	if (prev->next != NULL)
		prev->next = node->next;

	node->next = list->first;
	list->first = node;
}

void draw_tank(char* tank, Pos2 pos)
{
	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			mvaddch(pos.y + y, pos.x + x, tank[x+3*y]);
		}
	}
} 

void make_tank(char* tank, TANK_DIR dir, int gun_dir)
{
	if(dir == VERT)
	{
		for(int i = 0; i < 9; i++)
		{
			tank[i] = TANK_VERTICAL[i];
		}
	}
	else
	{
		for(int i = 0; i < 9; i++)
		{
			tank[i] = TANK_HORIZONTAL[i];
		}
	}

	switch (gun_dir)
	{
	case 0:
		tank[0]= '\\';
		break;
	case 1:
		tank[1]= '|';
		break;
	case 2:
		tank[2]= '/';
		break;
	case 3:
		tank[5]= '-';
		break;
	case 4:
		tank[8]= '\\';
		break;
	case 5:
		tank[7]= '|';
		break;
	case 6:
		tank[6]= '/';
		break;
	case 7:
		tank[3]= '-';
		break;
	
	default:
		break;
	}
	
}

// void fill_buffer_list_players()
// {
// 	int offset = 0;
// 	memset(buffer, 0, sizeof(buffer));
// 	memcpy(buffer, &current_players, sizeof(current_players));
// 	offset += sizeof(current_players);

// 	Node* node = players.first;
// 	for(; node != NULL; node = node->next)
// 	{
// 		memcpy(buffer+offset, node + sizeof(Node*)*2, sizeof(Node) - sizeof(Node*)*2);
// 		offset += sizeof(Node) - sizeof(Node*)*2;
// 	}
// }

// void apply_buffer_to_player_list()
// {
// 	int offset = 0;
// 	memcpy(&current_players, buffer, sizeof(current_players));
// 	offset += sizeof(current_players);
// 	for(int i = 0; i < current_players; i++)
// 	{
// 		Node* new = add_new_back(&players, 0);
// 		memcpy(new + sizeof(Node*)*2, buffer+offset, sizeof(Node) - sizeof(Node*)*2);
// 		offset += sizeof(Node) - sizeof(Node*)*2;
// 	}
// }

void process_input(Node* player, int ch)
{
	switch (ch)
	{
	case 'q':
		player->gun_dir = 0;
		break;
	case 'w':
		player->gun_dir = 1;
		break;
	case 'e':
		player->gun_dir = 2;
		break;
	case 'd':
		player->gun_dir = 3;
		break;
	case 'c':
		player->gun_dir = 4;
		break;
	case 'x':
		player->gun_dir = 5;
		break;
	case 'z':
		player->gun_dir = 6;
		break;
	case 'a':
		player->gun_dir = 7;
		break;
	default:
		break;
	}

	if(ch == ' ')
	{
		int shoot_pos_x, shoot_pos_y;
		int shoot_vel_x, shoot_vel_y;
		switch (player->gun_dir)
		{
		case 0:
			shoot_pos_x = -1;
			shoot_pos_y = -1;
			shoot_vel_x = -1;
			shoot_vel_y = -1;
			break;
		case 1:
			shoot_pos_x = 1;
			shoot_pos_y = -1;
			shoot_vel_x = 0;
			shoot_vel_y = -1;
			break;
		case 2:
			shoot_pos_x = 3;
			shoot_pos_y = -1;
			shoot_vel_x = 1;
			shoot_vel_y = -1;
			break;
		case 3:
			shoot_pos_x = 3;
			shoot_pos_y = 1;
			shoot_vel_x = 1;
			shoot_vel_y = 0;
			break;
		case 4:
			shoot_pos_x = 3;
			shoot_pos_y = 3;
			shoot_vel_x = 1;
			shoot_vel_y = 1;
			break;
		case 5:
			shoot_pos_x = 1;
			shoot_pos_y = 3;
			shoot_vel_x = 0;
			shoot_vel_y = 1;
			break;
		case 6:
			shoot_pos_x = -1;
			shoot_pos_y = 3;
			shoot_vel_x = -1;
			shoot_vel_y = 1;
			break;
		case 7:
			shoot_pos_x = -1;
			shoot_pos_y = 1;
			shoot_vel_x = -1;
			shoot_vel_y = 0;
			break;
		default:
			break;
		}
		shoot_bullet(player->pos.x + shoot_pos_x,
					player->pos.y + shoot_pos_y,
					shoot_vel_x,
					shoot_vel_y);
	}

	if(ch == KEY_UP)
	{
		player->tank_dir = VERT;

		player->pos.y -= 1;
		if(player->pos.y < 0)
			player->pos.y = 0;
	}
	if(ch == KEY_DOWN)
	{
		player->tank_dir = VERT;

		player->pos.y += 1;
		if(player->pos.y > LINES - 1 - 2)
			player->pos.y = LINES - 1 - 2;
	}
	if(ch == KEY_RIGHT)
	{
		player->tank_dir = HOR;

		player->pos.x += 1;
		if(player->pos.x > COLS - 1 - 2)
			player->pos.x = COLS - 1 - 2;
	}
	if(ch == KEY_LEFT)
	{
		player->tank_dir = HOR;

		player->pos.x -= 1;
		if(player->pos.x < 0)
			player->pos.x = 0;
	}
}

pthread_mutex_t mutex;

int tankDied = 0;
WINDOW* window;

void* udp_server(void* none)
{
    int sock, addr_len, count, ret;
    fd_set readfd;
    struct sockaddr_in addr;

	int game_started = 0;

	int opt = 3; 

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if ( 0 > sock ) 
    {
        perror("socket");
        return NULL;
    }

	setsockopt(sock, SOL_SOCKET, SO_RCVLOWAT, &opt, sizeof(opt)); 
    
    addr_len = sizeof(struct sockaddr_in);
    memset((void*)&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr*)&addr, addr_len) < 0)
    {
        perror("bind");
        close(sock);
        return NULL;
    }

	pthread_mutex_lock(&mutex);
	for (Node* node = players.first; node != NULL; node = node->next)
	{
		char tank[9];
		make_tank(tank, node->tank_dir, node->gun_dir);
		draw_tank(tank, node->pos);
	}

	mvprintw(LINES / 2, (COLS - 19) / 2, "PRESS 'S' TO START");
	refresh();
	pthread_mutex_unlock(&mutex);
    
    while (server_running)
    {
		struct timeval timeout;
		timeout.tv_sec = 0; 
    	timeout.tv_usec = 10000; 

		FD_ZERO(&readfd);
        FD_SET(sock, &readfd);

		int ch;
        ret = select(sock+1, &readfd, NULL, NULL, &timeout);
        if (ret > 0)
        {
            if (FD_ISSET(sock, &readfd))
            {
                count = recvfrom(sock, &ch, sizeof(ch), 0, (struct sockaddr*)&addr, &addr_len);
				Node* player;
				if (player = get_in_list(&players, addr.sin_addr.s_addr))
				{
					if (ch == 's' && game_started != 1)
					{
						make_first(&players, player);

						current_players = 0;

						for (Node* node = players.first; node != NULL; node = node->next)
						{
							current_players += 1;
							get_tank_pos_by_count(&(node->pos), current_players);
						}

						game_started = 1;
					}

					if (game_started == 1)
						process_input(player, ch);
				}
            }
        }

		if (current_players >= PLAYERS && game_started) 
		{
			pthread_mutex_lock(&mutex);
			clear();
			for (Bullet* bullet = bullets.first; bullet != NULL; bullet = bullet->next)
			{
				Pos2* bpos = &(bullet->pos);
				bpos->x += bullet->vel.x;
				bpos->y += bullet->vel.y;

				if (bpos->x < 1 || bpos->y < 1 || bpos->x > COLS-1 || bpos->y > LINES-1)
				{
					remove_bullet(bullet);
					continue;
				}

				mvaddch(bpos->y, bpos->x, '*');
			}

			int currentTank = 1;
			for (Node* node = players.first; node != NULL; node = node->next)
			{
				char tank[9];
				make_tank(tank, node->tank_dir, node->gun_dir);
				wattron(window, COLOR_PAIR(currentTank));
				draw_tank(tank, node->pos);
				wattroff(window, COLOR_PAIR(currentTank));

				for (Bullet* bullet = bullets.first; bullet != NULL; bullet = bullet->next)
				{
					Pos2* bpos = &(bullet->pos);
					if ( bpos->x >= node->pos.x && bpos->x <= node->pos.x + 2 &&
						 bpos->y >= node->pos.y && bpos->y <= node->pos.y + 2 )
					{
						tankDied = currentTank;
						break;
					}
				}

				if (tankDied != 0)
				{
					server_running = 0;
					return (void*)NULL;  
				}

				currentTank += 1;
			}
			refresh();
			pthread_mutex_unlock(&mutex);
		}
    }
    close(sock);
    return (void*)NULL;  
}

int get_input()
{
	pthread_mutex_lock(&mutex);
	int ans = getch();
	pthread_mutex_unlock(&mutex);
	usleep(100);
	return ans;
}

int main(int argc, char**argv)
{
	int sock, yes = 1;
    pthread_t pid;
    struct sockaddr_in addr;
    int addr_len;
    int count;
    fd_set readfd;
	int ret = 0;

	players.first = NULL;

	pthread_mutex_init(&mutex, 0);

	if (argc != 3)
	{
		printf("Usage: %s ip1 ip2\n", argv[0]);
		return -1;
	}

	//printf("Players: %u %u\n", inet_addr(argv[1]), inet_addr(argv[2]));

	window = initscr();

	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	timeout(1);
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_BLACK);

	add_new_back(&players, inet_addr(argv[1]));
	add_new_back(&players, inet_addr(argv[2]));

	pthread_create(&pid, NULL, udp_server, NULL);
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) 
    {
        perror("sock");
        return -1;
    }
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
    if (ret == -1) 
    {
        perror("setsockopt");
        return 0;
    }

    addr_len = sizeof(struct sockaddr_in);
    memset((void*)&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(PORT);


	int ch;

    while (server_running && (ch = get_input()) != 'p') 
    {
		if( ch > 0 )
		{
        	sendto(sock, &ch, sizeof(ch), 0, (struct sockaddr*)&addr, addr_len);
		}
    }

	server_running = 0;

	endwin();
	pthread_mutex_destroy(&mutex);
	pthread_join(pid, NULL);
    close(sock);

	if (tankDied != 0)
		printf("\tPLAYER %d LOST!!!!!!\n\n", tankDied);
    return 0;
}
		