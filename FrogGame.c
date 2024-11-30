#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CLOSE_TIME 4
#define BORDER 1
#define SETTINGS_NUM 5
#define SYMBOL_NUM 2
#define OBJ_HEIGHT 1
#define OBJ_WIDTH 1

#define QUIT 'k'
#define QUIT_T 4

// For testing

#define WIDTH 10
#define HEIGHT 10

// Colours
#define BG_COLOR 1 //black
#define FROG_COLOR 2 //green
#define CAR_COLOR 3 //red

//Timer

#define BET_TIME	2							
#define FRAME_TIME	25							
#define MVF	2							
#define MVC 5							

//#define FRAME_TIME 100000

// ----------- Structures ------------
typedef struct {
    WINDOW* window;
    int x,y;
    int width, height;
} WIN;

typedef struct {
    WIN* win;
    int x,y, width;
    char symbol;
    int color;
    int speed;
    int dir; // -1 || 1 , corresponds to L||R
    int xmin, xmax;
    int mv;
} OBJ;

typedef struct {
    int frameNum;
    unsigned int frameTime;
    float passTime;
} TIMER;

// ----------- Window etc functions --------------

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
        init_pair(CAR_COLOR, COLOR_RED, COLOR_BLACK);
    }

    curs_set(0);
    noecho();
    return(win);
}

void starterScreen(WINDOW* win){
    mvwaddstr(win, 1, 1, "Frogger Game by Alex");
	mvwaddstr(win, 2, 1, "Press any key to start");
	wgetch(win);								
	wclear(win);								
	wrefresh(win);
}


WIN* initWindow(WINDOW* parentW, int width, int height, int x, int y){
    WIN* win = (WIN*) malloc(sizeof(WIN));
    win->width = width; win->height = height; win->x = x; win->y = y;
    win->window = subwin(parentW, width, height, y, x);
    wrefresh(win->window);
    return win;
}

void cleanup(WIN* gWin, WIN* sWin, WINDOW* parentW){
    delwin(gWin->window);
    delwin(sWin->window);
    delwin(parentW);
    endwin();
}


// ----------------- obj (Frog & Car) functions ---------------------

//Drawing/clearing OBJs
void drawObj(OBJ* obj) {
    wattron(obj->win->window, COLOR_PAIR(obj->color));
    for (int i = 0; i < obj->width; i++) {
        mvwaddch(obj->win->window, obj->y, obj->x + i, obj->symbol);
    }
    wattroff(obj->win->window, COLOR_PAIR(obj->color));
}

void clearObj(OBJ* obj) {
    for (int i = 0; i < obj->width; i++) {
        mvwaddch(obj->win->window, obj->y, obj->x + i, ' ');
    }
}

// Creating OBJs
OBJ* initFrog(WIN* win, int color, char symbol){
    OBJ* frog	   = (OBJ*)malloc(sizeof(OBJ));
    frog->symbol = symbol;
    frog->color = color;
    frog->win = win;
    frog->xmax = win->width-1;
    frog->xmin = 0; 
    frog->x = win->width / 2;
    frog->y = win->height - 2; 
    frog->mv = MVF;
    return frog;
}

OBJ* initCar(WIN* win, int x, int y, int width, int color, char symbol, int dir, int speed, int mv) {
    OBJ* car = (OBJ*)malloc(sizeof(OBJ));
    car->win = win;
    car->x = x; car->y = y;
    car->width = width;
    car->color = color;
    car->symbol = symbol;
    car->dir = dir;
    car->speed = speed;
    car->xmin = 0;
    car->xmax = win->width - 1;
    car->mv = mv;
    return car;
}

void initCars(OBJ** cars,int carNum, WIN* gWin, char s, int minW, int maxW){
    for (int i=0; i<carNum; i++){
        int y = (i + 1) * 3;
        int x = rand() % 30;
        int width = minW + rand() % maxW;
        int speed = 1 + rand() % 2;
        int mv = MVC;
        cars[i] = initCar(gWin, x, y, width, CAR_COLOR, s , rand() % 2 ? 1 : -1, speed, mv);
    }
}

// Moving OBJs

void moveFrog(OBJ* frog, int dx, int dy, unsigned int frame) {
    if (frame - frog->mv >= MVF){
        clearObj(frog);
        frog->x = frog->x + dx < frog->xmin ? frog->xmin : frog->x + dx > frog->xmax ? frog->xmax : frog->x + dx;
        frog->y = frog->y + dy < 0 ? 0 : frog->y + dy;
        drawObj(frog);
    }
}

void moveCar(OBJ* car, unsigned int frame) {
    if (frame % car->mv == 0){
        clearObj(car);
        car->x += car->dir;
    if (car->x + car->width < car->xmin) {
        car->x = car->xmax;
    }
    else if (car->x > car->xmax) {
        car->x = car->xmin - car->width + 1;
    }
    drawObj(car);
    } 
}

// Collision check
int collision(OBJ* frog, OBJ* car) {
    if (frog->y != car->y) return 0;
    return (frog->x == car->x || frog->x == car->x + car->width || frog->x == car->x - car->width);
}							

// ------------- Status & Timer functions -----------------

TIMER* initTimer(WIN* status, int passtime )
{
	TIMER* timer = (TIMER*)malloc(sizeof(TIMER));
	timer->frameNum = 1;
	timer->frameTime = FRAME_TIME;
	timer->passTime = passtime / 1.0;
	return timer;
}

int timerUpdate(TIMER* timer, WIN* sWin, int passtime)							// return 1: time is over; otherwise: 0
{
	timer->frameNum++;
	timer->passTime = passtime - (timer->frameNum * timer->frameTime / 1000.0);
	if (timer->passTime < (timer->frameTime / 1000.0) ) timer->frameTime = 0; 		// make this zero (floating point!)
	else sSleep(timer->frameTime);
	//ShowTimer(sWin,timer->frameTime);
	if (timer->passTime == 0) return 1;
	return 0;
}

void sSleep(unsigned int tui) { usleep(tui * 1000); } 

void updateStatus(WIN* statusWin, int points, TIMER* timer) {
    wclear(statusWin->window);
    mvwprintw(statusWin->window, 0, 1, "Points: %d", points);
    mvwprintw(statusWin->window, 0, statusWin->width/2, "Time Left: %fs", timer->passTime);
    wrefresh(statusWin->window);
}

// ---------- File function --------------

FILE* readFromFiles(char* fileName){
    FILE *file = fopen(fileName, "r");
    if (file == NULL){
        fprintf(stderr, "File not found.");
        exit(EXIT_FAILURE);
    }
    return file;
}

void arrayFromFiles(int* gameSettings, char* gameSymbols){
    FILE *settings = readFromFiles("gameSettings.txt");
    FILE *symbols = readFromFiles("gameSymbols.txt");
    fscanf(settings, "%d %d %d %d %d", &gameSettings[0], &gameSettings[1], &gameSettings[2],&gameSettings[3], &gameSettings[4]);
    fscanf(symbols, "%c %c", &gameSymbols[0], &gameSymbols[1]);
    fclose(settings); fclose(symbols);
}

// ---------------- MAIN LOOP/TIMELINE ------------------

int mainLoop(WIN* gWin, WIN* sWin, WINDOW* pWin, OBJ* frog, OBJ** cars, TIMER* timer, int* gamesettings){
    int ch, pts = 0, fC = 0;

    nodelay(gWin->window, TRUE);
    while ((ch = wgetch(gWin->window)) != QUIT){
    switch (ch){
        case KEY_UP:
        moveFrog(frog, 0, -1, timer->frameNum);
        break;
        case KEY_DOWN:
        moveFrog(frog, 0, 1, timer->frameNum);
        break;
        case KEY_LEFT:
        moveFrog(frog, -1, 0, timer->frameNum);
        break;
        case KEY_RIGHT:
        moveFrog(frog, 1, 0, timer->frameNum);
        break;
        }
    for (int i = 0; i < gamesettings[1] - 2; i++){
        moveCar(cars[i], timer->frameNum);
        if (collision(frog, cars[i]) == 1){ // 1 = frog and car collided
            mvwaddstr(gWin->window, gWin->height / 2, gWin->width / 2 - 5, "GAME OVER");
            wrefresh(gWin->window);
            sSleep(2);
            //cleanup(gWin->window, sWin->window, pWin);
            return pts;
        }
    }
    if (frog->y == 0) { // Frog reaches the "top"
            pts++;
            moveFrog(frog, 0, gamesettings[1] - 2, timer->frameNum);
    }
    if (timerUpdate(timer, sWin, gamesettings[2])){
        return pts;
    }
    updateStatus(sWin, pts, timer);
    wrefresh(gWin->window);
    sSleep(timer->frameTime);
    }
    return 0;
}

// --------- MAIN ------------

int main(){
    srand(time(NULL));
    WINDOW *stdWin = startGame();
    starterScreen(stdWin);

    // Create arrays from files with settings and symbols
    int gameSettings[SETTINGS_NUM];         // 0 - width; 1 - height; 2 - timer; 3&4 - min/max car width
    char gameSymbols[SYMBOL_NUM];           // X - frog; # - car segment
    arrayFromFiles(gameSettings, gameSymbols);

    // Create windows and objects
    WIN* gameWin = initWindow(stdWin, gameSettings[0], gameSettings[1], 0, 0);
    WIN* statusWin = initWindow(stdWin, gameSettings[0], 1, 0, gameSettings[1]);

    OBJ* frog = initFrog(gameWin, FROG_COLOR, gameSymbols[0]);
    int carNum = gameSettings[1]-2;
    OBJ* cars[carNum];
    initCars(cars, carNum, gameWin,  gameSymbols[1], gameSettings[3], gameSettings[4]);
    
    TIMER* timer = initTimer(statusWin, gameSettings[2]);

    int result = mainLoop(gameWin, statusWin, stdWin, frog, cars, timer, gameSettings);
	char info[100];
	sprintf(info," Game over, Your points = %d.",result);

    // --------- Cleanup ----------

    cleanup(gameWin, statusWin, stdWin);
    refresh();
    return EXIT_SUCCESS;
}