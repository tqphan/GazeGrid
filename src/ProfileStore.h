#pragma once
#include <QJsonObject>
#include <QString>

// Versioned preferences only: imported keyboards and learned words are separate.
class ProfileStore {
public:
    struct Loaded { QJsonObject values; QString message; };
    explicit ProfileStore(QString path);
    static QJsonObject defaults();
    static bool valid(const QJsonObject &values);
    Loaded load() const;
    bool save(const QJsonObject &values, QString &error, bool reset = false) const;
private:
    QString path;
};
