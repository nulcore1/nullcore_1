// ============================================================
//  nullcore mini-games — original, button-driven, no extra HW.
//  Controls (3-button nullcore layout):
//    Jumper:  OK = jump,            UP+DOWN = exit
//    Snake:   UP = turn left,       DOWN = turn right, UP+DOWN = exit
// ============================================================
#include "games.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <Arduino.h>
#include <globals.h>

// nullcore palette (matches the wiki): dark ground, light-blue sprites
#define NC_BG    0x0841 // ~ #0d141d dark
#define NC_FG    0x96FE // light blue
#define NC_RED   0xF9A6

static void waitRetryOrExit(bool &retry) {
    // true -> play again (OK), false -> leave (UP+DOWN / LEFT)
    while (true) {
        if (check(EscPress)) { retry = false; return; }
        if (check(SelPress)) { retry = true; return; }
        delay(40);
    }
}

static void gameOverScreen(int score) {
    tft.fillScreen(NC_BG);
    tft.setTextColor(NC_RED, NC_BG);
    tft.setTextSize(2);
    tft.setCursor(tftWidth / 2 - 52, tftHeight / 2 - 24);
    tft.print("GAME OVER");
    tft.setTextSize(1);
    tft.setTextColor(NC_FG, NC_BG);
    tft.setCursor(tftWidth / 2 - 30, tftHeight / 2 + 4);
    tft.printf("score %d", score);
    tft.setTextColor(TFT_LIGHTGREY, NC_BG);
    tft.setCursor(tftWidth / 2 - 66, tftHeight / 2 + 26);
    tft.print("OK = retry");
    tft.setCursor(tftWidth / 2 - 66, tftHeight / 2 + 40);
    tft.print("UP+DOWN = exit");
}

// ---------------------------------------------------------------------------
//  JUMPER — endless runner, tap OK to jump the red blocks
// ---------------------------------------------------------------------------
void game_jumper() {
    const int W = tftWidth, H = tftHeight;
    const int ps = 16;              // player size
    const int groundTop = H - 30;   // y of the ground line
    const float G = 1.15f, JUMP = -11.5f;

    while (true) {
        float px = 26, py = groundTop - ps, vy = 0;
        bool onGround = true;
        int obW = 12, obH = 18, obX = W, speed = 5;
        int score = 0, prevObX = obX, prevPy = (int)py;
        bool over = false;

        tft.fillScreen(NC_BG);
        tft.drawFastHLine(0, groundTop, W, NC_FG);

        while (!over) {
            if (check(EscPress)) return;
            if (check(SelPress) && onGround) {
                vy = JUMP;
                onGround = false;
            }

            py += vy;
            vy += G;
            if (py >= groundTop - ps) {
                py = groundTop - ps;
                vy = 0;
                onGround = true;
            }

            obX -= speed;
            if (obX < -obW) {
                obX = W + random(0, 140);
                score++;
                if (score % 6 == 0 && speed < 11) speed++;
            }

            int pxi = (int)px, pyi = (int)py;
            bool hit = (pxi < obX + obW) && (pxi + ps > obX) && (pyi + ps > groundTop - obH);
            if (hit) over = true;

            // erase previous, repaint ground, draw current
            tft.fillRect(prevObX, groundTop - obH, obW, obH, NC_BG);
            tft.fillRect((int)px, prevPy, ps, ps, NC_BG);
            tft.drawFastHLine(0, groundTop, W, NC_FG);
            tft.fillRect(obX, groundTop - obH, obW, obH, NC_RED);
            tft.fillRect(pxi, pyi, ps, ps, NC_FG);
            prevObX = obX;
            prevPy = pyi;

            tft.setTextColor(NC_FG, NC_BG);
            tft.setTextSize(1);
            tft.setCursor(6, 6);
            tft.printf("%d  ", score);

            delay(33);
        }

        gameOverScreen(score);
        bool retry;
        waitRetryOrExit(retry);
        if (!retry) return;
    }
}

// ---------------------------------------------------------------------------
//  SNAKE — UP turns left, DOWN turns right (relative)
// ---------------------------------------------------------------------------
void game_snake() {
    const int cell = 10;
    const int offY = 18;
    const int cols = tftWidth / cell;
    const int rows = (tftHeight - offY) / cell;
    static const int dx[4] = {1, 0, -1, 0};
    static const int dy[4] = {0, 1, 0, -1};
    static int bx[600], by[600];

    while (true) {
        int dir = 0, len = 3;
        bx[0] = cols / 2;     by[0] = rows / 2;
        bx[1] = cols / 2 - 1; by[1] = rows / 2;
        bx[2] = cols / 2 - 2; by[2] = rows / 2;
        int fx = random(0, cols), fy = random(0, rows);
        int score = 0;
        bool over = false;

        tft.fillScreen(NC_BG);
        for (int i = 0; i < len; i++)
            tft.fillRect(bx[i] * cell, offY + by[i] * cell, cell - 1, cell - 1, NC_FG);
        tft.fillRect(fx * cell, offY + fy * cell, cell - 1, cell - 1, NC_RED);

        while (!over) {
            if (check(EscPress)) return;
            if (check(PrevPress)) dir = (dir + 3) & 3; // UP = turn left
            if (check(NextPress)) dir = (dir + 1) & 3; // DOWN = turn right

            int nx = bx[0] + dx[dir], ny = by[0] + dy[dir];
            if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) { over = true; break; }
            for (int i = 0; i < len; i++)
                if (bx[i] == nx && by[i] == ny) { over = true; break; }
            if (over) break;

            bool grow = (nx == fx && ny == fy);
            int tailx = bx[len - 1], taily = by[len - 1];
            for (int i = len - 1; i > 0; i--) { bx[i] = bx[i - 1]; by[i] = by[i - 1]; }
            bx[0] = nx; by[0] = ny;

            if (grow) {
                if (len < 590) { bx[len] = tailx; by[len] = taily; len++; }
                score++;
                // place new food off the snake
                for (int tries = 0; tries < 40; tries++) {
                    fx = random(0, cols); fy = random(0, rows);
                    bool onSnake = false;
                    for (int i = 0; i < len; i++) if (bx[i] == fx && by[i] == fy) { onSnake = true; break; }
                    if (!onSnake) break;
                }
                tft.fillRect(fx * cell, offY + fy * cell, cell - 1, cell - 1, NC_RED);
            } else {
                tft.fillRect(tailx * cell, offY + taily * cell, cell - 1, cell - 1, NC_BG);
            }

            tft.fillRect(nx * cell, offY + ny * cell, cell - 1, cell - 1, NC_FG);
            tft.setTextColor(NC_FG, NC_BG);
            tft.setTextSize(1);
            tft.setCursor(6, 4);
            tft.printf("SNAKE  %d  ", score);

            int d = 160 - score * 4;
            delay(d < 70 ? 70 : d);
        }

        gameOverScreen(score);
        bool retry;
        waitRetryOrExit(retry);
        if (!retry) return;
    }
}
