#ifndef QTHREADBOTWRAPPER_HPP
#define QTHREADBOTWRAPPER_HPP

#include "bot.hpp"
#include "mouse.hpp"
#include <QThread>

class QThreadBotWrapper : public QThread, public Bot
{
    Q_OBJECT

public:
    QThreadBotWrapper() {
        initOnly = false;
    }
    void Stop() {
        if (!stop) {
            cout << "-I- QThreadBotWrapper::Stop - Stopping bot. Please wait..." << endl;
            stop = true;
        }
    }
    void makeBestPlay() {
        cout << endl;
        if (!StartUp(3)) {
            cout << "-E- QThreadBotWrapper::makeBestPlay - Couldn't find a valid Planeswalker." << endl;
            return;
        }
        vector<pair<int,int>> p(6, pair<int,int>(0, 0));
        vector<vector<char>> Board;
        UpdateGameWindowStatus();
        UpdateBoardState(Board);
        int BestMove = FindBestMove(Board, p);
        cout << "-D- QThreadBotWrapper::makeBestPlay - Current board:" << endl;
        for (unsigned int j = 0; j < Board.size(); j++) {
            cout << "    ";
            for (unsigned int i = 0; i < Board[j].size(); i++) {
                cout << Board[i][j];
                if (i < Board[j].size()-1) cout << "  ";
            }
            cout << endl;
        }
        cout << "-D- QThreadBotWrapper::makeBestPlay - Best move ID: " << BestMove << endl;
        if (BestMove >= 0) {
            cout << "-D- QThreadBotWrapper::makeBestPlay - Best move: " << Moves[BestMove] << endl;
            cout << "-D- QThreadBotWrapper::makeBestPlay - Best move position: (" << p[4].first+1 << "," << p[4].second+1 << ") (" << p[5].first+1 << "," << p[5].second+1 << ")" << endl;
            cout << "-D- QThreadBotWrapper::makeBestPlay - Best move color: " << Board[p[1].first][p[1].second] << endl;
            Mouse::Drag({xBoardCoords[p[4].first], yBoardCoords[p[4].second]},
                        {xBoardCoords[p[5].first], yBoardCoords[p[5].second]}, 5);
        } else cout << "-E- QThreadBotWrapper::makeBestPlay - Couldn't find a valid move." << endl;
    }
    bool initOnly;

signals:
    void done();

protected:
    void run() {
        if (initOnly) {
            cout << endl;
            Initialize();
        } else {
            stop = false;
            Run();
            emit done();
            Sleep(1000);
            cout << "-I- QThreadBotWrapper::run - Bot stopped." << endl;
        }
    }
};

#endif // QTHREADBOTWRAPPER_HPP
