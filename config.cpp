#include "config.h"
#include <iostream>
#include <QtDebug>
#include <QtGlobal>

/*
 * Stellarium
 * Copyright (C) 2008 Matthew Gates
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

//#include "StelIniParser.hpp"

#include <QSettings>
#include <QString>
#include <QVariant>
#include <QStringList>
#include <QIODevice>
#include <QRegExp>
#include <QFile>

bool readConfFile(QIODevice &device, QSettings::SettingsMap &map)
{
        // Is this the right conversion?
        const QString& data = QString::fromUtf8(device.readAll().data());

        // Split by a RE which should match any platform's line breaking rules
        QRegExp matchLbr("[\\n\\r]+");
        const QStringList& lines = data.split(matchLbr, QString::SkipEmptyParts);

        //QString currentSection = "";
        //QRegExp sectionRe("^\\[(.+)\\]$");
        QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");
        QRegExp cleanComment("#.*$");
        QRegExp cleanWhiteSpaces("^\\s+");
        QRegExp reg1("\\s+$");

        for(int i=0; i<lines.size(); i++)
        {
                QString l = lines.at(i);
                l.replace(cleanComment, "");            // clean comments
                l.replace(cleanWhiteSpaces, "");        // clean whitespace
                l.replace(reg1, "");

                // Otherwise only process if it macthes an re which looks like: key = value
                if (keyRe.exactMatch(l))
                {
                        // Let REs do the work for us exatracting the key and value
                        // and cleaning them up by removing whitespace
                        QString k = keyRe.cap(1);
                        QString v = keyRe.cap(2);
                        v.replace(cleanWhiteSpaces, "");
                        k.replace(reg1, "");

                        // Set the map item.
                        map[k] = QVariant(v);
                }
        }

        return true;
}

bool writeConfFile(QIODevice &device, const QSettings::SettingsMap &map)
{
        int maxKeyWidth = 20;
        QRegExp reKeyXt("^(.+)/(.+)$");  // for extracting keys/values

        // first go over map and find longest key length
        for(int i=0; i<map.keys().size(); i++)
        {
                QString k = map.keys().at(i);
                QString key = k;
                if (reKeyXt.exactMatch(k))
                        key = reKeyXt.cap(2);
                if (key.size() > maxKeyWidth) maxKeyWidth = key.size();
        }

        // OK, this time actually write to the file - first non-section values
        QString outputLine;
        for(int i=0; i<map.keys().size(); i++)
        {
                QString k = map.keys().at(i);
                if (!reKeyXt.exactMatch(k))
                {
                        // this is for those keys without a section
                        outputLine = QString("%1=%2\n").arg(k,map[k].toString());
                        device.write(outputLine.toUtf8());
                }
        }

        return true;
}
static const QSettings::Format FusInvConfFormat = QSettings::registerFormat("conf", readConfFile, writeConfFile);


Config::Config(const QString & cfgPath)
{
#ifdef Q_OS_WIN32
    settings = new QSettings( "HKEY_LOCAL_MACHINE\\Software\\FusionInventory-Agent", QSettings::NativeFormat );
#else
    settings = new QSettings( cfgPath, FusInvConfFormat );
    readWrite = QFile::permissions(cfgPath).testFlag(QFile::WriteUser);
#endif

}

QString Config::get(const QString & key) {
    return settings->value(key).toString();
}

bool Config::isReadOnly() {
    if (readWrite) {
        return false;
    } else {
        return true;
    }
}

bool Config::set(const QString & key, const QString & server) {
    settings->setValue(key, server);

    return true;
}

bool Config::save() {
    settings->sync();

    return true;
}
