#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLOSE_TIME 4
#define BORDER 1
#define SETTINGS_NUM 5
#define SYMBOL_NUM 3

// For testing

#define WIDTH 10
#define HEIGHT 10


// Colours
#define BG_COLOR 1 //black
#define FROG_COLOR 2 //green
#define LANE_COLOR 3 //white
#define SAFE_LANE_COLOR 4 //blue
#define CAR_COLOR 5 //red
#define TRUCK_COLOR 6 //magenta
#define START_COLOR 7 //black

// Window
typedef struct {
    WINDOW* window;
    int x,y;
    int width, height;
} WIN;

typedef struct {
    WIN* win;
    int x,y;
    char symbol;
    int color;
    int move;
} OBJ;

typedef struct {
    int frameNum;
    unsigned int frameTime;
    float passTime;
} TIMER;


WINDOW* startGame(){
    
    WINDOW* win;
    if ((win = initscr())== NULL){
        fprintf(stderr, "Ncurses could not be initialized.");
        exit(EXIT_FAILURE);
    }
    //initialize colours
    if (has_colors()){ 
        start_color();
        init_pair(BG_COLOR, COLOR_WHITE, COLOR_BLACK);
        init_pair(FROG_COLOR, COLOR_GREEN, COLOR_BLACK);
        init_pair(LANE_COLOR, COLOR_BLUE, COLOR_WHITE);
        init_pair(SAFE_LANE_COLOR, COLOR_WHITE, COLOR_BLUE);
        init_pair(CAR_COLOR, COLOR_RED, COLOR_BLACK);
        init_pair(TRUCK_COLOR, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(START_COLOR, COLOR_YELLOW, COLOR_BLACK);
    }

    curs_set(0);
    noecho();
    return(win);
}

void starterScreen(WINDOW* win){
    mvwaddstr(win, 1, 1, "Frogger Game by ");
	mvwaddstr(win, 2, 1, "Press any key to start");
	wgetch(win);								
	wclear(win);								
	wrefresh(win);
}

void endGame(WIN* win){

}

WIN* initWindow(WINDOW* pWin, int width, int height, int x, int y){
    WIN* win = (WIN*) malloc(sizeof(WIN));
    win->width = width; win->height = height; win->x = x; win->y = y;
    win->window = subwin(pWin, width, height, y, x);
    clearWindow(win);
    wrefresh(win->window);
    return win;
}

void clearWindow(WIN* win){
    int r,c;
	for(int i = 0; i < win->width ; i++)
		for(int j = 0; j < win->height ; j++)
			mvwprintw(win->window,i,j," ");

}



// Frog & Car functions

OBJ* initFrog(WIN* win, int color, char symbol){
    OBJ* frog	   = (OBJ*)malloc(sizeof(OBJ));
    frog->symbol = symbol; frog->color = color; frog->win = win;
    initPos(frog, (frog->win->width - 1), frog->win->height);
    return frog;
}

void initPos(OBJ* obj, int xs, int ys)
{
	obj->x = xs;
	obj->y = ys;
}


// Timer functions

int timerUpdate(TIMER* TIM, WIN* WINSTAT){
    TIM->frameNum++;
    
}

// File function

FILE* readFromFiles(char* fileName){
    FILE *file = fopen(fileName, "r");
    if (file == NULL){
        fprintf(stderr, "File not found.");
        exit(EXIT_FAILURE);
    }
    return file;
}

// Main Loop

int mainLoop(){
    
}


int main(){

    WINDOW *stdWin = startGame();
    starterScreen(stdWin);

    FILE *settings = readFromFiles("gameSettings.txt");
    FILE *symbols = readFromFiles("gameSymbols.txt");
    int gameSettings[SETTINGS_NUM];
    char gameSymbols[SYMBOL_NUM];

    fscanf(settings, "%d %d %d %d %d", &gameSettings[0], &gameSettings[1], &gameSettings[2], &gameSettings[3], &gameSettings[4]);
    fscanf(symbols, "%c %c %c", &gameSymbols[0], &gameSymbols[1], &gameSymbols[2]);

    WIN* gameWin = initWindow(stdWin, gameSettings[0], gameSettings[1], 0, 0);
    OBJ* frog = initFrog(gameWin, FROG_COLOR, gameSymbols[0]);

    delwin(gameWin->window);							
	delwin(stdWin);
    endwin();
    refresh();
    return EXIT_SUCCESS;
}