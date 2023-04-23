#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
//    QFont font("Tahoma");
    QFont font;
    font.setPixelSize(11);

    QApplication a(argc, argv);
    a.setFont(font);
    MainWindow w;
    w.show();
    return a.exec();
}
