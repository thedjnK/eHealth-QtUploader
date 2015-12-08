#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>

#define Hostname "192.168.1.94" //Internal
//#define Hostname "ehealth.noip.me" //External

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

private slots:
    void on_btn_Connect_clicked();
    void TimeoutTimerTrigger();
    void SerialDataWaiting();
    void on_btn_Login_clicked();
    void sslErrors(QNetworkReply* nrReply, QList<QSslError> lstSSLErrors);
    void replyFinished(QNetworkReply* nrReply);

private:
    Ui::MainWindow *ui;
    void RefreshSerial();
    QSerialPort MainSerialPort; //
    QTimer TimeoutTimer; //
    int ProgramState; //
    QByteArray TempDataBuffer; //
};

#endif // MAINWINDOW_H
