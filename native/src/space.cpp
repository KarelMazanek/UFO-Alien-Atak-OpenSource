#include "engine.hpp"
#include "globe.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace uaa {
Json Engine::permanentISSCrew()const{return readJson(dataDir/"space/iss-permanent-crew.json");}
void Engine::ensureLocations(){
 state["issPermanentCrew"]=permanentISSCrew();
 if(!state.contains("astronauts"))state["astronauts"]=Json::array();
 if(!state.contains("spaceHousing"))state["spaceHousing"]=Json::array();
 if(!state.contains("flightCrew"))state["flightCrew"]=Json::array();
 if(!state.contains("transports"))state["transports"]=Json::array();
 if(!state.contains("spaceOffers")){state["spaceOffers"]=Json::array();for(const auto& role:{"commander","navigator","technician","researcher"})for(int tier=0;tier<6;++tier)state["spaceOffers"].push_back(astronautCandidate(role,tier));}
 if(!state.contains("leases"))state["leases"]=Json::object();
 if(!state.contains("satellites"))state["satellites"]=Json::array();
 if(!state.contains("radarOutposts"))state["radarOutposts"]=Json::array();
 if(!state.contains("radarContacts"))state["radarContacts"]=Json::object();
 if(!state.contains("ufoSites"))state["ufoSites"]=Json::array();
 if(!state.contains("satelliteScans"))state["satelliteScans"]={{"EARTH",Json::object()},{"WATER",Json::object()},{"AIR",Json::object()}};
 if(!state.contains("launchQueue"))state["launchQueue"]=Json::array();
}
int Engine::astronautCapacity()const{int capacity=0;if(state.contains("spaceHousing"))for(const auto& ready:state["spaceHousing"])if(ready.get<double>()<=state["hours"].get<double>())capacity+=4;return capacity;}
void Engine::ensureHouston(){
 ensureLocations();auto layout=readJson(dataDir/"space/houston-layout.json");
 if(!state.value("houstonLayoutInitialized",false)){int added=std::max(0,layout["initialHousing"].get<int>()-int(state["spaceHousing"].size()));for(int i=0;i<added;++i)state["spaceHousing"].push_back(state["hours"]);state["money"]=std::max(0LL,state["money"].get<long long>()-added*constants["costs"].value("residential",240000LL));state["houstonLayoutInitialized"]=true;}
 auto buildings=layout["buildings"];bool occupied[10][10]={};auto claim=[&](const Json& b){auto [w,h]=footprint(b["id"]);for(int y=b["y"];y<b["y"].get<int>()+h;++y)for(int x=b["x"];x<b["x"].get<int>()+w;++x)if(x>=0&&x<10&&y>=0&&y<10)occupied[y][x]=true;};for(const auto& b:buildings)claim(b);
 for(int i=0;i<int(state["spaceHousing"].size());++i){int x=-1,y=-1;if(i<int(layout["housingPositions"].size())){x=layout["housingPositions"][i][0];y=layout["housingPositions"][i][1];}else for(int row=0;row<=8&&y<0;++row)for(int col=0;col<=8&&y<0;++col)if(!occupied[row][col]&&!occupied[row+1][col]&&!occupied[row][col+1]&&!occupied[row+1][col+1]){x=col;y=row;}if(y<0)break;Json b={{"id","residential"},{"x",x},{"y",y},{"floor",0},{"ready",state["spaceHousing"][i]}};buildings.push_back(b);claim(b);}
 state["houstonBuildings"]=buildings;
}
void Engine::ensureSpaceBase(const std::string& key){if(key=="nasa"){ensureHouston();return;}if(key!="esa"&&key!="roscosmos")return;ensureLocations();if(!state.contains("spaceBases"))state["spaceBases"]=Json::object();if(state["spaceBases"].contains(key))return;auto layout=readJson(dataDir/"space/control-layouts.json")[key];state["spaceBases"][key]=layout["buildings"];state["money"]=std::max(0LL,state["money"].get<long long>()-layout.value("initialHousingCost",0LL));}
bool Engine::spaceLease(const std::string& key)const{return state.contains("leases")&&state["leases"].contains(key)&&state["leases"][key].get<double>()>state["hours"].get<double>();}
Result Engine::rentLocation(const std::string& key){
 long long cost=key=="nasa"||key=="esa"?400000:key=="roscosmos"?300000:0;if(!cost)return {false,"Unknown space complex"};
 if(state["money"].get<long long>()<cost)return {false,"Insufficient funds"};ensureLocations();
 double until=std::max(state["hours"].get<double>(),state["leases"].value(key,0.));state["leases"][key]=until+720;state["money"]=state["money"].get<long long>()-cost;log("SPACE COMPLEX RENTED: "+key);return {true,"Lease extended by 30 game days"};
}
Json Engine::satelliteQuote(int fuel)const{
 fuel=std::clamp(fuel,0,100);double progress=fuel/100.;return {{"fuel",fuel},{"altitude",200+fuel*13},{"strength",int(std::round(10-progress*9))},{"coverage",int(std::round(450+progress*750))},{"cost",constants["costs"].value("satelliteLaunchUnit",500000LL)+std::max(0,fuel-60)*12000LL}};
}
Result Engine::launchSatellite(const std::string& type,int fuel,int count){
 if(type!="EARTH"&&type!="WATER"&&type!="AIR")return {false,"Unknown scan type"};
 if(fuel<0||fuel>100||count<1||count>5)return {false,"Invalid launch configuration"};
 if(!spaceLease("esa"))return {false,"Rent the ESA space complex first"};
 auto quote=satelliteQuote(fuel);long long cost=quote["cost"].get<long long>()*count;if(state["money"].get<long long>()<cost)return {false,"Insufficient funds"};
 ensureLocations();double ready=state["hours"];if(!state["launchQueue"].empty())ready=std::max(ready,state["launchQueue"].back()["ready"].get<double>());
 for(int i=0;i<count;++i){ready+=36;state["launchQueue"].push_back({{"type",type},{"ready",ready},{"altitude",quote["altitude"]},{"fuel",fuel}});}
 state["money"]=state["money"].get<long long>()-cost;log("KOUROU: SATELLITE LAUNCH QUEUED");return {true,"Launch queued: 12 h loading + 24 h preparation"};
}
void Engine::processSpace(double hours){
 ensureLocations();auto& queue=state["launchQueue"];while(!queue.empty()&&queue[0]["ready"].get<double>()<=hours){auto item=queue[0];item["launchTime"]=item["ready"];
  if(item["type"]=="CREW"||item["type"]=="CARGO"){item["arrival"]=item["ready"].get<double>()+(item["type"]=="CREW"?18:48);item["delivered"]=false;state["transports"].push_back(item);log("ISS TRANSPORT LAUNCHED");}
  else{item["number"]=state["satellites"].size()+1;state["satellites"].push_back(item);log("SATELLITE LAUNCHED: "+item["type"].get<std::string>());}queue.erase(0);
 }
 for(auto& flight:state["transports"])if(!flight.value("delivered",false)&&flight["arrival"].get<double>()<=hours){flight["delivered"]=true;state["issCargo"]=state.value("issCargo",0)+flight.value("cargoUnits",1);log("GENERAL CARGO DELIVERED TO ISS");}
}
Json Engine::satelliteStatus(std::size_t i)const{
 if(!state.contains("satellites")||i>=state["satellites"].size())return nullptr;auto sat=state["satellites"][i];
 double days=std::max(0.,(state["hours"].get<double>()-sat["launchTime"].get<double>())/24),life=365.25*10;
 double correction=days<life?std::fmod(days,30.)/30.*18:std::fmod(life,30.)/30.*18+(days-life)/30.*18;
 double altitude=std::max(100.,sat["altitude"].get<double>()-days/365.25*24-correction-std::max(0.,days-life)/365.25*180);
 double progress=std::clamp((altitude-200)/1300.,0.,1.);sat["currentAltitude"]=altitude;sat["fuelPercent"]=std::max(0.,100-days/life*100);sat["strength"]=int(std::round(10-progress*9));sat["coverage"]=int(std::round(450+progress*750));return sat;
}
// Persistent 5-degree survey cells and footprint radius follow recordSatelliteScan in the HTML.
Json Engine::satelliteGroundPoint(std::size_t index,double at)const{
 if(index>=state["satellites"].size())return nullptr;const auto& sat=state["satellites"][index];double elapsed=std::max(0.,at-sat["launchTime"].get<double>()),altitude=sat["altitude"],period=2*M_PI*std::sqrt(std::pow(6371+altitude,3)/398600.4418)/3600,u=elapsed/period*2*M_PI+index*1.7,inc=85*M_PI/180;
 double lat=std::asin(std::sin(inc)*std::sin(u))*180/M_PI,lon=(std::atan2(std::cos(inc)*std::sin(u),std::cos(u))-elapsed/23.9344*2*M_PI)*180/M_PI;lon=std::fmod(std::fmod(lon+180,360)+360,360)-180;return {{"lat",lat},{"lon",lon}};
}
void Engine::recordSatelliteScan(double lat,double lon,double at,const std::string& channel,double coverage,int satellite){
 if((channel!="EARTH"&&channel!="WATER"&&channel!="AIR")||!std::isfinite(lat)||!std::isfinite(lon)||!std::isfinite(at)||!std::isfinite(coverage)||std::abs(lat)>90||at<0||at>state["hours"].get<double>()+1e-8||coverage<=0||coverage>1500||satellite<1)return;
 if(!state.contains("satelliteScans"))ensureLocations();lon=std::remainder(lon,360.);auto& cells=state["satelliteScans"][channel];int row0=std::clamp(int(std::floor((lat+90)/5)),0,35),reach=int(std::ceil(coverage/6371*180/M_PI/5))+1,cols=std::min(36,int(std::ceil(coverage/6371*180/M_PI/std::max(.12,std::cos(lat*M_PI/180))/5))+1),col0=int(std::floor((lon+180)/5));
 auto capture=[&](int row,int col)->Json*{std::string key=std::to_string(row)+":"+std::to_string(col);if(cells.contains(key)&&cells[key]["at"].get<double>()>at)return nullptr;cells[key]={{"row",row},{"col",col},{"at",at},{"satellite",satellite},{"contacts",Json::array()}};return &cells[key];};
 for(int row=std::max(0,row0-reach);row<=std::min(35,row0+reach);++row)for(int offset=-cols;offset<=cols;++offset){int col=(col0+offset+144)%72;if(distance(lat,lon,-87.5+row*5,-177.5+col*5)<=coverage)capture(row,col);}
 for(const auto* collection:{&state["ufos"],&state["ufoSites"]})for(const auto& ufo:*collection){double born=ufo.value("spawned",std::max(0.,ufo.value("expires",720.)-720.));if(at<born||at>=ufo.value("expires",0.))continue;std::string environment=ufoEnvironmentAt(ufo,at);if((channel=="AIR"&&environment!="AIR")||(channel=="EARTH"&&environment!="LAND")||(channel=="WATER"&&environment!="SEA"))continue;double ulat=ufo["lat"],ulon=ufo["lon"];if(distance(lat,lon,ulat,ulon)>coverage)continue;int row=std::clamp(int(std::floor((ulat+90)/5)),0,35),col=(int(std::floor((ulon+180)/5))+72)%72;std::string key=std::to_string(row)+":"+std::to_string(col);if(!cells.contains(key)||cells[key]["at"].get<double>()<at)capture(row,col);if(cells[key]["at"].get<double>()!=at)continue;
  std::string id=ufoContactId(ufo);auto& contacts=cells[key]["contacts"];bool duplicate=false;for(const auto& contact:contacts)if(contact["id"]==id)duplicate=true;if(!duplicate)contacts.push_back({{"id",id},{"type",ufo["type"]},{"lat",ulat},{"lon",ulon},{"at",at},{"environment",environment},{"kind",ufo.value("kind",environment=="AIR"?std::string("AIR"):std::string("LANDED"))}});
 }
}
std::string Engine::ufoContactId(const Json& ufo){return ufo.value("contactId",ufo["type"].get<std::string>()+":"+std::to_string(ufo["lat"].get<double>())+":"+std::to_string(ufo["lon"].get<double>())+":"+std::to_string(ufo.value("spawned",std::max(0.,ufo.value("expires",720.)-720.))));}
std::string Engine::ufoEnvironmentAt(const Json& ufo,double at){if(ufo.contains("landingAt"))return at>=ufo["landingAt"].get<double>()&&at<ufo["takeoffAt"].get<double>()?ufo["surfaceEnvironment"].get<std::string>():"AIR";return ufo.value("environment",std::string("AIR"));}
void Engine::processUfoLandings(double old,double now){
 normalizeUfoLifecycle();Json countries;for(auto& ufo:state["ufos"]){double born=ufo.value("spawned",std::max(0.,ufo.value("expires",720.)-720.));if(!ufo.value("landingPlanInitialized",false)&&!ufo.contains("landingAt")&&ufo.value("environment",std::string("AIR"))=="AIR"){
  ufo["landingPlanInitialized"]=true;unsigned long long seed=1469598103934665603ULL;for(unsigned char c:ufoContactId(ufo)){seed^=c;seed*=1099511628211ULL;}if(seed%100<60){double landing=std::max(old,born)+6+(seed/100)%25,takeoff=landing+1+(seed/4300)%18;if(takeoff<=ufo["expires"].get<double>()){if(countries.is_null())countries=readJson(dataDir/"globe/countries.json");ufo["landingAt"]=landing;ufo["takeoffAt"]=takeoff;ufo["surfaceEnvironment"]=globeCountryAt(countries,ufo["lon"],ufo["lat"]).empty()?"SEA":"LAND";}}
 }
 if(ufo.contains("landingAt")){double landing=ufo["landingAt"],takeoff=ufo["takeoffAt"];if(landing>old&&landing<=now)log("UFO LANDED: "+ufo["surfaceEnvironment"].get<std::string>());if(takeoff>old&&takeoff<=now)log("UFO TOOK OFF");}ufo["environment"]=ufoEnvironmentAt(ufo,now);
 }
}
int Engine::radarRange(const std::string& id)const{return constants["radarRanges"].value(id,0);}
Result Engine::createRadarOutpost(double lat,double lon){
 if(state["bases"].empty())return {false,"Create the first base before placing radar outposts"};
 if(!std::isfinite(lat)||!std::isfinite(lon)||std::abs(lat)>90||std::abs(lon)>180)return {false,"Invalid radar coordinates"};
 auto country=globeCountryAt(readJson(dataDir/"globe/countries.json"),lon,lat);if(country.empty())return {false,"Radar outposts can only be placed on land"};ensureLocations();
 for(const auto& site:state["radarOutposts"])if(std::hypot(lat-site["lat"].get<double>(),std::remainder(lon-site["lon"].get<double>(),360.)*std::cos(lat*3.14159265358979323846/180.))<3)return {false,"A radar already exists here"};
 long long cost=constants["costs"]["radar"];if(state["money"].get<long long>()<cost)return {false,"Insufficient funds"};double now=state["hours"];int id=1;for(const auto& site:state["radarOutposts"])id=std::max(id,site["id"].get<int>()+1);
 state["radarOutposts"].push_back({{"id",id},{"name","RADAR #"+std::to_string(id)},{"lat",lat},{"lon",lon},{"country",country},{"started",now},{"ready",now+RadarOutpostBuildHours}});state["money"]=state["money"].get<long long>()-cost;log("RADAR OUTPOST UNDER CONSTRUCTION: "+country);return {true,"Radar outpost under construction: 10 days"};
}
void Engine::processRadarContacts(double old,double now){
 ensureLocations();auto& contacts=state["radarContacts"];for(auto it=contacts.begin();it!=contacts.end();)if(now-it.value()["at"].get<double>()>72)it=contacts.erase(it);else ++it;
 // Sweep endpoints and state transitions as well as hourly samples, so a long time step cannot skip a landing.
 std::vector<double> times{old,now};for(double t=std::floor(old)+1;t<now;t+=1)times.push_back(t);for(const auto& u:state["ufos"])for(const auto& key:{"spawned","landingAt","takeoffAt"})if(u.contains(key)){double t=u[key];if(t>old&&t<now)times.push_back(t);}std::sort(times.begin(),times.end());
 struct Sensor{double lat,lon,range,ready;std::string name;};std::vector<Sensor> sensors;
 for(const auto& base:state["bases"])for(const auto& building:base["buildings"])if(building.value("floor",0)==0&&radarRange(building["id"])>0){double ready=building["ready"];sensors.push_back({base["lat"],base["lon"],double(radarRange(building["id"])),ready,base["name"]});if(ready>old&&ready<now)times.push_back(ready);}
 for(const auto& site:state["radarOutposts"]){double ready=site["ready"];sensors.push_back({site["lat"],site["lon"],double(radarRange("radar")),ready,site["name"]});if(ready>old&&ready<now)times.push_back(ready);}std::sort(times.begin(),times.end());times.erase(std::unique(times.begin(),times.end()),times.end());
 for(double at:times)for(const auto& sensor:sensors){if(sensor.ready>at)continue;
  for(const auto* list:{&state["ufos"],&state["ufoSites"]})for(const auto& u:*list){double born=u.value("spawned",std::max(0.,u.value("expires",720.)-720.));if(at<born||at>=u["expires"].get<double>()||now-at>72||distance(sensor.lat,sensor.lon,u["lat"],u["lon"])>sensor.range)continue;std::string env=ufoEnvironmentAt(u,at),id=ufoContactId(u);contacts[id]={{"id",id},{"type",u["type"]},{"lat",u["lat"]},{"lon",u["lon"]},{"at",at},{"environment",env},{"kind",u.value("kind",env=="AIR"?std::string("AIR"):std::string("LANDED"))},{"source","RADAR"},{"base",sensor.name},{"channel",env=="AIR"?"AIR":env=="LAND"?"EARTH":"WATER"}};}
 }
}
Json Engine::ufoContact(std::size_t index)const{if(index>=state["ufos"].size())return nullptr;auto survey=satelliteSurvey();std::string id=ufoContactId(state["ufos"][index]);if(!survey["contacts"].contains(id))return nullptr;auto contact=survey["contacts"][id];if(state["hours"].get<double>()-contact["at"].get<double>()>72)return nullptr;return contact;}
void Engine::recordUfoCrashSite(const Json& ufo){ensureLocations();double now=state["hours"],lat=ufo["lat"],lon=ufo["lon"];auto country=globeCountryAt(readJson(dataDir/"globe/countries.json"),lon,lat);state["ufoSites"].push_back({{"type",ufo["type"]},{"lat",lat},{"lon",lon},{"spawned",now},{"expires",now+72},{"environment",country.empty()?"SEA":"LAND"},{"contactId","site:"+std::to_string(now)+":"+std::to_string(lat)+":"+std::to_string(lon)},{"country",country},{"kind","CRASH"}});}
void Engine::processSatelliteScans(double old,double now){
 ensureLocations();double lease=state["leases"].value("esa",0.);if(lease<=old)return;constexpr double step=2./60.;for(std::size_t i=0;i<state["satellites"].size();++i){auto& sat=state["satellites"][i];double launch=sat["launchTime"],start=sat.contains("scanThrough")?sat["scanThrough"].get<double>()+step:std::max(old,launch),end=std::min(now,lease);if(start<old-1e-8)start=std::max(old,launch);auto metrics=satelliteStatus(i);if(metrics["currentAltitude"].get<double>()<=120)continue;
  for(double at=start;at<=end+1e-9&&at<lease;at+=step){auto p=satelliteGroundPoint(i,at);recordSatelliteScan(p["lat"],p["lon"],at,sat["type"],metrics["coverage"],int(i+1));sat["scanThrough"]=at;}
 }
}
Json Engine::satelliteSurvey(const std::string& channel)const{
 Json cells=Json::object(),contacts=Json::object();if(state.contains("satelliteScans"))for(const auto& layer:state["satelliteScans"].items()){if(channel!="ALL"&&channel!=layer.key())continue;for(const auto& entry:layer.value().items()){auto cell=entry.value();cell["channel"]=layer.key();if(!cells.contains(entry.key())||cells[entry.key()]["at"].get<double>()<cell["at"].get<double>())cells[entry.key()]=cell;for(const auto& contact:entry.value()["contacts"]){std::string id=contact["id"];if(!contacts.contains(id)||contacts[id]["at"].get<double>()<contact["at"].get<double>()){contacts[id]=contact;contacts[id]["channel"]=layer.key();}}}}
 if(state.contains("radarContacts"))for(const auto& entry:state["radarContacts"].items()){const auto& contact=entry.value();if(channel!="ALL"&&contact["channel"]!=channel)continue;if(!contacts.contains(entry.key())||contacts[entry.key()]["at"].get<double>()<=contact["at"].get<double>())contacts[entry.key()]=contact;}
 return {{"cells",cells},{"contacts",contacts}};
}
bool Engine::validSatelliteSurvey(const Json& campaign){try{
 double now=campaign["hours"];
 for(const auto& u:campaign["ufos"]){double lat=u.at("lat"),lon=u.at("lon"),end=u.at("expires");if(!std::isfinite(lat)||!std::isfinite(lon)||!std::isfinite(end)||std::abs(lat)>90||std::abs(lon)>180)return false;if(u.contains("landingAt")||u.contains("takeoffAt")||u.contains("surfaceEnvironment")){double landing=u.at("landingAt"),takeoff=u.at("takeoffAt");if(!std::isfinite(landing)||!std::isfinite(takeoff)||landing<0||takeoff<=landing||takeoff>end||(u.at("surfaceEnvironment")!="LAND"&&u.at("surfaceEnvironment")!="SEA"))return false;}if(u.contains("environment")&&u["environment"]!="AIR"&&u["environment"]!="LAND"&&u["environment"]!="SEA")return false;}
 if(campaign.contains("radarOutposts")){if(!campaign["radarOutposts"].is_array()||campaign["radarOutposts"].size()>1000)return false;std::set<int> ids;for(const auto& site:campaign["radarOutposts"]){if(!site.at("id").is_number_integer()||!ids.insert(site.at("id").get<int>()).second||site.at("id").get<int>()<1||site.at("id").get<int>()>1000000||!site.at("name").is_string()||site.at("name").get<std::string>().empty()||site.at("name").get<std::string>().size()>80||!site.at("country").is_string())return false;double lat=site.at("lat"),lon=site.at("lon"),started=site.at("started"),ready=site.at("ready");if(!std::isfinite(lat)||!std::isfinite(lon)||!std::isfinite(started)||!std::isfinite(ready)||std::abs(lat)>90||std::abs(lon)>180||started<0||started>now+1e-8||std::abs(ready-started-RadarOutpostBuildHours)>1e-8)return false;}}
 if(campaign.contains("radarContacts")){if(!campaign["radarContacts"].is_object()||campaign["radarContacts"].size()>10000)return false;for(const auto& entry:campaign["radarContacts"].items()){const auto& c=entry.value();double at=c.at("at"),lat=c.at("lat"),lon=c.at("lon");std::string env=c.at("environment");if(!std::isfinite(at)||!std::isfinite(lat)||!std::isfinite(lon)||at<0||at>now+1e-8||std::abs(lat)>90||std::abs(lon)>180||c.at("id")!=entry.key()||!c.at("type").is_string()||!c.at("base").is_string()||c.at("source")!="RADAR"||(c.at("kind")!="AIR"&&c.at("kind")!="LANDED"&&c.at("kind")!="CRASH")||(env!="AIR"&&env!="LAND"&&env!="SEA")||c.at("channel")!=(env=="AIR"?"AIR":env=="LAND"?"EARTH":"WATER"))return false;}}
 if(campaign.contains("ufoSites")){if(!campaign["ufoSites"].is_array()||campaign["ufoSites"].size()>10000)return false;for(const auto& site:campaign["ufoSites"]){double lat=site.at("lat"),lon=site.at("lon"),born=site.at("spawned"),end=site.at("expires");if(!std::isfinite(lat)||!std::isfinite(lon)||!std::isfinite(born)||!std::isfinite(end)||std::abs(lat)>90||std::abs(lon)>180||born<0||born>now||end<born||!site.at("type").is_string()||!site.at("contactId").is_string()||(site.at("environment")!="SEA"&&site.at("environment")!="LAND"))return false;}}
 if(campaign.contains("satelliteScans")){const auto& layers=campaign["satelliteScans"];if(!layers.is_object())return false;for(const auto& layer:layers.items()){if((layer.key()!="AIR"&&layer.key()!="EARTH"&&layer.key()!="WATER")||!layer.value().is_object()||layer.value().size()>2592)return false;for(const auto& entry:layer.value().items()){const auto& cell=entry.value();if(!cell.is_object()||!cell.at("row").is_number_integer()||!cell.at("col").is_number_integer()||!cell.at("satellite").is_number_integer())return false;int row=cell["row"],col=cell["col"];double at=cell["at"];if(row<0||row>35||col<0||col>71||entry.key()!=std::to_string(row)+":"+std::to_string(col)||!std::isfinite(at)||at<0||at>now+1e-8||cell["satellite"].get<int>()<1||!cell["contacts"].is_array()||cell["contacts"].size()>10000)return false;for(const auto& c:cell["contacts"]){double lat=c.at("lat"),lon=c.at("lon"),time=c.at("at");if(!c.at("id").is_string()||!c.at("type").is_string()||!c.at("environment").is_string()||!std::isfinite(lat)||!std::isfinite(lon)||!std::isfinite(time)||std::abs(lat)>90||std::abs(lon)>180||std::abs(time-at)>1e-8)return false;std::string env=c["environment"];if((layer.key()=="AIR"&&env!="AIR")||(layer.key()=="EARTH"&&env!="LAND")||(layer.key()=="WATER"&&env!="SEA"))return false;}}}}
 if(campaign.contains("satellites"))for(const auto& sat:campaign["satellites"])if(sat.contains("scanThrough")){double at=sat["scanThrough"];if(!std::isfinite(at)||at<sat["launchTime"].get<double>()||at>now+1e-8)return false;}return true;
 }catch(...){return false;}}
Json Engine::astronautCandidate(const std::string& role,int tier){
 static const int qualities[]={42,50,58,76,84,92};static const char* labels[]={"ENTRY","QUALIFIED","STANDARD","EXPERIENCED","VETERAN","ELITE"};
 static const char* names[]={"ALEX MORGAN","JAMIE PARK","TAYLOR REED","RILEY SANTOS","CASEY NOVAK","JORDAN BLAKE","SAMIR KHAN","ELENA PETROVA","MORGAN LEE","DREW HAYES","NIKITA VOLKOV","MAYA CHEN","NOAH WILLIAMS","LEO MARTIN","AMINA DIALLO","SOFIA RUIZ","ETHAN CLARKE","LINA HADDAD","MARCUS REED","ANIKA SHAH"};
 tier=std::clamp(tier,0,5);int quality=qualities[tier],lo=18000,hi=28000;Json ranges;
 if(role=="commander"){ranges={{"COMMAND",{70,95}},{"NAVIGATION",{40,75}},{"ENGINEERING",{30,65}},{"SCIENCE",{30,65}}};lo=22000;hi=32000;}
 else if(role=="navigator"){ranges={{"COMMAND",{40,70}},{"NAVIGATION",{70,95}},{"ENGINEERING",{35,65}},{"SCIENCE",{30,65}}};lo=16000;hi=24000;}
 else if(role=="technician"){ranges={{"COMMAND",{30,65}},{"NAVIGATION",{40,70}},{"ENGINEERING",{70,95}},{"SCIENCE",{40,75}}};lo=18000;hi=26000;}
 else ranges={{"COMMAND",{30,60}},{"NAVIGATION",{30,60}},{"ENGINEERING",{45,75}},{"SCIENCE",{70,95}}};
 Json stats=Json::object();for(const auto& a:ranges.items()){int low=std::max(a.value()[0].get<int>(),quality-9),high=std::max(low,std::min(a.value()[1].get<int>(),quality+9));stats[a.key()]=std::uniform_int_distribution<int>(low,high)(rng);}
 return {{"role",role},{"name",names[rng()%20]},{"tier",labels[tier]},{"tierIndex",tier},{"quality",quality},{"salary",int(std::round((lo+std::clamp((quality-40)/52.,0.,1.)*(hi-lo))/100))*100},{"attributes",stats}};
}
Result Engine::buildSpaceHousing(){
 ensureHouston();if(!spaceLease("nasa"))return {false,"Houston lease required"};bool occupied[10][10]={};for(const auto& b:state["houstonBuildings"]){auto [w,h]=footprint(b["id"]);for(int y=b["y"];y<b["y"].get<int>()+h;++y)for(int x=b["x"];x<b["x"].get<int>()+w;++x)occupied[y][x]=true;}bool room=false;for(int y=0;y<9;++y)for(int x=0;x<9;++x)if(!occupied[y][x]&&!occupied[y+1][x]&&!occupied[y][x+1]&&!occupied[y+1][x+1])room=true;if(!room)return {false,"Houston residential area is full"};long long price=constants["costs"].value("residential",240000LL);if(state["money"].get<long long>()<price)return {false,"Insufficient funds"};
 ensureLocations();state["spaceHousing"].push_back(state["hours"].get<double>()+72);ensureHouston();state["money"]=state["money"].get<long long>()-price;return {true,"Residential cell queued: 4 astronaut places"};
}
Result Engine::hireAstronaut(std::size_t offer){
 ensureHouston();if(!spaceLease("nasa"))return {false,"Houston lease required"};if(offer>=state["spaceOffers"].size())return {false,"Select an astronaut"};
 int capacity=astronautCapacity();
 if(int(state["astronauts"].size())>=capacity)return {false,"Build a residential cell for astronaut accommodation"};
 auto person=state["spaceOffers"][offer];state["astronauts"].push_back(person);state["spaceOffers"][offer]=astronautCandidate(person["role"],person["tierIndex"]);return {true,"Astronaut hired; monthly salary applies"};
}
Result Engine::toggleFlightCrew(std::size_t person){
 ensureLocations();if(person>=state["astronauts"].size())return {false,"Select an astronaut"};auto& selected=state["flightCrew"];
 auto found=std::find(selected.begin(),selected.end(),Json(person));if(found!=selected.end()){selected.erase(found);return {true,"Crew member removed"};}
 std::string role=state["astronauts"][person]["role"];if(role=="commander"||role=="navigator")for(std::size_t i=0;i<selected.size();)if(state["astronauts"][selected[i].get<std::size_t>()]["role"]==role)selected.erase(i);else ++i;
 if(selected.size()>=8)return {false,"Maximum eight crew members"};selected.push_back(person);return {true,"Crew member selected"};
}
Result Engine::launchTransport(const std::string& complex,int units,int count){
 if((complex!="nasa"&&complex!="roscosmos")||units<1||units>3||count<1||count>5||(complex=="nasa"&&count!=1))return {false,"Invalid flight configuration"};
 if(!spaceLease(complex))return {false,"Space complex lease required"};ensureLocations();
 if(complex=="nasa"){int commander=0,navigator=0,technician=0;for(const auto& index:state["flightCrew"]){std::string role=state["astronauts"][index.get<std::size_t>()]["role"];commander+=role=="commander";navigator+=role=="navigator";technician+=role=="technician";}if(!commander||!navigator||!technician||state["flightCrew"].size()>8)return {false,"Select a commander, navigator and at least one technician"};}
 int fuel=40+units*20;long long cost=(complex=="nasa"?1500000+(units==1?100000:units==2?250000:400000)+units*500000LL:650000LL)+std::max(0,fuel-60)*12000LL;cost*=count;
 if(state["money"].get<long long>()<cost)return {false,"Insufficient funds"};double ready=state["hours"];if(!state["launchQueue"].empty())ready=std::max(ready,state["launchQueue"].back()["ready"].get<double>());
 for(int i=0;i<count;++i){ready+=36;state["launchQueue"].push_back({{"type",complex=="nasa"?"CREW":"CARGO"},{"ready",ready},{"altitude",420},{"fuel",fuel},{"cargoUnits",units},{"crew",complex=="nasa"?state["flightCrew"]:Json::array()}});}
 state["money"]=state["money"].get<long long>()-cost;return {true,"ISS flight scheduled"};
}
Result Engine::rentISS(int months){
 if(months<1||months>12||!spaceLease("nasa"))return {false,"Houston lease required; select 1–12 months"};long long cost=months*400000LL;if(state["money"].get<long long>()<cost)return {false,"Insufficient funds"};
 state["issLease"]=std::max(state.value("issLease",0.),state["hours"].get<double>())+months*720;state["money"]=state["money"].get<long long>()-cost;ensureISS();if(!state["iss"]["supplyPlan"]["enabled"].get<bool>())setISSSupplyPlan("florida",14,true);return {true,"ISS lease extended"};
}

Json Engine::unacknowledgedUfoContacts()const{Json result=Json::array();auto known=state.value("acknowledgedUfoContacts",Json::array());auto survey=satelliteSurvey();for(const auto& pair:survey["contacts"].items()){const auto& contact=pair.value();bool active=false;for(const auto& u:state["ufos"])if(ufoContactId(u)==pair.key()&&u["expires"].get<double>()>state["hours"].get<double>())active=true;if(!active||contact.value("kind",std::string(""))=="CRASH"||state["hours"].get<double>()-contact["at"].get<double>()>72)continue;if(std::find(known.begin(),known.end(),Json(pair.key()))==known.end())result.push_back(contact);}return result;}
void Engine::acknowledgeUfoContacts(const Json& contacts){if(!state.contains("acknowledgedUfoContacts"))state["acknowledgedUfoContacts"]=Json::array();auto& known=state["acknowledgedUfoContacts"];for(const auto& contact:contacts){std::string id=contact["id"];if(std::find(known.begin(),known.end(),Json(id))==known.end())known.push_back(id);}}
Json Engine::advanceUntilUfoContact(double hours){if(!std::isfinite(hours)||hours<0||hours>24*366||state["bases"].empty())return Json::array();double end=state["hours"].get<double>()+hours;normalizeUfoLifecycle();processRadarContacts(state["hours"],state["hours"]);processSatelliteScans(state["hours"],state["hours"]);auto contacts=unacknowledgedUfoContacts();if(!contacts.empty())return contacts;while(state["hours"].get<double>()<end-1e-9){double old=state["hours"],next=end;auto boundary=[&](double at){if(at>old+1e-9&&at<next)next=at;};boundary(state["nextUfo"]);for(const auto& base:state["bases"])for(const auto& building:base["buildings"])boundary(building["ready"]);if(state.contains("radarOutposts"))for(const auto& site:state["radarOutposts"])boundary(site["ready"]);for(const auto& u:state["ufos"])boundary(u.value("spawned",old));if(state.contains("satellites")&&state.contains("leases")&&state["leases"].value("esa",0.)>old)for(const auto& sat:state["satellites"])boundary(sat.value("scanThrough",old)+2./60.);advance(next-old);contacts=unacknowledgedUfoContacts();if(!contacts.empty())return contacts;}return contacts;}
int Engine::alertLevel()const{int count=0;for(std::size_t i=0;i<state["ufos"].size();++i){auto contact=ufoContact(i);if(contact.is_null())continue;++count;for(const auto& base:state["bases"])if(distance(base["lat"],base["lon"],contact["lat"],contact["lon"])<=1000)return 2;}return count>=3?2:count?1:0;}
Json Engine::fighterFuel(std::size_t ci)const{if(ci>=state["craft"].size()||state["craft"][ci]["type"]!="fighter")return nullptr;const auto& config=constants["fighterFuel"];bool external=aircraftModule(ci,"system")=="FUEL_TANK";double range=config[external?"externalRangeKm":"internalRangeKm"],capacity=config["internalLitres"].get<double>()+(external?config["externalLitres"].get<double>():0.),available=state["craft"][ci]["fuel"].get<double>()/100.;double radius=std::max(0.,(available-config["reserveFraction"].get<double>()-config["combatFraction"].get<double>())*range/2);return {{"capacityLitres",capacity},{"fuelLitres",capacity*available},{"rangeKm",range},{"remainingRangeKm",range*available},{"combatRadiusKm",radius},{"external",external}};}

void Engine::normalizeUfoLifecycle(){double stay=constants["ufoLifecycle"]["maxStayHours"],ground=constants["ufoLifecycle"]["maxLandingHours"];for(auto& u:state["ufos"]){std::string id=ufoContactId(u);double born=u.value("spawned",std::max(0.,u.value("expires",720.)-720.));u["spawned"]=born;u["contactId"]=id;u["expires"]=std::min(u["expires"].get<double>(),born+stay);if(!u.contains("landingAt")&&u.value("environment",std::string("AIR"))!="AIR"){u["landingAt"]=born;u["takeoffAt"]=std::min(born+ground,u["expires"].get<double>());u["surfaceEnvironment"]=u["environment"];}if(u.contains("landingAt")){double landing=u["landingAt"],takeoff=std::min({u["takeoffAt"].get<double>(),landing+ground,u["expires"].get<double>()});if(takeoff<=landing){u.erase("landingAt");u.erase("takeoffAt");u.erase("surfaceEnvironment");u["environment"]="AIR";u["landingPlanInitialized"]=true;}else{u["takeoffAt"]=takeoff;u["environment"]=ufoEnvironmentAt(u,state["hours"]);}}}}

}
