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

//TEST
    TmpTest = new DeviceSettings;
    connect(TmpTest, SIGNAL(UpdateConfig()), this, SLOT(SaveDeviceConfig()));
    connect(TmpTest, SIGNAL(LoadConfig()), this, SLOT(LoadDeviceConfig()));

    //
    UserReadingsPos = 0;
    while (UserReadingsPos < ReadingsBuffer)
    {
        UserReadings[UserReadingsPos] = new UserReadingsStruct;
        UserReadings[UserReadingsPos]->Reading = 0;
        UserReadings[UserReadingsPos]->ReadingTaken = 0;
        UserReadings[UserReadingsPos]->ReadingType = 0;
        ++UserReadingsPos;
    }
    UserReadingsPos = 0;

    //
    UserReadingsTimer.setSingleShot(true);
    UserReadingsTimer.setInterval(1000);
    connect(&UserReadingsTimer, SIGNAL(timeout()), this, SLOT(SendReadCommand()));
}

MainWindow::~MainWindow()
{
    //
    disconnect(&TimeoutTimer, SIGNAL(timeout()));
    disconnect(NetworkManager, SIGNAL(finished(QNetworkReply*)));
    disconnect(NetworkManager, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)));
    disconnect(TmpTest, SIGNAL(UpdateConfig()));
    disconnect(TmpTest, SIGNAL(LoadConfig()));

    UserReadingsPos = 0;
    while (UserReadingsPos < ReadingsBuffer)
    {
        delete UserReadings[UserReadingsPos];
        ++UserReadingsPos;
    }

    //
    delete TmpTest;

    //
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //Runs when window is closed (exit application)
    TmpTest->close();

    //Accept close event
    event->accept();
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

void MainWindow::TimeoutTimerTrigger()
{
    //
    qDebug() << "Timeout!";
    MainSerialPort.close();
    ui->combo_Ports->setEnabled(true);
    ui->btn_Close->setEnabled(false);
    ui->btn_Open->setEnabled(true);
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
    qDebug() << "Read: " << SerialData;

    //Newline found
    if (ProgramState == State_DevID)
    {
        //Device ID
        TempDataBuffer.append(SerialData);
        if (TempDataBuffer.contains("\r\n") == true)
        {
            //
            this->DeviceID = TempDataBuffer.left(TempDataBuffer.size()-2);
            ui->edit_Log->appendPlainText(QString("Device: ").append(this->DeviceID));
            TmpTest->UpdateInputs(this->DeviceID);
            //ProgramState = State_DevReadings;
            ProgramState = State_Idle;
            TempDataBuffer.clear();
            ui->btn_Close->setEnabled(true);
            /*
            TimeoutTimer.start();
            MainSerialPort.write("Read\r\n");

            TempReadings = new ReadingStruct[10];
            TempReadingCount = 0;*/
        }
    }
    else if (ProgramState == State_ReadBP)
    {
        //
        QRegularExpression abc("^([0-9]+)\\|([0-9]+)\\|([0-9]+),([0-9]+),([0-9]+)\\|([0-9]+)$");
        TempDataBuffer.append(SerialData);
        bool Finished = false;
        while (TempDataBuffer.contains("\r\n") == true)
        {
            //
            int SegEnd = TempDataBuffer.indexOf("\r\n");

            //
            ui->edit_Log->appendPlainText(QString("Segment: ").append(TempDataBuffer.left(SegEnd)));
            bool DoUpload = false;

            if (TempDataBuffer.left(SegEnd) == "EOF")
            {
                //
                ui->edit_Log->appendPlainText("Finished.");
                qDebug() << "Finished @ " << UserReadingsPos;
                Finished = true;
                TempDataBuffer.clear();
                ProgramState = State_Idle;
                DoUpload = true;
            }
            else
            {
                //
                QRegularExpressionMatch qqq = abc.match(TempDataBuffer.left(SegEnd));
                if (qqq.hasMatch() == true)
                {
                    qDebug() << "yee @ " << UserReadingsPos;

                    UserReadings[UserReadingsPos]->Reading = qqq.captured(3).toFloat();
                    UserReadings[UserReadingsPos]->Reading2 = qqq.captured(4).toFloat();
                    UserReadings[UserReadingsPos]->Reading3 = qqq.captured(5).toFloat();
                    UserReadings[UserReadingsPos]->ReadingTaken = qqq.captured(2).toInt();
                    UserReadings[UserReadingsPos]->ReadingType = qqq.captured(6).toInt();
                    ++UserReadingsPos;
                }

                //
                TempDataBuffer.remove(0, SegEnd+2);
            }

            if (UserReadingsPos > 30 || DoUpload == true)
            {
                int i = 0;
                QByteArray baPostData;
                while (i < UserReadingsPos)
                {
                    if (UserReadings[i]->ReadingType == 4)
                    {
                        baPostData.append(QString("T").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingType)).append("&R").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->Reading)).append("&R").append(QString::number(i)).append("b=").append(QString::number(UserReadings[i]->Reading2)).append("&R").append(QString::number(i)).append("c=").append(QString::number(UserReadings[i]->Reading3)).append("&A").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingTaken)).append("&"));
                    }
                    else
                    {
                        baPostData.append(QString("T").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingType)).append("&R").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->Reading)).append("&A").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingTaken)).append("&"));
                    }
                    ++i;
                }

            //
                qDebug() << "Token: " << this->DeviceID;
                QNetworkRequest nrThisReq(QUrl(QString("https://").append(Hostname).append(":444/upload.php?TK=").append(QUrl::toPercentEncoding(this->DeviceID)).append("&RD=").append(QString::number(UserReadingsPos))));
                nrThisReq.setRawHeader("Content-Type", QString("application/x-www-form-urlencoded").toUtf8());
                nrThisReq.setRawHeader("Content-Length", QString(baPostData.length()).toUtf8());
                NetworkManager->post(nrThisReq, baPostData);
                ui->label_Loading->setText("LOADING...");
                qDebug() << baPostData;

                UserReadingsPos = 0;
            }
        }

        if (Finished == false)
        {
            TimeoutTimer.start();
        }
    }
    else if (ProgramState == State_ReadSensors)
    {
        //
        QRegularExpression abc("^([0-9]+)\\|([0-9]+)\\|([0-9\\.]+)\\|([0-9]+)$");
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
                qDebug() << "Finished @ " << UserReadingsPos;
                Finished = true;
                TempDataBuffer.clear();
                UserReadingsTimer.start();
            }
            else
            {
                //
                QRegularExpressionMatch qqq = abc.match(TempDataBuffer.left(SegEnd));
                if (qqq.hasMatch() == true)
                {
                    qDebug() << "yee @ " << UserReadingsPos;

                    UserReadings[UserReadingsPos]->Reading = qqq.captured(3).toFloat();
                    UserReadings[UserReadingsPos]->ReadingTaken = (int)(QDateTime::currentMSecsSinceEpoch()/1000);
                    UserReadings[UserReadingsPos]->ReadingType = qqq.captured(4).toInt();
                    ++UserReadingsPos;
                }

                //
                TempDataBuffer.remove(0, SegEnd+2);
            }

            if (UserReadingsPos > 30)
            {
                int i = 0;
                QByteArray baPostData;
                while (i < UserReadingsPos)
                {
                    baPostData.append(QString("T").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingType)).append("&R").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->Reading)).append("&A").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingTaken)).append("&"));
                    ++i;
                }

            //
            //ProgramState = State_WebUpload;
                qDebug() << "Token: " << this->DeviceID;
                QNetworkRequest nrThisReq(QUrl(QString("https://").append(Hostname).append(":444/upload.php?TK=").append(QUrl::toPercentEncoding(this->DeviceID)).append("&RD=").append(QString::number(UserReadingsPos))));
                nrThisReq.setRawHeader("Content-Type", QString("application/x-www-form-urlencoded").toUtf8());
                nrThisReq.setRawHeader("Content-Length", QString(baPostData.length()).toUtf8());
                NetworkManager->post(nrThisReq, baPostData);
                ui->label_Loading->setText("LOADING...");
                qDebug() << baPostData;

                UserReadingsPos = 0;
            }
        }

        if (Finished == false)
        {
            TimeoutTimer.start();
        }
    }
}

void MainWindow::replyFinished(QNetworkReply* nrReply)
{
    //
    ui->label_Loading->setText("");
    if (nrReply->error() != QNetworkReply::NoError)
    {
        //Error
        qDebug() << "Err:" << nrReply->errorString();
    }
    else
    {
        //OK
        qDebug() << "OK:";
        //if (ProgramState == State_WebUpload)
        //{
            qDebug() << "Upload response: " << nrReply->readAll();
        //}
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

void MainWindow::on_btn_DevConfig_clicked()
{
    //Show device settings dialogue
    TmpTest->show();
}

void MainWindow::SaveDeviceConfig()
{
    //Save configuration to device
}

void MainWindow::LoadDeviceConfig()
{
    //Load configuration from device
    if (ProgramState == State_Idle)
    {
        //
        ProgramState = State_DevID;
        TempDataBuffer.clear();
        TimeoutTimer.start();
        MainSerialPort.write("ID\r\n");
    }
}

void MainWindow::on_btn_Open_clicked()
{
    //Open serial port
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
            //Successful, set DTR
            //MainSerialPort.setDataTerminalReady(true);

            //Change enabled status of controls
            ui->combo_Ports->setEnabled(false);
            ui->btn_Open->setEnabled(false);

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
}

void MainWindow::on_btn_Close_clicked()
{
    //Close serial port
    if (MainSerialPort.isOpen())
    {
        //Serial port is opened, close it
        MainSerialPort.close();
    }
    ui->btn_Close->setEnabled(false);
    ui->btn_Open->setEnabled(true);
    ui->combo_Ports->setEnabled(true);
}

void MainWindow::on_btn_ReadBP_clicked()
{
    //
    ProgramState = State_ReadBP;
    TempDataBuffer.clear();
    //TimeoutTimer.start();
    MainSerialPort.write("Rd#2\r\n");
//State_UploadBP
//State_UploadSensors
}

void MainWindow::on_check_ReadSensors_stateChanged(int arg1)
{
    //
    if (ui->check_ReadSensors->isChecked() == true)
    {
        //
        ProgramState = State_ReadSensors;
        TempDataBuffer.clear();
        //TimeoutTimer.start();
//        MainSerialPort.write("EnA\r\n");
//        MainSerialPort.write("EnB\r\n");
//        MainSerialPort.write("EnC\r\n");
//        MainSerialPort.write("EnD\r\n");
//        MainSerialPort.write("EnE\r\n");
//        MainSerialPort.write("EnF\r\n");
        MainSerialPort.write("Rd#1\r\n");
        qDebug() << "Wrote all.";
    }
    else
    {
        //
//TODO: Upload readings read but not uploaded
        ProgramState = State_Idle;
        if (UserReadingsTimer.isActive())
        {
            UserReadingsTimer.stop();
        }

            int i = 0;
            QByteArray baPostData;
            while (i < UserReadingsPos)
            {
                baPostData.append(QString("T").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingType)).append("&R").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->Reading)).append("&A").append(QString::number(i)).append("=").append(QString::number(UserReadings[i]->ReadingTaken)).append("&"));
                ++i;
            }

        //
        //ProgramState = State_WebUpload;
            qDebug() << "Token: " << this->DeviceID;
            QNetworkRequest nrThisReq(QUrl(QString("https://").append(Hostname).append(":444/upload.php?TK=").append(QUrl::toPercentEncoding(this->DeviceID)).append("&RD=").append(QString::number(UserReadingsPos))));
            nrThisReq.setRawHeader("Content-Type", QString("application/x-www-form-urlencoded").toUtf8());
            nrThisReq.setRawHeader("Content-Length", QString(baPostData.length()).toUtf8());
            NetworkManager->post(nrThisReq, baPostData);
            ui->label_Loading->setText("LOADING...");
            qDebug() << baPostData;

            UserReadingsPos = 0;
    }
}

void MainWindow::on_btn_En1_clicked()
{
    MainSerialPort.write("EnA\r\n");
}

void MainWindow::on_btn_En2_clicked()
{
    MainSerialPort.write("EnB\r\n");
}

void MainWindow::on_btn_En3_clicked()
{
    MainSerialPort.write("EnC\r\n");
}

void MainWindow::on_btn_En4_clicked()
{
    MainSerialPort.write("EnD\r\n");
}

void MainWindow::on_btn_En5_clicked()
{
    MainSerialPort.write("EnE\r\n");
}

void MainWindow::SendReadCommand()
{
    TempDataBuffer.clear();
    MainSerialPort.write("Rd#1\r\n");
}
