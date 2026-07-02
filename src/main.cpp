#include "AppController.h"

#include <QColor>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QPalette>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("DesktopGit"));
    QCoreApplication::setOrganizationName(QStringLiteral("DesktopGit"));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(QStringLiteral("#1f2023")));
    darkPalette.setColor(QPalette::WindowText, QColor(QStringLiteral("#e7e9ee")));
    darkPalette.setColor(QPalette::Base, QColor(QStringLiteral("#26282c")));
    darkPalette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#2b2d31")));
    darkPalette.setColor(QPalette::Text, QColor(QStringLiteral("#e7e9ee")));
    darkPalette.setColor(QPalette::Button, QColor(QStringLiteral("#2b2d31")));
    darkPalette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#e7e9ee")));
    darkPalette.setColor(QPalette::BrightText, QColor(QStringLiteral("#ffffff")));
    darkPalette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#9aa0aa")));
    darkPalette.setColor(QPalette::Highlight, QColor(QStringLiteral("#6f93dd")));
    darkPalette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
    darkPalette.setColor(QPalette::Link, QColor(QStringLiteral("#7aa2f7")));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#2b2d31")));
    darkPalette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#e7e9ee")));
    app.setPalette(darkPalette);

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
