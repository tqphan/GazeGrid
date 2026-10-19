#include "SymbolProvider.h"
#include <QFile>
#include <QJsonDocument>
#include <QSvgRenderer>
#include <QPainter>
#include <QUrl>
SymbolProvider::SymbolProvider():QQuickImageProvider(QQuickImageProvider::Image){QFile f(":/resources/icons.json");if(f.open(QIODevice::ReadOnly))paths=QJsonDocument::fromJson(f.readAll()).object();}
QImage SymbolProvider::requestImage(const QString &id,QSize *size,const QSize &requestedSize){
    QString name=id.section('/',0,0),color=QUrl::fromPercentEncoding(id.section('/',1,1).toUtf8());
    if(!QColor(color).isValid())color="#17212d";
    QString path=paths[name].toString();QString rule="evenodd";
    if(path.startsWith("F1")){rule="nonzero";path=path.mid(2);}else if(path.startsWith("F0"))path=path.mid(2);
    auto svg=QString("<svg xmlns='http://www.w3.org/2000/svg'><path id='icon' fill='%1' fill-rule='%2' d='%3'/></svg>").arg(color,rule,path.toHtmlEscaped()).toUtf8();
    QSvgRenderer renderer(svg);auto bounds=renderer.boundsOnElement("icon");renderer.setViewBox(bounds);renderer.setAspectRatioMode(Qt::KeepAspectRatio);
    QSize target=requestedSize.isValid()?requestedSize:QSize(64,64);target=target.boundedTo(QSize(512,512));
    QImage image(target,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
    if(renderer.isValid()&&!bounds.isEmpty()){QPainter painter(&image);renderer.render(&painter);}
    if(size)*size=target;return image;
}
