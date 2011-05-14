#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QSettings>

class Config : private QObject
{
public:
    Config(const QString & cfgPath);
    QString get(const QString & key);
    bool set(const QString & key, const QString & server);
    bool save();
    bool isReadOnly();
    QSettings * settings;
    bool readWrite;
};

#endif // CONFIG_H
