#include <QApplication>
#include <QDebug>
#include "lib/ui/Main_Menu/Main_Menu.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("ULI Compositor");
    app.setOrganizationName("Antigravity");
    
    qDebug() << "Starting ULI Compositor...";
    
    uli::compositor::ui::Main_Menu window;
    window.show();
    
    return app.exec();
}
