#pragma once
#include <QRectF>
#include <QSize>
#include <cmath>
namespace MagnifierGeometry {
inline bool validPoint(QPointF p){return std::isfinite(p.x())&&std::isfinite(p.y())&&p.x()>=0&&p.x()<=1&&p.y()>=0&&p.y()<=1;}
inline QRect crop(QSize image,QPointF point,double zoom=4){
 if(!image.isValid()||!validPoint(point)||!std::isfinite(zoom)||zoom<1||zoom>8)return {};
 const int width=qMax(1,int(image.width()/zoom)),height=qMax(1,int(image.height()/zoom));
 const int x=qBound(0,int(point.x()*image.width())-width/2,image.width()-width);
 const int y=qBound(0,int(point.y()*image.height())-height/2,image.height()-height);
 return {x,y,width,height};
}
// Shrink around the point in the displayed crop, preserving the source point
// beneath it. This avoids jumps and feedback drift as gaze refines the target.
inline QRect zoomAt(QRect previous,QSize image,QPointF point,double zoom){
 const auto size=crop(image,{.5,.5},zoom).size();
 if(!previous.isValid()||!validPoint(point)||!size.isValid())return {};
 const int x=qBound(0,qRound(previous.x()+point.x()*(previous.width()-size.width())),image.width()-size.width());
 const int y=qBound(0,qRound(previous.y()+point.y()*(previous.height()-size.height())),image.height()-size.height());
 return {QPoint(x,y),size};
}
inline QPointF map(QRect crop,QSize image,QPointF point){
 if(!crop.isValid()||!image.isValid()||!validPoint(point))return {-1,-1};
 return {(crop.x()+point.x()*crop.width())/image.width(),(crop.y()+point.y()*crop.height())/image.height()};
}

}
