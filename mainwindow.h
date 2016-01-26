#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFile>
#include <QDebug>

#include "devicesettings.h"

//Structures
struct ReadingStruct
{
    qint32 Timestamp;
    float Reading;
    char Type;
};

//Defines
#define Hostname "192.168.1.94" //Internal
//#define Hostname "ehealth.noip.me" //External
#define SettingsFilename "eHealth.ini" //

//Constants
const char State_Idle = 0;
const char State_DevID = 1;
const char State_DevReadings = 2;
const char State_WebLogin = 3;
const char State_WebUpload = 4;
const char State_ReadBP = 4;
const char State_UploadBP = 5;
const char State_ReadSensors = 6;
const char State_UploadSensors = 7;
//const char State_ = 3;
//const char State_ = 3;
//const char State_ = 3;
//const char State_ = 3;
const QString Version = "v0.09";

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

private slots:
    void on_btn_Connect_clicked();
    void TimeoutTimerTrigger();
    void SerialDataWaiting();
    void sslErrors(QNetworkReply* nrReply, QList<QSslError> lstSSLErrors);
    void replyFinished(QNetworkReply* nrReply);
    void on_btn_DevConfig_clicked();
    void SaveDeviceConfig();
    void LoadDeviceConfig();
    void on_btn_Open_clicked();
    void on_btn_Close_clicked();
    void on_btn_ReadBP_clicked();
    void on_check_ReadSensors_stateChanged(int arg1);
    void on_btn_En1_clicked();
    void on_btn_En2_clicked();
    void on_btn_En3_clicked();
    void on_btn_En4_clicked();
    void on_btn_En5_clicked();
    void SendReadCommand();

private:
    Ui::MainWindow *ui;
    void RefreshSerial();
    QSerialPort MainSerialPort; //
    QTimer TimeoutTimer; //
    char ProgramState; //
    QByteArray TempDataBuffer; //
    QNetworkAccessManager *NetworkManager; //
    QSslCertificate *SSLCertificate; //
    QString DeviceID; //Holds the ID of the device

    //TEMP
    ReadingStruct *TempReadings; //
    int TempReadingCount; //
};

#endif // MAINWINDOW_H
