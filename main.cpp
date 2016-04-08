#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    //Create application
    QApplication a(argc, argv);

    //Main window object
    MainWindow w;

    //Show main window
    w.show();

    //Executea application
    return a.exec();
}
