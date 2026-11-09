#include "globe.hpp"
#include <algorithm>
#include <cmath>
namespace uaa {
namespace {constexpr double radians=3.14159265358979323846/180.;
bool inside(const Json& ring,double x,double y){bool result=false;if(ring.size()<3)return false;
 for(std::size_t i=0,j=ring.size()-1;i<ring.size();j=i++){
  double ax=ring[i][0],ay=ring[i][1],bx=ring[j][0],by=ring[j][1];
  if((ay>y)!=(by>y)&&x<(bx-ax)*(y-ay)/(by-ay)+ax)result=!result;
 }return result;
}
bool polygon(const Json& rings,double lon,double lat){if(rings.empty()||!inside(rings[0],lon,lat))return false;for(std::size_t i=1;i<rings.size();++i)if(inside(rings[i],lon,lat))return false;return true;}
}
GlobePoint GlobeView::unit(double lon,double lat){double a=lon*radians,b=lat*radians;return {std::cos(b)*std::sin(a),std::sin(b),std::cos(b)*std::cos(a),(lon+180)/360,(90-lat)/180};}
GlobePoint GlobeView::rotate(GlobePoint p)const{double a=rotation*radians,t=tilt*radians;double x=p.x*std::cos(a)+p.z*std::sin(a),z=p.z*std::cos(a)-p.x*std::sin(a);return {x,p.y*std::cos(t)-z*std::sin(t),p.y*std::sin(t)+z*std::cos(t),p.u,p.v};}
bool GlobeView::project(double lon,double lat,double& x,double& y)const{auto p=rotate(unit(lon,lat));x=cx+p.x*radius*zoom;y=cy-p.y*radius*zoom;return p.z>=0;}
bool GlobeView::unproject(double x,double y,double& lon,double& lat)const{double xx=(x-cx)/(radius*zoom),yy=(cy-y)/(radius*zoom),zz=1-xx*xx-yy*yy;if(zz< -1e-12)return false;double z=std::sqrt(std::max(0.,zz)),t=tilt*radians;double sy=yy*std::cos(t)+z*std::sin(t),sz=z*std::cos(t)-yy*std::sin(t);lat=std::asin(std::clamp(sy,-1.,1.))/radians;lon=std::remainder(std::atan2(xx,sz)/radians-rotation,360.);return true;}
void GlobeGesture::down(double x,double y){pressed=true;moved=false;startX=lastX=x;startY=lastY=y;}
void GlobeGesture::motion(double x,double y,GlobeView& view){if(!pressed)return;if(!moved&&std::hypot(x-startX,y-startY)<6)return;moved=true;view.rotation=std::remainder(view.rotation-(x-lastX)*(.45/std::max(1.,view.zoom)),360.);view.tilt=std::clamp(view.tilt-(y-lastY)*(.45/std::max(1.,view.zoom)),-89.,89.);lastX=x;lastY=y;}
bool GlobeGesture::up(){bool click=pressed&&!moved;cancel();return click;}
std::string globeCountryAt(const Json& countries,double lon,double lat){if(!std::isfinite(lon)||!std::isfinite(lat)||std::abs(lat)>90)return {};lon=std::remainder(lon,360.);
 for(const auto& f:countries.at("features")){const auto& g=f["geometry"];if(g.is_null())continue;bool hit=false;if(g["type"]=="Polygon")hit=polygon(g["coordinates"],lon,lat);else if(g["type"]=="MultiPolygon")for(const auto& rings:g["coordinates"])if(polygon(rings,lon,lat)){hit=true;break;}if(hit)return f["properties"].value("name",std::string("LAND"));}return {};
}
}
