#include <stdio.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#define TIME_TO_SLEEP 200000


int lines, cols;



///< структура-описатель корабля
typedef struct ship_descr {
	int point_y[5]; ///< Точка, задающая положение корабля (у)
	int point_x; ///< Точка, задающая положение корабля (х)
	int life; ///< Статус игры
} ship_description;



///< структура-описатель пушки
typedef struct gun_descr {
	int point_y; ///< Точка, задающая положение пушки (у)
	int point_x; ///< Точка, задающая положение пушки (х)
	int shell_status[3]; ///< массив статусов существования снарядов 
	int shell_position[3]; ///< массив позиций снарядов
	int life; ///< Статус игры
} gun_description;



///< структура-описатель всех объектов
typedef struct gun_ship_point {
	ship_description *ship_pointer; ///< корабль
	gun_description *gun_pointer; ///< пушка
} gun_ship_pointer;



/**
 * Считывает символ с клавиатуры в неблокирующем режиме
 * @return код считанного символа
 */
char get_symb()
{
	struct termios old_attr, new_attr; //< атрибуты потока
	char symb;

	tcgetattr(STDIN_FILENO, &old_attr);
	new_attr = old_attr;
	new_attr.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &new_attr);
	symb = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &old_attr);
	
	return symb;
}



/**
 * Осуществляет поиск первого свободного определителя снаряда
 * @param [in] shells массив, описывающий статусы снаряды
 * @return номер свободной ячейки или -1, если такой нет
 */
int shell_search(int *shells)
{
	for (int i = 0; i < 3; i++)
	{
		if (shells[i] == 0)
			return i;
	}

	return -1;
}



/**
 * Проверяет поподание снаряда в корабль
 * @param [in] objects указатели на объекты
 * @return 1, если игра завершена
 */
int check_game_life(gun_ship_pointer *objects)
{
	for (int i = 0; i < 3; i++)
	{
		if (objects->gun_pointer->shell_position[i] == objects->ship_pointer->point_x)
			for (int k = 0; k < 5; k++)
			{
				if (objects->gun_pointer->point_y == objects->ship_pointer->point_y[k])
					return 1;
			}
	}

	return 0;
}



/**
 * Движение корабля
 * @param [in]  args ссылка на экземпляр кораблика
 */
void *Ship_Movement(void *args)
{
	ship_description *ship = (ship_description*)args;

	while(ship->life != 0)
	{
		usleep(TIME_TO_SLEEP);
		
		for (int i = 0; i < 5; i++)
		{
			ship->point_y[i]--;
			if (ship->point_y[i] == 0)
		        	ship->point_y[i] = lines;
		}	
	}
}



/**
 * Описание пушки
 * @param [in] args ссылка на экземпляр пушки
 */
void *Gun_Movement(void *args)
{
	gun_description *gun = (gun_description*)args;

	while (gun->life != 0)
	{
		usleep(TIME_TO_SLEEP / 4);

		if (get_symb() == ' ')
		{
			int number;

			if ((number = shell_search(gun->shell_status)) != -1) 
			{
				gun->shell_status[number] = 1;
				gun->shell_position[number] = gun->point_x + 1;
			}
		}

		for (int i = 0; i < 3; i++)
		{
			if (gun->shell_position[i] >= cols)
			{
				gun->shell_position[i] = gun->point_x;
				gun->shell_status[i] = 0;
			}	
			else if (gun->shell_status[i] == 1)
				gun->shell_position[i]++;
		}
	}
}



/**
 * Отрисовка
 * @param [in] args указатели на объекты
 */
void *Rendering(void *args)
{
	initscr();
	keypad(stdscr, TRUE);
	curs_set(0);

	gun_ship_pointer *objects = (gun_ship_pointer*)args;

	while(1)
	{
		usleep(TIME_TO_SLEEP / 4);

		clear();
		// корабль
		for (int i = 0; i < 5; i++)
		{
			mvaddch(objects->ship_pointer->point_y[i], objects->ship_pointer->point_x, '*');
		}
	
		// пушка
		for (int i = 0; i < 3; i++)
		{
			if (objects->gun_pointer->shell_status[i] == 1)
				mvaddch(objects->gun_pointer->point_y, objects->gun_pointer->shell_position[i], '+');
		}
		mvaddch(objects->gun_pointer->point_y, objects->gun_pointer->point_x, '=');
	
		refresh();
		
		if (check_game_life(objects) == 1) // проверка условия победы
		{
			objects->gun_pointer->life = 0;
			objects->ship_pointer->life = 0;
			break;
		}
	}

	endwin();
}



/**
 *
 * @return 0
 */
int main()
{
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

	initscr();
	lines = LINES;
	cols = COLS;
	endwin();

	// Инициализация корабля
	ship_description ship;
	for (int i = 0; i < 5; i++)
		ship.point_y[i] = lines - i;
	ship.point_x = cols / 2 + cols / 4;
	ship.life = 1;

	// Инициализация пушки
	gun_description gun;
	gun.point_y = lines / 2;
	gun.point_x = cols / 2 - cols / 4;
	gun.life = 1;
	for (int i = 0; i < 3; i++)
	{
		gun.shell_position[i] = gun.point_x;
		gun.shell_status[i] = 0;
	}

	gun_ship_pointer objects;
	objects.ship_pointer = &ship;
	objects.gun_pointer = &gun;

	pthread_t thread_ship, thread_gun, thread_render;
	void *retval;

	pthread_create(&thread_ship, NULL, Ship_Movement, (void*)&ship);
	pthread_create(&thread_gun, NULL, Gun_Movement, (void*)&gun);
	pthread_create(&thread_render, NULL, Rendering, (void*)&objects);

	pthread_join(thread_render, &retval);
	pthread_join(thread_ship, &retval);
	pthread_join(thread_gun, &retval);

	printf("WIN!!!\n");

	return 0;
}