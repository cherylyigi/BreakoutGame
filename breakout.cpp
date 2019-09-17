// Author: Jieying(Cheryl) Lin
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include <sstream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;

// X11 structures
Display* display;
Window window;

// fixed frames per second animation
int FPS = 60;
int score = 0;
bool playGame = false;
bool endGame = false;

// get current time
unsigned long now() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

struct Brick {
    int x, y;
    bool firstKill = false;
};

// entry point
int main( int argc, char *argv[] ) {
    istringstream fps(argv[1]);
    if (!(fps >> FPS)) {
        std::cerr << "Invalid number: " << argv[1] << '\n';
        return 1;
    }
    if (FPS < 10 || FPS > 60) {
        std::cerr << "Invalid number: " << argv[1] << '\n';
        return 1;
    }
    
    // create window
    display = XOpenDisplay("");
    if (display == NULL) exit (-1);
    int screennum = DefaultScreen(display);
    long background = WhitePixel(display, screennum);
    long foreground = BlackPixel(display, screennum);
    window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                 10, 10, 1280, 800, 2, foreground, background);
    
    // set events to monitor and display window
    XSelectInput(display, window, ButtonPressMask | KeyPressMask);
    XMapRaised(display, window);
    XFlush(display);
    
    // ball postition, size, and velocity
    XPoint ballPos;
    ballPos.x = 50;
    ballPos.y = 300;
    int ballSize = 30;
    int ballSpeed;
    
    istringstream enterSpeed(argv[2]);
    if (!(enterSpeed >> ballSpeed)) {
        std::cerr << "Invalid number: " << argv[2] << '\n';
        return 1;
    }
    if (ballSpeed < 1 || ballSpeed > 10) {
        std::cerr << "Invalid number: " << argv[2] << '\n';
        return 1;
    }
    XPoint ballDir;
    ballDir.x = ballSpeed;
    ballDir.y = ballSpeed;
    
    // paddle position, size
    XPoint rectPos;
    rectPos.x = 225;
    rectPos.y = 600;
    int paddleWide = 500;
    
    // bricks position, size
    vector <Brick> bricks;
    for (int row = 0; row < 15; row++) {
        for (int colum = 0; colum < 5; colum++) {
            Brick brick;
            brick.x = 40 + (row * 80);
            brick.y = 40 + (colum * 30);
            bricks.push_back(brick);
        }
    }
    
    // create gc for drawing
    GC gc = XCreateGC(display, window, 0, 0);
    XWindowAttributes w;
    XGetWindowAttributes(display, window, &w);
    
    // create gc for drawing bricks
    GC brickColor = XCreateGC(display, window, 0, 0);
    XColor xcolour;
    Colormap cmap;
    cmap= XDefaultColormap(display,screennum);
    xcolour.flags = DoRed | DoGreen | DoBlue;
    xcolour.red = 54000; xcolour.green = 12800; xcolour.blue = 12800;
    XAllocColor(display, cmap, &xcolour);
    XSetForeground(display, brickColor, xcolour.pixel);
    XSetBackground(display, brickColor, WhitePixel(display, screennum));
    XSetFillStyle(display, brickColor, FillSolid);
    
    // create gc for bloody bricks
    GC bloodyBrick = XCreateGC(display, window, 0, 0);
    XColor xcolourB;
    xcolourB.flags = DoRed | DoGreen | DoBlue;
    xcolourB.red = 54000; xcolourB.green = 12800; xcolourB.blue = 12800;
    XAllocColor(display, cmap, &xcolourB);
    XSetForeground(display, bloodyBrick, xcolourB.pixel);
    XSetBackground(display, bloodyBrick, WhitePixel(display, screennum));
    XSetFillStyle(display, bloodyBrick, FillSolid);
    
    // create gc for drawing message
    GC message = XCreateGC(display, window, 0, 0);
    XColor xcolourM;
    xcolourM.flags = DoBlue | DoRed | DoGreen;
    xcolourM.red = 17600; xcolourM.green = 17600; xcolourM.blue = 54000;
    XAllocColor(display, cmap, &xcolourM);
    XSetForeground(display, message, xcolourM.pixel);
    XSetBackground(display, message, WhitePixel(display, screennum));
    XSetFillStyle(display, message, FillSolid);
    
    // load a larger font
    XFontStruct * font;
    font = XLoadQueryFont (display, "12x24");
    XSetFont (display, message, font->fid);
    
    // DOUBLE BUFFER
    // create bimap (pximap) to us a other buffer
    int depth = DefaultDepth(display, DefaultScreen(display));
    Pixmap    buffer = XCreatePixmap(display, window, w.width, w.height, depth);
    
    // save time of last window paint
    unsigned long lastRepaint = 0;
    
    // event handle for current event
    XEvent event;
    
    // event loop
    while ( true ) {
        
        // process if we have any events
        if (XPending(display) > 0) {
            XNextEvent( display, &event );
            
            switch ( event.type ) {
                    
                    // mouse button press
                case ButtonPress:
                    playGame = true;
                    break;
                    
                case KeyPress: // any keypress
                    KeySym key;
                    char text[10];
                    int i = XLookupString( (XKeyEvent*)&event, text, 10, &key, 0 );
                    
                    // move right
                    if ( i == 1 && (text[0] == 'd' || text[0] == 'D')) {
                        if (rectPos.x < w.width - paddleWide) {
                            rectPos.x += 40;
                        }
                    }
                    
                    // move left
                    if ( i == 1 && (text[0] == 'a' || text[0] == 'A')) {
                        if (rectPos.x > 0) {
                            rectPos.x -= 40;
                        }
                    }
                    
                    // move up
                    if ( i == 1 && (text[0] == 'w' || text[0] == 'W')) {
                        if (rectPos.y > 300) {
                            rectPos.y -= 10;
                        }
                    }
                    
                    // move down
                    if ( i == 1 && (text[0] == 's' || text[0] == 'S')) {
                        if (rectPos.y < 600) {
                            rectPos.y += 10;
                        }
                    }
                    
                    // retry
                    if ( i == 1 && (text[0] == 'r' || text[0] == 'R')) {
                        if (endGame) {
                            endGame = false;
                            playGame = true;
                            score = 0;
                            ballPos.x = 50;
                            ballPos.y = 300;
                            ballDir.x = ballSpeed;
                            ballDir.y = ballSpeed;
                            rectPos.x = 225;
                            rectPos.y = 600;
                            bricks.clear();
                            for (int row = 0; row < 15; row++) {
                                for (int colum = 0; colum < 5; colum++) {
                                    Brick brick;
                                    brick.x = 40 + (row * 80);
                                    brick.y = 40 + (colum * 30);
                                    bricks.push_back(brick);
                                }
                            }
                        }
                    }
                    
                    // quit game
                    if ( i == 1 && (text[0] == 'q' || text[0] == 'Q')) {
                        XCloseDisplay(display);
                        exit(0);
                    }
                    break;
            }
        }
        
        unsigned long end = now();    // get current time in microsecond
        
        if (end - lastRepaint > 1000000 / FPS) {
            XClearWindow(display, window);
            XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
            XFillRectangle(display, buffer, gc,
                           0, 0, w.width, w.height);
            
            if (playGame) {
                
                // draw score
                std::ostringstream os;
                os << "Score: " << score;
                string currentScore = os.str();
                int length = currentScore.length();
                XDrawImageString(display, buffer, brickColor, 25, 25, currentScore.c_str(), currentScore.length());
                
                XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
                
                // draw rectangle
                XDrawRectangle(display, buffer, gc, rectPos.x, rectPos.y, paddleWide, 25);
                
                // draw bricks
                for (vector<Brick>::iterator it = bricks.begin(); it != bricks.end(); ++it) {
                    if (it->firstKill) {
                        XFillRectangle(display, buffer, bloodyBrick, it->x, it->y, 80, 30);
                    } else {
                        XDrawRectangle(display, buffer, brickColor, it->x, it->y, 80, 30);
                    }
                }
                
                
                
                // draw ball from centre
                XFillArc(display, buffer, gc,
                         ballPos.x - ballSize/2,
                         ballPos.y - ballSize/2,
                         ballSize, ballSize,
                         0, 360*64);
                
                // update ball position
                ballPos.x += ballDir.x;
                ballPos.y += ballDir.y;
                
                // bounce ball
                if (ballPos.x + ballSize/2 > w.width ||
                    ballPos.x - ballSize/2 < 0)
                    ballDir.x = -ballDir.x;
                if (ballPos.y - ballSize/2 < 0)
                    ballDir.y = -ballDir.y;
                
                if (ballPos.y + ballSize/2 > w.height) {
                    playGame = false;
                    endGame = true;
                }
                
                // touch paddle
                if ((ballPos.x + ballSize/2 > rectPos.x
                     && ballPos.x + ballSize/2 < rectPos.x + paddleWide) ||
                    (ballPos.x - ballSize/2 > rectPos.x
                     && ballPos.x - ballSize/2 < rectPos.x + paddleWide)
                    ) {
                    if ((ballPos.y + ballSize/2 > rectPos.y) &&
                        (ballPos.y + ballSize/2 < rectPos.y + ballSpeed + 1)){
                        if (ballDir.y > 0) {
                            ballDir.y = -ballDir.y;
                        }
                    }
                }
                
                // touch bricks
                for (vector<Brick>::iterator it = bricks.begin(); it != bricks.end(); ++it) {
                    if ((ballPos.x + ballSize/2 >= it->x &&
                         ballPos.x + ballSize/2 <= (it->x + 80)) ||
                        (ballPos.x - ballSize/2 >= it->x &&
                         ballPos.x - ballSize/2 <= (it->x + 80))){
                            if ((ballPos.y + ballSize/2 >= it->y &&
                                 ballPos.y + ballSize/2 <= (it->y + 30)) ||
                                (ballPos.y - ballSize/2 >= it->y &&
                                 ballPos.y - ballSize/2 <= (it->y + 30))) {
                                    // upper and lower bound of the brick
                                    if (ballPos.y + ballSize/2 < (it->y + 5) ||
                                        ballPos.y - ballSize/2 > (it->y + 30 - (ballSpeed + 1))) {
                                        ballDir.y = -ballDir.y;
                                    }
                                    // left and right bound of the brick
                                    if (ballPos.x + ballSize/2 < (it->x + 5) ||
                                        ballPos.x - ballSize/2 > (it->x + 80 - (ballSpeed + 1))) {
                                        ballDir.x = -ballDir.x;
                                    }
                                    if (it->firstKill) {
                                        score += 20;
                                        it = bricks.erase(it);
                                    } else {
                                        score += 10;
                                        it->firstKill = true;
                                    }
                                    break;
                                }
                        }
                }
                
                // reload bricks
                if (bricks.size() == 0) {
                    ballPos.x = 50;
                    ballPos.y = 300;
                    for (int row = 0; row < 15; row++) {
                        for (int colum = 0; colum < 5; colum++) {
                            Brick brick;
                            brick.x = 40 + (row * 80);
                            brick.y = 40 + (colum * 30);
                            bricks.push_back(brick);
                        }
                    }
                }
            } else if(!playGame && !endGame){
                string infomation = "Breakout! made by Jieying Lin";
                string ID = "Student ID: 20648298";
                string rule1 = "Press A to move left, D to move right, W to move up, S to move down";
                string rule2 = "Press Q to quit Game, Press R after fail to retry";
                string rule3 = "Each brick has two lives, ball needs to hit it twice to kill it";
                string rule4 = "First life of brick worths 10 marks, the second one worth 20 marks";
                string start = "Click screen to start!";
                XDrawImageString(display, buffer, message, 450, 200, infomation.c_str(), infomation.length());
                XDrawImageString(display, buffer, message, 475, 250, ID.c_str(), ID.length());
                XDrawImageString(display, buffer, message, 300, 350, rule1.c_str(), rule1.length());
                XDrawImageString(display, buffer, message, 350, 400, rule2.c_str(), rule2.length());
                XDrawImageString(display, buffer, message, 300, 450, rule4.c_str(), rule4.length());
                XDrawImageString(display, buffer, message, 300, 500, rule3.c_str(), rule3.length());
                XDrawImageString(display, buffer, message, 475, 550, start.c_str(), start.length());
            } else if (endGame) {
                string fail = "Sorry, try better next time!";
                string retry = "Press R to retry, Q to quite";
                XDrawImageString(display, buffer, message, 450, 250, fail.c_str(), fail.length());
                XDrawImageString(display, buffer, message, 450, 300, retry.c_str(), retry.length());
            }
            
            XCopyArea(display, buffer, window, gc,
                      0, 0, w.width, w.height,  // region of pixmap to copy
                      0, 0);
            
            XFlush( display );
            
            lastRepaint = now(); // remember when the paint happened
        }
        
        // IMPORTANT: sleep for a bit to let other processes work
        if (XPending(display) == 0) {
            usleep(1000000 / FPS - (now() - lastRepaint));
        }
    }
    
    XCloseDisplay(display);
}
