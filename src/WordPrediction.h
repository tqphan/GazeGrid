#pragma once
#include <QStringList>
#include <QMap>
namespace WordPrediction {
void prepare(const QStringList &dictionary);
QString prefix(const QString &text);
QString completedWord(const QString &text);
QStringList suggest(const QString &text,const QStringList &dictionary,const QMap<QString,int> &frequencies,int limit=8,bool useContext=true,bool suggestWhenEmpty=false);
}
