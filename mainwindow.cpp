#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    //
    ui->setupUi(this);

    //
    this->RefreshSerial();

    //
    this->statusBar()->showMessage(QString("UH eHealth Uploader ").append(Version));

    //
    TimeoutTimer.setSingleShot(true);
    TimeoutTimer.setInterval(1500);
    connect(&TimeoutTimer, SIGNAL(timeout()), this, SLOT(TimeoutTimerTrigger()));

    //
    connect(&MainSerialPort, SIGNAL(readyRead()), this, SLOT(SerialDataWaiting()));


    //Setup QNetwork for Online XCompiler
    NetworkManager = new QNetworkAccessManager();
    connect(NetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(NetworkManager, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));

    //
    QFile certFile(":/Server.pem");
    if (certFile.open(QIODevice::ReadOnly))
    {
        //Load certificate data
        SSLCertificate = new QSslCertificate(certFile.readAll());
        QSslSocket::addDefaultCaCertificate(*SSLCertificate);
        certFile.close();
    }
}

MainWindow::~MainWindow()
{
    //
    disconnect(&TimeoutTimer, SIGNAL(timeout()));
    disconnect(NetworkManager, SIGNAL(finished(QNetworkReply*)));
    disconnect(NetworkManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)));

    //
    delete ui;
}

void MainWindow::RefreshSerial()
{
    //
    ui->combo_Ports->clear();

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        //
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
            ProgramState = State_DevID;
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
    if (ProgramState == State_DevID)
    {
        //Device ID
        TempDataBuffer.append(SerialData);
        if (TempDataBuffer.contains("\r\n") == true)
        {
            //
            ui->edit_Log->appendPlainText(QString("Device: ").append(TempDataBuffer.left(TempDataBuffer.size()-2)));
            ProgramState = State_DevReadings;
            TempDataBuffer.clear();
            TimeoutTimer.start();
            MainSerialPort.write("Read\r\n");

            TempReadings = new ReadingStruct[10];
            TempReadingCount = 0;
        }
    }
    else if (ProgramState == State_DevReadings)
    {
        //Data reading
        QRegularExpression abc("([0-9])+\\|([0-9])+\\|([0-9\\.])+\\|([0-9])+");
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

                //
                int i = 0;
                QByteArray baPostData;
                while (i < TempReadingCount)
                {
                    //
                    ++i;
                    baPostData.append(QString("T").append(QString::number(i)).append("=").append(QString::number(TempReadings[i].Type)).append("&R").append(QString::number(i)).append("=").append(QString::number(TempReadings[i].Reading)).append("&A").append(QString::number(i)).append("=").append(QString::number(TempReadings[i].Timestamp)).append("&"));
                }

                //
                ProgramState = State_WebUpload;
                QNetworkRequest nrThisReq(QUrl(QString("https://").append(Hostname).append(":444/upload.php?TK=").append(QUrl::toPercentEncoding(ui->edit_Token->text())).append("&RD=").append(QString::number(TempReadingCount))));
                nrThisReq.setRawHeader("Content-Type", QString("application/x-www-form-urlencoded").toUtf8());
                nrThisReq.setRawHeader("Content-Length", QString(baPostData.length()).toUtf8());
                NetworkManager->post(nrThisReq, baPostData);
                qDebug() << baPostData;

                //
                delete[] TempReadings;
            }
            else
            {
                //
                QRegularExpressionMatch qqq = abc.match(TempDataBuffer.left(SegEnd));
                if (qqq.hasMatch() == true)
                {
                    qDebug() << "yee";
                    TempReadings[TempReadingCount].Reading = qqq.captured(3).toFloat();
                    TempReadings[TempReadingCount].Timestamp = qqq.captured(2).toInt();
                    TempReadings[TempReadingCount].Type = qqq.captured(4).toInt();
                    ++TempReadingCount;
                }

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
    ProgramState = State_WebLogin;
    NetworkManager->get(QNetworkRequest(QUrl(QString("https://").append(Hostname).append(":444/token.php?UN=").append(QUrl::toPercentEncoding(ui->edit_Username->text())).append("&PW=").append(QUrl::toPercentEncoding(ui->edit_Password->text())))));
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
        if (ProgramState == State_WebLogin)
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
        else if (ProgramState == State_WebUpload)
        {
            qDebug() << "Upload response: " << nrReply->readAll();
        }
    }
    nrReply->deleteLater();
}

void MainWindow::sslErrors(QNetworkReply* nrReply, QList<QSslError> lstSSLErrors)
{
    //
    if (SSLCertificate != NULL && nrReply->sslConfiguration().peerCertificate() == *SSLCertificate)
    {
        //Server certificate matches
        qDebug() << "cert error";
        nrReply->ignoreSslErrors(lstSSLErrors);
    }
}
