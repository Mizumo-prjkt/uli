#include "lib/ui/Main_Menu/Main_Menu.hpp"
#include "uli_version.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // Check for version flag before QApplication to avoid headless crash
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--version" || arg == "-v") {
      std::cout << "ULI Compositor version " << uli::PROJECT_VERSION << " ("
                << uli::BUILD_TYPE << ")" << std::endl;
      return 0;
    }
  }

  QApplication app(argc, argv);

  app.setApplicationName("ULI Compositor");
  app.setOrganizationName("Antigravity");
  app.setApplicationVersion(QString::fromStdString(uli::PROJECT_VERSION));

  QCommandLineParser parser;
  parser.setApplicationDescription("ULI Profile Compositor");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.process(app);

  qDebug() << "Starting ULI Compositor version" << uli::PROJECT_VERSION.c_str()
           << "(" << uli::BUILD_TYPE.c_str() << ")";

  uli::compositor::ui::Main_Menu window;
  window.show();

  return app.exec();
}
