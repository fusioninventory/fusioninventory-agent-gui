#ifndef CONFIG_H
#define CONFIG_H

#include <QSettings>

class Config
{
public:
    Config(char * cfgPath);
    QString get(const QString & key);
    bool set(const QString & key, const QString & server);
    bool save();
    QSettings * settings;

};

#endif // CONFIG_H
