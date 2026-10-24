#pragma once
#include <QWidget>
#include <QDomDocument>
class QListWidget;
class QStackedWidget;
class QPlainTextEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;

// A visual, ordered command editor. The DOM remains an implementation detail.
class KeyBehaviorEditor : public QWidget {
    Q_OBJECT
public:
    explicit KeyBehaviorEditor(QWidget *parent = nullptr);
    void load(const QDomElement &key, const QString &keyboardPath);
    void writeTo(QDomDocument &document, QDomElement key) const;
    QString validationError() const;
    void addStep(const QString &tag, const QString &value);
    void moveStep(int offset);
    static bool isCommand(const QString &tag);
signals:
    void edited();
private:
    QDomDocument storage;
    QList<QDomElement> steps;
    QListWidget *list;
    QStackedWidget *fields;
    QPlainTextEdit *text;
    QComboBox *keyChoice;
    QComboBox *actionChoice;
    QComboBox *destination;
    QSpinBox *delay;
    QCheckBox *returnHere;
    QLabel *description;
    bool loading = false;
    bool modified = false;
    void refresh(int selection);
    void inspect();
    void changeValue(const QString &value);
    void changed();
    void addShortcut();
    QString summary(const QDomElement &step) const;
};
