#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SW 80
#define SH 25
#define LBOUND 2
#define RBOUND 78
#define TBOUND 1
#define BBOUND 24

#define ROWS 3
#define COLS 8
#define INV_W 3
#define INV_H 2
#define XSTEP 6
#define YSTEP 3

#define MAX_EB 4
#define RND(x) (rand()%(x))

int invAlive[ROWS][COLS];
int baseX, baseY, dir, tick, moveDelay;
int px, py, lives, score, level;
int bx, by, bActive;
int ebx[MAX_EB], eby[MAX_EB], ebActive[MAX_EB];
int totalInv;
int animFrame;

int ufoX, ufoY, ufoActive, ufoDir;
int ufoSpawnsLeft;
long ufoTimer;

int bunkerState[SH][SW];
unsigned char vidBuf[SH][SW];
unsigned char colorBuf[SH][SW];
unsigned char oldVid[SH][SW];
unsigned char oldCol[SH][SW];

void sfxShoot() {
    sound(1200);
    delay(3);
    nosound();
}

void sfxEnemyHit() {
    sound(150);
    delay(5);
    nosound();
}

void sfxPlayerHit() {
    int i;
    for(i=200; i>50; i-=20) {
        sound(i);
        delay(10);
    }
    nosound();
}

void sfxUfoHit() {
    int i;
    nosound();
    for(i=0; i<5; i++) {
        sound(700 - (i * 120));
        delay(8);
    }
    nosound();
}

void sfxWin() {
    nosound();
    sound(500); delay(50);
    sound(700); delay(50);
    sound(900); delay(100);
    nosound();
}

void sfxLose() {
    nosound();
    sound(300); delay(100);
    sound(200); delay(100);
    sound(100); delay(200);
    nosound();
}

void sfxNextLevel() {
    int i;
    nosound();
    for(i=400; i<800; i+=50) {
        sound(i); delay(20);
    }
    nosound();
}

void initVideo() {
    int y, x;
    for(y=0; y<SH; y++) {
        for(x=0; x<SW; x++) {
            vidBuf[y][x] = ' ';
            colorBuf[y][x] = 7;
            oldVid[y][x] = 0;
            oldCol[y][x] = 0;
        }
    }
    clrscr();
}

void putBuf(int x, int y, unsigned char c) {
    if(x >= 1 && x <= SW && y >= 1 && y <= SH) {
        vidBuf[y-1][x-1] = c;
    }
}

void printStrBuf(int x, int y, char *s) {
    while(*s) {
        putBuf(x++, y, *s++);
    }
}

void render() {
    unsigned char far *vidMem = (unsigned char far *)0xB8000000;
    int y, x, offset;
    unsigned char c, col;
    
    for(y=0; y<SH; y++) {
        for(x=0; x<SW; x++) {
            c = vidBuf[y][x];
            
            if(y == 0 || y == SH-1 || x == 0 || x == SW-1) col = 7; 
            else if(y == BBOUND && c == 24) col = 11;
            else if(y == 2 && ufoActive && x >= ufoX && x <= ufoX+7) col = 13; 
            else if(c == '#' || c == '%' || c == ':' || c == '.') col = 14; 
            else if(c == '|') col = 11;
            else if(c == '*') col = 12; 
            else if(y < BBOUND - 3 && c != ' ' && c != '.') col = 10;
            else if(c == '.') col = 8;
            else col = 7;

            if(c != oldVid[y][x] || col != oldCol[y][x]) {
                offset = (y * SW + x) * 2;
                vidMem[offset] = c;
                vidMem[offset+1] = col;
                oldVid[y][x] = c;
                oldCol[y][x] = col;
            }
        }
    }
}

void clearLogicBuf() {
    int y, x;
    for(y=0; y<SH; y++) {
        for(x=0; x<SW; x++) {
            vidBuf[y][x] = ' ';
        }
    }
}

void drawStars() {
    int i, x, y;
    for(i=0; i<30; i++) {
        x = RND(SW) + 1;
        y = RND(SH) + 1;
        putBuf(x, y, '.');
    }
}

void intro() {
    int i, k;
    int waiting = 1;
    char *title[] = {
        "                     \xDA\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB \xDA\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB \xDA\xDB\xDB   \xDA\xDB\xDB \xDA\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB                    ",
        "                    \xB3\xDF\xDF  \xDB\xDB\xDF\xDF\xBF\xB3 \xDB\xDB\xDF\xDF\xDF\xDF\xDF\xBF\xB3 \xDB\xDB  \xDA \xDB\xDB\xB3\xDF\xDF  \xDB\xDB\xDF\xDF\xBF                    ",
        "                       \xB3 \xDB\xDB   \xB3 \xDB\xDB      \xB3  \xDB\xDB\xDA \xDB\xDB\xBF   \xB3 \xDB\xDB                       ",
        "                       \xB3 \xDB\xDB   \xB3 \xDB\xDB\xDB\xDB\xDB    \xBF  \xDB\xDB\xDB\xDB\xBF    \xB3 \xDB\xDB                       ",
        "                       \xB3 \xDB\xDB   \xB3 \xDB\xDB\xDF\xDF\xBF     \xB3\xDB\xDB  \xDB\xDB    \xB3 \xDB\xDB                       ",
        "                       \xB3 \xDB\xDB   \xB3 \xDB\xDB       \xDA\xDB\xDB\xBF\xBF  \xDB\xDB   \xB3 \xDB\xDB                       ",
        "                       \xB3 \xDB\xDB   \xB3 \xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xB3 \xDB\xDB  \xBF \xDB\xDB   \xB3 \xDB\xDB                       ",
        "                       \xB3\xDF\xDF\xBF   \xB3\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xBF\xB3\xDF\xDF\xBF  \xB3\xDF\xDF\xBF   \xB3\xDF\xDF\xBF                       ",
        " \xDA\xDB\xDB\xDB\xDB\xDB\xDB \xDA\xDB\xDB   \xDA\xDB\xDB \xDA\xDB\xDB    \xDA\xDB\xDB  \xDA\xDB\xDB\xDB\xDB\xDB\xDB  \xDA\xDB\xDB\xDB\xDB\xDB\xDB\xDB  \xDA\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB \xDA\xDB\xDB\xDB\xDB\xDB\xDB\xDB   \xDA\xDB\xDB\xDB\xDB\xDB\xDB  ",
        "\xB3\xDF  \xDB\xDB\xDF\xBF\xB3 \xDB\xDB\xDB \xB3 \xDB\xDB\xB3 \xDB\xDB   \xB3 \xDB\xDB \xDA\xDB\xDB\xDF\xDF  \xDB\xDB\xB3 \xDB\xDB\xDF\xDF  \xDB\xDB\xB3 \xDB\xDB\xDF\xDF\xDF\xDF\xDF\xBF\xB3 \xDB\xDB\xDF\xDF  \xDB\xDB \xDA\xDB\xDB\xDF\xDF  \xDB\xDB ",
        "  \xB3 \xDB\xDB  \xB3 \xDB\xDB\xDB\xDB\xB3 \xDB\xDB\xB3 \xDB\xDB   \xB3 \xDB\xDB\xB3 \xDB\xDB  \xBF \xDB\xDB\xB3 \xDB\xDB  \xBF \xDB\xDB\xB3 \xDB\xDB      \xB3 \xDB\xDB  \xBF \xDB\xDB\xB3 \xDB\xDB  \xBF\xDF\xDF\xBF ",
        "  \xB3 \xDB\xDB  \xB3 \xDB\xDB \xDB\xDB \xDB\xDB\xB3  \xDB\xDB \xDA \xDB\xDB\xBF\xB3 \xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xB3 \xDB\xDB  \xB3 \xDB\xDB\xB3 \xDB\xDB\xDB\xDB\xDB   \xB3 \xDB\xDB\xDB\xDB\xDB\xDB\xDB\xBF\xB3  \xDB\xDB\xDB\xDB\xDB\xDB  ",
        "  \xB3 \xDB\xDB  \xB3 \xDB\xDB  \xDB\xDB\xDB\xDB \xBF  \xDB\xDB \xDB\xDB\xBF \xB3 \xDB\xDB\xDF\xDF  \xDB\xDB\xB3 \xDB\xDB  \xB3 \xDB\xDB\xB3 \xDB\xDB\xDF\xDF\xBF   \xB3 \xDB\xDB\xDF\xDF  \xDB\xDB \xBF\xDF\xDF\xDF\xDF  \xDB\xDB ",
        "  \xB3 \xDB\xDB  \xB3 \xDB\xDB\xBF  \xDB\xDB\xDB  \xBF  \xDB\xDB\xDB\xBF  \xB3 \xDB\xDB  \xB3 \xDB\xDB\xB3 \xDB\xDB  \xB3 \xDB\xDB\xB3 \xDB\xDB      \xB3 \xDB\xDB  \xBF \xDB\xDB \xDA\xDB\xDB  \xBF \xDB\xDB ",
        " \xDA\xDB\xDB\xDB\xDB\xDB\xDB\xB3 \xDB\xDB \xBF  \xDB\xDB   \xBF  \xDB\xBF   \xB3 \xDB\xDB  \xB3 \xDB\xDB\xB3 \xDB\xDB\xDB\xDB\xDB\xDB\xDB\xBF\xB3 \xDB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xB3 \xDB\xDB  \xB3 \xDB\xDB\xB3  \xDB\xDB\xDB\xDB\xDB\xDB\xBF ",
        "\xB3\xDF\xDF\xDF\xDF\xDF\xDF\xBF\xB3\xDF\xDF\xBF  \xBF\xDF\xDF\xBF    \xBF\xDF\xBF    \xB3\xDF\xDF\xBF  \xB3\xDF\xDF\xBF\xB3\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xBF \xB3\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xBF\xB3\xDF\xDF\xBF  \xB3\xDF\xDF\xBF \xBF\xDF\xDF\xDF\xDF\xDF\xDF\xBF  "
    };

    _setcursortype(_NOCURSOR);
    
    k = 0;
    while(waiting) {
        clearLogicBuf();
        drawStars();
        for(i=0; i<16; i++) {
            printStrBuf(1, 4+i, title[i]);
        }
        
        k++;
        if((k / 5) % 2 == 0) {
            printStrBuf(30, 23, "PRESS SPACE TO START");
        }
        
        render();
        delay(50);
        
        if(kbhit()) { 
            int ch = getch();
            if(ch == 32) waiting = 0; 
            if(ch == 'q' || ch == 'Q') {
                nosound();
                _setcursortype(_NORMALCURSOR);
                exit(0);
            }
        }
    }
    clearLogicBuf();
    render();
}

void border() {
    int i;
    for(i=1; i<=SW; i++) { putBuf(i, 1, '-'); putBuf(i, SH, '-'); }
    for(i=1; i<=SH; i++) { putBuf(1, i, '|'); putBuf(SW, i, '|'); }
}

void hud() {
    char buf[80];
    sprintf(buf, "SCORE: %04d  LIVES: %d  LEVEL: %d", score, lives, level);
    printStrBuf(4, 1, buf);
    printStrBuf(SW-20, 1, "[Q] QUIT");
}

void initInv() {
    int r, c;
    for(r=0; r<ROWS; r++) for(c=0; c<COLS; c++) invAlive[r][c] = 1;
    totalInv = ROWS * COLS;
    baseX = 5; baseY = 3; dir = 1; tick = 0;
    animFrame = 0;
    
    moveDelay = 5 - level;
    if(moveDelay < 1) moveDelay = 1;
    
    ufoActive = 0;
    ufoSpawnsLeft = 2;
    ufoTimer = 200 + RND(300);
}

void initBunkers() {
    int i, r, c, startX, startY;
    int bX[3] = {10, 37, 64}; 
    int bW = 6;
    
    for(r=0; r<SH; r++) for(c=0; c<SW; c++) bunkerState[r][c] = 0;
    startY = BBOUND - 4; 
    for(i=0; i<3; i++) {
        startX = bX[i];
        for(r=0; r<2; r++)
            for(c=0; c<bW; c++) 
                bunkerState[startY+r][startX+c] = 4;
    }
}

void resetPlayerPos() {
    px = SW / 2; py = BBOUND; bActive = 0;
}

void softReset() {
    int i;
    baseX = 5; baseY = 3; dir = 1; 
    resetPlayerPos();
    for(i=0; i<MAX_EB; i++) ebActive[i] = 0;
    
    ufoActive = 0;
    nosound();
    
    delay(1000);
}

void initEB() {
    int i; for(i=0; i<MAX_EB; i++) { ebActive[i] = 0; }
}

void drawOneInvader(int x, int y) {
    if(animFrame == 0) {
        putBuf(x, y, 220); putBuf(x+1, y, ' '); putBuf(x+2, y, 220);
        putBuf(x+1, y+1, 223);
    } else {
        putBuf(x, y, ' '); putBuf(x+1, y, 220); putBuf(x+2, y, ' ');
        putBuf(x, y+1, 223); putBuf(x+2, y+1, 223);
    }
}

void drawUFO() {
    if(!ufoActive) return;
    
    if(tick % 4 < 2) {
        printStrBuf(ufoX, ufoY, "/o_o_o\\");
    } else {
        printStrBuf(ufoX, ufoY, "/O_0_O\\");
    }
}

void drawInv() {
    int r, c, x, y;
    for(r=0; r<ROWS; r++) {
        for(c=0; c<COLS; c++) {
            if(invAlive[r][c]) {
                x = baseX + c * XSTEP; y = baseY + r * YSTEP;
                if(x+INV_W < SW && y+INV_H < SH)
                    drawOneInvader(x, y);
            }
        }
    }
    drawUFO();
}

unsigned char bunkerChar(int health) {
    if(health >= 4) return 178;
    if(health == 3) return 177;
    if(health == 2) return 176;
    if(health == 1) return '.';
    return ' ';
}

void drawBunkers() {
    int r, c;
    for(r=0; r<SH; r++) {
        for(c=0; c<SW; c++) {
            if(bunkerState[r][c] > 0) {
                putBuf(c+1, r+1, bunkerChar(bunkerState[r][c]));
            }
        }
    }
}

void drawPlayer() {
    putBuf(px, py, 24); 
    putBuf(px-1, py+1, 223); 
    putBuf(px+1, py+1, 223);
}

void drawBullets() {
    int i;
    if(bActive) putBuf(bx, by, 179);
    for(i=0; i<MAX_EB; i++) if(ebActive[i]) putBuf(ebx[i], eby[i], '*');
}

void moveInv() {
    int r, c, minX = 1000, maxX = -1000, x;
    int edgeHit = 0;
    
    animFrame = 1 - animFrame;

    for(r=0; r<ROWS; r++) {
        for(c=0; c<COLS; c++) {
            if(invAlive[r][c]) {
                x = baseX + c * XSTEP;
                if(x < minX) minX = x;
                if(x + INV_W > maxX) maxX = x + INV_W;
            }
        }
    }
    
    if(dir > 0 && maxX >= RBOUND - 1) edgeHit = 1;
    if(dir < 0 && minX <= LBOUND + 1) edgeHit = 1;
    
    if(edgeHit) {
        dir = -dir;
        baseY++;
    } else {
        baseX += dir;
    }
}

void updateUFO() {
    int i;
    if(ufoActive) {
        ufoX += ufoDir;
        
        sound(150 + (tick % 4) * 25);
        
        if(RND(100) < 10) {
             for(i=0; i<MAX_EB; i++) if(!ebActive[i]) {
                ebActive[i] = 1; 
                ebx[i] = ufoX + 3; 
                eby[i] = ufoY + 1;
                break;
            }
        }

        if(ufoX < LBOUND || ufoX > RBOUND - 6) {
            ufoActive = 0;
            nosound();
        }
    } else {
        if(ufoSpawnsLeft > 0) {
            ufoTimer--;
            if(ufoTimer <= 0) {
                ufoActive = 1;
                ufoSpawnsLeft--;
                ufoY = 2;
                if(RND(2) == 0) {
                    ufoX = LBOUND + 1; ufoDir = 1;
                } else {
                    ufoX = RBOUND - 7; ufoDir = -1;
                }
                ufoTimer = 500 + RND(500);
            }
        }
    }
}

void updatePlayerBullet() {
    int r, c, x, y;
    if(!bActive) return;
    by--;
    if(by < TBOUND) { bActive = 0; return; }

    if(ufoActive && bx >= ufoX && bx <= ufoX+6 && by == ufoY) {
        ufoActive = 0;
        bActive = 0;
        score += 100 + RND(200);
        sfxUfoHit();
        return;
    }

    if(bunkerState[by-1][bx-1] > 0) {
        bunkerState[by-1][bx-1]--;
        bActive = 0;
        return;
    }

    for(r=0; r<ROWS; r++) {
        for(c=0; c<COLS; c++) {
            if(invAlive[r][c]) {
                x = baseX + c * XSTEP; 
                y = baseY + r * YSTEP;
                
                if(bx >= x && bx < x + INV_W && by >= y && by < y + INV_H) {
                    invAlive[r][c] = 0;
                    sfxEnemyHit();
                    score += 50; 
                    totalInv--;
                    bActive = 0;
                    return;
                }
            }
        }
    }
}

void firePlayer() {
    if(!bActive) { 
        bActive = 1; 
        bx = px; 
        by = py - 1; 
        sfxShoot();
    }
}

void spawnEnemyBullet() {
    int i, c, r, x, y, shooterR;
    int chance = 10 + (level * 2);
    
    if(RND(100) < chance) { 
        c = RND(COLS);
        shooterR = -1;
        for(r=ROWS-1; r>=0; r--) {
            if(invAlive[r][c]) { shooterR = r; break; }
        }
        
        if(shooterR != -1) {
            for(i=0; i<MAX_EB; i++) if(!ebActive[i]) {
                x = baseX + c * XSTEP + 1; 
                y = baseY + shooterR * YSTEP + 2;
                ebActive[i] = 1; ebx[i] = x; eby[i] = y;
                break;
            }
        }
    }
}

void updateEnemyBullets() {
    int i;
    for(i=0; i<MAX_EB; i++) {
        if(ebActive[i]) {
            eby[i]++;
            if(eby[i] > BBOUND) { ebActive[i] = 0; continue; }
            
            if(bunkerState[eby[i]-1][ebx[i]-1] > 0) {
                bunkerState[eby[i]-1][ebx[i]-1]--;
                ebActive[i] = 0;
                continue;
            }

            if(ebx[i] >= px-1 && ebx[i] <= px+1 && eby[i] >= py) { 
                ebActive[i] = 0; 
                lives--; 
                sfxPlayerHit();
                
                if(lives > 0) {
                    softReset();
                }
            }
        }
    }
}

int invReachedPlayer() {
    int r, c, y;
    for(r=0; r<ROWS; r++) {
        for(c=0; c<COLS; c++) {
            if(invAlive[r][c]) {
                y = baseY + r * YSTEP + INV_H;
                if(y >= py) return 1;
            }
        }
    }
    return 0;
}

void input() {
    int ch;
    if(kbhit()) {
        ch = getch();
        if(ch == 0 || ch == 0xE0) { 
            ch = getch();
            if(ch == 75) { if(px > LBOUND + 2) px--; } 
            else if(ch == 77) { if(px < RBOUND - 2) px++; }
        }
        else {
            if(ch == ' ') firePlayer();
            else if(ch == 'q' || ch == 'Q') { lives = 0; }
        }
    }
}

void frame() {
    clearLogicBuf();
    border();
    hud();
    drawBunkers();
    drawInv();
    drawPlayer();
    drawBullets();
    render();
}

int gameOverScreen() {
    int waiting = 1;
    char buf[50];
    
    nosound();
    sfxLose();
    
    while(waiting) {
        clearLogicBuf();
        border();
        printStrBuf(35, 10, "GAME OVER");
        sprintf(buf, "FINAL SCORE: %d", score);
        printStrBuf(32, 12, buf);
        printStrBuf(25, 15, "[SPACE] RESTART      [Q] QUIT");
        render();
        
        if(kbhit()) {
            int ch = getch();
            if(ch == 32) return 1;
            if(ch == 'q' || ch == 'Q') return 0;
        }
        delay(100);
    }
    return 0;
}

int main() {
    int gameRunning = 1;
    srand(time(NULL));
    initVideo();
    
    intro();
    
    while(gameRunning) {
        score = 0;
        lives = 3;
        level = 1;
        
        while(lives > 0) {
            initInv();
            resetPlayerPos();
            initBunkers();
            initEB();
            
            while(lives > 0 && totalInv > 0) {
                input();
                tick++;
                if(tick % moveDelay == 0) moveInv();
                
                if(tick % 2 == 0) updateUFO();

                updatePlayerBullet();
                if(tick % 4 == 0) spawnEnemyBullet();
                updateEnemyBullets();
                frame();
                
                if(invReachedPlayer()) { 
                    lives--;
                    sfxPlayerHit();
                    if(lives > 0) softReset();
                }
                delay(20);
            }
            
            if(lives > 0 && totalInv <= 0) {
                sfxNextLevel();
                level++;
                delay(1000);
            }
        }
        
        if(gameOverScreen() == 0) {
            gameRunning = 0;
        }
    }
    
    nosound();
    clrscr();
    _setcursortype(_NORMALCURSOR);
    return 0;
}
