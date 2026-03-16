#include <QApplication>
#include <QFile>

#include "MainWindow.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QFile styleFile(":/styles.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    MainWindow window;
    window.show();
    return app.exec();
}

