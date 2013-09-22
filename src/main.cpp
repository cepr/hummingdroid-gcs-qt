#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationDomain("hummingdroid.org");
    QCoreApplication::setOrganizationName("hummingdroid");
    QCoreApplication::setApplicationName("hummingdroid-gcs-qt");
    QApplication a(argc, argv);
    a.setApplicationDisplayName("Humming Droid");
    MainWindow w;
    w.show();
    return a.exec();
}
