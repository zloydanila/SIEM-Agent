#ifndef RULE_H
#define RULE_H

#include <QString>

struct Rule{
    QString id;
    QString name;
    QString ruleType;
    QString matchEventType;
    QString secondaryEventType;
    int threshold = 1;
    int windowSeconds = 60;
    int cooldownSeconds = 60;
    QString alertSeverity = "high";
    QString alertTitle;
    QString alertDescription;
    bool isEnabled = true;
};

#endif