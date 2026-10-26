#include "KeyNames.h"
#include <QMap>
QString KeyNames::canonical(QString name) {
    name=name.trimmed().toUpper();
    if(name.size()==4&&name.startsWith("VK_")&&name[3]>='0'&&name[3]<='9')name=name.mid(3);
    static const QMap<QString,QString> aliases{{"LCTRL","LCONTROL"},{"RCTRL","RCONTROL"},{"LEFTCTRL","LCONTROL"},{"RIGHTCTRL","RCONTROL"},{"LEFTSHIFT","LSHIFT"},{"RIGHTSHIFT","RSHIFT"},{"LEFTALT","LMENU"},{"RIGHTALT","RMENU"},{"LEFTWIN","LWIN"},{"RIGHTWIN","RWIN"},{"ENTER","RETURN"},{"ESC","ESCAPE"},{"BACKSPACE","BACK"},{"BACKONE","BACK"},{"PGUP","PRIOR"},{"PGDN","NEXT"},{"PRINTSCREEN","SYSRQ"}};
    return aliases.value(name,name);
}
static const QMap<QString,int> &keyCodes() {
    static const QMap<QString,int> table{{"ESCAPE",1},{"1",2},{"2",3},{"3",4},{"4",5},{"5",6},{"6",7},{"7",8},{"8",9},{"9",10},{"0",11},{"MINUS",12},{"EQUAL",13},{"BACK",14},{"TAB",15},{"Q",16},{"W",17},{"E",18},{"R",19},{"T",20},{"Y",21},{"U",22},{"I",23},{"O",24},{"P",25},{"LEFTBRACE",26},{"RIGHTBRACE",27},{"RETURN",28},{"LCONTROL",29},{"A",30},{"S",31},{"D",32},{"F",33},{"G",34},{"H",35},{"J",36},{"K",37},{"L",38},{"SEMICOLON",39},{"APOSTROPHE",40},{"GRAVE",41},{"LSHIFT",42},{"BACKSLASH",43},{"Z",44},{"X",45},{"C",46},{"V",47},{"B",48},{"N",49},{"M",50},{"COMMA",51},{"DOT",52},{"SLASH",53},{"RSHIFT",54},{"LMENU",56},{"SPACE",57},{"CAPITAL",58},{"F1",59},{"F2",60},{"F3",61},{"F4",62},{"F5",63},{"F6",64},{"F7",65},{"F8",66},{"F9",67},{"F10",68},{"F11",87},{"F12",88},{"F13",183},{"F14",184},{"F15",185},{"F16",186},{"F17",187},{"F18",188},{"F19",189},{"F20",190},{"F21",191},{"F22",192},{"F23",193},{"F24",194},{"SYSRQ",99},{"RCONTROL",97},{"RMENU",100},{"HOME",102},{"UP",103},{"PRIOR",104},{"LEFT",105},{"RIGHT",106},{"END",107},{"DOWN",108},{"NEXT",109},{"INSERT",110},{"DELETE",111},{"PAUSE",119},{"LWIN",125},{"RWIN",126}};
    return table;
}
int KeyNames::linuxCode(const QString &name) {return keyCodes().value(canonical(name),-1);}
QString KeyNames::fromLinuxCode(int code) {
    return keyCodes().key(code);
}
QPair<QString,bool> KeyNames::usCharacter(QChar c) {
    if((c>='a'&&c<='z')||(c>='A'&&c<='Z'))return {QString(c.toUpper()),c.isUpper()};
    if(c>='0'&&c<='9')return {QString(c),false};
    const QString plain="-=[];'`\\,./", shifted="_+{}:\"~|<>?";
    const QStringList names{"MINUS","EQUAL","LEFTBRACE","RIGHTBRACE","SEMICOLON","APOSTROPHE","GRAVE","BACKSLASH","COMMA","DOT","SLASH"};
    int i=plain.indexOf(c);if(i>=0)return {names[i],false};i=shifted.indexOf(c);if(i>=0)return {names[i],true};
    i=QString("!@#$%^&*()").indexOf(c);if(i>=0)return {QString::number((i+1)%10),true};
    if(c==' ')return {"SPACE",false};if(c=='\t')return {"TAB",false};if(c=='\r'||c=='\n')return {"RETURN",false};
    return {};
}
QChar KeyNames::usTypedCharacter(QChar c, bool shifted) {
    if(!shifted)return c;
    if(c.isLetter())return c.toUpper();
    const QString plain="1234567890-=[];'`\\,./";
    const QString upper="!@#$%^&*()_+{}:\"~|<>?";
    const int i=plain.indexOf(c);
    return i>=0?upper[i]:c;
}
QString KeyNames::functionKey(const QString &action){auto key=canonical(action);return linuxCode(key)>=0?key:QString();}
