#ifndef BOTMAINWINDOW_H
#define BOTMAINWINDOW_H

#include "qthreadbotwrapper.hpp"
#include "redirectstream.hpp"
#include <QMainWindow>

namespace Ui {
class BotMainWindow;
}

class BotMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BotMainWindow(QWidget* parent = 0);
    ~BotMainWindow();

private slots:
    void on_startBot_clicked();
    void on_stopBot_clicked();
    void on_play_qb_stateChanged(int arg1);
    void on_play_events_stateChanged(int arg1);
    void on_join_qb_stateChanged(int arg1);
    void on_join_events_stateChanged(int arg1);
    void custom_on_pw_prior_list_rowsMoved();
    void on_pauseBot_clicked();
    void on_setGameWindow_clicked();
    void on_makeBestPlay_clicked();
    void on_calibrateImage_clicked();
    void on_testFindImage_clicked();
    void on_imgName_editTextChanged(const QString &arg1);

private:
    bool ReadConfigFile();
    bool WriteConfigFile();
    bool imgNameTextMatches();

    Ui::BotMainWindow* ui;
    QThreadBotWrapper* theBot;
    bool configFileInUse;
    RedirectStream* stdOut;
};

#endif // BOTMAINWINDOW_H
