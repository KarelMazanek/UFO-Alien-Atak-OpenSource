#include "engine.hpp"
#include <fstream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <limits>
#include <set>
#include <map>
namespace uaa {
Json readJson(const fs::path& p){std::ifstream f(p);if(!f)throw std::runtime_error("Cannot open "+p.string());return Json::parse(f);}
void writeJson(const fs::path& p,const Json& j){
 fs::create_directories(p.parent_path());auto tmp=p;tmp+=".tmp";
 {std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("Cannot write "+tmp.string());f<<j.dump(2);f.flush();if(!f)throw std::runtime_error("Write failed "+tmp.string());}
 // Keep a previous generation on platforms that cannot atomically replace files.
 auto backup=p;backup+=".bak";std::error_code ec;
 if(fs::exists(p)){fs::remove(backup,ec);fs::rename(p,backup);}
 try{fs::rename(tmp,p);}catch(...){if(fs::exists(backup))fs::rename(backup,p,ec);throw;}
}
Engine::Engine(fs::path data,fs::path user):dataDir(std::move(data)),userDir(std::move(user)){
 catalog=readJson(dataDir/"catalog.json");constants=readJson(dataDir/"constants.json");constants["ufopediaCategories"]=readJson(dataDir/"ufopedia-categories.json");world=readJson(dataDir/"world.json");constants["radarRanges"]=readJson(dataDir/"radar-ranges.json");for(auto& object:constants["BASE_OBJECTS"])if(object["key"]=="radar"||object["key"]=="radarLarge")object["desc"]="Radar range: "+std::to_string(radarRange(object["key"]))+" km";for(auto& building:catalog["db"]["gamesets"][0]["buildings"])if(building["id"]=="radar"||building["id"]=="radarLarge")building["description"]="Radar range: "+std::to_string(radarRange(building["id"]))+" km";auto amphibious=readJson(dataDir/"amphibious.json");constants["amphibious"]=amphibious;constants["FOOTPRINTS"]["underwater-robot"]=amphibious["robot"]["footprint"];for(const auto& item:amphibious["equipment"])catalog["db"]["gamesets"][0][item["id"]=="underwater-robot"?"units":"equipment"].push_back(item);for(const auto& technology:amphibious["research"])catalog["db"]["gamesets"][0]["technologies"].push_back(technology);for(auto& unit:catalog["db"]["gamesets"][0]["units"])if(unit["id"]=="hovercraft"){unit["name"]="AMPHIBIOUS TRANSPORT";unit["description"]="Amphibious transport capable of flight and underwater operations. Four soldiers and two equipment pallets.";unit["environments"]=amphibious["modes"];}constants["tradePartners"]=readJson(dataDir/"trade-partners.json");for(const auto& profile:amphibious["tankStats"].items()){constants["FOOTPRINTS"][profile.key()]=profile.value()["footprint"];for(const auto& group:{"equipment","units"})for(auto& item:catalog["db"]["gamesets"][0][group])if(item["id"]==profile.key()){item["vehicleStats"]=profile.value();item["unitStats"]={{"health",profile.value()["health"]},{"tu",profile.value()["timeUnits"]},{"accuracy",profile.value()["aimedAccuracy"]}};}}
 if(const auto* tank=record("equipment","light-tank"))catalog["db"]["gamesets"][0]["units"].push_back(*tank);
 for(auto& weapon:catalog["db"]["gamesets"][0]["equipment"])if(weapon["id"]=="tank-main-gun"||weapon["id"]=="tank-rocket-pods"){weapon["combat"]["tu"]["hip"]=20;weapon["combat"]["curves"]["hip"]={{0,55},{900,40},{1800,20}};}
 constants["fighterFuel"]=readJson(dataDir/"fighter-fuel.json");constants["ufoLifecycle"]=readJson(dataDir/"ufo-lifecycle.json");
 for(auto& item:catalog["db"]["gamesets"][0]["equipment"])if(item["id"]=="fighter-engine"){item["specs"].push_back({"STANDARD FUEL TANK","4 500 l · 2 200 km"});item["combat"]["values"]["fuel"]=4500;}
 newGame();
}
std::string Engine::ufopediaCategory(const std::string& id,const std::string& source)const{for(const auto& category:constants["ufopediaCategories"].items())for(const auto& item:category.value())if(item==id)return category.key();return source;}
Json Engine::ufopediaRecords(const std::string& category)const{Json result=Json::array();std::map<std::string,Json> records;std::vector<std::string> order;for(const auto& source:{"buildings","weapons","equipment","units","technologies"})for(const auto& record:gameset()[source]){std::string id=record["id"];if(ufopediaCategory(id,source)!=category)continue;if(!records.count(id))order.push_back(id);records[id]=record;}for(const auto& id:order)result.push_back(records[id]);return result;}
const Json& Engine::referenceGameset()const{return catalog.at("db").at("gamesets").at(0);}
const Json& Engine::gameset()const{return selectedGameset.is_object()?selectedGameset:referenceGameset();}
const Json* Engine::record(const std::string& group,const std::string& id)const{if(!gameset().contains(group))return nullptr;for(const auto& r:gameset()[group])if(r.value("id","")==id)return &r;return nullptr;}
void Engine::newGame(){
 state={{"schema","uaa-native-save-v1"},{"version",Version},{"hours",0.0},{"money",6500000LL},{"bases",Json::array()},{"radarOutposts",Json::array()},{"research",Json::object()},{"researchStaff",Json::object()},{"production",Json::array()},{"productionStaff",Json::object()},{"stock",Json::object()},{"personnel",Json::array()},{"craft",Json::array()},{"ufos",Json::array()},{"events",Json::array()},{"corpses",Json::object()},{"wrecks",Json::object()},{"liveAliens",Json::object()},{"lastWeek",0},{"lastMonth",0},{"nextUfo",72.0}};
 if(selectedGameset.is_object())state["gamesetSnapshot"]=selectedGameset;
 state["issPermanentCrew"]=permanentISSCrew();ensureISS();
 rng.seed(2042);
 for(const auto& group:{"weapons","equipment"})for(const auto& r:gameset()[group])state["stock"][r.at("id").get<std::string>()]=r.value("startingCount",0);
 // Local starting reserves sized for the eight-person squad and initial fleet.
 const Json startingStock={{"pistol",8},{"rifle",8},{"machine-gun",2},{"rocket-launcher",1},{"pistol-ammo",16},{"rifle-ammo",24},{"machine-gun-ammo",8},{"rocket-ammo",4},{"incendiary-rocket",2},{"fragmentation-grenade",8},{"anti-personnel-mine",2},{"flash-grenade",8},{"rotary-cannon",1},{"missile-rack",1},{"cannon-rounds",4},{"air-to-air-missiles",8},{"four-slot-belt",8},{"field-aid",8},{"medikit",2},{"combat-helmet",8},{"armor-vest",8},{"reinforced-vest",2},{"arm-armor",8},{"leg-armor",8},{"radiation-suit",2},{"fighter-engine",1},{"hover-engine",1},{"awacs-engine",1},{"aux-radar",1},{"shield",1},{"light-tank",0},{"tank-rocket-pods",0},{"tank-rocket",0},{"tank-main-gun",0},{"tank-main-gun-round",0},{"tank-machine-gun-rounds",0},{"metals",0},{"electronic-components",0},{"uranium",0}};
 for(const auto& item:startingStock.items())state["stock"][item.key()]=item.value();
 if(selectedGameset.is_object())for(const auto& group:{"weapons","equipment"})for(const auto& r:gameset()[group]){const Json* original=nullptr;for(const auto& base:referenceGameset()[group])if(base["id"]==r["id"])original=&base;if(!original||r.value("startingCount",0)!=original->value("startingCount",0))state["stock"][r.at("id").get<std::string>()]=r.value("startingCount",0);}
 for(const auto& name:{"iron","plastics","electronics","glass"})state["stock"][name]=100;
 for(const auto& role:{"pilot","navigator","technician","scientist","soldier"}){int count=std::string(role)=="soldier"?8:std::string(role)=="technician"||std::string(role)=="scientist"?10:3;const auto* current=record("units",role);if(selectedGameset.is_object()&&current)for(const auto& baseline:referenceGameset()["units"])if(baseline["id"]==role&&baseline.value("startingCount",0)!=current->value("startingCount",0))count=std::min(1000,current->value("startingCount",0));for(int i=0;i<count;++i)state["personnel"].push_back(makeCandidate(role,0));}
 ensureWorkforce();
 std::vector<std::size_t> unarmed;for(std::size_t i=0;i<state["personnel"].size();++i)if(state["personnel"][i]["role"]=="soldier")unarmed.push_back(i);
 auto weaponScore=[this](std::size_t i,bool rocket){const auto& p=state["personnel"][i];const auto& a=p["attributes"];return 2LL*p.value("missions",0)+a.value("MARKSMANSHIP",0)*(rocket?3:1)+a.value("STRENGTH",0)*2+a.value("STAMINA",0)*(rocket?1:2);};
 auto assignBest=[&](const std::string& weapon,bool rocket){std::stable_sort(unarmed.begin(),unarmed.end(),[&](std::size_t a,std::size_t b){return weaponScore(a,rocket)>weaponScore(b,rocket);});for(auto it=unarmed.begin();it!=unarmed.end();++it)if(equipSoldier(*it,"primaryWeapon",weapon).ok){unarmed.erase(it);return;}};
 assignBest("rocket-launcher",true);assignBest("machine-gun",false);assignBest("machine-gun",false);
 for(auto i:unarmed){auto equipped=equipSoldier(i,"primaryWeapon","rifle");if(!equipped.ok&&!selectedGameset.is_object())throw std::runtime_error("Initial soldier weapon: "+equipped.message);}
 for(const auto& type:{"fighter","hovercraft","awacs"})state["craft"].push_back({{"type",type},{"hull",100.0},{"fuel",100.0},{"ammo",std::string(type)=="fighter"?300:0},{"missiles",std::string(type)=="fighter"?8:0},{"missions",0}});
 log("Native development build: space operations and tactical missions are not ported.");
}
void Engine::log(const std::string& s){auto& e=state["events"];e.push_back({{"hours",state["hours"]},{"text",s}});while(e.size()>100)e.erase(e.begin());}
std::pair<int,int> Engine::footprint(const std::string& id)const{if(id=="runway")return {6,4};if(id=="awacsPad")return {6,2};if(!constants["FOOTPRINTS"].contains(id))return {1,1};auto p=constants["FOOTPRINTS"][id];return {p[0].get<int>(),p[1].get<int>()};}
Result Engine::createBase(std::string name,double lat,double lon){
 if(name.empty()||name.size()>80||!std::isfinite(lat)||!std::isfinite(lon)||std::abs(lat)>90||std::abs(lon)>180)return {false,"Invalid base name or coordinates"};
 const long long price=state["bases"].empty()?0:1000000;
 if(state["money"].get<long long>()<price)return {false,"Insufficient funds"};
 Json base={{"name",name},{"lat",lat},{"lon",lon},{"floors",0},{"buildings",Json::array()}};
 if(state["bases"].empty()){for(auto it=constants["COMPACT_LAYOUT"].begin();it!=constants["COMPACT_LAYOUT"].end();++it)base["buildings"].push_back({{"id",it.key()},{"x",it.value()[0]},{"y",it.value()[1]},{"floor",0},{"ready",0.0}});
  for(int y=1;y<=3;++y)base["buildings"].push_back({{"id","storage"},{"x",9},{"y",y},{"floor",0},{"ready",0.0}});
 }
 state["money"]=state["money"].get<long long>()-price;state["bases"].push_back(base);log("BASE: "+name);return {true,"Base created"};
}
int Engine::energy(std::size_t bi)const{
 if(bi>=state["bases"].size())return 0;int balance=0;double h=state["hours"].get<double>();
 for(const auto& b:state["bases"][bi]["buildings"]){if(b["ready"].get<double>()>h)continue;auto id=b["id"].get<std::string>();balance+=id=="power"?150:-constants["BUILDING_ENERGY_USE"].value(id,0);}
 return balance-state["bases"][bi]["floors"].get<int>()*15;
}
Json Engine::facilityBuildings()const{Json result=Json::array();double hour=state["hours"],leaseHour=facilityTime>=0?facilityTime:hour;for(const auto& base:state["bases"])for(const auto& b:base["buildings"])if(b["ready"].get<double>()<=hour)result.push_back(b);if(state.contains("spaceBases"))for(const auto& site:state["spaceBases"].items())if(state.contains("leases")&&state["leases"].value(site.key(),0.)>leaseHour)for(const auto& b:site.value())if(b["ready"].get<double>()<=hour)result.push_back(b);if(state.contains("houstonBuildings")&&state.contains("leases")&&state["leases"].value("nasa",0.)>leaseHour)for(const auto& b:state["houstonBuildings"])if(b["ready"].get<double>()<=hour)result.push_back(b);return result;}
bool Engine::facility(const std::string& id)const{for(const auto& b:facilityBuildings())if(b["id"]==id)return true;return false;}
int Engine::staffCapacity(const std::string& id)const{if(id!="research"&&id!="workshop")return 0;int count=0;for(const auto& b:facilityBuildings())if(b["id"]==id)++count;return count*25;}
void Engine::enforceWorkplaceCapacity(){int removed=0;auto trim=[&](Json& team,int& capacity){if(int(team.size())>capacity){removed+=int(team.size())-capacity;team.erase(team.begin()+capacity,team.end());}capacity-=int(team.size());};int capacity=staffCapacity("research");if(state.contains("researchStaff"))for(const auto& id:researchQueue())if(state["researchStaff"].contains(id.get<std::string>()))trim(state["researchStaff"][id.get<std::string>()],capacity);capacity=staffCapacity("workshop");if(state.contains("productionStaff"))for(auto& team:state["productionStaff"].items())trim(team.value(),capacity);if(removed)log("STAFF RELEASED: WORKPLACE LEASE EXPIRED");}
int Engine::assignedStaff(const std::string& id)const{int count=0;for(std::size_t i=0;i<state["personnel"].size();++i)if(id=="research"?!scientistProject(i).empty():!technicianProject(i).empty())++count;return count;}
Result Engine::canBuild(std::size_t bi,const std::string& id,int x,int y,int floor)const{
 if(bi>=state["bases"].size())return {false,"Select a base"};const auto& base=state["bases"][bi];
 bool known=false;long long price=0;for(const auto& obj:constants["BASE_OBJECTS"])if(obj["key"]==id){known=true;price=obj.value("cost",0LL);}
 price=buildingPrice(id);
 if(!known||id=="command"||id=="crewTraining")return {false,"Building is fixed or unavailable"};
 if(floor>0||floor< -base["floors"].get<int>())return {false,"Floor is unavailable"};
 if(floor<0&&(id=="power"||id=="hangar"||id=="runway"||id=="radar"||id=="radarLarge"||id=="defence"||id=="satellite"||id=="awacsPad"))return {false,"This building requires the surface"};
 const auto [w,h]=footprint(id);if(x<0||y<0||x+w>10||y+h>10)return {false,"Outside 10 x 10 base"};
 for(const auto& b:base["buildings"]){if(b["floor"]!=floor)continue;auto [bw,bh]=footprint(b["id"]);int bx=b["x"],by=b["y"];if(x<bx+bw&&x+w>bx&&y<by+bh&&y+h>by)return {false,"Building overlaps another facility"};}
 if(state["money"].get<long long>()<price)return {false,"Insufficient funds"};
 if(id!="power"&&energy(bi)<constants["BUILDING_ENERGY_USE"].value(id,0))return {false,"Insufficient power"};
 return {true,"Placement is valid"};
}
long long Engine::demolitionRefund(const std::string& id)const{return buildingPrice(id)/10;}
Result Engine::canDemolish(std::size_t bi,std::size_t object)const{if(bi>=state["bases"].size()||object>=state["bases"][bi]["buildings"].size())return {false,"Select an existing building"};const auto& item=state["bases"][bi]["buildings"][object];std::string id=item["id"];if(id=="command"||id=="elevator")return {false,"Command center and elevator cannot be demolished"};if((id=="research"||id=="workshop")&&item["ready"].get<double>()<=state["hours"].get<double>()&&assignedStaff(id)>staffCapacity(id)-25)return {false,"Release staff before removing occupied workplaces"};if(id=="power"&&item["ready"].get<double>()<=state["hours"].get<double>()&&energy(bi)<150)return {false,"Demolition would leave insufficient power"};return {true,"Building can be demolished"};}
Result Engine::demolish(std::size_t bi,std::size_t object){auto allowed=canDemolish(bi,object);if(!allowed.ok)return allowed;auto& objects=state["bases"][bi]["buildings"];std::string id=objects[object]["id"];state["money"]=state["money"].get<long long>()+demolitionRefund(id);objects.erase(object);log("DEMOLISHED: "+id);return {true,"Building removed"};}
Result Engine::build(std::size_t bi,const std::string& id,int x,int y,int floor){auto allowed=canBuild(bi,id,x,y,floor);if(!allowed.ok)return allowed;auto& base=state["bases"][bi];long long price=buildingPrice(id);
 base["buildings"].push_back({{"id",id},{"x",x},{"y",y},{"floor",floor},{"ready",state["hours"].get<double>()+buildingHours(id)}});
 state["money"]=state["money"].get<long long>()-price;log("CONSTRUCTION: "+id);return {true,"Construction started: "+std::to_string(int(buildingHours(id)))+" h"};
}
Result Engine::addFloor(std::size_t bi){if(bi>=state["bases"].size())return {false,"Select a base"};if(!facility("elevator"))return {false,"Build an elevator first"};if(state["bases"][bi]["floors"].get<int>()>=4)return {false,"Maximum 4 underground floors"};if(state["money"].get<long long>()<750000)return {false,"Insufficient funds"};state["money"]=state["money"].get<long long>()-750000;state["bases"][bi]["floors"]=state["bases"][bi]["floors"].get<int>()+1;return {true,"Floor created (development implementation)"};}
Json Engine::researchTeam(const std::string& id)const{
 if(!state.contains("researchStaff")||!state["researchStaff"].contains(id))return Json::array();return state["researchStaff"][id];
}
std::string Engine::scientistProject(std::size_t person)const{
 if(!state.contains("researchStaff"))return "";for(const auto& team:state["researchStaff"].items())if(state["research"].contains(team.key())&&state["research"][team.key()].get<double>()<1&&std::find(team.value().begin(),team.value().end(),Json(person))!=team.value().end())return team.key();return "";
}
Json Engine::researchQueue()const{Json queue=Json::array();std::set<std::string> seen;if(state.contains("researchQueue"))for(const auto& value:state["researchQueue"]){std::string id=value;if(state["research"].contains(id)&&state["research"][id].get<double>()<1&&seen.insert(id).second)queue.push_back(id);}for(const auto& project:state["research"].items())if(project.value().get<double>()<1&&seen.insert(project.key()).second)queue.push_back(project.key());return queue;}
Json Engine::researchSchedule()const{
 Json schedule=Json::array();struct Available{double at;Json people;};std::vector<Available> free;
 auto plan=[&](Json& row,double start,const Json& team){auto design=record("technologies",row["id"]);double work=(1-state["research"].value(row["id"].get<std::string>(),0.))*design->value("researchHours",72.)*std::max(1,design->value("researchScientists",10))/team.size();row["team"]=team;row["startInHours"]=start;row["finishInHours"]=start+work;free.push_back({start+work,team});};
 for(const auto& id:researchQueue()){auto team=researchTeam(id);schedule.push_back({{"id",id},{"team",team},{"assigned",team.size()},{"startInHours",nullptr},{"finishInHours",nullptr}});if(!team.empty()&&researchAvailable(id))plan(schedule.back(),0.,team);}
 for(auto& row:schedule)if(row["startInHours"].is_null()&&row["assigned"].get<int>()==0&&researchAvailable(row["id"])&&!free.empty()){auto first=std::min_element(free.begin(),free.end(),[](const Available& a,const Available& b){return a.at<b.at;});auto team=*first;free.erase(first);plan(row,team.at,team.people);}return schedule;
}
bool Engine::researchAvailable(const std::string& id)const{
 const auto* r=record("technologies",id);if(!r)return false;if(state["research"].value(id,0.)>=1)return true;
 auto foundAny=[this](const std::string& collection){for(const auto& amount:state[collection])if(amount.get<int>()>0)return true;return false;};
 if(id=="research_alien_biology"&&!foundAny("corpses")&&!foundAny("liveAliens"))return false;
 if((id=="research_alien_materials"||id=="research_alien_propulsion"||id=="research_ufo_navigation")&&!foundAny("wrecks"))return false;
 if(id=="research_psionics"&&state["liveAliens"].value("alien-psionic",0)<1&&state["research"].value("research_autopsy_alien-psionic",0.)<1)return false;
 auto prerequisite=r->value("requiresResearch",std::string(""));if(!prerequisite.empty()&&state["research"].value(prerequisite,0.)<1)return false;
 for(const auto& requirement:{std::pair{"requiresCorpse","corpses"},{"requiresUfoWreck","wrecks"},{"requiresLiveAlien","liveAliens"}}){auto target=r->value(requirement.first,std::string(""));if(!target.empty()&&state[requirement.second].value(target,0)<1)return false;}
 auto alien=r->value("requiresAlien",std::string(""));if(!alien.empty()&&r->value("requiresLiveAlienOrAutopsy",false)&&state["liveAliens"].value(alien,0)<1&&state["research"].value("research_autopsy_"+alien,0.)<1)return false;
 if(r->value("availability",std::string("")).find("locked_")==0&&!r->contains("requiresCorpse")&&!r->contains("requiresUfoWreck")&&!r->contains("requiresLiveAlien")&&!r->contains("requiresAlien"))return false;
 return true;
}
Json Engine::researchQuote(const std::string& id)const{
 const auto* r=record("technologies",id);Json quote={{"ok",false},{"reason","Unknown research"},{"started",state["research"].contains(id)},{"progress",state["research"].value(id,0.)},{"hours",0.},{"scientists",0},{"assigned",researchTeam(id).size()}};if(!r)return quote;
 quote["hours"]=r->value("researchHours",72.);quote["scientists"]=r->value("researchScientists",10);
 if(quote["progress"].get<double>()>=1){quote["reason"]="Research complete";return quote;}
 if(!researchAvailable(id)){quote["reason"]="Required recovery or research is unavailable";return quote;}
 if(!facility("research")){quote["reason"]="A laboratory is required";return quote;}
 const auto req=r->value("requiresResearch",std::string(""));if(!req.empty()&&state["research"].value(req,0.)<1){quote["reason"]="Complete prerequisite research: "+req;return quote;}
 for(const auto& entry:{std::pair{"requiresCorpse","corpses"},{"requiresUfoWreck","wrecks"},{"requiresLiveAlien","liveAliens"}}){auto target=r->value(entry.first,std::string(""));if(!target.empty()&&state[entry.second].value(target,0)<1){quote["reason"]="Required specimen or wreck: "+target;return quote;}}
 if(r->value("availability",std::string("")).find("locked_")==0&&!r->contains("requiresCorpse")&&!r->contains("requiresUfoWreck")&&!r->contains("requiresLiveAlien")&&!r->contains("requiresAlien")){quote["reason"]="Required recovery or research is unavailable";return quote;}
 quote["ok"]=true;quote["reason"]=quote["started"].get<bool>()?(researchTeam(id).empty()?"Assign scientists to this project":"Research in progress"):"Ready to start research";return quote;
}
Result Engine::startResearch(const std::string& id){
 auto quote=researchQuote(id);if(!quote["ok"].get<bool>())return {false,quote["reason"]};if(quote["started"].get<bool>())return {true,"Select scientists for this project"};
 state["research"][id]=0.;if(!state.contains("researchStaff"))state["researchStaff"]=Json::object();state["researchStaff"][id]=Json::array();if(!state.contains("researchQueue"))state["researchQueue"]=Json::array();state["researchQueue"].push_back(id);log("RESEARCH: "+id);return {true,"Research selected; assign scientists"};
}
Result Engine::assignScientist(const std::string& id,std::size_t person,bool assigned){
 if(person>=state["personnel"].size()||state["personnel"][person]["role"]!="scientist")return {false,"Select a scientist"};
 if(!state["research"].contains(id)||state["research"][id].get<double>()>=1)return {false,"Select an active research project"};
 std::string busy=scientistProject(person);if(assigned&&!busy.empty()&&busy!=id)return {false,"Scientist is assigned to another project"};
 if(!state.contains("researchStaff"))state["researchStaff"]=Json::object();if(!state["researchStaff"].contains(id))state["researchStaff"][id]=Json::array();auto& team=state["researchStaff"][id];auto found=std::find(team.begin(),team.end(),Json(person));
 if(assigned&&found==team.end()&&assignedStaff("research")>=staffCapacity("research"))return {false,"Laboratory capacity reached (25 scientists per laboratory)"};if(assigned&&found==team.end())team.push_back(person);else if(!assigned&&found!=team.end())team.erase(found);return {true,assigned?"Scientist assigned":"Scientist released"};
}
Json Engine::productionSchedule()const{
 Json batches=Json::array();std::map<std::string,int> positions;
 for(std::size_t i=0;i<state["production"].size();++i){const auto& order=state["production"][i];std::string group=order.value("group",std::string("")),id=order["id"];if(group.empty())for(const auto& candidate:{"weapons","equipment","units"})if(record(candidate,id)){group=candidate;break;}std::string key=group+":"+id;if(!positions.count(key)){positions[key]=int(batches.size());auto team=productionTeam(group,id);batches.push_back({{"group",group},{"id",id},{"indices",Json::array()},{"team",team},{"assigned",team.size()},{"startInHours",nullptr},{"finishInHours",nullptr}});}batches[positions[key]]["indices"].push_back(i);}
 struct AvailableTeam{double time;Json people;};std::vector<AvailableTeam> teams;
 auto schedule=[&](Json& batch,double start,const Json& team){std::vector<double> clocks(team.size(),start);for(const auto& index:batch["indices"]){const auto& order=state["production"][index.get<std::size_t>()];double work=order.value("remaining",std::max(0.,order["ready"].get<double>()-state["hours"].get<double>()));auto worker=std::min_element(clocks.begin(),clocks.end());*worker+=work;}double finish=*std::max_element(clocks.begin(),clocks.end());batch["startInHours"]=start;batch["finishInHours"]=finish;batch["team"]=team;teams.push_back({finish,team});};
 for(auto& batch:batches)if(!batch["team"].empty())schedule(batch,0.,batch["team"]);
 for(auto& batch:batches)if(batch["startInHours"].is_null()&&!teams.empty()){auto first=std::min_element(teams.begin(),teams.end(),[](const AvailableTeam& a,const AvailableTeam& b){return a.time<b.time;});auto team=*first;teams.erase(first);schedule(batch,team.time,team.people);}
 return batches;
}
Json Engine::productionProgress(const std::string& group,const std::string& id)const{
 const auto* design=record(group,id);double duration=design&&design->contains("production")?(*design)["production"].value("productionHours",1.):1.;int remaining=0,done=state.contains("productionFinished")?state["productionFinished"].value(group+":"+id,0):0;double partial=0.;
 for(const auto& order:state["production"])if(order.value("group",group)==group&&order["id"]==id){++remaining;double work=order.value("remaining",std::max(0.,order["ready"].get<double>()-state["hours"].get<double>()));partial+=std::clamp(1-work/std::max(.001,duration),0.,1.);}
 int total=done+remaining;return {{"completed",done},{"remaining",remaining},{"total",total},{"progress",total?std::clamp((done+partial)/total,0.,1.):0.}};
}
Json Engine::productionTeam(const std::string& group,const std::string& id)const{std::string key=group+":"+id;if(!state.contains("productionStaff")||!state["productionStaff"].contains(key))return Json::array();return state["productionStaff"][key];}
std::string Engine::technicianProject(std::size_t person)const{if(state.contains("productionStaff"))for(const auto& team:state["productionStaff"].items())if(std::find(team.value().begin(),team.value().end(),Json(person))!=team.value().end())return team.key();return "";}
Result Engine::assignTechnician(const std::string& group,const std::string& id,std::size_t person,bool assigned){
 const auto* design=record(group,id);if(!design||!design->contains("production"))return {false,"Select a production design"};if(person>=state["personnel"].size()||state["personnel"][person]["role"]!="technician")return {false,"Select a technician"};
 std::string key=group+":"+id,busy=technicianProject(person);if(assigned&&!busy.empty()&&busy!=key)return {false,"Technician is assigned to another production team"};
 if(assigned&&busy.empty()&&assignedStaff("workshop")>=staffCapacity("workshop"))return {false,"Workshop capacity reached (25 technicians per workshop)"};if(!state.contains("productionStaff"))state["productionStaff"]=Json::object();if(!state["productionStaff"].contains(key))state["productionStaff"][key]=Json::array();auto& team=state["productionStaff"][key];auto found=std::find(team.begin(),team.end(),Json(person));if(assigned&&found==team.end())team.push_back(person);else if(!assigned&&found!=team.end())team.erase(found);return {true,"Production team updated"};
}
void Engine::processProduction(double oldHours,double hours){
 if(!facility("workshop"))return;auto& queue=state["production"];for(auto& order:queue)if(!order.contains("remaining"))order["remaining"]=std::max(0.,order["ready"].get<double>()-oldHours);auto schedule=productionSchedule();std::stable_sort(schedule.begin(),schedule.end(),[](const Json& a,const Json& b){return a["startInHours"].is_null()?false:b["startInHours"].is_null()?true:a["startInHours"].get<double>()<b["startInHours"].get<double>();});
 for(const auto& batch:schedule){if(batch["startInHours"].is_null()||batch["startInHours"].get<double>()>hours)continue;std::string group=batch["group"],id=batch["id"],key=group+":"+id;const auto& team=batch["team"];if(batch["assigned"].get<int>()==0)for(const auto& person:team)assignTechnician(group,id,person.get<std::size_t>(),true);std::vector<double> clocks(team.size(),batch["startInHours"].get<double>());
  for(const auto& index:batch["indices"]){auto& order=queue[index.get<std::size_t>()];order["group"]=group;double remaining=order["remaining"];auto worker=std::min_element(clocks.begin(),clocks.end());double start=*worker;*worker+=remaining;remaining=std::max(0.,remaining-std::max(0.,hours-start));order["remaining"]=remaining;order["ready"]=oldHours+hours+remaining;if(remaining>1e-8)continue;
   if(group=="units"&&(id=="fighter"||id=="hovercraft"||id=="awacs"))state["craft"].push_back({{"type",id},{"hull",100.0},{"fuel",100.0},{"ammo",id=="fighter"?300:0},{"missiles",id=="fighter"?8:0},{"missions",0}});else state["stock"][id]=state["stock"].value(id,0)+1;
   if(!state.contains("productionFinished"))state["productionFinished"]=Json::object();state["productionFinished"][key]=state["productionFinished"].value(key,0)+1;state["pendingProductionCompletions"]=state.value("pendingProductionCompletions",0)+1;log("MANUFACTURED: "+id);
  }
  if(batch["finishInHours"].get<double>()<=hours+1e-8)state["productionStaff"].erase(key);
 }
 for(std::size_t i=0;i<queue.size();)if(queue[i]["remaining"].get<double>()<=1e-8)queue.erase(i);else ++i;
 // Reserve a team at a boundary even when the next unit receives no work yet.
 for(const auto& batch:schedule)if(!batch["startInHours"].is_null()&&batch["startInHours"].get<double>()<=hours+1e-8&&batch["finishInHours"].get<double>()>hours){std::string group=batch["group"],id=batch["id"];if(productionTeam(group,id).empty())for(const auto& person:batch["team"])assignTechnician(group,id,person.get<std::size_t>(),true);}
}

Json Engine::manufactureQuote(const std::string& group,const std::string& id,int count)const{
 Json quote={{"ok",false},{"reason","No production specification"},{"cost",0LL},{"hours",0.},{"materials",Json::array()},{"count",count}};
 if(count<1||count>100){quote["reason"]="Select 1–100 units";return quote;}
 const auto* r=record(group,id);if((group!="weapons"&&group!="equipment"&&group!="units")||!r||!r->contains("production"))return quote;
 auto research=r->value("requiresResearch",std::string(""));if(!research.empty()&&state["research"].value(research,0.)<1){quote["reason"]="Required research is incomplete";return quote;}
 const auto& p=(*r)["production"];quote["cost"]=p.value("productionCost",0LL)*count;quote["hours"]=p.value("productionHours",1.0);
 bool enough=true;
 for(const auto& m:p["materials"].items()){
  int required=m.value().get<int>()*count,have=state["stock"].value(m.key(),0),missing=std::max(0,required-have);
  quote["materials"].push_back({{"id",m.key()},{"required",required},{"have",have},{"missing",missing}});if(missing)enough=false;
 }

 if(!facility("workshop"))quote["reason"]="A workshop is required";
 else if(r->value("availableAfterDays",0)>state["hours"].get<double>()/24)quote["reason"]="Item is not yet available";
 else if(!enough)quote["reason"]="Missing materials: buy or deliver the displayed requirements";
 else if(state["money"].get<long long>()<quote["cost"].get<long long>())quote["reason"]="Insufficient funds for the whole order";
 else{
  int capacity=id=="fighter"?3:id=="hovercraft"?2:id=="awacs"?1:0,used=0;
  if(group=="units"&&capacity){for(const auto& craft:state["craft"])if(craft["type"]==id)++used;for(const auto& order:state["production"])if(order["id"]==id)++used;}
  if(group=="units"&&capacity&&used+count>capacity)quote["reason"]="Aircraft capacity reached";
  else{quote["ok"]=true;quote["reason"]="Ready to manufacture";}
 }
 return quote;
}
Result Engine::manufacture(const std::string& group,const std::string& id,int count){
 auto quote=manufactureQuote(group,id,count);if(!quote["ok"].get<bool>())return {false,quote["reason"]};
 bool pending=false;for(const auto& order:state["production"])if(order.value("group",group)==group&&order["id"]==id)pending=true;if(!state.contains("productionFinished"))state["productionFinished"]=Json::object();if(!pending)state["productionFinished"][group+":"+id]=0;
 for(const auto& m:quote["materials"]){std::string material=m["id"];state["stock"][material]=state["stock"].value(material,0)-m["required"].get<int>();}
 state["money"]=state["money"].get<long long>()-quote["cost"].get<long long>();double start=state["hours"],ready=start+quote["hours"].get<double>();
 for(int i=0;i<count;++i)state["production"].push_back({{"id",id},{"group",group},{"started",start},{"ready",ready},{"remaining",quote["hours"]}});
 log("MANUFACTURING QUEUED: "+id+" × "+std::to_string(count));return {true,"Manufacturing queued; advance game time to complete the order"};
}
Result Engine::recruit(const std::string& role){ensureWorkforce();for(std::size_t i=0;i<state["offers"].size();++i)if(state["offers"][i]["role"]==role)return hireCandidate(i);return {false,"No candidate available"};}

double Engine::distance(double a,double b,double c,double d){constexpr double rad=3.14159265358979323846/180;auto p=std::sin((c-a)*rad/2),q=std::sin((d-b)*rad/2);return 6371*2*std::asin(std::sqrt(std::clamp(p*p+std::cos(a*rad)*std::cos(c*rad)*q*q,0.0,1.0)));}
Result Engine::interceptionReadiness(std::size_t ci,std::size_t ui)const{
 if(ci>=state["craft"].size()||ui>=state["ufos"].size())return {false,"Select aircraft and UFO"};const auto& c=state["craft"][ci];const auto& u=state["ufos"][ui];
 if(ufoEnvironmentAt(u,state["hours"])!="AIR")return {false,"UFO has landed; aerial interception unavailable"};
 if(c["type"]!="fighter")return {false,"Select a fighter"};
 if(c.contains("serviceReady"))return {false,"Aircraft service in progress"};
 if(c["hull"].get<double>()<=30)return {false,"Fighter hull requires repair"};
 if(c["fuel"].get<double>()<20)return {false,"Insufficient fighter fuel"};
 if(!facility("runway"))return {false,"No operational runway"};
 double dist=std::numeric_limits<double>::max();for(const auto& b:state["bases"]){bool runway=false;for(const auto& object:b["buildings"])if(object["id"]=="runway"&&object.value("floor",0)==0&&object["ready"].get<double>()<=state["hours"].get<double>())runway=true;if(runway)dist=std::min(dist,distance(b["lat"],b["lon"],u["lat"],u["lon"]));}auto fuel=fighterFuel(ci);if(dist>fuel["combatRadiusKm"].get<double>()+1e-8)return {false,"Target outside safe combat radius; allow fuel for return and reserve"};return {true,"Ready for interception"};
}
Result Engine::intercept(std::size_t ci,std::size_t ui){
 auto ready=interceptionReadiness(ci,ui);if(!ready.ok)return ready;auto& c=state["craft"][ci];auto& u=state["ufos"][ui];double dist=std::numeric_limits<double>::max();for(const auto& b:state["bases"]){bool runway=false;for(const auto& object:b["buildings"])if(object["id"]=="runway"&&object.value("floor",0)==0&&object["ready"].get<double>()<=state["hours"].get<double>())runway=true;if(runway)dist=std::min(dist,distance(b["lat"],b["lon"],u["lat"],u["lon"]));}auto fuel=fighterFuel(ci);double fuelUsed=100*(2*dist/fuel["rangeKm"].get<double>()+constants["fighterFuel"]["combatFraction"].get<double>());
 double hull=u["hull"],fighter=c["hull"];int cannon=aircraftModule(ci,"primary").empty()?0:c["ammo"].get<int>(),missiles=aircraftModule(ci,"secondary").empty()?0:c["missiles"].get<int>();
 for(int tick=0;tick<2000&&hull>0&&fighter>0;++tick){if(cannon>0){--cannon;if(rng()%100<55)hull-=.18;}if(tick%25==0&&missiles>0){--missiles;if(rng()%100<72)hull-=12;}if(u["type"]=="SINGLE_SEAT_CIRCULAR"&&tick%6==0&&rng()%100>=8)fighter-=4.5;if(cannon==0&&missiles==0)break;}
 if(!aircraftModule(ci,"primary").empty())c["ammo"]=cannon;if(!aircraftModule(ci,"secondary").empty())c["missiles"]=missiles;c["hull"]=std::max(0.,fighter);c["fuel"]=std::max(0.,c["fuel"].get<double>()-fuelUsed);c["missions"]=c["missions"].get<int>()+1;
 if(hull<=0){log("UFO DOWN: native air-combat resolution");recordUfoCrashSite(u);state["ufos"].erase(ui);if(fighter<=0){damageAircraft(ci,1,"UFO");return {true,"Fighter destroyed; UFO also destroyed"};}return {true,"UFO destroyed; crash site recorded for satellite survey"};}u["hull"]=hull;if(fighter<=0){damageAircraft(ci,1,"UFO");return {true,"Aircraft destroyed and removed from fleet"};}return {true,"Fighter withdrew"};
}
double Engine::storageUnitLitres(const std::string& id)const{
 const auto* item=record("equipment",id);if(!item)item=record("units",id);if(item&&item->contains("storageLitres"))return (*item)["storageLitres"];
 const auto dimensions=readJson(dataDir/"storage-dimensions.json");std::string key=id=="iron"?"metals":id;
 if(!dimensions["dimensionsCm"].contains(key))return 0.;const auto& size=dimensions["dimensionsCm"][key];return size[0].get<double>()*size[1].get<double>()*size[2].get<double>()/1000.;
}
Json Engine::storageCapacity()const{
 const auto data=readJson(dataDir/"storage-dimensions.json");double pallet=data.value("palletLitres",1200.),used=0.,uranium=0.;int capacity=0,uraniumCapacity=0;
 for(const auto& building:facilityBuildings()){
  if(building["id"]=="storage")capacity+=40;else if(building["id"]=="uraniumStorage")uraniumCapacity+=10;
 }
 for(const auto& item:state["stock"].items()){
  std::string key=item.key()=="iron"?"metals":item.key();if(!data["dimensionsCm"].contains(key))continue;const auto& size=data["dimensionsCm"][key];double volume=size[0].get<double>()*size[1].get<double>()*size[2].get<double>()/1000.;
  if(item.key()=="uranium")uranium+=item.value().get<double>()*volume/pallet;else used+=item.value().get<double>()*volume/pallet;
 }
 return {{"used",used},{"capacity",capacity},{"uraniumUsed",uranium},{"uraniumCapacity",uraniumCapacity}};
}
Json Engine::countrySupport()const{
 Json result=Json::array();int progress=28+std::min(20,int(state["bases"].size())*7)+std::min(10,int(state["craft"].size())*3)+(state["money"].get<long long>()>3000000?8:5);
 int radar=0;for(const auto& b:state["bases"])for(const auto& item:b["buildings"])if((item["id"]=="radar"||item["id"]=="radarLarge")&&item["ready"].get<double>()<=state["hours"].get<double>())radar++;
 progress=std::min(96,progress+std::min(18,radar*2));
 for(const auto& country:constants["COUNTRY_FINANCE_FALLBACK"]){int tier=1;for(const auto& [key,t]:{std::pair{"highest",5},{"high",4},{"middle",3}})for(const auto& n:constants["COUNTRY_ECONOMIC_PROFILES"][key])if(n==country)tier=t;const int base[]={0,1200,5000,13000,30000,65000};long long contribution=std::llround(base[tier]*(.62+progress*.0034)/100)*100;result.push_back({{"name",country},{"approval",progress},{"contribution",contribution}});}
 if(selectedGameset.is_object()&&gameset().contains("finance")){
  Json configured=Json::array();for(const auto& country:gameset()["finance"]["countries"]){int tier=country["tier"],approval=country.contains("approval")&&!country["approval"].is_null()?country["approval"].get<int>():progress;const int base[]={0,1200,5000,13000,30000,65000};long long amount=country.contains("contribution")&&!country["contribution"].is_null()?country["contribution"].get<long long>():std::llround(base[tier]*(.62+approval*.0034)/100)*100;configured.push_back({{"name",country["name"]},{"approval",approval},{"contribution",amount}});}result=std::move(configured);
 }

 return result;
}
void Engine::advance(double hours){
 if(!std::isfinite(hours)||hours<=0||hours>24*366||state["bases"].empty())return;
 removeDestroyedAircraft();double old=state["hours"],now=old+hours,boundary=now;if(state.contains("leases"))for(const auto& lease:state["leases"])if(lease.is_number()){double end=lease.get<double>();if(end>old+1e-8&&end<boundary-1e-8)boundary=end;}if(boundary<now-1e-8){advance(boundary-old);advance(now-boundary);return;}enforceWorkplaceCapacity();facilityTime=old;state["hours"]=now;
 if(state.contains("nextWorkforceRefresh")&&now>=state["nextWorkforceRefresh"].get<double>()){state.erase("offers");ensureWorkforce();state["nextWorkforceRefresh"]=(std::floor(now/168)+1)*168;log("WORKFORCE MARKET UPDATED");}
 for(auto& base:state["bases"])for(auto& b:base["buildings"])if(b["ready"].get<double>()>old&&b["ready"].get<double>()<=now)log("COMPLETE: "+b["id"].get<std::string>());
 if(facility("research")){auto schedule=researchSchedule();std::stable_sort(schedule.begin(),schedule.end(),[](const Json& a,const Json& b){return a["startInHours"].is_null()?false:b["startInHours"].is_null()?true:a["startInHours"].get<double>()<b["startInHours"].get<double>();});for(const auto& row:schedule){if(row["startInHours"].is_null()||row["startInHours"].get<double>()>hours)continue;std::string id=row["id"];if(row["assigned"].get<int>()==0)for(const auto& person:row["team"])assignScientist(id,person.get<std::size_t>(),true);const auto* design=record("technologies",id);double work=std::max(0.,hours-row["startInHours"].get<double>());double progress=state["research"].value(id,0.)+work*row["team"].size()/(std::max(1,design->value("researchScientists",10))*std::max(1.,design->value("researchHours",72.)));state["research"][id]=std::min(1.,progress);if(progress>=1)log("RESEARCH COMPLETE: "+id);}}
 processProduction(old,hours);
 for(int w=int(old/168)+1;w<=int(now/168);++w){long long total=0;for(const auto& row:countrySupport())total+=row["contribution"].get<long long>();state["money"]=state["money"].get<long long>()+total;log("WEEKLY SUPPORT: "+std::to_string(total));}
 for(int m=int(old/720)+1;m<=int(now/720);++m){long long total=0;for(const auto& p:state["personnel"])total+=p["salary"].get<long long>();if(state.contains("astronauts"))for(const auto& p:state["astronauts"])total+=p["salary"].get<long long>();state["money"]=state["money"].get<long long>()-total;log("PAYROLL: "+std::to_string(total));}
 processGymTraining(now);
 processAircraftService(now);
 processISS(old,now);processSpace(now);
 while(state["nextUfo"].get<double>()<=now){double spawn=state["nextUfo"];auto type=rng()%2==0?"UNMANNED_DRONE":"SINGLE_SEAT_CIRCULAR";double lat=double(int(rng()%140)-70),lon=double(int(rng()%360)-180);state["ufos"].push_back({{"type",type},{"lat",lat},{"lon",lon},{"hull",100.0},{"spawned",spawn},{"environment","AIR"},{"expires",spawn+constants["ufoLifecycle"]["maxStayHours"].get<double>()}});state["nextUfo"]=spawn+24*(1+int(rng()%10));log("UFO CONTACT");}
 if(state.contains("radarOutposts"))for(const auto& site:state["radarOutposts"])if(site["ready"].get<double>()>old&&site["ready"].get<double>()<=now)log("COMPLETE: "+site["name"].get<std::string>());
 processUfoLandings(old,now);processFreightThreats(old,now);processFreight(now);processSatelliteScans(old,now);processRadarContacts(old,now);
 if(state.contains("ufoSites"))for(std::size_t i=0;i<state["ufoSites"].size();)if(state["ufoSites"][i]["expires"].get<double>()<=now)state["ufoSites"].erase(i);else ++i;
 for(std::size_t i=0;i<state["ufos"].size();)if(state["ufos"][i]["expires"].get<double>()<=now){log("UFO escaped");state["ufos"].erase(i);}else ++i;
 facilityTime=-1;enforceWorkplaceCapacity();
}
std::string Engine::date()const{
 const auto total=static_cast<long long>(std::floor(std::max(0.,state["hours"].get<double>())*3600));
 // Civil calendar from epoch days; 2042-01-01 is day 26298 after 1970-01-01.
 const long long z=total/86400+26298+719468,era=z/146097;
 const unsigned doe=unsigned(z-era*146097),yoe=(doe-doe/1460+doe/36524-doe/146096)/365;
 long long year=yoe+era*400;const unsigned doy=doe-(365*yoe+yoe/4-yoe/100),mp=(5*doy+2)/153;
 const unsigned day=doy-(153*mp+2)/5+1;const int month=int(mp)+(mp<10?3:-9);year+=month<=2;
 std::ostringstream out;out<<std::setfill('0')<<std::setw(4)<<year<<"-"<<std::setw(2)<<month<<"-"<<std::setw(2)<<day<<" "<<std::setw(2)<<(total/3600)%24<<":"<<std::setw(2)<<(total/60)%60<<":"<<std::setw(2)<<total%60;return out.str();
}
void Engine::save(int slot)const{if(slot<1||slot>5)throw std::runtime_error("Invalid save slot");auto j=state;std::ostringstream stream;stream<<rng;j["rng"]=stream.str();writeJson(userDir/("save-"+std::to_string(slot)+".json"),j);}
Result Engine::load(int slot){
 if(slot<1||slot>5)return {false,"Invalid save slot"};try{
  auto j=readJson(userDir/("save-"+std::to_string(slot)+".json"));
  if(j.value("schema","")!="uaa-native-save-v1")return {false,"Unsupported save format"};
  Json previousSet=selectedGameset;bool committedSet=false;
  struct SetRollback {Json& target;Json previous;bool& committed;~SetRollback(){if(!committed)target=std::move(previous);}} rollback{selectedGameset,previousSet,committedSet};
  if(j.contains("gamesetSnapshot")){auto valid=validateGameset(j["gamesetSnapshot"],true);if(!valid.ok)return valid;selectedGameset=j["gamesetSnapshot"];}else selectedGameset=nullptr;
  if(!j["hours"].is_number()||!std::isfinite(j["hours"].get<double>())||j["hours"].get<double>()<0||!j["money"].is_number_integer())return {false,"Invalid campaign values"};
  for(const auto& key:{"bases","production","personnel","craft","ufos","events"})if(!j[key].is_array())return {false,"Invalid save collection"};
  if(j.contains("researchQueue")){if(!j["researchQueue"].is_array())return {false,"Invalid research queue"};std::set<std::string> seen;for(const auto& id:j["researchQueue"])if(!id.is_string()||!record("technologies",id.get<std::string>())||!j["research"].contains(id.get<std::string>())||!seen.insert(id.get<std::string>()).second)return {false,"Invalid queued research project"};}
  if(j.contains("productionFinished")){if(!j["productionFinished"].is_object())return {false,"Invalid production progress"};for(const auto& value:j["productionFinished"])if(!value.is_number_integer()||value.get<long long>()<0||value.get<long long>()>1000000000)return {false,"Invalid production progress count"};}
  if(j.contains("pendingProductionCompletions")&&(!j["pendingProductionCompletions"].is_number_integer()||j["pendingProductionCompletions"].get<long long>()<0||j["pendingProductionCompletions"].get<long long>()>2147483647))return {false,"Invalid completion notification count"};
  for(const auto& key:{"research","stock","corpses","wrecks","liveAliens"})if(!j[key].is_object())return {false,"Invalid save table"};
  for(const auto& b:j["bases"]){if(!b["name"].is_string()||!b["lat"].is_number()||!b["lon"].is_number()||std::abs(b["lat"].get<double>())>90||std::abs(b["lon"].get<double>())>180||!b["floors"].is_number_integer()||b["floors"].get<int>()<0||b["floors"].get<int>()>4||!b["buildings"].is_array())return {false,"Invalid base"};for(const auto& item:b["buildings"]){if(!item["id"].is_string()||!item["x"].is_number_integer()||!item["y"].is_number_integer()||!item["floor"].is_number_integer()||!item["ready"].is_number())return {false,"Invalid building"};auto [w,h]=footprint(item["id"]);if(item["x"].get<int>()<0||item["y"].get<int>()<0||item["x"].get<int>()+w>10||item["y"].get<int>()+h>10)return {false,"Building outside grid"};}}
  for(const auto& order:j["production"]){
   if(!order.is_object()||!order.contains("id")||!order["id"].is_string()||!order.contains("ready")||!order["ready"].is_number()||!std::isfinite(order["ready"].get<double>())||order["ready"].get<double>()<0)return {false,"Invalid production order"};
   if(order.contains("started")&&(!order["started"].is_number()||!std::isfinite(order["started"].get<double>())||order["started"].get<double>()<0||order["started"].get<double>()>order["ready"].get<double>()))return {false,"Invalid production schedule"};
   if(order.contains("remaining")&&(!order["remaining"].is_number()||!std::isfinite(order["remaining"].get<double>())||order["remaining"].get<double>()<0))return {false,"Invalid remaining production time"};
   if(order.contains("group")&&(!order["group"].is_string()||!record(order["group"],order["id"])))return {false,"Invalid production design"};
  }
  for(const auto& value:j["research"].items())if(!value.value().is_number()||value.value().get<double>()<0||value.value().get<double>()>1||!record("technologies",value.key()))return {false,"Invalid research progress"};
  if(j.contains("researchStaff")){
   if(!j["researchStaff"].is_object())return {false,"Invalid research staffing"};std::set<int> activeScientists;
   for(const auto& team:j["researchStaff"].items()){
    if(!j["research"].contains(team.key())||!team.value().is_array())return {false,"Invalid research team"};std::set<int> unique;
    for(const auto& person:team.value()){
     if(!person.is_number_integer()||person.get<long long>()<0||person.get<long long>()>=static_cast<long long>(j["personnel"].size()))return {false,"Invalid scientist index"};int index=person.get<int>();
     if(j["personnel"][index].value("role",std::string(""))!="scientist"||!unique.insert(index).second)return {false,"Invalid scientist assignment"};
     if(j["research"][team.key()].get<double>()<1&&!activeScientists.insert(index).second)return {false,"Scientist assigned to multiple active projects"};
    }
   }
  }
  if(j.contains("productionStaff")){
   if(!j["productionStaff"].is_object())return {false,"Invalid production teams"};std::set<int> used;
   for(const auto& team:j["productionStaff"].items()){auto colon=team.key().find(':');if(colon==std::string::npos||!record(team.key().substr(0,colon),team.key().substr(colon+1))||!team.value().is_array())return {false,"Invalid production team"};for(const auto& person:team.value())if(!person.is_number_integer()||person.get<long long>()<0||person.get<long long>()>=static_cast<long long>(j["personnel"].size())||j["personnel"][person.get<std::size_t>()].value("role",std::string(""))!="technician"||!used.insert(person.get<int>()).second)return {false,"Invalid technician assignment"};}
  }
  for(const auto& value:j["stock"].items())if(!value.value().is_number_integer()||value.value().get<long long>()<0||value.value().get<long long>()>1000000000)return {false,"Invalid stock"};
  auto next=j["nextUfo"].get<double>();if(!std::isfinite(next)||next<j["hours"].get<double>())return {false,"Invalid UFO schedule"};
  std::mt19937 restored;std::istringstream stream(j.at("rng").get<std::string>());if(!(stream>>restored))return {false,"Invalid random state"};
  if(j.contains("acknowledgedUfoContacts")){if(!j["acknowledgedUfoContacts"].is_array()||j["acknowledgedUfoContacts"].size()>100000)return {false,"Invalid acknowledged UFO contacts"};for(const auto& id:j["acknowledgedUfoContacts"])if(!id.is_string()||id.get<std::string>().size()>512)return {false,"Invalid acknowledged UFO contact"};}
  if(j.contains("aircraftLosses")){if(!j["aircraftLosses"].is_array())return {false,"Invalid aircraft losses"};for(const auto& loss:j["aircraftLosses"])if(!loss.is_object()||!loss.contains("craft")||!loss["craft"].is_object()||!loss.contains("lostAt")||!loss["lostAt"].is_number()||!std::isfinite(loss["lostAt"].get<double>())||loss["lostAt"].get<double>()<0||loss["lostAt"].get<double>()>j["hours"].get<double>()||!loss.contains("casualties")||!loss["casualties"].is_array())return {false,"Invalid aircraft loss record"};}
  if(!validFreight(j))return {false,"Invalid cargo flights"};
  if(!validSatelliteSurvey(j))return {false,"Invalid satellite survey"};
  auto personnelValid=validatePersonnelSave(j);if(!personnelValid.ok)return personnelValid;
  if(j.contains("offers")){
   if(!j["offers"].is_array()||j["offers"].size()>100)return {false,"Invalid workforce market"};
   Json candidates=j;candidates["personnel"]=j["offers"];candidates["craft"]=Json::array();auto valid=validatePersonnelSave(candidates);if(!valid.ok)return valid;
   for(const auto& offer:j["offers"])if(!offer.contains("quality")||!offer["quality"].is_number_integer()||offer["quality"].get<int>()<0||offer["quality"].get<int>()>100||!offer.contains("tier")||!offer["tier"].is_string()||!offer.contains("attributes"))return {false,"Invalid candidate"};
  }
  if(j.contains("nextWorkforceRefresh")&&(!j["nextWorkforceRefresh"].is_number()||!std::isfinite(j["nextWorkforceRefresh"].get<double>())))return {false,"Invalid workforce refresh"};
  if(j.contains("leases")){if(!j["leases"].is_object())return {false,"Invalid leases"};for(const auto& lease:j["leases"].items())if((lease.key()!="nasa"&&lease.key()!="esa"&&lease.key()!="roscosmos")||!lease.value().is_number()||!std::isfinite(lease.value().get<double>())||lease.value().get<double>()<0)return {false,"Invalid lease"};}
  for(const auto& key:{"satellites","launchQueue"})if(j.contains(key)){
   if(!j[key].is_array()||j[key].size()>10000)return {false,"Invalid space operations"};
   for(const auto& item:j[key]){
    if(!item.is_object()||!item.contains("type")||!item["type"].is_string()||(item["type"]!="EARTH"&&item["type"]!="AIR"&&item["type"]!="WATER"&&(std::string(key)!="launchQueue"||(item["type"]!="CREW"&&item["type"]!="CARGO"))))return {false,"Invalid satellite type"};
    for(const auto& field:{"altitude","fuel","ready"})if(!item.contains(field)||!item[field].is_number()||!std::isfinite(item[field].get<double>())||item[field].get<double>()<0)return {false,"Invalid satellite value"};
    if(item["altitude"].get<double>()<200||item["altitude"].get<double>()>1500||item["fuel"].get<double>()>100)return {false,"Invalid orbit"};
    if(std::string(key)=="satellites"&&(!item.contains("launchTime")||!item["launchTime"].is_number()||!std::isfinite(item["launchTime"].get<double>())||item["launchTime"].get<double>()<0))return {false,"Invalid launch time"};
   }
  }
  for(const auto& key:{"astronauts","spaceOffers"})if(j.contains(key)){
   if(!j[key].is_array()||j[key].size()>1000)return {false,"Invalid astronaut collection"};
   for(const auto& person:j[key]){
    if(!person.is_object()||!person.contains("name")||!person["name"].is_string()||!person.contains("role")||!person["role"].is_string())return {false,"Invalid astronaut"};
    std::string role=person["role"];if(role!="commander"&&role!="navigator"&&role!="technician"&&role!="researcher")return {false,"Invalid astronaut profession"};
    for(const auto& field:{"salary","quality","tierIndex"})if(!person.contains(field)||!person[field].is_number_integer()||person[field].get<long long>()<0)return {false,"Invalid astronaut value"};
    if(!person.contains("tier")||!person["tier"].is_string()||person["tierIndex"].get<int>()>5||person["quality"].get<int>()>100||!person.contains("attributes")||!person["attributes"].is_object())return {false,"Invalid astronaut skills"};
    for(const auto& a:person["attributes"])if(!a.is_number_integer()||a.get<int>()<0||a.get<int>()>100)return {false,"Invalid astronaut skill"};
   }
  }
  if(j.contains("spaceHousing")){if(!j["spaceHousing"].is_array())return {false,"Invalid space housing"};for(const auto& ready:j["spaceHousing"])if(!ready.is_number()||!std::isfinite(ready.get<double>())||ready.get<double>()<0)return {false,"Invalid housing completion"};}
  if(j.contains("flightCrew")){if(!j["flightCrew"].is_array()||j["flightCrew"].size()>8||!j.contains("astronauts"))return {false,"Invalid space crew"};std::set<int> unique;for(const auto& index:j["flightCrew"])if(!index.is_number_integer()||index.get<long long>()<0||index.get<long long>()>=static_cast<long long>(j["astronauts"].size())||!unique.insert(index.get<int>()).second)return {false,"Invalid space crew assignment"};}
  if(j.contains("transports")){if(!j["transports"].is_array())return {false,"Invalid ISS flights"};for(const auto& flight:j["transports"]){if(!flight.is_object()||!flight.contains("type")||(flight["type"]!="CREW"&&flight["type"]!="CARGO")||!flight.contains("delivered")||!flight["delivered"].is_boolean()||!flight.contains("cargoUnits")||!flight["cargoUnits"].is_number_integer()||flight["cargoUnits"].get<int>()<1||flight["cargoUnits"].get<int>()>3)return {false,"Invalid ISS transport"};for(const auto& field:{"ready","arrival"})if(!flight.contains(field)||!flight[field].is_number()||!std::isfinite(flight[field].get<double>())||flight[field].get<double>()<0)return {false,"Invalid ISS flight time"};}}
  if(j.contains("issCargo")&&(!j["issCargo"].is_number_integer()||j["issCargo"].get<long long>()<0))return {false,"Invalid ISS cargo"};
  if(j.contains("issLease")&&(!j["issLease"].is_number()||!std::isfinite(j["issLease"].get<double>())||j["issLease"].get<double>()<0))return {false,"Invalid ISS lease"};
  if(j.contains("iss")&&!validISSState(j["iss"],j["hours"]))return {false,"Invalid ISS station state"};
  if(!j.contains("radarOutposts"))j["radarOutposts"]=Json::array();state=std::move(j);rng=restored;normalizeUfoLifecycle();removeDestroyedAircraft();for(std::size_t i=0;i<state["ufos"].size();)if(state["ufos"][i]["expires"].get<double>()<=state["hours"].get<double>())state["ufos"].erase(i);else ++i;state["issPermanentCrew"]=permanentISSCrew();ensureISS();
  // Old native saves had one shared skill value and no individual attributes.
  for(auto& person:state["personnel"])if(!person.contains("attributes")){
   auto generated=makeCandidate(person["role"],0);int previousHealth=person.value("health",100);
   person["attributes"]=generated["attributes"];person["skill"]=generated["skill"];person["maxHealth"]=generated["maxHealth"];
   if(person["role"]=="soldier")person["health"]=int(std::round(previousHealth/100.*generated["maxHealth"].get<int>()));
   if(!person.contains("portraitGender"))person["portraitGender"]=generated["portraitGender"];if(!person.contains("portraitType"))person["portraitType"]=generated["portraitType"];
  }
  committedSet=true;return {true,"Campaign loaded"};
 }catch(const std::exception& e){return {false,e.what()};}
}
}
