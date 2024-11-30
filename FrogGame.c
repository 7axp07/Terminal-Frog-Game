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
#define STATUS_WIDTH 25

#define DELAY_OFF 0
#define DELAY_ON 1


#define QUIT 'k'
#define NOKEY ' '
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
#define FRAME_TIME	100							
#define MVF	1							
#define MVC 2							

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


WIN* initWindow(WINDOW* parentW, int width, int height, int x, int y, int delay){
    WIN* win = (WIN*)malloc(sizeof(WIN));
    win->width = width;
    win->height = height;
    win->x = x;
    win->y = y;
    win->window = subwin(parentW, height, width, y, x);
    if (delay == DELAY_OFF) nodelay(win->window, TRUE);
    box(win->window, 0, 0);
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
    wrefresh(obj->win->window);
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
    frog->width = 1;
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
        if (y==gWin->height-2) y--;
        int x = rand() % gWin->width;
        int width = minW + rand() % maxW;
        int speed = 1 + rand() % 2;
        int mv = MVC;
        cars[i] = initCar(gWin, x, y, width, CAR_COLOR, s , rand() % 2 ? 1 : -1, speed, mv);
        drawObj(cars[i]);
    }
}

// Moving OBJs

void moveFrog(OBJ* frog, int dx, int dy, unsigned int frame) {
     if (frame % frog->mv == 0) {
        clearObj(frog);
        frog->x = frog->x + dx < frog->xmin ? frog->xmin : frog->x + dx > frog->xmax ? frog->xmax : frog->x + dx;
        frog->y = frog->y + dy < 1 ? 1 : frog->y + dy;
        drawObj(frog);
    }
}

void moveCar(OBJ* car, unsigned int frame) {
    if (frame % car->mv == 0){
    clearObj(car);
        car->x += car->dir * car->speed;
        if (car->x + car->width < car->xmin) car->x = car->xmax;
        else if (car->x > car->xmax) car->x = car->xmin - car->width;
        drawObj(car);
    } 
}

// Collision check
int collision(OBJ* frog, OBJ* car) {
    if (frog->y != car->y) return 0;
    return (frog->x >= car->x && frog->x < car->x + car->width);
}							

// ------------- Status functions -----------------


void updateStatus(WIN* sWin, int pts, float timer) {
    mvwprintw(sWin->window, 0, 1, "Points: %d", pts);
    mvwprintw(sWin->window, 0, 12, "Time Left: %.1fs", timer);
    wrefresh(sWin->window);
}
void gameOver(WIN* gWin, WIN* sWin, WINDOW* pWin){
    mvwaddstr(gWin->window, gWin->height / 2, gWin->width / 2 - 5, "GAME OVER");
    wrefresh(gWin->window);
    sleep(2);
    cleanup(gWin, sWin, pWin);
    exit(EXIT_FAILURE);
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

void mainLoop(WIN* gWin, WIN* sWin, WINDOW* pWin, OBJ* frog, OBJ** cars, int* gamesettings){
    int ch, pts = 0, fC = 0;
    float timer = gamesettings[2];

    nodelay(gWin->window, TRUE);
    keypad(gWin->window, TRUE);

    while (timer > 0){
        ch = wgetch(gWin->window);
        if (ch == ERR) ch = NOKEY;
    else {
       switch (ch){
        case KEY_UP:
        moveFrog(frog, 0, -1, fC);
        break;
        case KEY_DOWN:
        moveFrog(frog, 0, 1, fC);
        break;
        case KEY_LEFT:
        moveFrog(frog, -1, 0, fC);
        break;
        case KEY_RIGHT:
        moveFrog(frog, 1, 0, fC);
        break;
        }
    }	
    for (int i = 0; i < gamesettings[1] - 2; i++){
        moveCar(cars[i], fC);
        if (collision(frog, cars[i]) == 1){ // 1 = frog and car collided
            gameOver(gWin, sWin, pWin);
        }
    }
    if (frog->y == 1) { // Frog reaches the "top"
            pts++;
            moveFrog(frog, 0, gWin->height-3, fC);
    }

    updateStatus(sWin, pts, timer);
    usleep(FRAME_TIME * 1000);
    timer -= FRAME_TIME / 1000.0;
    // flushinp(); 
    fC++;
    }
    gameOver(gWin, sWin, pWin);
}

// --------- MAIN ------------

int main(){
    srand(time(NULL));
    WINDOW *stdWin = startGame();
    starterScreen(stdWin);

    // Create arrays from files with settings and symbols
    int gameSettings[SETTINGS_NUM];         // 0 - width; 1 - height; 2 - timer; 3&4 - min/max car width,
    char gameSymbols[SYMBOL_NUM];           // X - frog; # - car segment
    arrayFromFiles(gameSettings, gameSymbols);
    
    // Create windows and objects
    WIN* gameWin = initWindow(stdWin, gameSettings[0], gameSettings[1], 0, 0, DELAY_ON);
    WIN* statusWin = initWindow(stdWin, STATUS_WIDTH, 1, 0, gameSettings[1]+1, DELAY_ON);


    OBJ* frog = initFrog(gameWin, FROG_COLOR, gameSymbols[0]);
    drawObj(frog);
    int carNum = 5 + (gameSettings[1]-2);
    OBJ* cars[carNum];
    initCars(cars, carNum, gameWin,  gameSymbols[1], gameSettings[3], gameSettings[4]);
   
    mainLoop(gameWin, statusWin, stdWin, frog, cars, gameSettings);
    
    // --------- Cleanup ----------
    cleanup(gameWin, statusWin, stdWin);
    refresh();
    return EXIT_SUCCESS;
}