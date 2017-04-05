#include "botmainwindow.hpp"
#include "ui_botmainwindow.h"
#include "util.hpp"
#include <fstream>
#include <QMessageBox>

#define CONFIG_FILE "config.txt"
#define LOG_FILE "mtgpq_bot_log.txt"

BotMainWindow::BotMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BotMainWindow)
{
    ui->setupUi(this);
    ui->pw_prior_list->setDragDropMode(QAbstractItemView::InternalMove);
    connect(ui->pw_prior_list->model(), SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)), this, SLOT(custom_on_pw_prior_list_rowsMoved()));
    ui->pauseBot->setEnabled(false);
    ui->stopBot->setEnabled(false);
    ui->makeBestPlay->setEnabled(false);
    ui->calibrateImage->setEnabled(false);
    ui->testFindImage->setEnabled(false);
    stdOut = new RedirectStream(cout, ui->log, LOG_FILE);
    theBot = new QThreadBotWrapper();
    connect(theBot, SIGNAL(done()), ui->stopBot, SLOT(click()));
    vector<string> PWNames = theBot->GetPWsOrder();
    for (int i = 0; i < PWNames.size(); i++)
        ui->pw_prior_list->insertItem(i, QString::fromStdString(PWNames[i]));
    vector<string> orderedImages;
    for (auto &im : theBot->Images)
        orderedImages.push_back(im.first);
    sort(orderedImages.begin(), orderedImages.end());
    for (int i = 0; i < orderedImages.size(); i++)
        ui->imgName->addItem(QString::fromStdString(orderedImages[i]));
    ReadConfigFile();
}

BotMainWindow::~BotMainWindow()
{
    delete ui;
    delete theBot;
    delete stdOut;
}

bool BotMainWindow::ReadConfigFile()
{
    ifstream configFile(CONFIG_FILE);
    if (!configFile.is_open()) {
        cout << "-E- BotMainWindow::ReadConfigFile - Can't open file: '" << CONFIG_FILE << "'. Default settings will be used." << endl;
        configFileInUse = false;
        WriteConfigFile();
        ReadConfigFile();
        return false;
    }
    configFileInUse = true;
    string line;
    while (getline(configFile, line)) {
        stringstream ss_line(line);
        string varName;
        if (getline(ss_line, varName, '=')) {
            string varValue;
            if (getline(ss_line, varValue)) {
                if (varName == "PlayQB") {
                    theBot->playQB = (bool)atoi(varValue.c_str());
                } else if (varName == "PlayEvents") {
                    theBot->playEvents = (bool)atoi(varValue.c_str());
                } else if (varName == "JoinQB") {
                    theBot->joinQB = (bool)atoi(varValue.c_str());
                } else if (varName == "JoinEvents") {
                    theBot->joinEvents = (bool)atoi(varValue.c_str());
                } else if (varName == "PWPriorities") {
                    vector<string> PWNames = Util::SplitString(varValue, ',');
                    theBot->SetPWsOrder(PWNames);
                } else {
                    cout << "-E- BotMainWindow::ReadConfigFile - Option '" << varName << "' is not valid, ignoring it." << endl;
                }
            } else {
                cout << "-E- BotMainWindow::ReadConfigFile - Format error reading config file line " << line << endl;
            }
        } else {
            cout << "-E- BotMainWindow::ReadConfigFile - Format error reading config file line " << line << endl;
        }
    }
    configFile.close();

    // Update GUI
    ui->play_qb->setChecked(theBot->playQB);
    ui->play_events->setChecked(theBot->playEvents);
    ui->join_qb->setChecked(theBot->joinQB);
    ui->join_events->setChecked(theBot->joinEvents);
    vector<string> PWNames = theBot->GetPWsOrder();
    ui->pw_prior_list->clear();
    for (int i = 0; i < PWNames.size(); i++) {
        ui->pw_prior_list->insertItem(i, QString::fromStdString(PWNames[i]));
    }

    configFileInUse = false;
    WriteConfigFile();
    return true;
}

bool BotMainWindow::WriteConfigFile()
{
    if (configFileInUse) return false;
    ofstream configFile(CONFIG_FILE);
    if (!configFile.is_open()) {
        cout << "-E- BotMainWindow::WriteConfigFile - Can't open file: '" << CONFIG_FILE << "'" << endl;
        return false;
    }
    configFile << "PlayQB=" << (theBot->playQB?1:0) << endl;
    configFile << "PlayEvents=" << (theBot->playEvents?1:0) << endl;
    configFile << "JoinQB=" << (theBot->joinQB?1:0) << endl;
    configFile << "JoinEvents=" << (theBot->joinEvents?1:0) << endl;
    configFile << "PWPriorities=";
    vector<string> PWNames = theBot->GetPWsOrder();
    for (int i = 0; i < PWNames.size(); i++) {
        configFile << PWNames[i];
        if (i < PWNames.size()-1) configFile << ",";
    }
    configFile.close();
    return true;
}

void BotMainWindow::on_startBot_clicked()
{
    if (ui->play_qb->isChecked() == false && ui->play_events->isChecked() == false) {
        cout << "-E- BotMainWindow::on_startBot_clicked - At least one of the following options must be checked: Play QuickBattles/Play Events." << endl;
        return;
    }
    stringstream ss;
    ss << "mtgpq_bot_log_" << time(NULL) << ".txt";
    rename(LOG_FILE, ss.str().c_str());
    ofstream logFile(LOG_FILE);
    if (!logFile.is_open()) {
        cout << "-E- BotMainWindow::on_startBot_clicked - Can't open file: '" << LOG_FILE << "'" << endl;
        return;
    }
    logFile.close();
    ui->log->setTextInteractionFlags(Qt::NoTextInteraction);
    ui->log->clear();
    ui->startBot->setEnabled(false);
    ui->play_events->setEnabled(false);
    ui->play_qb->setEnabled(false);
    ui->pauseBot->setEnabled(true);
    ui->stopBot->setEnabled(true);
    ui->makeBestPlay->setEnabled(false);
    ui->calibrateImage->setEnabled(false);
    ui->testFindImage->setEnabled(false);
    ui->pw_prior_list->setEnabled(false);
    ui->setGameWindow->setEnabled(false);
    theBot->initOnly = false;
    QMessageBox::information(0, "Starting bot...", "To start the bot click OK and then place the mouse cursor over the game window.");
    theBot->start();
}

void BotMainWindow::on_stopBot_clicked()
{
    theBot->Stop();
    ui->log->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);
    ui->startBot->setEnabled(true);
    ui->play_events->setEnabled(true);
    ui->play_qb->setEnabled(true);
    ui->pauseBot->setText("Pause");
    ui->pauseBot->setEnabled(false);
    ui->stopBot->setEnabled(false);
    ui->makeBestPlay->setEnabled(true);
    ui->setGameWindow->setEnabled(true);
    ui->pw_prior_list->setEnabled(true);
    theBot->pause = false;
    if (imgNameTextMatches()) {
        ui->calibrateImage->setEnabled(true);
        ui->testFindImage->setEnabled(true);
    } else {
        ui->calibrateImage->setEnabled(false);
        ui->testFindImage->setEnabled(false);
    }
}

void BotMainWindow::on_play_qb_stateChanged(int arg1)
{
    theBot->playQB = (bool)arg1;
    WriteConfigFile();
}

void BotMainWindow::on_play_events_stateChanged(int arg1)
{
    theBot->playEvents = (bool)arg1;
    WriteConfigFile();
}

void BotMainWindow::on_join_qb_stateChanged(int arg1)
{
    theBot->joinQB = (bool)arg1;
    WriteConfigFile();
}

void BotMainWindow::on_join_events_stateChanged(int arg1)
{
    theBot->joinEvents = (bool)arg1;
    WriteConfigFile();
}

void BotMainWindow::custom_on_pw_prior_list_rowsMoved()
{
    vector<string> PWNames;
    for(int i = 0; i < ui->pw_prior_list->count(); i++) {
        PWNames.push_back(ui->pw_prior_list->item(i)->text().toStdString());
    }
    theBot->SetPWsOrder(PWNames);
    WriteConfigFile();
}

void BotMainWindow::on_pauseBot_clicked()
{
    if (ui->pauseBot->text() == "Pause") {
        cout << "-I- BotMainWindow::on_pauseBot_clicked - Pausing bot..." << endl;
        theBot->pause = true;
        ui->makeBestPlay->setEnabled(true);
        ui->pauseBot->setText("Resume");
    } else {
        cout << "-I- BotMainWindow::on_pauseBot_clicked - Resuming bot..." << endl;
        theBot->pause = false;
        ui->pauseBot->setText("Pause");
        ui->makeBestPlay->setEnabled(false);
    }
}

void BotMainWindow::on_setGameWindow_clicked()
{
    theBot->initOnly = true;
    theBot->start();
    ui->makeBestPlay->setEnabled(true);
}

void BotMainWindow::on_makeBestPlay_clicked()
{
    theBot->makeBestPlay();
}

void BotMainWindow::on_calibrateImage_clicked()
{
    theBot->CalibrateImage(ui->imgName->currentText().toStdString(), ui->imgCount->value());
}

void BotMainWindow::on_testFindImage_clicked()
{
    theBot->TestFindImage(ui->imgName->currentText().toStdString(), ui->imgTol->value());
}

bool BotMainWindow::imgNameTextMatches() {
    string RootFolder = "./images";
    vector<string> FolderNames = Util::GetFileNamesFromDir(RootFolder, "*", false);
    for (unsigned int i = 0; i < FolderNames.size(); i++) {
        vector<string> FileNames = Util::GetFileNamesFromDir(RootFolder + "/" + FolderNames[i], "*.bmp", true);
        for (unsigned int j = 0; j < FileNames.size(); j++) {
            string myImageName = FileNames[j].substr(0, FileNames[j].length()-4);
            if (myImageName == ui->imgName->currentText().toStdString() && theBot->GameWindowHWND != NULL) {
                return true;
            }
        }
    }
    return false;
}

void BotMainWindow::on_imgName_editTextChanged(const QString &arg1)
{
    if (imgNameTextMatches() && theBot->stop) {
        ui->calibrateImage->setEnabled(true);
        ui->testFindImage->setEnabled(true);
    } else {
        ui->calibrateImage->setEnabled(false);
        ui->testFindImage->setEnabled(false);
    }
}
