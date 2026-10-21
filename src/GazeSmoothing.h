#pragma once
// Port of OptiKey's DataFilters/KalmanFilter.cs.
// Original: Copyright (c) 2022 OPTIKEY LTD. Distributed under GPL-3.0;
// see LICENSE.txt. Positions here are display pixels, not normalized coordinates.
#include <QPointF>
#include <algorithm>
#include <cmath>

class GazeSmoothing {
public:
 void reset(){initialized=false;estimate={};previous={};older={};}
 QPointF update(QPointF measurement,int level,bool smoothTargetChanges){
  if(!std::isfinite(measurement.x())||!std::isfinite(measurement.y())){reset();return measurement;}
  if(level<=0){reset();return measurement;}
  // Seed from the first real point, avoiding an artificial sweep from (0,0).
  if(!initialized){initialized=true;estimate=previous=older=measurement;return measurement;}
  const double factor=std::clamp(level,1,4);
  const double processScale=10.0*factor-5.0;
  const double curveOffset=-factor;
  if(smoothTargetChanges){
   measurement=measurement*.45+previous*.30+older*.25;
   older=previous;previous=measurement;
  }
  const auto difference=measurement-estimate;
  const double delta=std::hypot(difference.x(),difference.y());
  const double exponent=(delta+processScale*curveOffset)/processScale;
  // Upstream replaces estimation noise with capped current process noise each
  // update. Preserve that behavior (including its filtered measurement history).
  const double noise=exponent>=std::log(1e100)?1e100:
      std::max(0.0,std::exp(exponent)-std::exp(curveOffset));
  const double gain=noise/(noise+2000.0);
  estimate+=difference*gain;
  return estimate;
 }
private:
 bool initialized=false;
 QPointF estimate,previous,older;
};
