#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QString>

class Application : public QObject {
    Q_OBJECT

public:
    static Application* instance();

    QString appName()    const { return "SIEM Agent-007"; }
    QString appVersion() const { return "1.0.0"; }

private:
    explicit Application();
};

#endif