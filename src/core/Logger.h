#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QtMessageHandler>

class Logger {
public:
    static Logger &instance();

    bool init(const QString &logDir = "logs");
    void close();

    static void messageHandler(QtMsgType type,
                                const QMessageLogContext &ctx,
                                const QString &msg);
private:
    Logger() = default;
    ~Logger() { close(); }

    QFile        m_file;
    QTextStream  m_stream;
    QMutex       m_mutex;
};

#endif