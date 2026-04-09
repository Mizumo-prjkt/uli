#include <QApplication>
#include <QDebug>
#include <QCommandLineParser>
#include "uli_version.hpp"
#include "lib/ui/Main_Menu/Main_Menu.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("ULI Compositor");
    app.setOrganizationName("Antigravity");
    app.setApplicationVersion(QString::fromStdString(uli::PROJECT_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription("ULI Profile Compositor");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    qDebug() << "Starting ULI Compositor version" << uli::PROJECT_VERSION.c_str() << "(" << uli::BUILD_TYPE.c_str() << ")";
    
    uli::compositor::ui::Main_Menu window;
    window.show();
    
    return app.exec();
}
