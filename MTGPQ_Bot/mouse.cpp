#include "mouse.hpp"
#include <cstdlib>

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Initialize customization options to default values.
////////////////////////////////////////////////////////////////////////////////////////////////////
HWND Mouse::WindowHWND = NULL;
int Mouse::PositionMode = MOUSE_POSITION_MODE_WINDOW;
int Mouse::DragMode = MOUSE_DRAG_MODE_MOUSEMOVE;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Mouse::LeftClick Send a left click to WindowHWND.
/// \param [in] p Coordinate where to click.
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mouse::LeftClick(POINT p) {
    if (Mouse::PositionMode == MOUSE_POSITION_MODE_SCREEN)
        ScreenToClient(Mouse::WindowHWND, &p);
    SendMessage(Mouse::WindowHWND, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y));
    SendMessage(Mouse::WindowHWND, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(p.x, p.y));
    SendMessage(Mouse::WindowHWND, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Mouse::Drag Send a drag command (press, move, release) to WindowHWND.
/// \param [in] p1 Start coordinate.
/// \param [in] p2 End coordinate.
/// \param [in] speed How fast is the mouse moved. 0 = no delay (fastest), default = 1.
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mouse::Drag(POINT p1, POINT p2, int speed) {
    if (Mouse::PositionMode == MOUSE_POSITION_MODE_SCREEN) {
        ScreenToClient(Mouse::WindowHWND, &p1);
        ScreenToClient(Mouse::WindowHWND, &p2);
    }
    int x_diff = p2.x - p1.x;
    int y_diff = p2.y - p1.y;
    int i_max = abs(x_diff) > abs(y_diff) ? abs(x_diff) : abs(y_diff);
    POINT p = p1;
    SendMessage(Mouse::WindowHWND, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y));
    for (int i = 1; i <= i_max; i++) {
        p.x = p1.x + i * x_diff / i_max;
        p.y = p1.y + i * y_diff / i_max;
        if (Mouse::DragMode == MOUSE_DRAG_MODE_MOUSEMOVE)
            SendMessage(Mouse::WindowHWND, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(p.x, p.y));
        else
            SendMessage(Mouse::WindowHWND, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y));
        if (speed < 0) speed = 0;
        Sleep(speed);
    }
    SendMessage(Mouse::WindowHWND, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y));
}
