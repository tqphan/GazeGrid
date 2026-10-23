#include "KeyboardLayout.h"
#include "ActionRunner.h"
#include "GameController.h"
#include <QSet>
#include <cmath>
namespace {
QString field(const QVariantMap &node,const QString &tag,QString fallback={}){for(auto v:node["children"].toList()){auto n=v.toMap();if(n["tag"]==tag)return n["text"].toString();}return fallback;}
}
QVariantMap KeyboardLayout::fromXml(const QVariantMap &root){
    if(root.contains("error"))return root;
    QVariantMap layout;QVariantList keys,groups;double rows=0,cols=0;bool canvas=false,grid=false;
    for(auto v:root["children"].toList()){auto n=v.toMap();auto tag=n["tag"].toString();
        if(tag=="Canvas"){canvas=true;rows=field(n,"Height").toDouble();cols=field(n,"Width").toDouble();}
        else if(tag=="Grid"){grid=true;rows=field(n,"Rows").toInt();cols=field(n,"Cols").toInt();}
        else if(tag=="KeyGroup")groups<<n["attributes"];
        else if(tag!="Content")layout[tag.toLower()]=n["text"].toString().trimmed();
    }
    if(canvas==grid||!std::isfinite(rows)||!std::isfinite(cols)||rows<=0||cols<=0||(!canvas&&(rows>64||cols>64)))return {{"error","Invalid keyboard Grid or Canvas"}};
    QSet<QString> occupied;
    for(auto v:root["children"].toList()){auto content=v.toMap();if(content["tag"]!="Content")continue;
        for(auto entry:content["children"].toList()){
            auto n=entry.toMap(),attr=n["attributes"].toMap();auto tag=n["tag"].toString();
            if(tag!="Joystick"&&tag!="DynamicKey"&&tag!="SuggestionRow"&&tag!="Scratchpad")return {{"error","Unsupported content item: "+tag}};
            if(tag=="Joystick"&&attr.value("stick","Left").toString()!="Left"&&attr.value("stick").toString()!="Right")return {{"error","Joystick stick must be Left or Right"}};
            bool rowOk=false,colOk=false;double row=attr.value("row").toString().toInt(&rowOk),col=attr.value("col").toString().toInt(&colOk);
            double width=attr.value("width",1).toInt(),height=attr.value("height",1).toInt();
            int suggestions=0;
            if(canvas){
                bool widthOk=false,heightOk=false;
                row=attr.value("y").toString().toDouble(&rowOk);
                col=attr.value("x").toString().toDouble(&colOk);
                width=attr.value("width").toString().toDouble(&widthOk);
                height=attr.value("height").toString().toDouble(&heightOk);
                if(attr.contains("row")||attr.contains("col")||!rowOk||!colOk||!widthOk||!heightOk||
                   !std::isfinite(row)||!std::isfinite(col)||!std::isfinite(width)||!std::isfinite(height)||
                   row<0||col<0||width<=0||height<=0||row+height>rows||col+width>cols)
                    return {{"error","Invalid key rectangle in Canvas"}};
                if(tag=="SuggestionRow"){
                    bool ok=false;suggestions=attr.value("count",4).toString().toInt(&ok);
                    if(!ok||suggestions<1||suggestions>64)return {{"error","Invalid suggestion Count"}};
                }
            }else{
                // Unpositioned items flow into the first free rectangle in row-major order.
                if(!attr.contains("row")&&!attr.contains("col")&&width>0&&height>0&&width<=cols&&height<=rows){
                    for(int y=0;y<=rows-height&&!rowOk;++y)for(int x=0;x<=cols-width&&!rowOk;++x){
                        bool free=true;
                        for(int dy=0;dy<height;++dy)for(int dx=0;dx<width;++dx)
                            if(occupied.contains(QString("%1:%2").arg(y+dy).arg(x+dx)))free=false;
                        if(free){row=y;col=x;rowOk=colOk=true;}
                    }
                }
                if(!rowOk||!colOk||row<0||col<0||width<1||height<1||row+height>rows||col+width>cols)return {{"error","Key outside keyboard grid"}};
                for(int y=row;y<row+height;++y)for(int x=col;x<col+width;++x){auto cell=QString("%1:%2").arg(y).arg(x);if(occupied.contains(cell))return {{"error","Overlapping keyboard cells"}};occupied.insert(cell);}
            }
            if(!canvas&&tag=="SuggestionRow")suggestions=int(width)/2;
            QVariantMap style{{"backgroundcolor","#f7f8fa"},{"foregroundcolor","#17212d"},{"foregroundopacity",1.0},{"opacity",1.0}};
            QSet<QString> memberships;for(auto c:n["children"].toList()){auto child=c.toMap();if(child["tag"]=="KeyGroup")memberships.insert(child["text"].toString().trimmed());}
            for(auto group:groups){auto g=group.toMap();if(g["name"].toString().compare("All",Qt::CaseInsensitive)==0||g["name"].toString().compare("Default",Qt::CaseInsensitive)==0||memberships.contains(g["name"].toString()))for(auto it=g.begin();it!=g.end();++it)if(it.key()!="name")style[it.key()]=it.value();}
            for(auto it=attr.begin();it!=attr.end();++it)style[it.key()]=it.value();
            QVariantList commands;
            static const QSet<QString> commandTags{"Action","Text","KeyDown","KeyUp","KeyToggle","Wait","ChangeKeyboard"};
            for(auto c:n["children"].toList()){auto child=c.toMap();if(commandTags.contains(child["tag"].toString()))commands<<child;}
            QString mode;
            if(attr.contains("mode")){
                const auto value=attr["mode"].toString().trimmed().toLower();
                const QMap<QString,QString> modes{{"tap","Tap"},{"repeat","Repeat"},{"hold","Hold"},{"toggle","Toggle"}};
                if(!modes.contains(value))return {{"error","Key mode must be Tap, Repeat, Hold or Toggle"}};
                mode=modes.value(value);
                const auto command=commands.size()==1?commands.front().toMap():QVariantMap();
                const bool button=command["tag"]=="Action"&&!GameController::buttonAction(command["text"].toString(),"Press").isEmpty();
                if(tag!="DynamicKey"||(!button&&ActionRunner::dwellKey(commands).first.isEmpty()))
                    return {{"error","Key mode requires a single non-modifier keyboard key or digital gamepad button"}};
            }
            int dwell=attr.value("completiontimes",0).toString().split(',').first().toInt();
            keys<<QVariantMap{{"mode",mode},{"suggestionCount",suggestions},{"row",row},{"col",col},{"width",width},{"height",height},{"kind",tag},{"label",field(n,"Label")},{"shiftLabel",field(n,"ShiftDownLabel")},{"symbol",field(n,"Symbol").trimmed()},{"style",style},{"commands",commands},{"dwell",dwell}};
        }
    }
    layout["layoutMode"]=canvas?"canvas":"grid";layout["rows"]=rows;layout["cols"]=cols;layout["keys"]=keys;return layout;
}
