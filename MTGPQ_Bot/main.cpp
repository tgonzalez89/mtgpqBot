#include "botmainwindow.hpp"
#include <QApplication>

#pragma comment(lib, "User32.Lib")
#pragma comment(lib, "Gdi32.Lib")

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    BotMainWindow w;
    w.show();

    return a.exec();
}
