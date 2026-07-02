#include "AppController.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("DesktopGit"));
    QCoreApplication::setOrganizationName(QStringLiteral("DesktopGit"));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    AppController appController;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &appController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("DesktopGit"), QStringLiteral("Main"));

    return app.exec();
}
