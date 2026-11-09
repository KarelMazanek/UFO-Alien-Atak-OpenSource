#pragma once
#include "core.hpp"
namespace uaa {
inline bool moduleType(const std::string&t){return starts(t,"wall")||starts(t,"fence")||t=="wire_fence"||t=="stone_wall";}
inline std::string moduleShape(const std::string&t,const std::string&requested="straight"){
 if(t=="wall_ufo_curve_reverse")return "corner_left";if(starts(t,"wall_ufo_curve"))return "corner_right";
 if(starts(t,"wall_ufo_window")||t=="wall_ufo_door_wide")return "straight";if(requested!="straight")return requested;
 if(t=="wall_inner"||t=="fence_corner")return "corner_right";if(t=="wall_outer")return "corner_left";if(t=="wall_end"||t=="fence_end")return "end";return requested;
}
inline double moduleHeight(const std::string&t){return t=="stone_wall"?80:t=="fence2"?60:t=="wire_fence"?100:starts(t,"fence")?70:97;}
inline double moduleThickness(const std::string&t){return starts(t,"wall_ufo_curve")?.5:starts(t,"wall_ufo")?1:t=="stone_wall"?.42:t=="wire_fence"?.04:starts(t,"fence")?.06:.02;}
inline P ufoPoint(double x,double y,int r,int width,P p){P q=r==0?p:r==1?P{1-p.y,p.x}:r==2?P{width-p.x,1-p.y}:P{p.y,width-p.x};return {x+q.x,y+q.y};}
inline std::vector<P> modulePath(const std::string&t,double x,double y,int r,const std::string&shape){
 if(starts(t,"wall_ufo")){int width=t=="wall_ufo_window_wide"||t=="wall_ufo_door_wide"?2:1;std::vector<P>out;
  if(starts(t,"wall_ufo_curve")){for(int i=0;i<=24;i++){double a=i*3.14159265358979323846/48;P p{.5*std::cos(a),.5*std::sin(a)};if(t=="wall_ufo_curve_reverse")p.x=1-p.x;out.push_back(ufoPoint(x,y,r,width,p));}}
  else{out.push_back(ufoPoint(x,y,r,width,{0,.5}));out.push_back(ufoPoint(x,y,r,width,{(double)width,.5}));}return out;
 }
 bool wall=starts(t,"wall")||t=="stone_wall";std::vector<P>anchor=wall?std::vector<P>{{x,y+1},{x+1,y},{x+1,y},{x,y+1}}:std::vector<P>{{x,y},{x+1,y},{x+1,y+1},{x,y+1}};P a=anchor[r],d=r==0?P{1,0}:r==1?P{0,1}:r==2?P{-1,0}:P{0,-1},b{a.x+d.x,a.y+d.y};std::vector<P>out={a,b};
 if(shape=="corner_right"||shape=="corner_left"){P turn=shape=="corner_right"?P{-d.y,d.x}:P{d.y,-d.x};out.push_back({b.x+turn.x,b.y+turn.y});}return out;
}
inline J segmentFoot(P a,P b,double thickness){double length=std::hypot(b.x-a.x,b.y-a.y),nx=-(b.y-a.y)/length*thickness/2,ny=(b.x-a.x)/length*thickness/2;return J::array({J::array({a.x+nx,a.y+ny}),J::array({b.x+nx,b.y+ny}),J::array({b.x-nx,b.y-ny}),J::array({a.x-nx,a.y-ny})});}
inline J moduleObject(const std::string&type,double x,double y,int r,const std::string&requested="straight"){
 auto shape=moduleShape(type,requested);auto path=modulePath(type,x,y,rot(r),shape);double height=moduleHeight(type),thickness=moduleThickness(type);bool door=type.find("door")!=std::string::npos;J full=J::array(),segments=J::array(),volumes=J::array(),polys=J::array();for(auto p:path)full.push_back(J::array({p.x,p.y}));
 auto add=[&](P a,P b){J foot=segmentFoot(a,b,thickness);volumes.push_back({{"foot",foot},{"z",0},{"h",height}});polys.push_back(foot);segments.push_back(J::array({J::array({a.x,a.y}),J::array({b.x,b.y})}));};
 for(size_t i=1;i<path.size();i++){auto a=path[i-1],b=path[i];if(door&&i==1){P l{a.x+(b.x-a.x)*.16,a.y+(b.y-a.y)*.16},rr{a.x+(b.x-a.x)*.84,a.y+(b.y-a.y)*.84};add(a,l);add(rr,b);}else add(a,b);}
 J o={{"type",type},{"x",x},{"y",y},{"rotation",rot(r)},{"shape",shape},{"role","wall"},{"walkable",false},{"boundary",true},{"boundarySegments",segments},{"nativePath",full},{"collision",volumes},{"footprintPolygons",polys},{"footprint",{{"w",1},{"d",1}}},{"door",door?J{{"open",true},{"passable",true},{"clearanceCells",type=="wall_ufo_door_wide"?2:1}}:J(nullptr)}};
 if(type=="wall_ufo_window_wide"||type=="wall_ufo_door_wide")o["footprint"]={{"w",r%2?1:2},{"d",r%2?2:1}};
 return o;
}
}
