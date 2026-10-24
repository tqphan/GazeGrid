#pragma once
#include <QMainWindow>
#include <QDomDocument>
#include <QVariantMap>
class QGraphicsScene;
class QGraphicsView;
class QLineEdit;
class KeyBehaviorEditor;
class QComboBox;
class QLabel;
class QTabWidget;
class QDoubleSpinBox;
class KeyboardEditor : public QMainWindow {
public:
    explicit KeyboardEditor(QWidget *parent = nullptr);
    ~KeyboardEditor() override;
    bool openFile(const QString &path);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    friend class KeyboardEditorTests;
    bool formDirty = false;
    QDomDocument document;
    QVariantMap layout;
    QString path;
    QString saved;
    QStringList history;
    int historyIndex = -1;
    int selected = -1;
    QGraphicsScene *scene;
    QGraphicsView *view;
    QLineEdit *name;
    QLineEdit *label;
    KeyBehaviorEditor *behavior;
    QComboBox *mode;
    QComboBox *symbol;
    QLineEdit *shiftLabel;
    QLabel *selectionTitle;
    QTabWidget *tabs;
    bool inspecting = false;
    void markDirty();
    QDoubleSpinBox *geometry[4];
    QList<QDomElement> items() const;
    bool validate(const QDomDocument &doc, QVariantMap *result = nullptr);
    bool commit(const QDomDocument &next);
    void render();
    void inspect();
    bool apply();
    void moveKey(int index, QPointF position);
    bool save(bool choosePath);
    bool discard();
    void newDocument();
    void restore(int offset);
};
