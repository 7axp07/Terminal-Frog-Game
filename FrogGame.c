#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define SETTINGS_NUM 7
#define SYMBOL_NUM 3
#define STATUS_WIDTH 25

#define DELAY_OFF 0
#define DELAY_ON 1

#define NOKEY ' '
#define QUIT_KEY 'k'

// Colours
#define BG_COLOR 1 //black
#define FROG_COLOR 2 //green
#define CAR_COLOR 3 //red
#define CAR_COLOR_FRIEND 4 //blue

//Timer
#define FRAME_TIME	10
#define MVF	5
#define MVC 10

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
    int ymin, ymax;
    int mv;
    bool isFriendly;
    bool isInCar;
    bool demandsCar;
} OBJ;

 typedef struct {
    int frameNum;
    int gameTime;
    float timeLeft;
    WIN* win;
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
        init_pair(CAR_COLOR_FRIEND, COLOR_BLUE, COLOR_BLACK);
    }

    curs_set(0);
    noecho();
    return(win);
}

void starterScreen(WINDOW* win){
    mvwaddstr(win, 1, 1, "Frogger Game by Alex");
    mvwaddstr(win, 2, 1, "Rules: Get to the top of the screen");
    mvwaddstr(win, 2, 1, "Avoid cars ('##'s) and holes ('O's)");
	mvwaddstr(win, 3, 1, "Press any key to start");
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

// ------------- Status functions -----------------

TIMER* initTimer(WIN* win, int gameTime){
    TIMER* timer = (TIMER*)malloc(sizeof(TIMER));
    timer->frameNum = 0;
    timer->gameTime = gameTime;
    timer->timeLeft = gameTime;
    timer->win = win;
    return timer;
}

void updateStatus(WIN* sWin, int pts, TIMER* timer) {
    mvwprintw(sWin->window, 0, 1, "Points: %d", pts);
    mvwprintw(sWin->window, 0, 12, "Time Left: %.1fs", timer->timeLeft);
    mvwaddstr(sWin->window, 1, 0, "Name Surname ID");
    wrefresh(sWin->window);
}

bool updateTimer(TIMER* timer, clock_t start, clock_t end, int pts){
    usleep((FRAME_TIME-(end-start)/CLOCKS_PER_SEC/1000.0)*1000.0);
    timer->frameNum++;
    timer->timeLeft = timer->gameTime - timer->frameNum*FRAME_TIME /1000.0;
    if (timer->timeLeft <= 0){
        return FALSE;
    }
    updateStatus(timer->win, pts, timer);
    return TRUE;
}

void gameOver(WIN* gWin, WIN* sWin, WINDOW* pWin){
    mvwaddstr(gWin->window, gWin->height / 2, gWin->width / 2 - 5, "GAME OVER");
    wrefresh(gWin->window);
    sleep(3);
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
    fscanf(settings, "%d %d %d %d %d %d %d", &gameSettings[0], &gameSettings[1], &gameSettings[2],&gameSettings[3], &gameSettings[4], &gameSettings[5], &gameSettings[6]);
    fscanf(symbols, "%c %c %c", &gameSymbols[0], &gameSymbols[1], &gameSymbols[2]);
    fclose(settings); fclose(symbols);
}

// ----------------- obj (Frog & Car) functions ---------------------

int yCheck(OBJ* obj){
    if (obj->y<=1) obj->y=2;
    else if (obj->y >= obj->ymax) obj->y = obj->ymax-1;
    return obj->y;
}

bool isFriendly(int friendliness){
    int isF = rand()%friendliness;
    if (isF == 0) return true;
    else return false;
}


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
    frog->xmin = 1; 
    frog->x = win->width / 2;
    frog->y = win->height - 2; 
    frog->mv = MVF;
    frog->width = 1;
    frog->isInCar = false;
    frog->demandsCar = false;
    return frog;
}

OBJ* initCar(WIN* win, int x, int y, int width, char symbol, int dir, int speed, int friendliness) {
    OBJ* car = (OBJ*)malloc(sizeof(OBJ));
    car->win = win;
    car->x = x; car->y = y;
    car->width = width;
    car->symbol = symbol;
    car->dir = dir;
    car->speed = speed;
    car->xmin = 0;
    car->xmax = win->width-2;
    car->ymin = 1;
    car->ymax = win->height-2;
    car->mv = MVC;
    car->isFriendly = isFriendly(friendliness);
    if (car->isFriendly) car->color = CAR_COLOR_FRIEND;
    else car->color = CAR_COLOR;
    car->isInCar = false; // is FROG in car
    return car;
}

OBJ* carRand(OBJ* car, int* gamesettings){
    clearObj(car);
    if (car->x < 2)car->x = car->xmax;
    else car->x = 0;
    int a = rand() % 2 ? 1 : -1;
    car->y = rand() % car->ymax;
    int y = yCheck(car);
    car->y = y;
    car->color = 3+ rand()%3;
    car->width = gamesettings[3] + rand() % gamesettings[4];
    car->speed = 1 + rand() % 2;
    car->isFriendly = isFriendly(gamesettings[6]);
    if (car->isFriendly) car->color = CAR_COLOR_FRIEND;
    else car->color = CAR_COLOR;
    return car;
}

void initCars(OBJ** cars,int carNum, WIN* gWin, char s, int minW, int maxW, int f){
    for (int i=0; i<carNum; i++){
        int y = ((rand() % 2 ? 1 : 2)+i)%gWin->height; 
        if (y >= gWin->height-2) y = gWin->height-3;
        else if (y<=2) y= 3;
        int x = rand() % gWin->width;
        int width = minW + rand() % maxW;
        int speed = 1 + rand() % 3;
        cars[i] = initCar(gWin, x, y, width, s , rand() % 2 ? 1 : -1, speed, f);
        drawObj(cars[i]);
    }
}

OBJ* initObstacle(WIN* win, int x, int y, char symbol){
    OBJ* ob = (OBJ*)malloc(sizeof(OBJ));
    ob->x = x;
    ob->y = y;
    ob->win = win;
    ob->width = 1;
    ob->symbol = symbol;
    return ob;
}

void initObs(OBJ** obs, int obsNum, WIN* gWin, char s){
    for (int i= 0; i < obsNum; i++){
        int x = rand() % gWin->width-2;
        if (x<=1) x=2;
        int y = rand()%gWin->height-2; 
        if (y<=1) y=2;
        obs[i] = initObstacle(gWin,x,y,s);
        drawObj(obs[i]);
    }
}

// Moving OBJs

void moveFrog(OBJ* frog, int dx, int dy, unsigned int frame) {
     if (frame - frog->mv >= MVF) {
        clearObj(frog);
        frog->x = frog->x + dx < frog->xmin ? frog->xmin : frog->x + dx > frog->xmax ? frog->xmax : frog->x + dx;
        frog->y = frog->y + dy < 1 ? 1 : frog->y + dy;
        drawObj(frog);
    }
    frog->mv = frame;
}

void frogMover(int ch, OBJ* frog, int fC){
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

void moveCar(OBJ* car, unsigned int frame) {
    if (frame % car->mv == 0){
    clearObj(car);
        car->x += car->dir * car->speed;
        drawObj(car);
    } 
}

void toStart(OBJ* frog){
    clearObj(frog);
    frog->x = frog->win->width/2;
    frog->y = frog->win->height-2;
    drawObj(frog);
}

void changeLane(OBJ* obj){
    clearObj(obj);
    int a = (rand() % 2 ? 1 : -1) ;
    obj->y += a;
    obj->y = yCheck(obj);
    drawObj(obj);
}

// Collision checks
int collision(OBJ* obj1, OBJ* obj2) {
    if (obj1->y != obj2->y) return 0;
    return ((obj1->x >= obj2->x && obj1->x < obj2->x + obj2->width) || (obj1->x <= obj2->x && obj1->x > obj2->x + obj2->width) );
}

int stopCollision(OBJ* obj1, OBJ* obj2){
    if (obj1->y != obj2->y) return 0;
    if (obj2->dir){
        int x = obj1->x+2;
        return ((x >= obj2->x && x < obj2->x + obj2->width) || (x <= obj2->x && x > obj2->x + obj2->width) );
    }
    else {
        int x = obj1->x-2;
        return ((x >= obj2->x && x < obj2->x + obj2->width) || (x <= obj2->x && x > obj2->x + obj2->width) );
    }
    
}

void carCollision(OBJ* frog, OBJ** cars, int carnum, WINDOW* pWin, WIN* sWin, int f, int ch){
    for (int i = 0; i < carnum; i++){
        bool shouldStop = rand()%2;
        if (shouldStop && stopCollision(frog, cars[i]) == 1 && !cars[i]->isFriendly){
            cars[i]->speed = 0;
        }
        else if (!frog->demandsCar && stopCollision(frog, cars[i]) == 1 && cars[i]->isFriendly){
            changeLane(cars[i]);
        }
        else if (frog->demandsCar && stopCollision(frog, cars[i]) == 1 && cars[i]->isFriendly){
            if(frog->y == cars[i]->y && frog->x < frog->xmax-1 && frog->x > frog->xmin+1){
                clearObj(frog);
                frog->x += cars[i]->dir;
                frog->isInCar = true;
                drawObj(frog);
                
            }
            else {frog->isInCar = false;}
        }
        else if (!shouldStop && collision(frog, cars[i]) == 1 && !cars[i]->isFriendly && !frog->isInCar){ // 1 = frog and car collided
            gameOver(frog->win, sWin, pWin);
        }
        else if (collision(frog, cars[i]) == 0) {frog->isInCar = false; cars[i]->isInCar = false; drawObj(frog);} 
        for (int j = 0; j < carnum; j++){
            if (j!=i && (collision(cars[j], cars[i]) == 1)){
                changeLane(cars[i]);
                if (cars[i]->isInCar){
                    changeLane(frog);
                }
            }
        }
}
    }

void obstacleCollision(OBJ* frog, OBJ** obs, int obsnum, OBJ** cars, int carnum, WINDOW* pWin, WIN* sWin){
     for (int i = 0; i < obsnum; i++){
        if (!frog->isInCar && collision(frog, obs[i]) == 1){
            clearObj(frog);
            drawObj(obs[i]);
            gameOver(frog->win, sWin, pWin);
        }
        drawObj(obs[i]);
            
        for (int j = 0; j < carnum; j++){
            if(collision(obs[i], cars[j])==1){
                clearObj(obs[i]);
                drawObj(cars[j]);
            }
        }
    }	
}

void checkInput(int ch, OBJ* frog, WIN* sWin, WINDOW* pWin){
    if (ch == ERR) ch = NOKEY;
    else if (ch == 'e' && !frog->demandsCar){frog->demandsCar = true;}
    else if (ch == 'e' && frog->demandsCar){frog->demandsCar = false;}
    else if (ch == 'k') {gameOver(frog->win, sWin, pWin);}
}


// ---------------- MAIN LOOP/TIMELINE ------------------

void mainLoop(WIN* gWin, WIN* sWin, WINDOW* pWin, OBJ* frog, OBJ** cars, int* gamesettings, int carnum, OBJ** obs, int obsnum, TIMER* timer){
    int ch, pts = 0;
    char sC = cars[0]->symbol;
    char sO = obs[0]->symbol;
    clock_t start, end;

    nodelay(gWin->window, TRUE);
    keypad(gWin->window, TRUE);
    while (timer->timeLeft > 0){
    start = clock();
    ch = wgetch(gWin->window);
    checkInput(ch, frog, sWin, pWin); 
    frogMover(ch, frog,timer->frameNum);
    for (int i = 0; i < carnum; i++){
        moveCar(cars[i], timer->frameNum);
        if (rand()%2 == 1){cars[i]->speed = 1 + rand() % 3;}
        if (cars[i]->x > gWin->width || cars[i]->x < 0){ cars[i] = carRand(cars[i], gamesettings);}
        carCollision(frog, cars, carnum, pWin, sWin, gamesettings[6], ch);
        box(gWin->window, 0, 0);
    }

    if (frog->y == 1) { // Frog reaches the "top"
            pts++;
            toStart(frog);
            for (int i = 0; i < carnum; i++){clearObj(cars[i]);}
            for (int i = 0; i < obsnum; i++){clearObj(obs[i]);}
            initCars(cars, carnum, gWin, sC, gamesettings[3], gamesettings[4], gamesettings[6]);
            initObs(obs, obsnum, gWin, sO);
    }

    obstacleCollision(frog, obs,obsnum, cars, carnum, pWin, sWin);
    end = clock();
    if (!updateTimer(timer, start, end, pts)){
        gameOver(gWin, sWin, pWin);
    }
    }
    gameOver(gWin, sWin, pWin);
}

// --------- MAIN ------------

int main(){
    srand(time(NULL));
    WINDOW *stdWin = startGame();
    starterScreen(stdWin);

    // Create arrays from files with settings and symbols
    int gameSettings[SETTINGS_NUM];         // 0 - width; 1 - height; 2 - timer; 3&4 - min/max car width, 5 - car num, 6 - 1/x friendly cars
    char gameSymbols[SYMBOL_NUM];           // X - frog; # - car segment; O - obstacle
    arrayFromFiles(gameSettings, gameSymbols);
    
    // Create windows and objects
    WIN* gameWin = initWindow(stdWin, gameSettings[0], gameSettings[1], 0, 0, DELAY_ON);
    WIN* statusWin = initWindow(stdWin, STATUS_WIDTH, 2, 0, gameSettings[1]+1, DELAY_ON);
    TIMER* timer = initTimer(statusWin, gameSettings[2]);


    OBJ* frog = initFrog(gameWin, FROG_COLOR, gameSymbols[0]);
    drawObj(frog);
    
    int carNum = gameSettings[5];
    carNum += 5;
    OBJ* cars[carNum];
    initCars(cars, carNum, gameWin,  gameSymbols[1], gameSettings[3], gameSettings[4], gameSettings[6]);

    int obsNum = gameSettings[1]/2;
    OBJ* obstacles[obsNum];
    initObs(obstacles, obsNum, gameWin, gameSymbols[2]);
   
    mainLoop(gameWin, statusWin, stdWin, frog, cars, gameSettings, carNum, obstacles, obsNum, timer);
    
    // --------- Cleanup ----------
    cleanup(gameWin, statusWin, stdWin);
    refresh();
    return EXIT_SUCCESS;
}