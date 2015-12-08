#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QDebug>

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

private:
    Ui::MainWindow *ui;
    void RefreshSerial();
    QSerialPort MainSerialPort; //
    QTimer TimeoutTimer; //
};

#endif // MAINWINDOW_H
