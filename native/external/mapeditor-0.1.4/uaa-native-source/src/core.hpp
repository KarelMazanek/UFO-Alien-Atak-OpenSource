#pragma once
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
namespace uaa {
using J=nlohmann::json;
struct P{double x=0,y=0;};
struct Bounds{double x0=0,y0=0,x1=1,y1=1;};
inline int rot(int r){return ((r%4)+4)%4;}
inline bool starts(const std::string&s,const std::string&p){return s.rfind(p,0)==0;}
inline P iso(double x,double y){return {(x-y)*35.355339,(x+y)*20.4124145};}
inline P uniso(double x,double y){return {(x/35.355339+y/20.4124145)/2,(y/20.4124145-x/35.355339)/2};}
inline double num(const J&o,const char*k,double fallback=0){auto i=o.find(k);return i!=o.end()&&i->is_number()?i->get<double>():fallback;}
inline std::string str(const J&o,const char*k,const std::string&fallback=""){auto i=o.find(k);return i!=o.end()&&i->is_string()?i->get<std::string>():fallback;}
inline std::vector<P> polygon(const J&v){std::vector<P>p;for(auto&a:v)if(a.is_array()&&a.size()>=2)p.push_back({a[0].get<double>(),a[1].get<double>()});return p;}
inline bool overlap(const std::vector<P>&a,const std::vector<P>&b,double tolerance=.001){
 if(a.size()<3||b.size()<3)return false;
 for(auto*side:{&a,&b})for(size_t i=0;i<side->size();i++){
  P p=(*side)[i],q=(*side)[(i+1)%side->size()],axis={p.y-q.y,q.x-p.x};double length=std::hypot(axis.x,axis.y);if(length<1e-9)continue;axis.x/=length;axis.y/=length;
  double al=1e30,ah=-1e30,bl=1e30,bh=-1e30;for(auto v:a){double d=v.x*axis.x+v.y*axis.y;al=std::min(al,d);ah=std::max(ah,d);}for(auto v:b){double d=v.x*axis.x+v.y*axis.y;bl=std::min(bl,d);bh=std::max(bh,d);}if(std::min(ah,bh)-std::max(al,bl)<=tolerance)return false;
 }return true;
}
inline Bounds bounds(const J&o){Bounds b;double x=num(o,"x"),y=num(o,"y");b={x,y,x+num(o.value("footprint",J::object()),"w",1),y+num(o.value("footprint",J::object()),"d",1)};
 if(o.contains("collision")&&!o["collision"].empty()){b={1e30,1e30,-1e30,-1e30};for(auto&v:o["collision"])for(auto p:polygon(v["foot"])){b.x0=std::min(b.x0,p.x);b.y0=std::min(b.y0,p.y);b.x1=std::max(b.x1,p.x);b.y1=std::max(b.y1,p.y);}}return b;
}
inline void translate(J&o,double dx,double dy){o["x"]=num(o,"x")+dx;o["y"]=num(o,"y")+dy;
 for(const char*key:{"footprintPolygons","boundarySegments"})if(o.contains(key))for(auto&poly:o[key])for(auto&p:poly){p[0]=p[0].get<double>()+dx;p[1]=p[1].get<double>()+dy;}
 if(o.contains("collision"))for(auto&v:o["collision"])for(auto&p:v["foot"]){p[0]=p[0].get<double>()+dx;p[1]=p[1].get<double>()+dy;}if(o.contains("nativePath"))for(auto&p:o["nativePath"]){p[0]=p[0].get<double>()+dx;p[1]=p[1].get<double>()+dy;}
}
inline void rotateObject(J&o,int direction=1){int d=rot(direction);if(!d)return;double x=num(o,"x"),y=num(o,"y"),w=num(o.value("footprint",J::object()),"w",1),h=num(o.value("footprint",J::object()),"d",1);
 auto turn=[&](J&p){double a=p[0].get<double>()-x,b=p[1].get<double>()-y;P q=d==1?P{h-b,a}:d==2?P{w-a,h-b}:P{b,w-a};p[0]=q.x+x;p[1]=q.y+y;};
 for(const char*key:{"footprintPolygons","boundarySegments"})if(o.contains(key))for(auto&poly:o[key])for(auto&p:poly)turn(p);
 if(o.contains("collision"))for(auto&v:o["collision"])for(auto&p:v["foot"])turn(p);if(o.contains("nativePath"))for(auto&p:o["nativePath"])turn(p);
 o["rotation"]=rot((int)num(o,"rotation")+d);if(d%2){o["footprint"]["w"]=h;o["footprint"]["d"]=w;}
}
inline bool isWall(const J&o){return str(o,"role")=="wall"||starts(str(o,"type"),"wall")||str(o,"kind")=="wall";}
inline bool isUnit(const J&o){auto t=str(o,"type");return str(o,"role")=="unitStart"||t=="soldier"||t=="soldier_spawn"||t=="tank"||t=="tank_spawn"||t=="alien"||t=="alien_spawn";}
inline bool transportPair(const J&a,const J&b){if(str(a,"type")!="transport"||!isUnit(b)||starts(str(b,"type"),"alien"))return false;double x=num(a,"x"),y=num(a,"y");int r=rot((int)num(a,"rotation"));std::vector<P>deck;for(auto p:std::vector<P>{{0,4},{8,4},{8,6},{0,6}}){P q=r==0?p:r==1?P{10-p.y,p.x}:r==2?P{12-p.x,10-p.y}:P{p.y,12-p.x};deck.push_back({x+q.x,y+q.y});}auto bb=bounds(b);double loX=1e9,loY=1e9,hiX=-1e9,hiY=-1e9;for(auto p:deck){loX=std::min(loX,p.x);hiX=std::max(hiX,p.x);loY=std::min(loY,p.y);hiY=std::max(hiY,p.y);}return bb.x0>=loX-.001&&bb.y0>=loY-.001&&bb.x1<=hiX+.001&&bb.y1<=hiY+.001;}
inline bool objectsOverlap(const J&a,const J&b){if(transportPair(a,b)||transportPair(b,a))return false;
 auto aa=bounds(a),bb=bounds(b);if(aa.x1<=bb.x0+.001||bb.x1<=aa.x0+.001||aa.y1<=bb.y0+.001||bb.y1<=aa.y0+.001)return false;
 if((str(a,"type")=="rug")!=(str(b,"type")=="rug"))return false;
 if(!a.contains("collision")||!b.contains("collision"))return false;
 for(auto&u:a["collision"])for(auto&v:b["collision"])if(std::min(num(u,"h",100),num(v,"h",100))-std::max(num(u,"z"),num(v,"z"))>.01&&overlap(polygon(u["foot"]),polygon(v["foot"])))return true;return false;
}
inline std::vector<int> renderOrder(const std::vector<J>&objects){
 size_t n=objects.size();std::vector<Bounds>b;std::vector<double>depth;for(auto&o:objects){b.push_back(bounds(o));auto q=b.back();depth.push_back((q.x0+q.y0+q.x1+q.y1)/2);}std::vector<std::vector<int>>edges(n);std::vector<int>degree(n);std::vector<bool>done(n);
 auto edge=[&](int a,int b){edges[a].push_back(b);degree[b]++;};
 for(size_t i=0;i<n;i++)for(size_t j=i+1;j<n;j++){
  if(transportPair(objects[i],objects[j])){edge(i,j);continue;}if(transportPair(objects[j],objects[i])){edge(j,i);continue;}
  bool ab=b[i].x1<=b[j].x0+.001||b[i].y1<=b[j].y0+.001,ba=b[j].x1<=b[i].x0+.001||b[j].y1<=b[i].y0+.001;if(ab!=ba)edge(ab?i:j,ab?j:i);
 }
 std::vector<int>result;while(result.size()<n){int best=-1;for(size_t i=0;i<n;i++)if(!done[i]&&degree[i]==0&&(best<0||depth[i]<depth[best]))best=(int)i;if(best<0)for(size_t i=0;i<n;i++)if(!done[i]&&(best<0||depth[i]<depth[best]))best=(int)i;done[best]=true;result.push_back(best);for(int j:edges[best])degree[j]--;}
 return result;
}

inline P transportPoint(const J&o,double x,double y){int r=rot((int)num(o,"rotation"));P p=r==0?P{x,y}:r==1?P{10-y,x}:r==2?P{12-x,10-y}:P{y,12-x};return {num(o,"x")+p.x,num(o,"y")+p.y};}
inline J transportRect(const J&o,double x,double y,double w,double d){Bounds b{1e9,1e9,-1e9,-1e9};for(auto p:std::vector<P>{{x,y},{x+w,y},{x+w,y+d},{x,y+d}}){auto q=transportPoint(o,p.x,p.y);b.x0=std::min(b.x0,q.x);b.y0=std::min(b.y0,q.y);b.x1=std::max(b.x1,q.x);b.y1=std::max(b.y1,q.y);}return {{"x",b.x0},{"y",b.y0},{"w",b.x1-b.x0},{"d",b.y1-b.y0}};}
inline J transportPlan(const J&o,const std::vector<J>&entities){int r=rot((int)num(o,"rotation"));auto id=str(o,"id");J slots=J::array();
 struct Slot{const char*id;double x,y,w,d;bool tank;};int index=0;
 for(auto s:std::vector<Slot>{{"T1",2,4,2,2,true},{"T2",4,4,2,2,true},{"V1",6,4,1,1,false},{"V2",7,4,1,1,false},{"V3",6,5,1,1,false},{"V4",7,5,1,1,false}}){auto rect=transportRect(o,s.x,s.y,s.w,s.d);bool tank=false,soldiers=false;for(auto&u:entities){auto t=str(u,"type");auto b=bounds(u);bool in=b.x0>=num(rect,"x")-.001&&b.y0>=num(rect,"y")-.001&&b.x1<=num(rect,"x")+num(rect,"w")+.001&&b.y1<=num(rect,"y")+num(rect,"d")+.001;if(in&&(t=="tank"||t=="tank_spawn"))tank=true;if(in&&(t=="soldier"||t=="soldier_spawn"))soldiers=true;}
  J slot=rect;slot["id"]=s.id;slot["owner"]=id;slot["rotation"]=rot(r+2);slot["startInside"]=true;slot["localY"]=s.y;slot["exitOrder"]=++index;slot["type"]=s.tank?(tank?"tank":soldiers?"soldier":"flexible"):"soldier";slot["flexible"]=s.tank&&!tank&&!soldiers;slot["occupied"]=tank||soldiers;slot["maxSoldiers"]=s.tank?4:1;slot["accepts"]=s.tank?J::array({"tank","soldier"}):J::array({"soldier"});
  if(s.tank&&soldiers){for(int j=0;j<4;j++){J v=slot;auto q=transportRect(o,s.x+j%2,s.y+j/2,1,1);for(auto&e:q.items())v[e.key()]=e.value();v["id"]=std::string(s.id)+"-V"+std::to_string(j+1);v["zone"]=s.id;v["localY"]=s.y+j/2;slots.push_back(v);}}else slots.push_back(slot);
 }
 auto cabin=transportRect(o,2,4,6,2);cabin["area"]=12;J plan={{"id",id},{"x",num(o,"x")},{"y",num(o,"y")},{"rotation",r},{"footprint",o.value("footprint",J::object())},{"capacity",{{"soldiers",12},{"tanks",2},{"tankCost",4},{"area",12}}},{"cabin",cabin},{"collision",{{"type","compound"},{"volumes",o.value("collision",J::array())},{"walkableDeck",transportRect(o,0,4,8,2)}}},{"startPositions",slots},{"exits",J::array({transportRect(o,0,4,2,2)})},{"exitPolicy","tanks-first"},{"interiorMode","empty-start-zone"},{"startTurn",1},{"exitOrder",J::array()},{"departure",J::array()}};
 std::vector<J>occupied;for(auto&s:slots)if(str(s,"type")!="flexible")occupied.push_back(s);std::stable_sort(occupied.begin(),occupied.end(),[](auto&a,auto&b){return (str(a,"type")=="tank")>(str(b,"type")=="tank");});for(size_t i=0;i<occupied.size();i++){auto&s=occupied[i];plan["exitOrder"].push_back(s["id"]);plan["departure"].push_back({{"unit",s["id"]},{"type",s["type"]},{"order",i+1},{"waitFor",i?J::array({occupied[i-1]["id"]}):J::array()},{"route",J::array({J{{"x",s["x"]},{"y",s["y"]}},transportRect(o,0,num(s,"localY"),num(s,"w"),num(s,"d"))})},{"requiresClearExit",true}});}return plan;
}
struct Level{
 int n=50;std::vector<std::vector<std::string>>terrain;std::vector<J>objects;J source=J::object(),assets=J::object();std::vector<std::string>history,future;
 J snapshot()const{return J{{"size",n},{"terrain",terrain},{"entities",objects}};}
 void checkpoint(){history.push_back(snapshot().dump());future.clear();size_t bytes=0;for(auto&s:history)bytes+=s.size();while(history.size()>32||bytes>32u*1024u*1024u){bytes-=history.front().size();history.erase(history.begin());}}
 void restore(const J&d){n=d.at("size").get<int>();terrain=d.at("terrain").get<decltype(terrain)>();objects=d.at("entities").get<decltype(objects)>();}
 void undo(){if(history.empty())return;future.push_back(snapshot().dump());restore(J::parse(history.back()));history.pop_back();}
 void redo(){if(future.empty())return;history.push_back(snapshot().dump());restore(J::parse(future.back()));future.pop_back();}
 void load(const J&d){int size=d.value("size",d.value("width",50));auto t=d.contains("terrain")?d["terrain"]:d.at("map");if(size<1||size>120||!t.is_array()||(int)t.size()!=size)throw std::runtime_error("Nepodporovaná velikost mapy (1–120).");for(auto&r:t){if(!r.is_array()||(int)r.size()!=size)throw std::runtime_error("Neplatná data terénu.");for(auto&id:r)if(!id.is_string())throw std::runtime_error("Neplatný identifikátor terénu.");}
  std::vector<J>e;if(d.contains("entities"))e=d["entities"].get<decltype(e)>();else{if(d.contains("objects"))e=d["objects"].get<decltype(e)>();if(d.contains("placed"))e=d["placed"].get<decltype(e)>();if(d.contains("walls"))for(auto&o:d["walls"])e.push_back(o);}
  if(e.size()>20000)throw std::runtime_error("Mapa má příliš mnoho objektů.");for(auto&o:e)if(!o.is_object()||!o.contains("type")||!o["type"].is_string()||!o.contains("x")||!o.contains("y")||!o["x"].is_number()||!o["y"].is_number()||!std::isfinite(num(o,"x"))||!std::isfinite(num(o,"y")))throw std::runtime_error("Neplatný objekt mapy.");
  n=size;terrain=t.get<decltype(terrain)>();objects=std::move(e);source=d;assets=d.value("assets",J::object());history.clear();future.clear();
 }
 bool inside(const J&o)const{auto b=bounds(o);return b.x0>=-.02&&b.y0>=-.02&&b.x1<=n+.02&&b.y1<=n+.02;}
 int collision(const J&o,int ignore=-1)const{for(size_t i=0;i<objects.size();i++)if((int)i!=ignore&&objectsOverlap(o,objects[i]))return (int)i;return -1;}
 void resize(int size,bool crop=false){if(size!=20&&size!=50&&size!=80&&size!=120)throw std::runtime_error("Zvolte 20, 50, 80 nebo 120 polí.");for(auto&o:objects){auto b=bounds(o);if((b.x1>size||b.y1>size)&&!crop)throw std::runtime_error("Objekty přesahují novou velikost mapy.");}checkpoint();if(crop)objects.erase(std::remove_if(objects.begin(),objects.end(),[&](auto&o){auto b=bounds(o);return b.x1>size||b.y1>size;}),objects.end());terrain.resize(size);for(auto&r:terrain)r.resize(size,"grass");n=size;}
};
inline std::string b64encode(const std::vector<unsigned char>&data){static const char*t="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string out;out.reserve((data.size()+2)/3*4);for(size_t i=0;i<data.size();i+=3){unsigned v=(unsigned)data[i]<<16;if(i+1<data.size())v|=(unsigned)data[i+1]<<8;if(i+2<data.size())v|=data[i+2];out+=t[(v>>18)&63];out+=t[(v>>12)&63];out+=i+1<data.size()?t[(v>>6)&63]:'=';out+=i+2<data.size()?t[v&63]:'=';}return out;}
inline std::vector<unsigned char>b64decode(const std::string&s){std::vector<unsigned char>out;unsigned val=0;int bits=-8;for(unsigned char c:s){int d=c>='A'&&c<='Z'?c-'A':c>='a'&&c<='z'?c-'a'+26:c>='0'&&c<='9'?c-'0'+52:c=='+'?62:c=='/'?63:-1;if(d<0)continue;val=(val<<6)|d;bits+=6;if(bits>=0){out.push_back((val>>bits)&255);bits-=8;}}return out;}
}
