#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    //
    ui->setupUi(this);

    //
    this->RefreshSerial();

    //
    TimeoutTimer.setSingleShot(true);
    TimeoutTimer.setInterval(1500);
    connect(&TimeoutTimer, SIGNAL(timeout()), this, SLOT(TimeoutTimerTrigger()));
}

MainWindow::~MainWindow()
{
    //
    disconnect(&TimeoutTimer, SIGNAL(timeout()));

    //
    delete ui;
}

void MainWindow::RefreshSerial()
{
    //
    ui->combo_Ports->clear();

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->combo_Ports->addItem(info.portName());
    }
}

void MainWindow::on_btn_Connect_clicked()
{
    //Connect button clicked
    ui->combo_Ports->setEnabled(false);
    ui->btn_Connect->setEnabled(false);

    if (ui->combo_Ports->currentText().length() > 0)
    {
        //
        MainSerialPort.setPortName(ui->combo_Ports->currentText());
        MainSerialPort.setBaudRate(QSerialPort::Baud115200);
        MainSerialPort.setDataBits(QSerialPort::Data8);
        MainSerialPort.setStopBits(QSerialPort::OneStop);
        MainSerialPort.setParity(QSerialPort::NoParity);
        MainSerialPort.setFlowControl(QSerialPort::NoFlowControl); //QSerialPort::HardwareControl

        if (MainSerialPort.open(QIODevice::ReadWrite))
        {
            //Successful

            //DTR
            MainSerialPort.setDataTerminalReady(true);

                //Not hardware handshaking - RTS
                //MainSerialPort.setRequestToSend(ui->check_RTS->isChecked());

            //Send wakeup message and start timeout timer
            TimeoutTimer.start();
        }
        else
        {
            //Error whilst opening
        }
    }
    else
    {
        //No serial port selected
    }
}

void MainWindow::TimeoutTimerTrigger()
{
    //
    qDebug() << "Timeout!";
}
