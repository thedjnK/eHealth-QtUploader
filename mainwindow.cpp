#include "mainwindow.h"
#include "ui_mainwindow.h"

QNetworkAccessManager *gnmManager;

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

    //
    connect(&MainSerialPort, SIGNAL(readyRead()), this, SLOT(SerialDataWaiting()));


    //Setup QNetwork for Online XCompiler
    gnmManager = new QNetworkAccessManager();
    connect(gnmManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(gnmManager, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
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
            ProgramState = 1;
            TempDataBuffer.clear();
            TimeoutTimer.start();
            MainSerialPort.write("ID\r\n");
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
    MainSerialPort.close();
    ui->combo_Ports->setEnabled(true);
    ui->btn_Connect->setEnabled(true);
}

void MainWindow::SerialDataWaiting()
{
    //
    if (TimeoutTimer.isActive())
    {
        //
        TimeoutTimer.stop();
    }

    //
    QByteArray SerialData = MainSerialPort.readAll();

    //Newline found
    if (ProgramState == 1)
    {
        //Device ID
        TempDataBuffer.append(SerialData);
        if (TempDataBuffer.contains("\r\n") == true)
        {
            //
            ui->edit_Log->appendPlainText(QString("Device: ").append(TempDataBuffer.left(TempDataBuffer.size()-2)));
            ProgramState = 2;
            TempDataBuffer.clear();
            TimeoutTimer.start();
            MainSerialPort.write("Read\r\n");
        }
    }
    else if (ProgramState == 2)
    {
        //Data reading
        TempDataBuffer.append(SerialData);
        bool Finished = false;
        while (TempDataBuffer.contains("\r\n") == true)
        {
            //
            int SegEnd = TempDataBuffer.indexOf("\r\n");

            //
            ui->edit_Log->appendPlainText(QString("Segment: ").append(TempDataBuffer.left(SegEnd)));

            if (TempDataBuffer.left(SegEnd) == "EOF")
            {
                //
                ui->edit_Log->appendPlainText("Finished.");
                Finished = true;
                TempDataBuffer.clear();
            }
            else
            {
                //
                TempDataBuffer.remove(0, SegEnd+2);
            }
        }

        if (Finished == false)
        {
            TimeoutTimer.start();
        }
    }
}

void MainWindow::on_btn_Login_clicked()
{
    //Get token from cloud server
    gnmManager->get(QNetworkRequest(QUrl(QString("https://").append(Hostname).append(":444/token.php?UN=").append(QUrl::toPercentEncoding(ui->edit_Username->text())).append("&PW=").append(QUrl::toPercentEncoding(ui->edit_Password->text())))));
    ProgramState = 5;
}

void MainWindow::replyFinished(QNetworkReply* nrReply)
{
    //
    if (nrReply->error() != QNetworkReply::NoError)
    {
        //Error
        qDebug() << "Err:" << nrReply->errorString();
    }
    else
    {
        //OK
        qDebug() << "OK:";
        if (ProgramState == 5)
        {
            QByteArray RecData = nrReply->readAll();
            qDebug() << "Token response:";
            if (RecData.left(2) == "#0")
            {
                //Failed
                qDebug() << "Bad";
            }
            else if (RecData.left(3) == "#1:")
            {
                //Success
                ui->edit_Token->setText(RecData.right(RecData.size()-3));
                qDebug() << "Good";
            }
            else
            {
                //Unknown
                qDebug() << "??";
            }
        }
    }
    nrReply->deleteLater();
}

void MainWindow::sslErrors(QNetworkReply* nrReply, QList<QSslError> lstSSLErrors)
{
    //
    //if (sslcLairdSSL != NULL && nrReply->sslConfiguration().peerCertificate() == *sslcLairdSSL)
    //{
        //Server certificate matches
    qDebug() << "cert error";
        nrReply->ignoreSslErrors(lstSSLErrors);
    //}
}
