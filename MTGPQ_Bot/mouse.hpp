#ifndef MOUSE_H
#define MOUSE_H

#include <Windows.h>

#define MOUSE_POSITION_MODE_SCREEN 0
#define MOUSE_POSITION_MODE_WINDOW 1
#define MOUSE_DRAG_MODE_MOUSEMOVE 0
#define MOUSE_DRAG_MODE_LBUTTONDOWN 1

class Mouse
{
public:
    static void LeftClick(POINT p);
    static void Drag(POINT p1, POINT p2, int speed = 1);

    static HWND WindowHWND;
    static int PositionMode;    // MOUSE_POSITION_MODE_SCREEN  = Positions passed to Mouse methods are assumed to be relative to the screen
                                // MOUSE_POSITION_MODE_WINDOW  = Positions passed to Mouse methods are assumed to be relative to the window defined by WindowHWND
    static int DragMode;        // MOUSE_DRAG_MODE_MOUSEMOVE   = Use the WM_MOUSEMOVE message to move the mouse while dragging
                                // MOUSE_DRAG_MODE_LBUTTONDOWN = Use the WM_LBUTTONDOWN message to move the mouse while dragging
};

#endif // MOUSE_H
