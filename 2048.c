#include "2048.h"

#include <stdlib.h>//+perror,exit
#include <string.h>
//추가
#include <errno.h>
#include <fcntl.h>//oflag option,mode
#include <unistd.h>//open,close
#define ARR_LEN(arr) (sizeof((arr))/sizeof((arr)[0]))
#define BOARD(y, x) board[BOARD_SIZE*(y) + (x)]

static int moved = 0;

inline static void board_step(direction dir);

const int board_size = BOARD_SIZE;

const char* const title = "2048";

unsigned int score = 0;
unsigned int change = 0;
unsigned int high_score = 0;

unsigned int enable_colors = 1;
unsigned int color = 0;

unsigned int game_over = 0;
//추가
struct timespec ssleep = {0,150000000};
struct timespec anim8 = {0,  37500000};

const char* const save_file_name = "save.game";

const char* relation[] = {
    ".",
    "2",
    "4",
    "8",
    "16",
    "32",
    "64",
    "128",
    "256",
    "512",
    "1024",
    "2048",
    "4096",
    "8192",
    "16384",
    "32768",
    "65536",
    "131072",
    "262144",
    "524288",
    "1048576",
    "2097152",
    "4194304"
};

unsigned char board[BOARD_SIZE*BOARD_SIZE] = {0};
int free_tiles[BOARD_SIZE*BOARD_SIZE];

WINDOW* tiles[BOARD_SIZE][BOARD_SIZE];

void save_game(void) {


    int fd;

	if((fd=open("save_file_name",O_WRONLY))<0){
		perror("open");
		exit(1);
	}

        if (write(fd,&board_size, sizeof(board_size)) <0) {
            perror("write_board_size");
            exit(1);
        }
        if (write(fd,&score, sizeof(score)) <0) {
            perror("write_score");
            exit(1);
        }
        if (write(fd,&high_score, sizeof(high_score))<0) {
            perror("write_highscore");
            exit(1);
        }

	for(int i=0;i<board_size*board_size;i++){
        if (write(fd,board, sizeof(*board)) <0) {
            perror("write_board");
            exit(1);
        }
	}
      
  close(fd);
    
}



int load_game(void) {
    int loaded = 0;
     int fd;

		if((fd=open("save_file_name",O_RDONLY|O_CREAT))<0){
			perror("open");
			exit(1);
		}
		

        int read_board_size = 0;

        if (read(fd,&read_board_size, sizeof(board_size))<0) {
            printf("read_board_size error_num: %d",errno);
            exit(1);
        } 
            if (read_board_size != board_size) {
                perror("Save_actual_nomatch");
                //exit(1);
            }
        
        if (read(fd,&score, sizeof(score)) <0) {
            perror("read_score");
            exit(1);
        }
        if (read(fd,&high_score, sizeof(high_score)) < 0) {
            perror("read_high_score");
            exit(1);
        }

	for(int i=0;i<board_size*board_size;i++){
       if (read(fd,board, sizeof(*board))<0) {
            perror("read_board");
            exit(1);
        }
     }

        loaded = 1;
        close(fd);
    
    return loaded;
}

void is_game_over() {
    int y, x;
    for (y = 0; y < BOARD_SIZE; ++y) {
        for (x = 0; x < BOARD_SIZE; ++x) {
            if (((x + 1 < BOARD_SIZE) && BOARD(y, x) == BOARD(y, x + 1)) ||
                ((y + 1 < BOARD_SIZE) && BOARD(y, x) == BOARD(y + 1, x))) {
                return;
            }
        }
    }
    game_over = 1;
}

void init_tiles(void) {
    int y, x;
    for (y = 0; y < board_size; ++y) {
        for (x = 0; x < board_size; ++x) {
            tiles[y][x] = subwin(stdscr,
                                 TILE_SIZE/2, TILE_SIZE,
                                 1 + y*(TILE_SIZE/2), x*TILE_SIZE);
        }
    }
}

void del_tiles(void) {
    int y, x;
    for (y = 0; y < board_size; ++y) {
        for (x = 0; x < board_size; ++x) {
            delwin(tiles[y][x]);
        }
    }
}

void print_board(void) {
    int y, x;
    int rev = 0;
    (void)mvwaddstr(stdscr, 0, 1, title);
    (void)mvwprintw(stdscr, 0, 2*TILE_SIZE, "score: %d", score);
    for (y = 0; y < board_size; ++y) {
        for (x = 0; x < board_size; ++x) {
            if (color) {
                if (BOARD(y, x) < ARR_LEN(relation)) {
                    wbkgd(tiles[y][x], ' ' |
                          COLOR_PAIR(BOARD(y, x)));
                } else {
                    wbkgd(tiles[y][x], ' ' | COLOR_PAIR(0));
                }
            } else {
                wbkgd(tiles[y][x], ' ' | ((rev)?A_REVERSE:0));
            }
            if (BOARD(y, x) < ARR_LEN(relation)) {
                mvwprintw(tiles[y][x],
                          TILE_SIZE/4, 1,
                          "%*s",
                          TILE_SIZE - 2,
                          relation[(size_t)BOARD(y, x)]);
            } else {
                mvwprintw(tiles[y][x],
                          TILE_SIZE/4, 1,
                          "2^%d",
                          BOARD(y, x));
            }
            wrefresh(tiles[y][x]);
            rev = ~rev;
        }
        rev = ~rev;
    }
    mvwprintw(stdscr,
              (TILE_SIZE/2)*BOARD_SIZE + 1, 0,
              "High-score: %u%s",
              high_score, (game_over)?" [GAME OVER]":"");
    refresh();
}

void move_tile(int y, int x,
               int dy, int dx) {
    if (BOARD(y, x) == 0) {
        return;
    }

    if (((x + dx >= BOARD_SIZE) || (x + dx < 0))
        || ((y + dy >= BOARD_SIZE) || (y + dy < 0))) {
        return;
    }

    if (BOARD(y + dy, x + dx) == 0) {
        change = 1;
        moved = 1;
        BOARD(y + dy, x + dx) = BOARD(y, x);
        BOARD(y, x) = 0;
    }
}

inline static void merge(direction dir) {
    int y, x;
    switch (dir) {
    case right:
        for (x = BOARD_SIZE - 2; x >= 0; --x) {
            for (y = 0; y < BOARD_SIZE; ++y) {
                merge_tiles(y, x, 0, 1);
            }
        }
        break;
    case down:
        for (y = BOARD_SIZE - 2; y >= 0; --y) {
            for (x = 0; x < BOARD_SIZE; ++x) {
                merge_tiles(y, x, 1, 0);
            }
        }
        break;
    case left:
        for (x = 1; x < BOARD_SIZE; ++x) {
            for (y = 0; y < BOARD_SIZE; ++y) {
                merge_tiles(y, x, 0, -1);
            }
        }
        break;
    case up:
        for (y = 1; y < BOARD_SIZE; ++y) {
            for (x = 0; x < BOARD_SIZE; ++x) {
                merge_tiles(y, x, -1, 0);
            }
        }
        break;
    }
}

void board_move(direction dir) {
    switch(dir) {
    case right:
        board_step(right);
        merge(right);
        board_step(right);
        break;
    case down:
        board_step(down);
        merge(down);
        board_step(down);        
        break;
    case left:
        board_step(left);
        merge(left);
        board_step(left);
        break;
    case up:
        board_step(up);
        merge(up);
        board_step(up);
        break;
    }
}

inline static void board_step(direction dir) {
    int y, x;
    switch(dir) {
    case right:
        for (x = BOARD_SIZE - 2; x >= 0; --x) {
            for (y = 0; y < BOARD_SIZE; ++y) {
                move_tile(y, x, 0, 1);
            }
        }
        break;
    case down:
        for (y = BOARD_SIZE - 2; y >= 0; --y) {
            for (x = 0; x < BOARD_SIZE; ++x) {
                move_tile(y, x, 1, 0);
            }
        }
        break;
    case left:
        for (x = 1; x < BOARD_SIZE; ++x) {
            for (y = 0; y < BOARD_SIZE; ++y) {
                move_tile(y, x, 0, -1);
            }
        }
        break;
    case up:
        for (y = 1; y < BOARD_SIZE; ++y) {
            for (x = 0; x < BOARD_SIZE; ++x) {
                move_tile(y, x, -1, 0);
            }
        }
        break;
    }
    print_board();

    nanosleep(&anim8, NULL);
    if (moved) {
        moved = 0;
        board_step(dir);
    }
}

void merge_tiles(int y, int x,
                 int dy, int dx) {
    if (BOARD(y, x) == 0) {
        return;
    }

    if (((x + dx >= BOARD_SIZE) || (x + dx < 0))
        || ((y + dy >= BOARD_SIZE) || (y + dy < 0))) {
        return;
    }

    if (BOARD(y + dy, x + dx) == BOARD(y, x)) {
        change = 1;
        ++BOARD(y + dy, x + dx);
        score += (1 << BOARD(y + dy, x + dx));
        BOARD(y, x) = 0;
        if (score > high_score) {
            high_score = score;
        }
    }
}

int refresh_free_tiles(void) {
    int i;
    int count = 0;
    for (i = 0; i < BOARD_SIZE*BOARD_SIZE; ++i) {
        if (board[i] == 0) {
            free_tiles[count] = i;
            ++count;
        }
    }
    return count;
}

void insert_random_tile(void) {
    int count = refresh_free_tiles();
    if (count) {
        int r = rand() % count;
        int face = ((rand() % 10) < 9)?1:2;
        board[free_tiles[r]] = face;
    }
}

void new_game(void) {
    game_over = 0;
    change = 0;
    score = 0;
    memset(board, 0, (size_t)board_size*board_size);
    insert_random_tile();
    insert_random_tile();
    clear();
    refresh();
}

void do_initialize_colors(void) {
    start_color();
    color = 1;
    init_pair(1, COLOR_BLACK, COLOR_RED);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);
    init_pair(4, COLOR_BLACK, COLOR_BLUE);
    init_pair(5, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(6, COLOR_BLACK, COLOR_CYAN);
    init_pair(7, COLOR_BLACK, COLOR_WHITE);
    init_pair(8, COLOR_RED, COLOR_BLACK);
    init_pair(9, COLOR_RED, COLOR_GREEN);
    init_pair(10, COLOR_RED, COLOR_YELLOW);
    init_pair(11, COLOR_RED, COLOR_BLUE);
    init_pair(12, COLOR_RED, COLOR_MAGENTA);
    init_pair(13, COLOR_RED, COLOR_CYAN);
    init_pair(14, COLOR_RED, COLOR_WHITE);
    init_pair(15, COLOR_GREEN, COLOR_RED);
    init_pair(16, COLOR_GREEN, COLOR_WHITE);
    init_pair(17, COLOR_GREEN, COLOR_YELLOW);
    init_pair(18, COLOR_GREEN, COLOR_BLUE);
    init_pair(19, COLOR_GREEN, COLOR_MAGENTA);
    init_pair(20, COLOR_GREEN, COLOR_CYAN);
    init_pair(21, COLOR_GREEN, COLOR_WHITE);
    init_pair(22, COLOR_YELLOW, COLOR_RED);
    init_pair(23, COLOR_YELLOW, COLOR_GREEN);
    init_pair(24, COLOR_YELLOW, COLOR_BLACK);
    init_pair(25, COLOR_YELLOW, COLOR_BLUE);
    init_pair(26, COLOR_YELLOW, COLOR_MAGENTA);
    init_pair(27, COLOR_YELLOW, COLOR_CYAN);
    init_pair(28, COLOR_YELLOW, COLOR_WHITE);
}
