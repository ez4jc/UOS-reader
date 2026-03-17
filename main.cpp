#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("阅读器");
    app.setApplicationVersion("1.0.0");

    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::blue);
    QPainter painter(&pixmap);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 32, QFont::Bold));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "R");
    app.setWindowIcon(QIcon(pixmap));

    MainWindow w;
    w.show();

    return app.exec();
}
