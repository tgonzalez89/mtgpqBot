#ifndef BOT_H
#define BOT_H

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <ctime>
#include <opencv2/opencv.hpp>
#include <Windows.h>

using namespace std;
using namespace cv;

class Bot
{

private:
    struct ImageData {
        Mat mat;
        double tol;
        char color;
    };

public:
    // Methods
    Bot();
    void Run();
    bool SetPWsOrder(vector<string> PWsOrderedList);
    vector<string> GetPWsOrder();
    bool Initialize();
    void CalibrateImage(string ImageName, int imgCount);
    void TestFindImage(string ImageName, double TolerancePerc);

    // Variables
    bool pause;
    bool stop;
    bool playQB;
    bool playEvents;
    bool joinQB;
    bool joinEvents;
    HWND GameWindowHWND;
    unordered_map<string,ImageData> Images;

protected:
    // Methods
    int FindBestMove(vector<vector<char>> Board, vector<pair<int,int>> &p);
    bool UpdateGameWindowStatus();
    void UpdateBoardState(vector<vector<char>> &Board);
    bool StartUp(int maxWait);

    // Variables
    int xBoardCoords[7];
    int yBoardCoords[7];

    // Consts
    const vector<string> Moves = {
        "N2,N1,L1,R1,S1", "S2,S1,L1,R1,N1", "L2,L1,N1,S1,R1", "R2,R1,N1,S1,L1",
        "N2,N1,L2,L1,S1", "N2,N1,L2,L1,R1", "N2,N1,R2,R1,S1", "N2,N1,R2,R1,L1",
        "S2,S1,L2,L1,N1", "S2,S1,L2,L1,R1", "S2,S1,R2,R1,N1", "S2,S1,R2,R1,L1",
        "N2,N1,S2,S1,L1", "N2,N1,S2,S1,R1", "L2,L1,R2,R1,N1", "L2,L1,R2,R1,S1",
        "N2,N1,S1,L1", "N2,N1,S1,R1", "S2,S1,N1,L1", "S2,S1,N1,R1",
        "L2,L1,R1,N1", "L2,L1,R1,S1", "R2,R1,L1,N1", "R2,R1,L1,S1",
        "N2,N1,S1", "N2,N1,L1", "N2,N1,R1",
        "S2,S1,N1", "S2,S1,L1", "S2,S1,R1",
        "L2,L1,N1", "L2,L1,S1", "L2,L1,R1",
        "R2,R1,N1", "R2,R1,S1", "R2,R1,L1",
        "N1,S1,L1", "N1,S1,R1",
        "L1,R1,N1", "L1,R1,S1"
    };

private:
    // Structs
    struct PWData {
        string name;
        vector<char> colors;
        unordered_map<char,int> mana;
        int powerAction;
    };

    // Variables
    string logFile;
    vector<PWData> PWs;
    time_t startTime;
    time_t gameStartTime;
    Mat GameWindowImage;
    mutex mtx;
    int GameWindowWidth;
    int GameWindowHeight;
    int GameWindowLeft;
    int activePW;
    char gameMode;
    char screenMode;
    int gamesWon;
    int totalGames;
    int eventIndex;
    int program;
    HWND GameWindowParentHWND;
    int continue_count;

    // Methods
    void InitializePWData();
    bool ReadImages();
    bool ReadImagesTolerances();
    void UpdateBoardStateForOneManaColor(ImageData myImageData, string ImageName, vector<vector<vector<char>>> &TempBoard, vector<vector<vector<double>>> &TempValues);
    bool GetMoveCoords(int Move, int x, int y, vector<pair<int,int>> &p);
    char GoToGameMode(char _gameMode, int maxWait = 1000);
    void FindImages(unordered_map<string,bool> &imBools);
    bool RunBattleMode(vector<vector<char>> &Board);
    bool RunMenusMode(bool &bossEvent, bool &canHealPW, bool &thisGameWon);
    bool RunOtherMode(bool &thisGameWon, int &rewards_earned_count);
    int UsePWPower(vector<vector<char>> Board, vector<pair<int,int>> p, bool checkOnly = false);
    bool ChangePW(int desiredPW, bool click = true);
    int CheckIfBotStuck(int idle, time_t &startTimeQB, time_t timeInScreenMode);
};

#endif // BOT_H
