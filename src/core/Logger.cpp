#include "Logger.h"
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QMutexLocker>
#include <iostream>

Logger &Logger::instance() {
    static Logger inst;
    return inst;
}

bool Logger::init(const QString &logDir) {
    QDir().mkpath(logDir);

    QString filename = logDir + "/siem_"
        + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")
        + ".log";

    m_file.setFileName(filename);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        std::cerr << "Logger: не удалось открыть файл " << filename.toStdString() << "\n";
        return false;
    }

    m_stream.setDevice(&m_file);
    qInstallMessageHandler(Logger::messageHandler);
    return true;
}

void Logger::close() {
    QMutexLocker lock(&m_mutex);
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

void Logger::messageHandler(QtMsgType type,
                             const QMessageLogContext &,
                             const QString &msg)
{
    Logger &log = Logger::instance();
    QMutexLocker lock(&log.m_mutex);

    QString level;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG";    break;
        case QtInfoMsg:     level = "INFO";     break;
        case QtWarningMsg:  level = "WARNING";  break;
        case QtCriticalMsg: level = "CRITICAL"; break;
        case QtFatalMsg:    level = "FATAL";    break;
    }

    QString line = QString("[%1] [%2] %3")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
        .arg(level, -8)
        .arg(msg);

    if (log.m_file.isOpen()) {
        log.m_stream << line << "\n";
        log.m_stream.flush();
    }

    std::cout << line.toStdString() << "\n";

    if (type == QtFatalMsg)
        abort();
}