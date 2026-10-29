#pragma once
#include <QPointF>
#include <algorithm>
#include <cmath>

// Cursor feedback and commands use native screen pixels. Never integrate gaze
// deltas: the cursor may have been moved independently or accelerated by the OS.
class RelativePointer {
public:
    QPointF step(QPointF cursor,QPointF target){
        QPointF result;
        for(int axis=0;axis<2;++axis){
            const double current=axis?cursor.y():cursor.x();
            const double previous=axis?lastCursor.y():lastCursor.x();
            const double command=axis?lastCommand.y():lastCommand.x();
            double &gain=axis?gainY:gainX;
            if(command!=0){
                const double measured=(current-previous)/command;
                if(std::isfinite(measured)&&measured>0)gain=std::clamp(measured,0.125,16.0);
            }
            const double error=std::round(axis?target.y():target.x())-current;
            // Small relative commands can move several pixels at high pointer
            // speeds. Avoid oscillating around an unreachable pixel.
            double amount=0;
            if(std::abs(error)>std::max(0.5,gain/2)){
                amount=std::round(error*0.5/gain);
                if(amount==0)amount=std::copysign(1.0,error);
            }
            if(axis)result.setY(amount);else result.setX(amount);
        }
        lastCursor=cursor;lastCommand=result;
        return result;
    }
    void reset(){*this=RelativePointer{};}
private:
    QPointF lastCursor,lastCommand;
    double gainX=2,gainY=2;
};
