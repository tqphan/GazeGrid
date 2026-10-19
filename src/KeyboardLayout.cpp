#include "KeyboardLayout.h"
#include <QSet>
namespace {
QString field(const QVariantMap &node,const QString &tag,QString fallback={}){for(auto v:node["children"].toList()){auto n=v.toMap();if(n["tag"]==tag)return n["text"].toString();}return fallback;}
}
QVariantMap KeyboardLayout::fromXml(const QVariantMap &root){
    if(root.contains("error"))return root;
    QVariantMap layout;QVariantList keys,groups;int rows=0,cols=0;
    for(auto v:root["children"].toList()){auto n=v.toMap();auto tag=n["tag"].toString();
        if(tag=="Grid"){rows=field(n,"Rows").toInt();cols=field(n,"Cols").toInt();}
        else if(tag=="KeyGroup")groups<<n["attributes"];
        else if(tag!="Content")layout[tag.toLower()]=n["text"].toString().trimmed();
    }
    if(rows<1||cols<1||rows>64||cols>64)return {{"error","Invalid keyboard grid"}};
    QSet<QString> occupied;
    for(auto v:root["children"].toList()){auto content=v.toMap();if(content["tag"]!="Content")continue;
        for(auto entry:content["children"].toList()){
            auto n=entry.toMap(),attr=n["attributes"].toMap();auto tag=n["tag"].toString();
            if(tag!="DynamicKey"&&tag!="SuggestionRow")return {{"error","Unsupported content item: "+tag}};
            bool rowOk=false,colOk=false;int row=attr["row"].toString().toInt(&rowOk),col=attr["col"].toString().toInt(&colOk);
            int width=attr.value("width",1).toInt(),height=attr.value("height",1).toInt();
            if(!rowOk||!colOk||row<0||col<0||width<1||height<1||row+height>rows||col+width>cols)return {{"error","Key outside keyboard grid"}};
            for(int y=row;y<row+height;++y)for(int x=col;x<col+width;++x){auto cell=QString("%1:%2").arg(y).arg(x);if(occupied.contains(cell))return {{"error","Overlapping keyboard cells"}};occupied.insert(cell);}
            QVariantMap style{{"backgroundcolor","#f7f8fa"},{"foregroundcolor","#17212d"},{"foregroundopacity",1.0},{"opacity",1.0}};
            QSet<QString> memberships;for(auto c:n["children"].toList()){auto child=c.toMap();if(child["tag"]=="KeyGroup")memberships.insert(child["text"].toString().trimmed());}
            for(auto group:groups){auto g=group.toMap();if(g["name"].toString().compare("All",Qt::CaseInsensitive)==0||memberships.contains(g["name"].toString()))for(auto it=g.begin();it!=g.end();++it)if(it.key()!="name")style[it.key()]=it.value();}
            for(auto it=attr.begin();it!=attr.end();++it)style[it.key()]=it.value();
            QVariantList commands;
            static const QSet<QString> commandTags{"Action","Text","KeyDown","KeyUp","KeyToggle","Wait","ChangeKeyboard"};
            for(auto c:n["children"].toList()){auto child=c.toMap();if(commandTags.contains(child["tag"].toString()))commands<<child;}
            int dwell=attr.value("completiontimes",0).toString().split(',').first().toInt();
            keys<<QVariantMap{{"row",row},{"col",col},{"width",width},{"height",height},{"kind",tag},{"label",field(n,"Label")},{"shiftLabel",field(n,"ShiftDownLabel")},{"symbol",field(n,"Symbol").trimmed()},{"style",style},{"commands",commands},{"dwell",dwell}};
        }
    }
    layout["rows"]=rows;layout["cols"]=cols;layout["keys"]=keys;return layout;
}
