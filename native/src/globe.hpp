#pragma once
#include "engine.hpp"
namespace uaa {
struct GlobePoint { double x,y,z,u=0,v=0; };
struct GlobeView {
 double rotation=20,tilt=15,zoom=1;
 static constexpr double cx=501,cy=410,radius=267;
 static constexpr double earthMeters=6371000.,scalePixels=160.;
 static constexpr double closestScaleMeters=1000000.;
 static constexpr double maxZoom=scalePixels*earthMeters/(radius*closestScaleMeters);
 double scaleMeters()const{return scalePixels*earthMeters/(radius*zoom);}
 GlobePoint rotate(GlobePoint p)const;
 bool project(double lon,double lat,double& x,double& y)const;
 bool unproject(double x,double y,double& lon,double& lat)const;
 static GlobePoint unit(double lon,double lat);
};
// One gesture implementation is shared by real SDL input and regression tests.
struct GlobeGesture {
 bool pressed=false,moved=false;double startX=0,startY=0,lastX=0,lastY=0;
 void down(double x,double y);
 void motion(double x,double y,GlobeView& view);
 bool up();
 void cancel(){pressed=false;moved=false;}
};
std::string globeCountryAt(const Json& countries,double lon,double lat);
}
