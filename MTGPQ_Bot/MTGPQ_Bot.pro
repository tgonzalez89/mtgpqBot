QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MTGPQ_Bot
TEMPLATE = app

SOURCES  += main.cpp \
            botmainwindow.cpp \
            bot.cpp \
            image.cpp \
            mouse.cpp \
            util.cpp

HEADERS  += redirectstream.hpp \
            botmainwindow.hpp \
            bot.hpp \
            image.hpp \
            mouse.hpp \
            util.hpp \
            qthreadbotwrapper.hpp

FORMS += botmainwindow.ui

INCLUDEPATH += C:\OpenCV\OpenCV-3.1.0\build\include

LIBS += -LC:\OpenCV\OpenCV-3.1.0\build\x64\vc14\lib
LIBS += -lopencv_world310

DEFINES += WINVER=0x0602

win32:RC_ICONS += mtgpq_bot.ico
