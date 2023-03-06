#include <ncurses.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>


#ifndef PORT
#define PORT 9995
#endif

typedef struct {
    int x;
    int y;
} coordinats;


typedef struct Node {
    struct Node *prev;
    struct Node *next;

    in_addr_t ip;
    coordinats position;
    int inclination;
    int currentTrain;
} Node;

typedef struct {
    struct Node *first;
} List;


typedef struct Bullet {
    struct Bullet *next;

    coordinats pos;
    coordinats vel;
} Bullet;

typedef struct {
    Bullet *first;
} BulletsList;

int PLAYERS = 2;

int server_running = 1;
int current_players = 0;

List players = {NULL};
BulletsList bullets = {NULL};

void get_tank_pos_by_count(coordinats *position, int c) {
    switch (c) {
        case 1:
            position->x = 0;
            position->y = 0;
            break;
        case 2:
            position->x = COLS - 3;
            position->y = LINES - 3;
            break;
        default:
            break;
    }
}

void shoot_bullet(int x_from, int y_from, int vel_x, int vel_y) {
    Bullet *new_bullet = (Bullet *) malloc(sizeof(Bullet));
    new_bullet->next = NULL;
    new_bullet->pos.x = x_from;
    new_bullet->vel.x = vel_x;
    new_bullet->pos.y = y_from;
    new_bullet->vel.y = vel_y;


    if (bullets.first == NULL) {
        bullets.first = new_bullet;
        return;
    }
    Bullet *bullet = bullets.first;
    for (; bullet->next != NULL; bullet = bullet->next);
    bullet->next = new_bullet;
}

void remove_bullet(Bullet *cur) {
    if (bullets.first == NULL)
        return;

    if (bullets.first == cur) {
        if (cur->next != NULL)
            bullets.first = cur->next;
        else {
            bullets.first = NULL;
        }
        return;
    }

    Bullet *prev = bullets.first;
    for (; prev->next != cur && prev->next != NULL; prev = prev->next);

    if (prev->next != NULL)
        prev->next = cur->next;
}

Node *add_new_back(List *list, in_addr_t ip) {
    Node *last = list->first;
    if (last != NULL)
        for (; last->next != NULL; last = last->next);

    Node *new_node = (Node *) malloc(sizeof(Node));
    new_node->ip = ip;

    current_players += 1;
    get_tank_pos_by_count(&(new_node->position), current_players);

    new_node->inclination = 0;
    new_node->currentTrain = 1;

    if (list->first) {
        new_node->prev = last;
        new_node->next = NULL;
        last->next = new_node;
    } else {
        list->first = new_node;
        new_node->prev = NULL;
        new_node->next = NULL;
    }
    return new_node;
}

Node *get_in_list(List *list, in_addr_t ip) {
    Node *ret = NULL;
    for (Node *node = list->first; node != NULL; node = node->next)
        if (node->ip == ip)
            return node;
    return ret;
}

void make_first(List *list, Node *node) {
    if (list->first == NULL || list->first == node)
        return;

    Node *prev = list->first;
    for (; prev->next != node && prev->next != NULL; prev = prev->next);

    if (prev->next != NULL)
        prev->next = node->next;

    node->next = list->first;
    list->first = node;
}

void process_input(Node *player, int ch) {
    switch (ch) {
        case 'q':
            player->currentTrain = 1;
            break;
        case 'w':
            player->currentTrain = 2;
            break;
        case 'e':
            player->currentTrain = 3;
            break;
        case 'd':
            player->currentTrain = 4;
            break;
        case 'c':
            player->currentTrain = 5;
            break;
        case 'x':
            player->currentTrain = 6;
            break;
        case 'z':
            player->currentTrain = 7;
            break;
        case 'a':
            player->currentTrain = 8;
            break;
        default:
            break;
    }

    if (ch == ' ') {
        int shoot_pos_x, shoot_pos_y;
        int shoot_vel_x, shoot_vel_y;
        switch (player->currentTrain) {
            case 1:
                shoot_pos_x = -1;
                shoot_pos_y = -1;
                shoot_vel_x = -1;
                shoot_vel_y = -1;
                break;
            case 2:
                shoot_pos_x = 1;
                shoot_pos_y = -1;
                shoot_vel_x = 0;
                shoot_vel_y = -1;
                break;
            case 3:
                shoot_pos_x = 3;
                shoot_pos_y = -1;
                shoot_vel_x = 1;
                shoot_vel_y = -1;
                break;
            case 4:
                shoot_pos_x = 3;
                shoot_pos_y = 1;
                shoot_vel_x = 1;
                shoot_vel_y = 0;
                break;
            case 5:
                shoot_pos_x = 3;
                shoot_pos_y = 3;
                shoot_vel_x = 1;
                shoot_vel_y = 1;
                break;
            case 6:
                shoot_pos_x = 1;
                shoot_pos_y = 3;
                shoot_vel_x = 0;
                shoot_vel_y = 1;
                break;
            case 7:
                shoot_pos_x = -1;
                shoot_pos_y = 3;
                shoot_vel_x = -1;
                shoot_vel_y = 1;
                break;
            case 8:
                shoot_pos_x = -1;
                shoot_pos_y = 1;
                shoot_vel_x = -1;
                shoot_vel_y = 0;
                break;
            default:
                break;
        }
        shoot_bullet(player->position.x + shoot_pos_x, player->position.y + shoot_pos_y, shoot_vel_x, shoot_vel_y);
    }

    if (ch == KEY_UP) {
        player->inclination = 0;

        player->position.y -= 1;
        if (player->position.y < 0)
            player->position.y = 0;
    }
    if (ch == KEY_DOWN) {
        player->inclination = 0;

        player->position.y += 1;
        if (player->position.y > LINES - 3)
            player->position.y = LINES - 3;
    }
    if (ch == KEY_RIGHT) {
        player->inclination = 1;

        player->position.x += 1;
        if (player->position.x > COLS - 3)
            player->position.x = COLS - 3;
    }
    if (ch == KEY_LEFT) {
        player->inclination = 1;

        player->position.x -= 1;
        if (player->position.x < 0)
            player->position.x = 0;
    }
}

void draw_tank(char *tank, coordinats pos) {
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            mvaddch(pos.y + y, pos.x + x, tank[x + 3 * y]);
        }
    }
}

void make_tank(char *tank, int a, int currentTrain) {
    for (int i = 0; i < 9; i++) {
        tank[i] = '*';
    }
    if (a == 0) {
        tank[1] = ' ';
        tank[7] = ' ';
    } else {
        tank[3] = ' ';
        tank[5] = ' ';
    }

    switch (currentTrain) {
        case 1:
            tank[0] = '\\';
            break;
        case 2:
            tank[1] = '|';
            break;
        case 3:
            tank[2] = '/';
            break;
        case 4:
            tank[5] = '-';
            break;
        case 5:
            tank[8] = '\\';
            break;
        case 6:
            tank[7] = '|';
            break;
        case 7:
            tank[6] = '/';
            break;
        case 8:
            tank[3] = '-';
            break;

        default:
            break;
    }

}


pthread_mutex_t mutex;

int whoWin = 0;

void *udp_server(void *none) {
    int sock, addr_len, count, ret;
    fd_set readfd;
    struct sockaddr_in addr;

    int game_started = 0;

    int opt = 3;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (0 > sock) {
        perror("socket");
        return NULL;
    }

    setsockopt(sock, SOL_SOCKET, SO_RCVLOWAT, &opt, sizeof(opt));

    addr_len = sizeof(struct sockaddr_in);
    memset((void *) &addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *) &addr, addr_len) < 0) {
        perror("bind");
        close(sock);
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    for (Node *node = players.first; node != NULL; node = node->next) {
        char tank[9];
        make_tank(tank, node->inclination, node->currentTrain);
        draw_tank(tank, node->position);
    }

   // mvprintw(LINES / 2, (COLS - 19) / 2, "PRESS 'S' TO START");
    refresh();
    pthread_mutex_unlock(&mutex);

    while (server_running) {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;

        FD_ZERO(&readfd);
        FD_SET(sock, &readfd);

        int ch;
        ret = select(sock + 1, &readfd, NULL, NULL, &timeout);
        if (ret < 0) {
            perror("select() failed");
            return NULL;
            //exit(-1);
        }
        if (ret > 0) {
            if (FD_ISSET(sock, &readfd)) {
                count = recvfrom(sock, &ch, sizeof(ch), 0, (struct sockaddr *) &addr, &addr_len);
                Node *player;
                if (player = get_in_list(&players, addr.sin_addr.s_addr)) {
                    if (ch == 's' && game_started != 1) {
                        make_first(&players, player);

                        current_players = 0;

                        for (Node *node = players.first; node != NULL; node = node->next) {
                            current_players += 1;
                            get_tank_pos_by_count(&(node->position), current_players);
                        }

                        game_started = 1;
                    }

                    if (game_started == 1)
                        process_input(player, ch);
                }
            }
        }

        if (current_players >= PLAYERS && game_started) {
            pthread_mutex_lock(&mutex);
            clear();
            for (Bullet *bullet = bullets.first; bullet != NULL; bullet = bullet->next) {
                coordinats *bulletinspace = &(bullet->pos);
                bulletinspace->x += bullet->vel.x;
                bulletinspace->y += bullet->vel.y;

                if (bulletinspace->x < 1 || bulletinspace->y < 1 || bulletinspace->x > COLS - 1 ||
                    bulletinspace->y > LINES - 1) {
                    remove_bullet(bullet);
                    continue;
                }

                mvaddch(bulletinspace->y, bulletinspace->x, '*');
            }

            int currentTank = 1;
            for (Node *node = players.first; node != NULL; node = node->next) {
                char tank[9];
                make_tank(tank, node->inclination, node->currentTrain);
                draw_tank(tank, node->position);


                for (Bullet *bullet = bullets.first; bullet != NULL; bullet = bullet->next) {
                    coordinats *bulletinspace = &(bullet->pos);
                    if (bulletinspace->x >= node->position.x && bulletinspace->x <= node->position.x + 2 &&
                        bulletinspace->y >= node->position.y && bulletinspace->y <= node->position.y + 2) {
                        whoWin = currentTank;
                        break;
                    }
                }

                if (whoWin != 0) {
                    server_running = 0;
                    return (void *) NULL;
                }

                currentTank += 1;
            }
            refresh();
            pthread_mutex_unlock(&mutex);
        }
    }
    close(sock);
    return (void *) NULL;
}

int get_input() {
    pthread_mutex_lock(&mutex);
    int a = getch();
    pthread_mutex_unlock(&mutex);
    usleep(100);
    return a;
}

int main(int argc, char **argv) {
    int sock, yes = 1;
    pthread_t pid;
    struct sockaddr_in addr;
    int addr_len;
    int ret = 0;

    players.first = NULL;

    pthread_mutex_init(&mutex, 0);

    if (argc != 3) {
        printf("Usage: %s ip1 ip2\n", argv[0]);
        return -1;
    }

    //window =
    initscr();

    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(1);

    add_new_back(&players, inet_addr(argv[1]));
    add_new_back(&players, inet_addr(argv[2]));

    pthread_create(&pid, NULL, udp_server, NULL);
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("sock");
        return -1;
    }
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &yes, sizeof(yes));
    if (ret == -1) {
        perror("setsockopt");
        return 0;
    }

    addr_len = sizeof(struct sockaddr_in);
    memset((void *) &addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(PORT);

    int ch;
    //const int width = 80;
    // const int height = 40;
    // WINDOW *win=newwin(height, width, 0, 0);
    // box(win, 0, 0);
    // wrefresh(win);

    while (server_running && (ch = get_input()) != 'i') {
        if (ch > 0) {
            sendto(sock, &ch, sizeof(ch), 0, (struct sockaddr *) &addr, addr_len);
        }
    }

    server_running = 0;

    // delwin(win);
    clear();
    refresh();
    pthread_join(pid, NULL);
    endwin();
    pthread_mutex_destroy(&mutex);

    close(sock);

    switch (whoWin) {
        case 1:
            printf("PLAYER 2 has been win\n");
            break;
        case 2:
            printf("PLAYER 1 has been win\n");
            break;
    }
    return 0;
}
