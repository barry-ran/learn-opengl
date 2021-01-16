#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
