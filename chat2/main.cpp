#include <QtGui/QApplication>
#include "widget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    QTextCodec::setCodecForTr(QTextCodec::codecForLocale());
    w.show();
    return a.exec();
}
