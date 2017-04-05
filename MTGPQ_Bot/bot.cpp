#include "bot.hpp"
#include "image.hpp"
#include "mouse.hpp"
#include "util.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <Psapi.h>

#define IMAGE_TOLERANCES_FILE "images_tolerances.txt"
#define WIN_LOSE_FILE "win_lose_log.txt"
#define CLICK_DELAY 1000
#define DEFAULT_GAME_WND_HEIGHT 982
#define MAIN_MENU_COORDS            {GameWindowLeft+(int)(GameWindowWidth*0.075),(int)(GameWindowHeight*0.03)}
#define CHANGE_PW_EVENTS_COORDS     {GameWindowLeft+(int)(GameWindowWidth*0.2),  (int)(GameWindowHeight*0.9)}
#define CHANGE_PW_QB_COORDS         {GameWindowLeft+(int)(GameWindowWidth*0.25), (int)(GameWindowHeight*0.85)}
#define CENTER_COORDS               {GameWindowLeft+(int)(GameWindowWidth*0.5),  (int)(GameWindowHeight*0.5)}
#define BOTTOM_CREATURE_COORDS      {GameWindowLeft+(int)(GameWindowWidth*0.65), (int)(GameWindowHeight*0.4)}
#define MIDDLE_CREATURE_COORDS      {GameWindowLeft+(int)(GameWindowWidth*0.65), (int)(GameWindowHeight*0.275)}
#define TOP_CREATURE_COORDS         {GameWindowLeft+(int)(GameWindowWidth*0.65), (int)(GameWindowHeight*0.15)}
#define USE_PW_POWER_COORDS         {GameWindowLeft+(int)(GameWindowWidth*0.075),(int)(GameWindowHeight*0.075)}
#define CLOSE_PW_POWER_COORDS       {GameWindowLeft+(int)(GameWindowWidth*0.5),  (int)(GameWindowHeight*0.015)}
#define SWIPE_PW_L_COORDS           {GameWindowLeft+(int)(GameWindowWidth*0.2),  (int)(GameWindowHeight*0.5)}
#define SWIPE_PW_R_COORDS           {GameWindowLeft+(int)(GameWindowWidth*0.8),  (int)(GameWindowHeight*0.5)}
#define NEXT_BOSS_COORDS            {GameWindowLeft+(int)(GameWindowWidth*0.95), (int)(GameWindowHeight*0.475)}
#define SWIPE_OATH_EVENT_U_COORDS   {GameWindowLeft+(int)(GameWindowWidth*0.5),  (int)(GameWindowHeight*0.4)}
#define SWIPE_OATH_EVENT_D_COORDS   {GameWindowLeft+(int)(GameWindowWidth*0.5),  (int)(GameWindowHeight*0.6)}

////////////////////////////////////////////////////////////////////////////////////////////////////
Bot::Bot() {
    playQB = true;
    playEvents = true;
    joinQB = true;
    joinEvents = true;
    InitializePWData();
    stop = true;
    pause = false;
    GameWindowHeight = DEFAULT_GAME_WND_HEIGHT;
    GameWindowHWND = NULL;
    ReadImages();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::Initialize() {
    cout << "-I- Bot::Initialize - Place the mouse cursor over the game window." << endl;
    cout << "    Window will be captured in..." << endl;
    for (int i = 3; i >= 1; i--) {
        cout << "    " << i << endl;
        Sleep(1000);
    }

    activePW = -1;
    gameMode = 0;
    screenMode = 0;
    gamesWon = 0;
    totalGames = 0;
    eventIndex = 0;
    GameWindowLeft = 0;
    program = 0;
    continue_count = 0;

    POINT MousePos;
    GetCursorPos(&MousePos);
    GameWindowHWND = WindowFromPoint(MousePos);
    Mouse::WindowHWND = GameWindowHWND;
    Mouse::DragMode = MOUSE_DRAG_MODE_LBUTTONDOWN;
    Mouse::PositionMode = MOUSE_POSITION_MODE_WINDOW;

    GameWindowParentHWND = GameWindowHWND;
    HWND temp = GameWindowParentHWND;
    while (GameWindowParentHWND != NULL) {
        temp = GameWindowParentHWND;
        GameWindowParentHWND = GetParent(GameWindowParentHWND);
    }
    GameWindowParentHWND = temp;

    DWORD GameWindowProcessId;
    DWORD GameWindowThreadId = GetWindowThreadProcessId(GameWindowParentHWND, &GameWindowProcessId);
    HANDLE GameWindowProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GameWindowProcessId);
    char GameWindowFileName[128];
    GetModuleFileNameExA(GameWindowProcessHandle, NULL, GameWindowFileName, 128);

    char GameWindowClass[128];
    char GameWindowTitle[128];
    GetClassNameA(GameWindowHWND, GameWindowClass, 128);
    GetWindowTextA(GameWindowHWND, GameWindowTitle, 128);
    cout << "-D- Bot::Initialize - Game window captured:" << endl;
    cout << "    WindowHWND:    " << GameWindowHWND << endl;
    cout << "    WindowClass:   " << GameWindowClass << endl;
    cout << "    WindowTitle:   " << GameWindowTitle << endl;
    GetClassNameA(GameWindowParentHWND, GameWindowClass, 128);
    GetWindowTextA(GameWindowParentHWND, GameWindowTitle, 128);
    cout << "    ParentHWND:    " << GameWindowParentHWND << endl;
    cout << "    ParentClass:   " << GameWindowClass << endl;
    cout << "    ParentTitle:   " << GameWindowTitle << endl;
    cout << "    WindowThread:  " << GameWindowThreadId << endl;
    cout << "    WindowProcess: " << GameWindowProcessId << endl;
    cout << "    ProcessHandle: " << GameWindowProcessHandle << endl;
    cout << "    ExecFileName:  " << GameWindowFileName << endl;

    UpdateGameWindowStatus();
    if (!ReadImages()) return false;
    ReadImagesTolerances();

    if (string(GameWindowFileName).find("Nox") != string::npos) {
        program = 1;
    } else if (string(GameWindowFileName).find("Bluestacks") != string::npos) {
        program = 2;
        /*Mat GameWindowParentImage = Image::HWND2Mat(GameWindowParentHWND);
        vector<Point> Coords = Image::SearchInsideOtherImage(Images["bluestacks_close"].mat, GameWindowParentImage, CV_TM_CCOEFF_NORMED, 's', 0.2, "bluestacks_close");
        if (Coords.size() > 0) {
            cout << Coords[0].x+Images["bluestacks_close"].mat.cols*4/10 << "," << Coords[0].y << endl;
            Sleep(CLICK_DELAY*2);
            Mouse::WindowHWND = GameWindowParentHWND;
            SendMessage(GameWindowParentHWND, WM_MOUSEACTIVATE, (WPARAM)GameWindowParentHWND, MAKELPARAM(HTCLIENT, WM_LBUTTONDOWN));
            SendMessage(GameWindowParentHWND, WM_SETCURSOR, 0x60AEE, MAKELPARAM(HTCLIENT, WM_LBUTTONDOWN));
            Mouse::LeftClick({Coords[0].x+Images["bluestacks_close"].mat.cols*4/10, Coords[0].y});
            SendMessage(GameWindowParentHWND, WM_LBUTTONUP, 0, 0);
            Mouse::WindowHWND = GameWindowHWND;
            cout << "done" << endl;
        }*/
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::StartUp(int maxWait) {
    time_t start = time(NULL);
    do {
        bool notUsed;
        int notUsed2;
        if (RunOtherMode(notUsed, notUsed2)) continue;
        if (Image::Find(Images["pause"].mat, GameWindowHWND, Images["pause"].tol, "pause")) {
            unordered_map<string,bool> imBools;
            for (unsigned int i = 0; i < PWs.size(); i++)
                imBools.emplace(PWs[i].name, false);
            UpdateGameWindowStatus();
            FindImages(imBools);
            for (auto it = imBools.begin(); it != imBools.end(); it++)
                if (it->second)
                    for (unsigned int i = 0; i < PWs.size(); i++)
                        if (it->first == PWs[i].name) {
                            activePW = i;
                            screenMode = 'b';
                            cout << "-D- Bot::StartUp - screenMode = '" << screenMode << "', activePW = " << PWs[i].name << endl;
                            return true;
                        }
            if (Image::Find(Images["arlinn_trans"].mat, GameWindowHWND, Images["arlinn_trans"].tol, "arlinn_trans")) {
                for (unsigned int i = 0; i < PWs.size(); i++)
                    if (PWs[i].name == "arlinn") {
                        activePW = i;
                        screenMode = 'b';
                        cout << "-D- Bot::StartUp - screenMode = '" << screenMode << "', activePW = " << PWs[i].name << endl;
                        return true;
                    }
            }
        } else {
            Image::Click(Images["game_icon"].mat, GameWindowHWND, 5000, Images["game_icon"].tol, "game_icon");
            if (GoToGameMode((playEvents ? 'e' : 'q'), 5)) return true;
        }
    } while (time(NULL) - start <= maxWait && !stop);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::RunBattleMode(vector<vector<char>> &Board) {
    vector<pair<int,int>> p(6, pair<int,int>(0, 0));

    // Deal with cards that are ready to be played.
    unordered_map<string,bool> imBools = {{"select_creature",false},
                                          {"creature_selected",false},
                                          {"not_now",false},
                                          {"replace_creature",false}};
    UpdateGameWindowStatus();
    FindImages(imBools);
    if (!imBools["replace_creature"]) {
        if (imBools["select_creature"]) {
            Mouse::LeftClick(BOTTOM_CREATURE_COORDS);
            Sleep(CLICK_DELAY/4);
            if (Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected"))
                return true;
            Mouse::LeftClick(MIDDLE_CREATURE_COORDS);
            Sleep(CLICK_DELAY/4);
            if (Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected"))
                return true;
            Mouse::LeftClick(TOP_CREATURE_COORDS);
            Sleep(CLICK_DELAY/4);
            if (Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected"))
                return true;
        } else if (imBools["creature_selected"]) {
            Image::Click(Images["creature_selected"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected");
            return true;
        }
    }
    if (imBools["not_now"]) {
        Image::Click(Images["not_now"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now");
        return true;
    }

    // Calculate best move.
    int checkCanPlay = UsePWPower(Board, p, true);
    if (!checkCanPlay) return false;
    UpdateGameWindowStatus();
    UpdateBoardState(Board);
    int BestMove = FindBestMove(Board, p);
    if (BestMove >= 0) {
        int usedPWPower = 1;
        if (BestMove > 16 && checkCanPlay != 1) usedPWPower = UsePWPower(Board, p);
        if (usedPWPower) {
            //if (Board[p[1].first][p[1].second] == '-' && BestMove <= 15) return false;
            cout << "-D- Bot::RunBattleMode - Current board:" << endl;
            for (unsigned int j = 0; j < Board.size(); j++) {
                cout << "    ";
                for (unsigned int i = 0; i < Board[j].size(); i++) {
                    cout << Board[i][j];
                    if (i < Board[j].size()-1) cout << "  ";
                }
                cout << endl;
            }
            cout << "-D- Bot::RunBattleMode - Best move ID: " << BestMove << endl;
            cout << "-D- Bot::RunBattleMode - Best move: " << Moves[BestMove] << endl;
            cout << "-D- Bot::RunBattleMode - Best move position: (" << p[4].first+1 << "," << p[4].second+1 << ") (" << p[5].first+1 << "," << p[5].second+1 << ")" << endl;
            cout << "-D- Bot::RunBattleMode - Best move color: " << Board[p[1].first][p[1].second] << endl;
            Mouse::Drag({xBoardCoords[p[4].first], yBoardCoords[p[4].second]},
                        {xBoardCoords[p[5].first], yBoardCoords[p[5].second]}, 5);
            return true;
        }
    } else {
        cout << "-D- Bot::RunBattleMode - Current board:" << endl;
        for (unsigned int j = 0; j < Board.size(); j++) {
            cout << "    ";
            for (unsigned int i = 0; i < Board[j].size(); i++) {
                cout << Board[i][j];
                if (i < Board[j].size()-1) cout << "  ";
            }
            cout << endl;
        }
        cout << "-D- Bot::RunBattleMode - Best move ID: " << BestMove << endl;
        cout << "-E- Bot::RunBattleMode - Couldn't find a valid move." << endl;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::RunMenusMode(bool &specialEvent, bool &canHealPW, bool &thisGameWon) {
    if (activePW < 0 && gameMode == 'q') return ChangePW(0);
    if (gameMode != 'e' && gameMode != 'q') return StartUp(30);

    UpdateGameWindowStatus();

    string startMatch = "fight1";
    string startMatch2 = "fight2";
    string startMatch3 = "fight3";
    string heal = "heal_qb";
    string heal0hp = "heal0hp_qb";
    if (gameMode == 'q') {
        specialEvent = false;
        if (Image::Find(Images["next_round"].mat, GameWindowImage, Images["next_round"].tol, "next_round") && joinQB) {
            Image::Click(Images["next_round"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["next_round"].tol, "next_round");
            screenMode = 'b';
            thisGameWon = false;
            gameStartTime = time(NULL);
            return true;
        }
    } else if (gameMode == 'e') {
        startMatch = "play1";
        startMatch2 = "play2";
        startMatch3 = "play3";
        heal = "heal_event";
        heal0hp = "heal0hp_event";

        unordered_map<string,bool> imBools = {{"0",false},
                                              {"1",false},
                                              {"2",false},
                                              {"3",false},
                                              {"tier_ok",false},
                                              {"rewards",false},
                                              {"joined",false},
                                              {"back",false},
                                              {"oath_event_0",false},
                                              {"oath_event_33",false},
                                              {"oath_event_67",false},
                                              {"oath_event_50",false},
                                              {"oath_event_100",false}};
        FindImages(imBools);
        if (imBools["tier_ok"]) {
            Image::Click(Images["tier_ok"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["tier_ok"].tol, "tier_ok");
            specialEvent = false;
            return true;
        } else if (imBools["rewards"]) {
            if (rand()%2) {
                Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                Sleep(CLICK_DELAY);
            }
            if (joinEvents) {
                if (Image::Click(Images["join_event"].mat, GameWindowHWND, CLICK_DELAY, Images["join_event"].tol, "join_event")) {
                    specialEvent = false;
                    return true;
                }
            }
            if (Image::Find(Images["continue_event"].mat, GameWindowHWND, Images["continue_event"].tol, "continue_event")) {
                UpdateGameWindowStatus();
                vector<Point> Coords = Image::SearchInsideOtherImage(Images["continue_event"].mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', Images["continue_event"].tol, "continue_event");
                if (eventIndex >= Coords.size()) eventIndex = 0;
                Mouse::LeftClick({Coords[eventIndex].x, Coords[eventIndex].y});
                Sleep(CLICK_DELAY);
                eventIndex++;
                specialEvent = false;
                return true;
            }
            if (rand()%2) {
                Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                Sleep(CLICK_DELAY);
            }
        } else if (imBools["joined"] && imBools["back"]) {
            Image::Click(Images["back"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["back"].tol, "back");
            specialEvent = false;
            return true;
        } else if (imBools["0"] || imBools["1"] || imBools["2"] || imBools["3"]) {
            specialEvent = true;
            if (imBools["0"]) {
                Mouse::LeftClick(NEXT_BOSS_COORDS);
                Sleep(CLICK_DELAY);
                return false;
            } else {
                Mouse::LeftClick(CENTER_COORDS);
                Sleep(CLICK_DELAY);
                return true;
            }
        } else if (imBools["oath_event_0"] || imBools["oath_event_33"] || imBools["oath_event_67"] || imBools["oath_event_100"]) {
            // TEMP WA TO EVADE OATH EVENTS
            //return false;
            specialEvent = true;
            if (imBools["oath_event_0"]) {
                Image::Click(Images["oath_event_0"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["oath_event_0"].tol, "oath_event_0");
                return true;
            } else if (imBools["oath_event_33"]) {
                Image::Click(Images["oath_event_33"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["oath_event_33"].tol, "oath_event_33");
                return true;
            } else if (imBools["oath_event_67"]) {
                if (rand()%2) {
                    vector<Point> Coords = Image::SearchInsideOtherImage(Images["oath_event_67"].mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', Images["oath_event_67"].tol, "oath_event_67");
                    int i = rand()%Coords.size();
                    Mouse::LeftClick({Coords[i].x, Coords[i].y});
                    Sleep(CLICK_DELAY);
                    return true;
                } else {
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    if (Image::Click(Images["oath_event_0"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_0"].tol, "oath_event_0"))
                        return true;
                    if (Image::Click(Images["oath_event_33"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_33"].tol, "oath_event_33"))
                        return true;
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    if (Image::Click(Images["oath_event_0"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_0"].tol, "oath_event_0"))
                        return true;
                    if (Image::Click(Images["oath_event_33"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_33"].tol, "oath_event_33"))
                        return true;
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    if (Image::Click(Images["oath_event_0"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_0"].tol, "oath_event_0"))
                        return true;
                    if (Image::Click(Images["oath_event_33"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_33"].tol, "oath_event_33"))
                        return true;
                    if (Image::Click(Images["oath_event_67"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_67"].tol, "oath_event_67"))
                        return true;
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    return false;
                }
            } else if (imBools["oath_event_50"]) {
                if (rand()%2) {
                    vector<Point> Coords = Image::SearchInsideOtherImage(Images["oath_event_50"].mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', Images["oath_event_50"].tol, "oath_event_50");
                    int i = rand()%Coords.size();
                    Mouse::LeftClick({Coords[i].x, Coords[i].y});
                    Sleep(CLICK_DELAY);
                    return true;
                } else {
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    if (Image::Click(Images["oath_event_0"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_0"].tol, "oath_event_0"))
                        return true;
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    if (Image::Click(Images["oath_event_0"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_0"].tol, "oath_event_0"))
                        return true;
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    if (Image::Click(Images["oath_event_0"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_0"].tol, "oath_event_0"))
                        return true;
                    if (Image::Click(Images["oath_event_50"].mat, GameWindowHWND, CLICK_DELAY, Images["oath_event_50"].tol, "oath_event_50"))
                        return true;
                    Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                    Sleep(CLICK_DELAY);
                    return false;
                }
            } else {
                Mouse::Drag(SWIPE_OATH_EVENT_D_COORDS, SWIPE_OATH_EVENT_U_COORDS, 3);
                Sleep(CLICK_DELAY);
                return false;
            }
        }

        if (!specialEvent) {
            unordered_map<string,bool> imBools = {{"choose_any_pw",false},
                                                  {"choose_black_pw",false},
                                                  {"choose_blue_pw",false},
                                                  {"choose_green_pw",false},
                                                  {"choose_red_pw",false},
                                                  {"choose_white_pw",false},
                                                  {"choose_black_blue_pw",false},
                                                  {"choose_green_red_pw",false},
                                                  {"choose_white_blue_pw",false},
                                                  {"choose_white_black_pw",false}};
            FindImages(imBools);
            vector<char> colors;
            string chooseColorPW;
            if (imBools["choose_black_pw"]) {
                colors = {'B'};
                chooseColorPW = "choose_black_pw";
            } else if (imBools["choose_blue_pw"]) {
                colors = {'U'};
                chooseColorPW = "choose_blue_pw";
            } else if (imBools["choose_green_pw"]) {
                colors = {'G'};
                chooseColorPW = "choose_green_pw";
            } else if (imBools["choose_red_pw"]) {
                colors = {'R'};
                chooseColorPW = "choose_red_pw";
            } else if (imBools["choose_white_pw"]) {
                colors = {'W'};
                chooseColorPW = "choose_white_pw";
            } else if (imBools["choose_black_blue_pw"]) {
                colors = {'B', 'U'};
                chooseColorPW = "choose_black_blue_pw";
            } else if (imBools["choose_green_red_pw"]) {
                colors = {'G', 'R'};
                chooseColorPW = "choose_green_red_pw";
            } else if (imBools["choose_white_black_pw"]) {
                colors = {'W', 'B'};
                chooseColorPW = "choose_white_black_pw";
            } else if (imBools["choose_white_blue_pw"]) {
                colors = {'W', 'U'};
                chooseColorPW = "choose_white_black_pw";
            } else if (imBools["choose_any_pw"]) {
                colors = {'U', 'B', 'G', 'R', 'W'};
                chooseColorPW = "choose_any_pw";
            }
            vector<int> availablePWs;
            for (unsigned int i = 0; i < PWs.size(); i++) {
                bool match = false;
                for (unsigned int j = 0; j < PWs[i].colors.size(); j++) {
                    for (unsigned int k = 0; k < colors.size(); k++) {
                        if (PWs[i].colors[j] == colors[k]) {
                            availablePWs.push_back(i);
                            match = true;
                            break;
                        }
                    }
                    if (match) break;
                }
            }
            for (unsigned int i = 0; i < availablePWs.size(); i++) {
                Image::Click(Images[chooseColorPW].mat, GameWindowHWND, CLICK_DELAY, Images[chooseColorPW].tol, chooseColorPW);
                if (ChangePW(availablePWs[i], false)) return true;
                Sleep(CLICK_DELAY);
            }
            if (colors.size() != 0) return false;

            imBools.clear();
            for (unsigned int i = 0; i < PWs.size(); i++)
                imBools.emplace(PWs[i].name+"_e", false);
            FindImages(imBools);
            for (unsigned int i = 0; i < PWs.size(); i++)
                for (auto it = imBools.begin(); it != imBools.end(); it++)
                    if ((canHealPW || activePW != i) && (it->first == PWs[i].name+"_e" && it->second)) {
                        Image::Click(Images[PWs[i].name+"_e"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images[PWs[i].name+"_e"].tol, PWs[i].name+"_e");
                        vector<Point> Coords = Image::SearchInsideOtherImage(Images[PWs[i].name+"_e"].mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', Images[PWs[i].name+"_e"].tol, PWs[i].name+"_e");
                        int index = rand()%Coords.size();
                        Mouse::LeftClick({Coords[index].x, Coords[index].y});
                        Sleep(CLICK_DELAY);
                        canHealPW = true;
                        activePW = i;
                        return true;
                    }
        }
    }

    // Heal PW and start next battle
    unordered_map<string,bool> imBools = {{"purchase_potions",false},
                                          {"purchase_potions_x",false},
                                          {startMatch,false},
                                          {startMatch2,false},
                                          {startMatch3,false},
                                          {heal,false},
                                          {heal0hp,false},
                                          {"no",false}};
    FindImages(imBools);
    if ((imBools[startMatch] || imBools[heal] || imBools[heal0hp]) && activePW < 0) return ChangePW(0);
    if (imBools["purchase_potions"] && imBools["purchase_potions_x"]) {
        Image::Click(Images["purchase_potions_x"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["purchase_potions_x"].tol, "purchase_potions_x");
        if (gameMode == 'q' || (gameMode == 'e' && specialEvent)) {
            /// BUG!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //if (gameMode == 'q') {
                //int activePWtemp = activePW;
                //GoToGameMode(gameMode, 5);
                //Sleep(CLICK_DELAY);
                //cout << PWs[activePWtemp+1].name << endl;
                //while (!ChangePW(activePWtemp+1)) {
                //    cout << "BUG WA!" <<  endl;
                //}
            //} else ChangePW(activePW+1);
            ChangePW(activePW+1);
        } else {
            if (!Image::Click(Images["back"].mat, GameWindowHWND, CLICK_DELAY, Images["back"].tol, "back")) return false;
            /// BUG!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //int activePWtemp = activePW;
            //GoToGameMode(gameMode, 5);
            //activePW = activePWtemp;
            ///
            canHealPW = false;
        }
        return true;
    } else if (imBools[startMatch] && gameMode == 'q') {
        Image::Click(Images[startMatch].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images[startMatch].tol, startMatch);
        screenMode = 'b';
        thisGameWon = false;
        gameStartTime = time(NULL);
        return true;
    } else if (imBools[heal] || imBools[heal0hp]) {
        if (gameMode == 'e' && !specialEvent) {
            if (!Image::Click(Images[heal].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images[heal].tol, heal))
                Image::Click(Images[heal0hp].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images[heal0hp].tol, heal0hp);
        } else if (imBools[heal0hp] || (imBools[heal] && (gameMode == 'e' || (gameMode == 'q' && (imBools[startMatch2] || imBools[startMatch3]))))) {
             if (activePW == 0) {
                if (!Image::Click(Images[heal].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images[heal].tol, heal))
                    Image::Click(Images[heal0hp].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images[heal0hp].tol, heal0hp);
            } else {
                int lastActivePW = activePW;
                ChangePW(0);
                if (Image::Find(Images[heal].mat, GameWindowHWND, Images[heal].tol, heal) ||
                    Image::Find(Images[heal0hp].mat, GameWindowHWND, Images[heal0hp].tol, heal0hp)) {
                    int PW = lastActivePW + 1;
                    if (PW >= PWs.size()) PW = 0;
                    ChangePW(PW);
                }
            }
        }
        return true;
    } else if (imBools[startMatch] && gameMode == 'e') {
        Image::Click(Images[startMatch].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images[startMatch].tol, startMatch);
        screenMode = 'b';
        thisGameWon = false;
        gameStartTime = time(NULL);
        return true;
    } else if (imBools["no"]) {
        Image::Click(Images["no"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["no"].tol, "no");
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::RunOtherMode(bool &thisGameWon, int &rewards_earned_count) {
    unordered_map<string,bool> imBools = {{"continue",false},//b
                                          {"rewards",false},//m
                                          {"objectives",false},//b
                                          {"ending_encounter",false},//b
                                          {"retry",false},//any
                                          {"retry2",false},//any
                                          {"retry3",false},//any
                                          {"card_mastered",false},//b
                                          {"rewards_earned",false},//any
                                          {"ribbons_earned",false},//b
                                          {"runes_earned",false},//any
                                          {"tier_up",false},//?
                                          {"tier_up_x",false},//?
                                          {"quick_battle_expired",false},//?
                                          {"claim_rewards",false},//m
                                          {"new_bracket",false},//m
                                          {"new_bracket_x",false}};//m
    UpdateGameWindowStatus();
    FindImages(imBools);
    if (imBools["new_bracket"] && imBools["new_bracket_x"]) {
        Image::Click(Images["new_bracket_x"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["new_bracket_x"].tol, "new_bracket_x");
    } else if (imBools["retry"]) {
        Image::Click(Images["retry"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["retry"].tol, "retry");
    } else if (imBools["retry2"]) {
        Image::Click(Images["retry2"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["retry2"].tol, "retry2");
    } else if (imBools["retry3"]) {
        if (!Image::Find(Images["story_sub"].mat, GameWindowHWND, Images["story_sub"].tol, "story_sub"))
            Image::Click(Images["retry3"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["retry3"].tol, "retry3");
        else return false;
    } else if (imBools["claim_rewards"]) {
        Image::Click(Images["claim_rewards"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["claim_rewards"].tol, "claim_rewards");
    } else if (imBools["tier_up"] && imBools["tier_up_x"]) {
        Image::Click(Images["tier_up_x"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["tier_up_x"].tol, "tier_up_x");
    } else if (imBools["card_mastered"] || imBools["quick_battle_expired"]) {
        Mouse::LeftClick(CENTER_COORDS);
        Sleep(CLICK_DELAY);
    } else if (imBools["ribbons_earned"]) {
        thisGameWon = true;
        Image::Click(Images["ribbons_earned"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["ribbons_earned"].tol, "ribbons_earned");
    } else if (imBools["runes_earned"]) {
        thisGameWon = true;
        for (int i = 0; i < 25; i++) {
            if (Image::Find(Images["50"].mat, GameWindowHWND, Images["50"].tol, "50")) {
                thisGameWon = false;
                break;
            }
            Sleep(1);
        }
        Image::Click(Images["runes_earned"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["runes_earned"].tol, "runes_earned");
    } else if (imBools["rewards_earned"]) {
        if (thisGameWon) {
            rewards_earned_count = 1;
            Mouse::LeftClick(CENTER_COORDS);
            Sleep(CLICK_DELAY);
            return true;
        }
        if (rewards_earned_count % 10 == 0) {
            rewards_earned_count = 1;
            Mouse::LeftClick(CENTER_COORDS);
            Sleep(CLICK_DELAY);
            return true;
        }
        rewards_earned_count++;
        return false;
    } else if (imBools["continue"] && !imBools["rewards"] && imBools["objectives"] && !imBools["ending_encounter"]) {
        continue_count++;
        if (continue_count < 5) return false;
        Image::Click(Images["continue"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["continue"].tol, "continue");
        for (unsigned int i = 0; i < PWs.size(); i++) PWs[i].powerAction = 0;
        continue_count = 0;
        screenMode = 'm';
        if (thisGameWon) {
            cout << "-I- Bot::RunOtherMode - Game won!" << endl;
            gamesWon++;
        } else {
            cout << "-I- Bot::RunOtherMode - Game lost." << endl;
        }
        totalGames++;
        double elapsedTime = difftime(time(NULL), startTime)/3600.0;
        double gameElapsedTime = difftime(time(NULL), gameStartTime)/60.0;
        cout << "-I- Bot::RunOtherMode - Games played:  " << totalGames << endl;
        cout << "-I- Bot::RunOtherMode - Games won:     " << gamesWon << endl;
        cout << "-I- Bot::RunOtherMode - Win rate:      " << 100*gamesWon/totalGames << "%" << endl;
        cout << "-I- Bot::RunOtherMode - Wins per hour: " << gamesWon/elapsedTime << endl;
        cout << "-I- Bot::RunOtherMode - Game time:     " << gameElapsedTime << " minutes" << endl;
        cout << "-I- Bot::RunOtherMode - Total time:    " << elapsedTime << " hours" << endl;
        ofstream winLoseFile(WIN_LOSE_FILE, ios_base::out | ios_base::app);
        if (!winLoseFile.is_open()) {
            cout << "-E- Bot::RunOtherMode - Can't open file: '" << WIN_LOSE_FILE << "'" << endl;
            return true;
        }
        char gameMode_file = gameMode;
        if (!gameMode_file) gameMode_file = '-';
        winLoseFile << PWs[activePW].name << "," << thisGameWon << "," << gameMode_file << "," << gameElapsedTime << "," << time(NULL) << endl;
        winLoseFile.close();
    } else return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int Bot::CheckIfBotStuck(int idle, time_t &startTimeQB, time_t timeInScreenMode) {
    if ((screenMode == 'm' && (idle > 60  || timeInScreenMode > 180)) ||
        (screenMode == 'b' && (idle > 120 || (timeInScreenMode > 600 && gameMode == 'q') || (timeInScreenMode > 1800 && gameMode == 'e')))) {
        vector<string> debugImages = Util::GetFileNamesFromDir(".", "bot_stuck_debug*.jpg");
        for (unsigned int i = 0; i < debugImages.size(); i++) remove(debugImages[i].c_str());
        UpdateGameWindowStatus();
        imwrite("bot_stuck_debug_1.jpg", GameWindowImage);
        if (Image::Find(Images["game_icon"].mat, GameWindowHWND, Images["game_icon"].tol, "game_icon")) {
            cout << "-I- Bot::CheckIfBotStuck - Game crashed. Restarting..." << endl;
            if (StartUp(60)) return 1;
            else return -1;
        }
        UpdateGameWindowStatus();
        imwrite("bot_stuck_debug_2.jpg", GameWindowImage);
        int i = 0;
        while (i <= 30) {
            if (Image::Click(Images["pause"].mat, GameWindowHWND, CLICK_DELAY, Images["pause"].tol, "pause")) break;
            i++;
        }
        if (i > 30) {
            if (GoToGameMode((playEvents ? 'e' : 'q'), 15)) {
                cout << "-I- Bot::CheckIfBotStuck - Bot stuck in menus. Resetting..." << endl;
                return 1;
            } else return -1;
        }
        UpdateGameWindowStatus();
        imwrite("bot_stuck_debug_3.jpg", GameWindowImage);
        i = 0;
        while (i <= 30) {
            if (Image::Click(Images["pause"].mat, GameWindowHWND, CLICK_DELAY, Images["pause"].tol, "pause")) break;
            i++;
        }
        UpdateGameWindowStatus();
        imwrite("bot_stuck_debug_4.jpg", GameWindowImage);
        i = 0;
        while (i <= 30) {
            if (Image::Click(Images["quit1"].mat, GameWindowHWND, CLICK_DELAY, Images["quit1"].tol, "quit1")) break;
            i++;
        }
        UpdateGameWindowStatus();
        imwrite("bot_stuck_debug_5.jpg", GameWindowImage);
        i = 0;
        while (i <= 30) {
            if (Image::Click(Images["quit2"].mat, GameWindowHWND, CLICK_DELAY, Images["quit2"].tol, "quit2")) break;
            i++;
        }
        UpdateGameWindowStatus();
        imwrite("bot_stuck_debug_6.jpg", GameWindowImage);
        if (i <= 30) {
            cout << "-I- Bot::CheckIfBotStuck - Bot stuck in battle mode. Quitting current battle..." << endl;
            screenMode = 'm';
            cout << "-I- Bot::RunOtherMode - Game lost." << endl;
            for (unsigned int i = 0; i < PWs.size(); i++) PWs[i].powerAction = 0;
            totalGames++;
            double elapsedTime = difftime(time(NULL), startTime)/3600.0;
            double gameElapsedTime = difftime(time(NULL), gameStartTime)/60.0;
            cout << "-I- Bot::RunOtherMode - Games played:  " << totalGames << endl;
            cout << "-I- Bot::RunOtherMode - Games won:     " << gamesWon << endl;
            cout << "-I- Bot::RunOtherMode - Win rate:      " << 100*gamesWon/totalGames << "%" << endl;
            cout << "-I- Bot::RunOtherMode - Wins per hour: " << gamesWon/elapsedTime << endl;
            cout << "-I- Bot::RunOtherMode - Game time:     " << gameElapsedTime << " minutes" << endl;
            cout << "-I- Bot::RunOtherMode - Total time:    " << elapsedTime << " hours" << endl;
            ofstream winLoseFile(WIN_LOSE_FILE, ios_base::out | ios_base::app);
            if (!winLoseFile.is_open()) {
                cout << "-E- Bot::RunOtherMode - Can't open file: '" << WIN_LOSE_FILE << "'" << endl;
                return -1;
            }
            char gameMode_file = gameMode;
            if (!gameMode_file) gameMode_file = '-';
            winLoseFile << PWs[activePW].name << "," << 0 << "," << gameMode_file << "," << gameElapsedTime << "," << time(NULL) << endl;
            winLoseFile.close();
            if (GoToGameMode((playEvents ? 'e' : 'q'), 90)) return 1;
            else return -1;
        } else {
            if (program == 1) { // Nox
                // ctrl+9 (close all apps)
                cout << "-I- Bot::CheckIfBotStuck - Trying to close app..." << endl;
                SendMessage(GameWindowParentHWND, WM_KEYDOWN, 0x11, 0x1D0001);
                SendMessage(GameWindowParentHWND, WM_KEYDOWN, 0x39, 0xA0001);
                SendMessage(GameWindowParentHWND, WM_KEYUP, 0x39, 0xA0001);
                SendMessage(GameWindowParentHWND, WM_KEYUP, 0x11, 0xC01D0001);
                Sleep(CLICK_DELAY*5);
            }/* else if (program == 2) { // BlueStacks
                // Not supported yet
            }*/
            UpdateGameWindowStatus();
            imwrite("bot_stuck_debug_7.jpg", GameWindowImage);
            if (Image::Find(Images["game_icon"].mat, GameWindowHWND, Images["game_icon"].tol, "game_icon")) {
                cout << "-I- Bot::CheckIfBotStuck - Game crashed. Restarting..." << endl;
                int ret = StartUp(90) ? 1 : -1;
                UpdateGameWindowStatus();
                imwrite("bot_stuck_debug_8.jpg", GameWindowImage);
                return ret;
            } else {
                cout << "-I- Bot::CheckIfBotStuck - Bot stuck. Stopping bot..." << endl;
                return -2;
            }
        }
    }
    if (idle > 15 && screenMode == 'm' && gameMode == 'e') {
        if (playQB) {
            if (Image::Find(Images["black_mastery"].mat, GameWindowHWND, Images["black_mastery"].tol, "black_mastery") ||
                Image::Find(Images["back"].mat, GameWindowHWND, Images["back"].tol, "back")) {
                cout << "-I- Bot::CheckIfBotStuck - Nothing else to do in this Event. Switching to QuickBattle..." << endl;
                if (GoToGameMode('q', 15)) {
                    startTimeQB = time(NULL);
                    return 1;
                } else return -1;
            }
        } else {
            if (Image::Click(Images["back"].mat, GameWindowHWND, CLICK_DELAY ,Images["back"].tol, "back")) return 1;
        }
    }
    if (screenMode == 'm' && gameMode == 'q' && playEvents && time(NULL) - startTimeQB > 1200) {
        cout << "-I- Bot::CheckIfBotStuck - Enough time in QuickBattle. Switching to Events to check if nodes have been regenerated..." << endl;
        if (GoToGameMode('e', 15)) return 1;
        else return -1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Bot::Run() {
    cout << "-I- Bot::Run - Initializing bot..." << endl;
    if (!Initialize()) return;
    if (stop) return;

    cout << "-I- Bot::Run - Starting bot..." << endl;
    if (!StartUp(60)) return;
    if (stop) return;

    startTime = time(NULL);
    gameStartTime = time(NULL);
    srand((unsigned int)time(NULL));
    ofstream winLoseFile(WIN_LOSE_FILE, ios_base::out | ios_base::app);
    if (!winLoseFile.is_open()) {
        cout << "-E- Bot::RunOtherMode - Can't open file: '" << WIN_LOSE_FILE << "'" << endl;
        return;
    }
    winLoseFile << endl;
    winLoseFile.close();

    cout << "-I- Bot::Run - Running bot..." << endl;
    vector<vector<char>> Board;
    bool specialEvent = false, canHealPW = true, thisGameWon = false;
    int idle = 0, rewards_earned_count = 1;
    time_t startTimeQB = time(NULL);
    time_t timeInScreenMode = 0;
    time_t startTimeScreenMode = time(NULL);
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
    char prevScreenMode = 0;

    while(!stop) {
        if (prevScreenMode != screenMode)
            startTimeScreenMode = time(NULL);
        timeInScreenMode = time(NULL) - startTimeScreenMode;
        cout << endl << "-I- Bot::Run - timeInScreenMode = " << timeInScreenMode << endl;
        prevScreenMode = screenMode;
        SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
        chrono::milliseconds calcTime = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now()-t1);
        if (calcTime.count() < CLICK_DELAY) {
            Sleep((DWORD)(CLICK_DELAY-calcTime.count()));
            cout << "-D- Bot::Run - Iteration lasted " << CLICK_DELAY << " ms" << endl;
        } else {
            cout << "-D- Bot::Run - Iteration lasted " << calcTime.count() << " ms" << endl;
        }
        t1 = chrono::high_resolution_clock::now();
        idle++;
        cout << "-I- Bot::Run - idle = " << idle << endl;
        int stuck = CheckIfBotStuck(idle, startTimeQB, timeInScreenMode);
        if (stuck == 1) {
            canHealPW = true;
            idle = 0;
            startTimeScreenMode = time(NULL);
            continue;
        } else if (stuck == -1) {
            if (program == 1) { // Nox
                // ctrl+9 (close all apps)
                cout << "-I- Bot::Run - Trying to close app..." << endl;
                SendMessage(GameWindowParentHWND, WM_KEYDOWN, 0x11, 0x1D0001);
                SendMessage(GameWindowParentHWND, WM_KEYDOWN, 0x39, 0xA0001);
                SendMessage(GameWindowParentHWND, WM_KEYUP, 0x39, 0xA0001);
                SendMessage(GameWindowParentHWND, WM_KEYUP, 0x11, 0xC01D0001);
                Sleep(CLICK_DELAY*5);
            }/* else if (program == 2) { // BlueStacks
                // Not supported yet
            }*/
            UpdateGameWindowStatus();
            imwrite("bot_stuck_debug_9.jpg", GameWindowImage);
            if (Image::Find(Images["game_icon"].mat, GameWindowHWND, Images["game_icon"].tol, "game_icon")) {
                cout << "-I- Bot::Run - Game crashed. Restarting..." << endl;
                StartUp(90);
                UpdateGameWindowStatus();
                imwrite("bot_stuck_debug_10.jpg", GameWindowImage);
                canHealPW = true;
                idle = 0;
                startTimeScreenMode = time(NULL);
                continue;
            } else {
                cout << "-I- Bot::Run - Bot stuck. Stopping bot..." << endl;
                cout << "-D- Bot::Run - Bot stuck, cannot recover." << endl;
                break;
            }
        } else if (stuck == -2) {
            cout << "-D- Bot::Run - Bot stuck, cannot recover." << endl;
            break;
        }
        if (RunOtherMode(thisGameWon, rewards_earned_count)) {idle = 0; continue;}
        if (screenMode == 'b') {
            if (RunBattleMode(Board)) idle = 0;
        } else if (screenMode == 'm') {
            if (RunMenusMode(specialEvent, canHealPW, thisGameWon)) idle = 0;
        }
        while (pause) {
            Sleep(1000);
            startTimeScreenMode = time(NULL);
            if (stop) break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
char Bot::GoToGameMode(char _gameMode, int maxWait) {
    time_t start = time(NULL);
    gameMode = 0;
    activePW = -1;
    string imageName;
    if (_gameMode == 'e') imageName = "events";
    else if (_gameMode == 'q') imageName = "quick_battle";
    else return gameMode;
    do {
        if (!Image::Find(Images[imageName].mat, GameWindowHWND, Images[imageName].tol, imageName)) {
            if (!Image::Click(Images["main_menu"].mat, GameWindowHWND, CLICK_DELAY, Images["main_menu"].tol, "main_menu")) {
                Mouse::LeftClick(MAIN_MENU_COORDS);
                Sleep(CLICK_DELAY);
            }
        }
        if (Image::Click(Images[imageName].mat, GameWindowHWND, CLICK_DELAY, Images[imageName].tol, imageName)) {
            cout << "-D- Bot::GoToGameMode - Switching to gameMode = '" << _gameMode << "'" << endl;
            screenMode = 'm';
            gameMode = _gameMode;
            break;
        }
    } while (time(NULL) - start <= maxWait && !stop);
    return gameMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int Bot::UsePWPower(vector<vector<char>> Board, vector<pair<int,int>> p, bool checkOnly) {
    // Temporary WA
    if (!Image::Find(Images["pause"].mat, GameWindowHWND, Images["pause"].tol, "pause")) {
        return 0;
    }
    int PWFound = 0;
    if (Image::Click(Images[PWs[activePW].name].mat, GameWindowHWND, CLICK_DELAY/2, Images[PWs[activePW].name].tol, PWs[activePW].name))
        PWFound = 1;
    else if (Image::Click(Images["arlinn_trans"].mat, GameWindowHWND, CLICK_DELAY/2, Images["arlinn_trans"].tol, "arlinn_trans"))
        PWFound = 2;
    if (PWFound == 0) return 0;
    unordered_map<string,bool> imBools = {{"ability1",false},
                                          {"ability2",false},
                                          {"ability3",false},
                                          {"abilities",false}};
    UpdateGameWindowStatus();
    FindImages(imBools);
    if (!imBools["abilities"]) {
        Mouse::LeftClick(CLOSE_PW_POWER_COORDS);
        return 0;
    } else if (!(imBools["ability1"] || imBools["ability2"] || imBools["ability3"])) {
        Mouse::LeftClick(CLOSE_PW_POWER_COORDS);
        Sleep(CLICK_DELAY/2);
        return 1;
    } else if (checkOnly) {
        Mouse::LeftClick(CLOSE_PW_POWER_COORDS);
        Sleep(CLICK_DELAY/2);
        return 2;
    }
    cout << "-D- Bot::UsePWPower - Trying to use " << PWs[activePW].name << "'s power..." << endl;

    if (PWs[activePW].name == "koth") {
        /*if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3"))
                if (!Image::Find(Images["ability3"].mat, GameWindowHWND, Images["ability3"].tol, "ability3"))
                    PWs[activePW].powerAction = 1;
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            if (Board[p[1].first][p[1].second] != 'R')
                Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
        }*/
        Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
        if (Board[p[1].first][p[1].second] != 'R')
            Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    } else if (PWs[activePW].name == "nissa") {
        Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    } else if (PWs[activePW].name == "kiora") {
        if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3")) {
                if (Image::Click(Images["not_now"].mat, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now")) {
                    Mouse::LeftClick(USE_PW_POWER_COORDS);
                    Sleep(CLICK_DELAY/2);
                } else {
                    PWs[activePW].powerAction = 1;
                }
            }
        } else {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3")) {
                if (Image::Click(Images["not_now"].mat, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now")) {
                    Mouse::LeftClick(USE_PW_POWER_COORDS);
                    Sleep(CLICK_DELAY/2);
                }
            }
            /*if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2")) {
                Mouse::LeftClick(USE_PW_POWER_COORDS);
                Sleep(CLICK_DELAY/2);
            }*/
            Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
        }
    } else if (PWs[activePW].name == "saheeli") {
        Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
        //Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
        Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    } else if (PWs[activePW].name == "sorin") {
        if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2"))
                if (!Image::Find(Images["ability2"].mat, GameWindowHWND, Images["ability2"].tol, "ability2"))
                    PWs[activePW].powerAction = 1;
        } else if (PWs[activePW].powerAction == 1) {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3"))
                if (!Image::Find(Images["ability3"].mat, GameWindowHWND, Images["ability3"].tol, "ability3"))
                    PWs[activePW].powerAction = 2;
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
            Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
        }
    } else if (PWs[activePW].name == "chandra") {
        Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
    } else if (PWs[activePW].name == "gideon_bf") {
        Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
    } else if (PWs[activePW].name == "liliana") {
        Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
        Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    } else if (PWs[activePW].name == "jace") {
        Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
    } else if (PWs[activePW].name == "ajani") {
        if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2")) {
            unordered_map<string,bool> imBools = {{"select_creature",false},
                                                  {"creature_selected",false},
                                                  {"not_now",false},
                                                  {"replace_creature",false}};
            UpdateGameWindowStatus();
            FindImages(imBools);
            if (!imBools["replace_creature"]) {
                if (imBools["select_creature"]) {
                    Mouse::LeftClick(BOTTOM_CREATURE_COORDS);
                    Sleep(CLICK_DELAY/8);
                    if (!Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected")) {
                        Mouse::LeftClick(MIDDLE_CREATURE_COORDS);
                        Sleep(CLICK_DELAY/8);
                        if (!Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected")) {
                            Mouse::LeftClick(TOP_CREATURE_COORDS);
                            Sleep(CLICK_DELAY/8);
                            Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected");
                        }
                    }
                } else if (imBools["creature_selected"]) {
                    Image::Click(Images["creature_selected"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected");
                    return 3;
                }
            }
            Image::Click(Images["not_now"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now");
        }
    } else if (PWs[activePW].name == "ob_nixilis") {
        if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3"))
                if (!Image::Find(Images["ability3"].mat, GameWindowHWND, Images["ability3"].tol, "ability3"))
                    PWs[activePW].powerAction = 1;
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2")) {
                unordered_map<string,bool> imBools = {{"select_creature",false},
                                                      {"creature_selected",false},
                                                      {"not_now",false},
                                                      {"replace_creature",false}};
                UpdateGameWindowStatus();
                FindImages(imBools);
                if (!imBools["replace_creature"]) {
                    if (imBools["select_creature"]) {
                        Mouse::LeftClick(BOTTOM_CREATURE_COORDS);
                        Sleep(CLICK_DELAY/4);
                        if (!Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected")) {
                            Mouse::LeftClick(MIDDLE_CREATURE_COORDS);
                            Sleep(CLICK_DELAY/4);
                            if (!Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected")) {
                                Mouse::LeftClick(TOP_CREATURE_COORDS);
                                Sleep(CLICK_DELAY/4);
                                Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected");
                            }
                        }
                    } else if (imBools["creature_selected"]) {
                        Image::Click(Images["creature_selected"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected");
                        return 3;
                    }
                }
                Image::Click(Images["not_now"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now");
            }
            if (rand()%3 == 0) Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
        }
    } else if (PWs[activePW].name == "jace_uos") {
        Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
    } else if (PWs[activePW].name == "arlinn") {
        if (PWFound == 2) {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2")) {
                Image::Click(Images["not_now"].mat, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now");
            }
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
        }
    } else if (PWs[activePW].name == "liliana_tlh") {
        /*if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3"))
                if (!Image::Find(Images["ability3"].mat, GameWindowHWND, Images["ability3"].tol, "ability3"))
                    PWs[activePW].powerAction = 1;
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
        }*/
        Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
    } else if (PWs[activePW].name == "gideon_aoz") {
        Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
        if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2"))
            Image::Click(Images["not_now"].mat, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now");
    } else if (PWs[activePW].name == "garruk") {
        if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2"))
            Image::Click(Images["not_now"].mat, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now");
    } else if (PWs[activePW].name == "nahiri") {
        Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    } else if (PWs[activePW].name == "dovin") {
        if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2"))
                if (!Image::Find(Images["ability2"].mat, GameWindowHWND, Images["ability2"].tol, "ability2"))
                    PWs[activePW].powerAction = 1;
        } else if (PWs[activePW].powerAction == 1) {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3"))
                if (!Image::Find(Images["ability3"].mat, GameWindowHWND, Images["ability3"].tol, "ability3"))
                    PWs[activePW].powerAction = 0;
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
            Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
        }
    } else if (PWs[activePW].name == "chandra_tod") {
        /*if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3"))
                if (!Image::Find(Images["ability3"].mat, GameWindowHWND, Images["ability3"].tol, "ability3"))
                    PWs[activePW].powerAction = 1;
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2")) {
                unordered_map<string,bool> imBools = {{"select_creature",false},
                                                      {"creature_selected",false},
                                                      {"not_now",false},
                                                      {"replace_creature",false}};
                UpdateGameWindowStatus();
                FindImages(imBools);
                if (!imBools["replace_creature"]) {
                    if (imBools["select_creature"]) {
                        Mouse::LeftClick(BOTTOM_CREATURE_COORDS);
                        Sleep(CLICK_DELAY/4);
                        if (!Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected")) {
                            Mouse::LeftClick(MIDDLE_CREATURE_COORDS);
                            Sleep(CLICK_DELAY/4);
                            if (!Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected")) {
                                Mouse::LeftClick(TOP_CREATURE_COORDS);
                                Sleep(CLICK_DELAY/4);
                                Image::Click(Images["creature_selected"].mat, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected");
                            }
                        }
                    } else if (imBools["creature_selected"]) {
                        Image::Click(Images["creature_selected"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["creature_selected"].tol, "creature_selected");
                        return 3;
                    }
                }
                Image::Click(Images["not_now"].mat, GameWindowImage, GameWindowHWND, CLICK_DELAY, Images["not_now"].tol, "not_now");
            }
            Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
        }*/
        Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    } else if (PWs[activePW].name == "nissa_vf") {
        if (PWs[activePW].powerAction == 0) {
            if (Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2"))
                if (!Image::Find(Images["ability2"].mat, GameWindowHWND, Images["ability2"].tol, "ability2"))
                    PWs[activePW].powerAction = 1;
        } else {
            Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
            Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
            Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
        }
    } else if (PWs[activePW].name == "sarkhan") {
        if (rand()%3 == 0) Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    } else {
        Image::Click(Images["ability3"].mat, GameWindowHWND, CLICK_DELAY, Images["ability3"].tol, "ability3");
        if (rand()%3 == 0) Image::Click(Images["ability2"].mat, GameWindowHWND, CLICK_DELAY, Images["ability2"].tol, "ability2");
        if (rand()%4 == 0) Image::Click(Images["ability1"].mat, GameWindowHWND, CLICK_DELAY, Images["ability1"].tol, "ability1");
    }

    Mouse::LeftClick(CLOSE_PW_POWER_COORDS);
    Sleep(CLICK_DELAY/2);
    return 3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::ChangePW(int desiredPW, bool click) {
    if (desiredPW < 0 || desiredPW > PWs.size()) return false;
    if (click) {
        if (gameMode == 'e') Mouse::LeftClick(CHANGE_PW_EVENTS_COORDS);
        else if (gameMode == 'q') Mouse::LeftClick(CHANGE_PW_QB_COORDS);
        else return false;
    }
    Sleep(CLICK_DELAY);
    if (!Image::Find(Images["select_pw"].mat, GameWindowHWND, Images["select_pw"].tol, "select_pw"))
        return false;
    cout << "-D- Bot::ChangePW - Searching for Planeswalker " << PWs[desiredPW].name << endl;
    unordered_map<string,bool> imBools;
    for (unsigned int i = 0; i < PWs.size(); i++)
        imBools.emplace(PWs[i].name+"2", false);
    int currentPW = -1, count = 0, dir = -1;
    while (currentPW != desiredPW && count < PWs.size()+2) {
        UpdateGameWindowStatus();
        FindImages(imBools);
        currentPW = -1;
        for (auto it = imBools.begin(); it != imBools.end(); it++)
            if (it->second) {
                for (unsigned int i = 0; i < PWs.size(); i++)
                    if (it->first == PWs[i].name+"2") {
                        currentPW = i;
                        break;
                    }
                break;
            }
        if (currentPW < 0) {
            activePW = -1;
            if (!Image::Click(Images["x_choose_pw"].mat, GameWindowHWND, CLICK_DELAY, Images["x_choose_pw"].tol, "x_choose_pw"))
                Image::Click(Images["select_pw"].mat, GameWindowHWND, CLICK_DELAY, Images["select_pw"].tol, "select_pw");
            return false;
        }
        dir = strcmp(PWs[desiredPW].name.c_str(), PWs[currentPW].name.c_str());
        if (dir < 0) Mouse::Drag(SWIPE_PW_L_COORDS, SWIPE_PW_R_COORDS, 3);
        else if (dir > 0) Mouse::Drag(SWIPE_PW_R_COORDS, SWIPE_PW_L_COORDS, 3);
        else if (Image::Click(Images["select_pw"].mat, GameWindowHWND, CLICK_DELAY, Images["select_pw"].tol, "select_pw")) {
            activePW = currentPW;
            cout << "-D- Bot::ChangePW - Active Planeswalker is now " << PWs[activePW].name << endl;
            return true;
        }
        count++;
        Sleep(CLICK_DELAY/4);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Bot::FindImages(unordered_map<string,bool> &imBools) {
    vector<std::thread> threads;
    unsigned int max_threads = std::thread::hardware_concurrency();
    if (max_threads > 2) max_threads--;
    unsigned int i = 0;
    auto it = imBools.begin();
    for (it; it != imBools.end(); it++) {
        threads.push_back(std::thread((bool (*)(const Mat &, const Mat &, double, bool &, string)) Image::Find,
                                      ref(Images[it->first].mat), ref(GameWindowImage), Images[it->first].tol, ref(imBools[it->first]), it->first));
        i++;
        if (i >= max_threads) {
            i = 0;
            for (auto &th : threads) th.join();
            threads.clear();
        }
    }
    for (auto &th : threads) th.join();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::GetMoveCoords(int Move, int x, int y, vector<pair<int,int>> &p) {
    vector<string> MovePositions = Util::SplitString(Moves[Move], ',');
    int offset = 0;
    for (unsigned int i = 0; i < p.size()-1; i++) {
        if (i < MovePositions.size()) {
            p[i].first = x;
            p[i].second = y;
            if (MovePositions[i][1] == '1') offset = 1;
            else if (MovePositions[i][1] == '2') offset = 2;
            if (MovePositions[i][0] == 'N' || MovePositions[i][0] == 'L') offset = -offset;
            if (MovePositions[i][0] == 'L' || MovePositions[i][0] == 'R') p[i].first += offset;
            else p[i].second += offset;
        } else {
            p[i].first = p[i-1].first;
            p[i].second = p[i-1].second;
        }
        if (p[i].first < 0 || p[i].first > 6 || p[i].second < 0 || p[i].second > 6) return false;
    }
    p[p.size()-1].first = x;
    p[p.size()-1].second = y;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int Bot::FindBestMove(vector<vector<char>> Board, vector<pair<int,int>> &p) {
    int BestMove = -1;
    vector<vector<int>> MovesData;

    for (unsigned int Move = 0; Move < Moves.size(); Move++) {
        for (unsigned int j = 0; j < Board.size(); j++) {
            for (unsigned int i = 0; i < Board[j].size(); i++) {
                if (GetMoveCoords(Move, i, j, p)) {
                    if (Board[p[1].first][p[1].second] == Board[p[0].first][p[0].second] &&
                        Board[p[2].first][p[2].second] == Board[p[0].first][p[0].second] &&
                        Board[p[3].first][p[3].second] == Board[p[0].first][p[0].second] &&
                        Board[p[4].first][p[4].second] == Board[p[0].first][p[0].second]) {
                        int x1, y1, x2, y2;
                        if (p[4].first + p[4].second <= p[5].first + p[5].second) {
                            x1 = p[4].first; y1 = p[4].second; x2 = p[5].first; y2 = p[5].second;
                        } else {
                            x1 = p[5].first; y1 = p[5].second; x2 = p[4].first; y2 = p[4].second;
                        }
                        int MoveMana = 0;
                        if (Moves[Move].size() == 14) MoveMana = 100;
                        else if (Moves[Move].size() == 11) MoveMana = 4;
                        else if (Moves[Move].size() == 8) MoveMana = 3;
                        if (Board[p[4].first][p[4].second] == '-' && Moves[Move].size() != 14) MoveMana = 0;
                        if (Board[p[4].first][p[4].second] == 'X' && Moves[Move].size() != 14) MoveMana = 1;
                        bool found = false;
                        for (unsigned int mm = 0; mm < MovesData.size(); mm++) {
                            if (MovesData[mm][0] == x1 && MovesData[mm][1] == y1 && MovesData[mm][2] == x2 && MovesData[mm][3] == y2 && MovesData[mm][6] != (int)Board[p[4].first][p[4].second]) {
                                if (PWs[activePW].mana[Board[p[4].first][p[4].second]] + MoveMana > MovesData[mm][4]) {
                                    MovesData[mm][5] = Move;
                                    MovesData[mm][6] = (int)Board[p[4].first][p[4].second];
                                }
                                MovesData[mm][4] += PWs[activePW].mana[Board[p[4].first][p[4].second]] + MoveMana;
                                found = true;
                                break;
                            }
                        }
                        if (!found) MovesData.push_back({x1, y1, x2, y2, PWs[activePW].mana[Board[p[4].first][p[4].second]] + MoveMana, (int)Move, (int)Board[p[4].first][p[4].second]});
                    }
                }
            }
        }
    }

    int best = -1; char color = ' ';
    for (unsigned int i = 0; i < MovesData.size(); i++) {
        if (MovesData[i][4] > best) {
            p[4].first = MovesData[i][0]; p[4].second = MovesData[i][1];
            p[5].first = MovesData[i][2]; p[5].second = MovesData[i][3];
            BestMove = MovesData[i][5]; best = MovesData[i][4]; color = (char)MovesData[i][6];
        }
    }

    if (Board[p[4].first][p[4].second] == color) {
        p[1].first = p[4].first;
        p[1].second = p[4].second;
    } else {
        p[1].first = p[5].first;
        p[1].second = p[5].second;
    }

    /*if (BestMove >= 0) {
        cout << "-D- Bot::FindBestMove - Best move ID: " << BestMove << endl;
        cout << "-D- Bot::FindBestMove - Best move: " << Moves[BestMove] << endl;
        cout << "-D- Bot::FindBestMove - Best move position: (" << p[4].first+1 << "," << p[4].second+1 << ") (" << p[5].first+1 << "," << p[5].second+1 << ")" << endl;
        cout << "-D- Bot::FindBestMove - Best move mana: " << best << endl;
        cout << "-D- Bot::FindBestMove - Best move color: " << color << endl;
    } else {
        cout << "-D- Bot::FindBestMove - Best move ID: " << BestMove << endl;
    }*/
    return BestMove;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Bot::UpdateBoardStateForOneManaColor(ImageData myImageData, string ImageName, vector<vector<vector<char>>> &TempBoard, vector<vector<vector<double>>> &TempValues) {
    vector<double> Values;
    vector<Point> Coordinates = Image::SearchInsideOtherImage(myImageData.mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', myImageData.tol, Values, ImageName);
    int BoardX = 1, BoardY = 1;
    for (unsigned int c = 0; c < Coordinates.size(); c++) {
        bool found = false;
        for (unsigned int i = 0; i < 7; i++) {
            int diff = 1000000000;
            for (unsigned int j = 0; j < 7; j++) {
                int tempdiff = (int)norm(Point(xBoardCoords[i], yBoardCoords[j]) - Point(Coordinates[c].x, Coordinates[c].y));
                if (tempdiff < diff && tempdiff < GameWindowWidth/50) {
                    found = true;
                    diff = tempdiff;
                    BoardX = i;
                    BoardY = j;
                }
            }
        }
        if (found) {
            mtx.lock();
            if (Values[c] < TempValues[BoardX][BoardY].back()) {
                TempValues[BoardX][BoardY].push_back(Values[c]);
                TempBoard[BoardX][BoardY].push_back(myImageData.color);
            }
            mtx.unlock();
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Bot::UpdateBoardState(vector<vector<char>> &Board) {
    Board = vector<vector<char>>(7, vector<char>(7, '-'));
    vector<vector<vector<char>>> TempBoard = vector<vector<vector<char>>>(7, Board);
    vector<vector<vector<double>>> TempValues = vector<vector<vector<double>>>(7, vector<vector<double>>(7, vector<double>(1, 1.1)));
    vector<std::thread> colorThreads;
    for (auto &im : Images)
        if (im.second.color != '-')
            colorThreads.push_back(std::thread(&Bot::UpdateBoardStateForOneManaColor, this, im.second, im.first, ref(TempBoard), ref(TempValues)));
    for (auto &th : colorThreads) th.join();

    for (unsigned int i = 0; i < 7; i++) {
        for (unsigned int j = 0; j < 7; j++) {
            Board[i][j] = TempBoard[i][j].back();
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::UpdateGameWindowStatus() {
    GameWindowImage = Image::HWND2Mat(GameWindowHWND);
    if (GameWindowImage.empty()) {
        cout << "-E- Bot::UpdateGameWindowStatus - Failed to get screenshot from game window." << endl;
        return false;
    }
    RECT WindowSize;
    if (!GetClientRect(GameWindowHWND, &WindowSize)) {
        cout << "-E- Bot::UpdateGameWindowStatus - GetClientRect on GameWindowHWND failed." << endl;
        return false;
    }
    GameWindowWidth = WindowSize.right;
    GameWindowHeight = WindowSize.bottom;
    if ((double)GameWindowWidth/GameWindowHeight <= 0.5 || (double)GameWindowWidth/GameWindowHeight >= 0.625) {
        GameWindowLeft = GameWindowWidth/2 - (GameWindowHeight * 9/16)/2;
        GameWindowWidth = GameWindowHeight * 9/16;
    }
    xBoardCoords[0] = GameWindowLeft+(int)(GameWindowWidth*0.125);
    xBoardCoords[1] = (int)(xBoardCoords[0]+GameWindowWidth*0.125);
    xBoardCoords[2] = (int)(xBoardCoords[1]+GameWindowWidth*0.125);
    xBoardCoords[3] = (int)(xBoardCoords[2]+GameWindowWidth*0.125);
    xBoardCoords[4] = (int)(xBoardCoords[3]+GameWindowWidth*0.125);
    xBoardCoords[5] = (int)(xBoardCoords[4]+GameWindowWidth*0.125);
    xBoardCoords[6] = (int)(xBoardCoords[5]+GameWindowWidth*0.125);
    yBoardCoords[0] = (int)(GameWindowHeight*0.5);
    yBoardCoords[1] = (int)(yBoardCoords[0]+GameWindowWidth*0.125);
    yBoardCoords[2] = (int)(yBoardCoords[1]+GameWindowWidth*0.125);
    yBoardCoords[3] = (int)(yBoardCoords[2]+GameWindowWidth*0.125);
    yBoardCoords[4] = (int)(yBoardCoords[3]+GameWindowWidth*0.125);
    yBoardCoords[5] = (int)(yBoardCoords[4]+GameWindowWidth*0.125);
    yBoardCoords[6] = (int)(yBoardCoords[5]+GameWindowWidth*0.125);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Bot::InitializePWData() {
    PWData tmpPWData;

    tmpPWData.powerAction = 0;

    tmpPWData.name = "koth";
    tmpPWData.colors = {'R'};
    tmpPWData.mana = {{'U', -1}, {'B', 0}, {'G', 0}, {'R', 9}, {'W', -1}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "nissa";
    tmpPWData.colors = {'G'};
    tmpPWData.mana = {{'U', 0}, {'B', 0}, {'G', 3}, {'R', 1}, {'W', 1}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "chandra";
    tmpPWData.colors = {'R'};
    tmpPWData.mana = {{'U', 0}, {'B', 1}, {'G', 1}, {'R', 3}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "gideon_bf";
    tmpPWData.colors = {'W'};
    tmpPWData.mana = {{'U', 1}, {'B', 0}, {'G', 1}, {'R', 0}, {'W', 3}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "jace";
    tmpPWData.colors = {'U'};
    tmpPWData.mana = {{'U', 3}, {'B', 1}, {'G', 0}, {'R', 0}, {'W', 1}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "liliana";
    tmpPWData.colors = {'B'};
    tmpPWData.mana = {{'U', 1}, {'B', 3}, {'G', 0}, {'R', 1}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "kiora";
    tmpPWData.colors = {'U', 'G'};
    tmpPWData.mana = {{'U', 3}, {'B', 0}, {'G', 3}, {'R', 0}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "saheeli";
    tmpPWData.colors = {'U', 'R'};
    tmpPWData.mana = {{'U', 4}, {'B', 1}, {'G', 1}, {'R', 4}, {'W', 1}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "sorin";
    tmpPWData.colors = {'B', 'W'};
    tmpPWData.mana = {{'U', 0}, {'B', 4}, {'G', 0}, {'R', 0}, {'W', 4}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "ajani";
    tmpPWData.colors = {'R', 'W'};
    tmpPWData.mana = {{'U', 1}, {'B', 0}, {'G', 2}, {'R', 2}, {'W', 3}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "ob_nixilis";
    tmpPWData.colors = {'B'};
    tmpPWData.mana = {{'U', 1}, {'B', 3}, {'G', 0}, {'R', 1}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "jace_uos";
    tmpPWData.colors = {'U'};
    tmpPWData.mana = {{'U', 5}, {'B', 4}, {'G', 0}, {'R', 0}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "arlinn";
    tmpPWData.colors = {'G', 'R'};
    tmpPWData.mana = {{'U', 0}, {'B', 0}, {'G', 3}, {'R', 3}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "liliana_tlh";
    tmpPWData.colors = {'B'};
    tmpPWData.mana = {{'U', 4}, {'B', 5}, {'G', 0}, {'R', 0}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "gideon_aoz";
    tmpPWData.colors = {'W'};
    tmpPWData.mana = {{'U', 2}, {'B', 0}, {'G', 2}, {'R', 0}, {'W', 2}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "dovin";
    tmpPWData.colors = {'U', 'W'};
    tmpPWData.mana = {{'U', 3}, {'B', 0}, {'G', 0}, {'R', 0}, {'W', 3}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "garruk";
    tmpPWData.colors = {'G'};
    tmpPWData.mana = {{'U', 0}, {'B', 1}, {'G', 3}, {'R', 2}, {'W', 2}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "nahiri";
    tmpPWData.colors = {'R', 'W'};
    tmpPWData.mana = {{'U', 0}, {'B', -1}, {'G', 1}, {'R', 4}, {'W', 4}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "chandra_tod";
    tmpPWData.colors = {'R'};
    tmpPWData.mana = {{'U', 0}, {'B', 4}, {'G', 0}, {'R', 5}, {'W', 0}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "nissa_vf";
    tmpPWData.colors = {'G'};
    tmpPWData.mana = {{'U', 0}, {'B', 0}, {'G', 4}, {'R', 2}, {'W', 3}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);

    tmpPWData.name = "sarkhan";
    tmpPWData.colors = {'B', 'R'};
    tmpPWData.mana = {{'U', 1}, {'B', 2}, {'G', 1}, {'R', 2}, {'W', 1}, {'X', 0}, {'-', 0}};
    PWs.push_back(tmpPWData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::ReadImages() {
    double resizeFactor = GameWindowHeight/(double)DEFAULT_GAME_WND_HEIGHT;
    string RootFolder = "./images";
    vector<string> FolderNames = Util::GetFileNamesFromDir(RootFolder, "*", false);
    for (unsigned int i = 0; i < FolderNames.size(); i++) {
        vector<string> FileNames = Util::GetFileNamesFromDir(RootFolder + "/" + FolderNames[i], "*.bmp", true);
        for (unsigned int j = 0; j < FileNames.size(); j++) {
            string myImageName = FileNames[j].substr(0, FileNames[j].length()-4);
            ImageData myImageData;
            Mat tempMat = imread(RootFolder + "/" + FolderNames[i] + "/" + FileNames[j]);
            if (tempMat.empty()) {
                cout << "-E- Bot::ReadImages - Can't read image " << RootFolder + "/" + FolderNames[i] + "/" + FileNames[j] << endl;
                return false;
            }
            cvtColor(tempMat, tempMat, CV_BGR2BGRA);
            myImageData.tol = 0.05;

            if      (myImageName == "pw")       myImageData.color = 'X';
            else if (myImageName == "black")    myImageData.color = 'B';
            else if (myImageName == "black2")   myImageData.color = 'B';
            else if (myImageName == "blacke")   myImageData.color = 'B';
            else if (myImageName == "blue")     myImageData.color = 'U';
            else if (myImageName == "blue2")    myImageData.color = 'U';
            else if (myImageName == "bluee")    myImageData.color = 'U';
            else if (myImageName == "green")    myImageData.color = 'G';
            else if (myImageName == "green2")   myImageData.color = 'G';
            else if (myImageName == "greene")   myImageData.color = 'G';
            else if (myImageName == "red")      myImageData.color = 'R';
            else if (myImageName == "red2")     myImageData.color = 'R';
            else if (myImageName == "rede")     myImageData.color = 'R';
            else if (myImageName == "white")    myImageData.color = 'W';
            else if (myImageName == "white2")   myImageData.color = 'W';
            else if (myImageName == "whitee")   myImageData.color = 'W';
            else                                myImageData.color = '-';

            RECT WindowSize;
            GetClientRect(GameWindowHWND, &WindowSize);
            if (resizeFactor > 100.0) resizeFactor = 100.0;
            else if (resizeFactor <= 0) resizeFactor = 1.0;
            if (resizeFactor > 1.0 && myImageName != "bluestacks_close" && myImageName != "game_icon")
                resize(tempMat, tempMat, Size(), resizeFactor, resizeFactor, INTER_CUBIC);
            else if (resizeFactor < 1.0 && myImageName != "bluestacks_close" && myImageName != "game_icon")
                resize(tempMat, tempMat, Size(), resizeFactor, resizeFactor, INTER_AREA);
            Images.emplace(myImageName, myImageData);
            Images[myImageName].mat = tempMat.clone();
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::ReadImagesTolerances() {
    ifstream configFile(IMAGE_TOLERANCES_FILE);
    if (!configFile.is_open()) {
        cout << "-E- Bot::ReadImagesTolerances - Can't open file: '" << IMAGE_TOLERANCES_FILE << "'" << endl;
        return false;
    }
    string line;
    while (getline(configFile, line)) {
        stringstream ss_line(line);
        string varName;
        if (getline(ss_line, varName, '=')) {
            string varValue;
            if (getline(ss_line, varValue)) {
                Images[varName].tol = atof(varValue.c_str());
            } else {
                cout << "-E- Bot::ReadImagesTolerances - Format error reading image tolerances config file line: " << line << endl;
            }
        } else {
            cout << "-E- Bot::ReadImagesTolerances - Format error reading image tolerances config file line: " << line << endl;
        }
    }
    configFile.close();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Bot::SetPWsOrder(vector<string> PWsOrderedList) {
    vector<PWData> tmpPWs = PWs;
    PWs.clear();
    for (int i = 0; i < PWsOrderedList.size(); i++) {
        for (int j = 0; j < tmpPWs.size(); j++) {
            if (PWsOrderedList[i] == tmpPWs[j].name) {
                PWs.push_back(tmpPWs[j]);
                break;
            }
        }
    }
    if (tmpPWs.size() != PWs.size()) {
        PWs = tmpPWs;
        cout << "-E- Bot::SetPWsOrder - The elements in the list don't match the PlanesWalker names." << endl;
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
vector<string> Bot::GetPWsOrder() {
    vector<string> PWsOrderedList;
    for (int i = 0; i < PWs.size(); i++) {
        PWsOrderedList.push_back(PWs[i].name);
    }
    return PWsOrderedList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Bot::CalibrateImage(string ImageName, int imgCount) {
    if (imgCount < 1) {
        cout << "-E- Bot::CalibrateImage - Number of images must be >= 1." << endl;
        return;
    }
    UpdateGameWindowStatus();

    double minTol = 1.0, maxTol = 0.0;
    //int minTolCount = -1, maxTolCount = -1;
    vector<double> Values;
    double i_min = 0.0;
    double i_max = 1.0;
    double i_step = 1.0;

    for (int a = 0; a < 3; a++) {
        double prev_i_step = i_step;
        i_step /= 10.0;
        if (minTol - prev_i_step >= 0.0) {
            i_min = minTol - prev_i_step;
            i_max = minTol;
            for (double i = i_min; i <= i_max; i += i_step) {
                vector<Point> Coords = Image::SearchInsideOtherImage(Images[ImageName].mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', i, Values);
                minTol = i;
                //minTolCount = (int)Coords.size();
                if (Coords.size() >= imgCount) {
                    break;
                }
            }
        }
        if (maxTol + prev_i_step <= 1.0) {
            i_min = maxTol;
            i_max = maxTol + prev_i_step;
            for (double i = i_min; i <= i_max; i += i_step) {
                vector<Point> Coords = Image::SearchInsideOtherImage(Images[ImageName].mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', i, Values);
                if (Coords.size() > imgCount) {
                    break;
                }
                maxTol = i;
                //maxTolCount = (int)Coords.size();
            }
        }
    }

    cout << "-I- Bot::CalibrateImage - Results:" << endl;
    cout << "    Min tolerance = " << minTol << endl;
    cout << "    Max tolerance = " << maxTol << endl;
    cout << endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Bot::TestFindImage(string ImageName, double TolerancePerc) {
    UpdateGameWindowStatus();
    vector<double> Values;
    vector<Point> Coordinates = Image::SearchInsideOtherImage(Images[ImageName].mat, GameWindowImage, CV_TM_CCOEFF_NORMED, 'm', TolerancePerc, Values, ImageName);
    Mat Temp = GameWindowImage.clone();
    for (unsigned int j = 0; j < Coordinates.size(); j++) {
        rectangle(Temp, Point(Coordinates[j].x-Images[ImageName].mat.cols/2, Coordinates[j].y-Images[ImageName].mat.rows/2), Point(Coordinates[j].x+Images[ImageName].mat.cols/2, Coordinates[j].y+Images[ImageName].mat.rows/2), Scalar::all(0), 2, 8, 0);
        rectangle(Temp, Point(Coordinates[j].x-1, Coordinates[j].y-1), Point(Coordinates[j].x+1, Coordinates[j].y+1), Scalar::all(255), 2, 8, 0);
        cout << "-I- Bot::CalibrateImage - Tolerance:   " << Values[j] << endl;
        cout << "-I- Bot::CalibrateImage - Coordinates: " << Coordinates[j].x << "," << Coordinates[j].y << endl;
    }
    cout << endl;
    imshow("TestFindImage", Temp);
    waitKey(0);
    return;
}
