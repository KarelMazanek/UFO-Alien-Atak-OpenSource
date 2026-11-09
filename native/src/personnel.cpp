#include "engine.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <set>
#include <unordered_set>
namespace uaa {
namespace {
Json emptyLoadout(){return {{"primaryWeapon",""},{"secondaryWeapon",""},{"armor",{{"helmet",""},{"torso",""},{"arms",""},{"legs",""}}},{"equipmentBelt",""},{"beltSlots",Json::array({"","","",""})},{"backpack",Json::array()},{"medicalKit",""},{"radiationSuit",""},{"divingSuit",""},{"rebreather",""},{"divingHood",""}};}
bool oneOf(const std::string& id,std::initializer_list<const char*> ids){return std::any_of(ids.begin(),ids.end(),[&](const char* v){return id==v;});}
}
Json Engine::makeCandidate(const std::string& role,int tier){
 static const char* names[]={"ALEX MORGAN","JAMIE PARK","TAYLOR REED","RILEY SANTOS","CASEY NOVAK","JORDAN BLAKE","SAMIR KHAN","ELENA PETROVA","MORGAN LEE","DREW HAYES","NIKITA VOLKOV","MAYA CHEN","NOAH WILLIAMS","LEO MARTIN","AMINA DIALLO","SOFIA RUIZ","ETHAN CLARKE","LINA HADDAD","MARCUS REED","ANIKA SHAH"};
 static const int qualities[]={42,58,76,92};static const char* tiers[]={"ENTRY","STANDARD","EXPERIENCED","ELITE"};tier=std::clamp(tier,0,3);int quality=qualities[tier];
 Json ranges;int lo=3000,hi=6000;
 if(role=="soldier")ranges={{"MARKSMANSHIP",{20,50}},{"LEADERSHIP",{20,50}},{"FITNESS",{20,50}},{"COMMAND",{20,50}},{"TIME_UNITS",{20,50}},{"STAMINA",{20,50}},{"HEALTH",{20,50}},{"REACTIONS",{20,50}},{"BRAVERY",{20,50}},{"THROWING_ACCURACY",{20,50}},{"STRENGTH",{20,50}},{"MEDICAL",{20,50}}};
 else if(role=="technician"){ranges={{"ENGINEERING",{35,98}},{"OPERATIONS",{30,95}},{"MAINTENANCE",{35,98}}};lo=4000;hi=8000;}
 else if(role=="scientist"){ranges={{"RESEARCH",{40,99}},{"ANALYSIS",{35,98}},{"FIELDWORK",{25,90}}};lo=5500;hi=10000;}
 else if(role=="pilot"){ranges={{"PILOTING",{40,95}},{"ACCURACY",{35,92}},{"EVASION",{30,90}}};lo=12000;hi=24000;}
 else if(role=="navigator"){ranges={{"NAVIGATION",{40,96}},{"AWARENESS",{35,92}},{"COORDINATION",{35,90}}};lo=10000;hi=20000;}
 else throw std::runtime_error("Unknown staff role");
 Json attributes=Json::object();int total=0;
 for(auto it=ranges.begin();it!=ranges.end();++it){int low,high;if(role=="soldier"){int center=int(std::round(24+(quality-42)*.4));low=std::max(20,center-8);high=std::min(50,center+8);}else{low=std::max(it.value()[0].get<int>(),quality-9);high=std::max(low,std::min(it.value()[1].get<int>(),quality+9));}int value=std::uniform_int_distribution<int>(low,high)(rng);attributes[it.key()]=value;total+=value;}
 int salary=int(std::round((lo+std::clamp((quality-40)/52.,0.,1.)*(hi-lo))/100.))*100;
 return {{"role",role},{"name",names[rng()%20]},{"tier",tiers[tier]},{"quality",quality},{"salary",salary},{"attributes",attributes},{"skill",total/int(attributes.size())},{"health",attributes.value("HEALTH",100)},{"maxHealth",attributes.value("HEALTH",100)},{"portraitGender",rng()%2?"male":"female"},{"portraitType",rng()%5},{"missions",0}};
}
void Engine::ensureWorkforce(){
 if(state.contains("offers"))return;state["nextWorkforceRefresh"]=state["hours"].get<double>()+168;state["offers"]=Json::array();
 for(const auto& role:{"soldier","technician","scientist","pilot","navigator"})for(int tier=0;tier<4;++tier)state["offers"].push_back(makeCandidate(role,tier));
}
Result Engine::hireCandidate(std::size_t candidate){
 ensureWorkforce();if(candidate>=state["offers"].size())return {false,"Select a candidate"};
 auto person=state["offers"][candidate];long long cost=person["role"]=="soldier"?person.value("quality",42)*1000LL:0;
 if(state["money"].get<long long>()<cost)return {false,"Insufficient funds"};
 state["money"]=state["money"].get<long long>()-cost;state["personnel"].push_back(person);state["offers"].erase(candidate);return {true,"Candidate recruited"};
}
Json Engine::soldierLoadout(std::size_t person)const{
 auto result=emptyLoadout();if(person>=state["personnel"].size())return result;
 const auto& p=state["personnel"][person];if(!p.contains("loadout"))return result;
 const auto& saved=p["loadout"];for(auto it=result.begin();it!=result.end();++it)if(saved.contains(it.key()))it.value()=saved[it.key()];return result;
}
int Engine::soldierItemSize(const std::string& id)const{
 if(oneOf(id,{"pistol-ammo","rifle-ammo","fragmentation-grenade","flash-grenade","field-aid"}))return 1;
 if(oneOf(id,{"rifle","rocket-ammo","incendiary-rocket"}))return 3;
 if(oneOf(id,{"machine-gun","rocket-launcher"}))return 4;return 2;
}
bool Engine::soldierItemAllowed(const std::string& slot,const std::string& id)const{
 if(id.empty())return true;
 if(!record("weapons",id)&&!record("equipment",id))return false;
 if(slot=="primaryWeapon"||slot=="secondaryWeapon")return oneOf(id,{"pistol","rifle","machine-gun","rocket-launcher"});
 if(slot=="helmet")return oneOf(id,{"helmet","combat-helmet"});
 if(slot=="torso")return oneOf(id,{"armor-vest","reinforced-vest"});
 if(slot=="arms")return id=="arm-armor";if(slot=="legs")return id=="leg-armor";
 if(slot=="equipmentBelt")return id=="four-slot-belt";if(slot=="radiationSuit")return id=="radiation-suit";
 if(slot=="divingSuit")return id=="diving-suit";if(slot=="rebreather")return id=="rebreather";if(slot=="divingHood")return id=="diving-hood";
 if(slot=="medicalKit")return oneOf(id,{"field-aid","medikit"});
 if(slot=="beltSlots")return oneOf(id,{"pistol-ammo","rifle-ammo","fragmentation-grenade","flash-grenade","field-aid"})&&soldierItemSize(id)==1;
 if(slot=="backpack")return oneOf(id,{"pistol","rifle","machine-gun","rocket-launcher","pistol-ammo","rifle-ammo","machine-gun-ammo","rocket-ammo","incendiary-rocket","fragmentation-grenade","flash-grenade","anti-personnel-mine","field-aid","medikit","four-slot-belt"});
 return false;
}
Json Engine::backpackPacking(const Json& items)const{
 // Same area, rotations and backtracking as soldierBackpackPacking in the HTML.
 if(!items.is_array()||items.size()>20)return nullptr;
 struct Shape{int original,w,h;};std::vector<Shape> shapes;int area=0;
 for(std::size_t i=0;i<items.size();++i){if(!items[i].is_string())return nullptr;std::string id=items[i];int w=1,h=soldierItemSize(id);
  if(id=="rifle")h=5;else if(id=="machine-gun"){w=2;h=5;}else if(id=="rocket-launcher"){w=2;h=4;}else if(oneOf(id,{"machine-gun-ammo","medikit","anti-personnel-mine"})){w=2;h=1;}
  area+=w*h;shapes.push_back({int(i),w,h});}
 if(area>20)return nullptr;
 std::stable_sort(shapes.begin(),shapes.end(),[](auto a,auto b){return a.w*a.h>b.w*b.h;});
 std::array<int,20> cells;cells.fill(-1);std::unordered_set<unsigned long long> failed;
 std::function<bool(std::size_t,unsigned)> pack=[&](std::size_t n,unsigned occupied){if(n==shapes.size())return true;auto key=(static_cast<unsigned long long>(n)<<20)|occupied;if(failed.count(key))return false;auto s=shapes[n];
  for(int rotate=0;rotate<(s.w==s.h?1:2);++rotate){int w=rotate?s.h:s.w,h=rotate?s.w:s.h;for(int y=0;y<=4-h;++y)for(int x=0;x<=5-w;++x){unsigned mask=0;for(int dy=0;dy<h;++dy)for(int dx=0;dx<w;++dx)mask|=1u<<((y+dy)*5+x+dx);if(mask&occupied)continue;for(int i=0;i<20;++i)if(mask&(1u<<i))cells[i]=s.original;if(pack(n+1,occupied|mask))return true;for(int i=0;i<20;++i)if(mask&(1u<<i))cells[i]=-1;}}
  failed.insert(key);return false;};
 if(!pack(0,0))return nullptr;Json result=Json::array();for(int i:cells)result.push_back(i);return result;
}
double Engine::loadoutWeight(const Json& loadout)const{
 auto weight=[this](const Json& value){if(!value.is_string()||value.get<std::string>().empty())return 0.;auto id=value.get<std::string>();const auto* r=record("weapons",id);if(!r)r=record("equipment",id);if(!r)return 1.;
  if(r->contains("combat")&&(*r)["combat"].contains("values")&&(*r)["combat"]["values"].contains("weight"))return (*r)["combat"]["values"]["weight"].get<double>();return 1.;};
 double total=0;for(auto key:{"primaryWeapon","secondaryWeapon","equipmentBelt","medicalKit","radiationSuit","divingSuit","rebreather","divingHood"})if(loadout.contains(key))total+=weight(loadout.at(key));for(const auto& value:loadout.at("armor"))total+=weight(value);for(const auto& value:loadout.at("beltSlots"))total+=weight(value);for(const auto& value:loadout.at("backpack"))total+=weight(value);return total;
}
double Engine::soldierCapacity(std::size_t person)const{if(person>=state["personnel"].size())return 0.;const auto& p=state["personnel"][person];int strength=p.value("skill",50);if(p.contains("attributes"))strength=p["attributes"].value("STRENGTH",p["attributes"].value("FITNESS",strength));return 15.+std::clamp(strength,0,100)*.25;}
Result Engine::equipSoldier(std::size_t person,const std::string& slot,const std::string& id,int itemIndex){
 if(person>=state["personnel"].size()||state["personnel"][person].value("role","")!="soldier"||state["personnel"][person].value("dead",false))return {false,"Select a soldier"};
 if(!oneOf(slot,{"primaryWeapon","secondaryWeapon","helmet","torso","arms","legs","equipmentBelt","beltSlots","backpack","medicalKit","radiationSuit","divingSuit","rebreather","divingHood"})||!soldierItemAllowed(slot,id))return {false,"Item is incompatible with this slot"};
 for(const auto& craft:state["craft"])if(craft.value("operationState",std::string("BASE"))=="DEPLOYED"&&craft.contains("squad")&&std::find(craft["squad"].begin(),craft["squad"].end(),Json(person))!=craft["squad"].end())return {false,"Soldier is on an operation"};
 auto next=soldierLoadout(person);std::string previous;
 if(slot=="backpack"){if(id.empty()){if(itemIndex<0||itemIndex>=int(next["backpack"].size()))return {false,"Select a backpack item"};previous=next["backpack"][itemIndex];next["backpack"].erase(itemIndex);}else next["backpack"].push_back(id);}
 else if(slot=="beltSlots"){if(itemIndex<0||itemIndex>=4)return {false,"Invalid belt slot"};previous=next["beltSlots"][itemIndex];next["beltSlots"][itemIndex]=id;}
 else if(oneOf(slot,{"helmet","torso","arms","legs"})){previous=next["armor"][slot];next["armor"][slot]=id;}
 else{previous=next[slot];next[slot]=id;}
 if(previous==id&&slot!="backpack")return {true,"Loadout unchanged"};
 if(!id.empty()&&state["stock"].value(id,0)<1)return {false,"Item is not available in storage"};
 if(!id.empty()&&backpackPacking(next["backpack"]).is_null())return {false,"BACKPACK FULL · ITEM DOES NOT FIT IN 5 × 4"};
 if(!id.empty()&&loadoutWeight(next)>soldierCapacity(person)+.00001)return {false,"COMBAT LOAD EXCEEDED"};
 // Commit after all validation, so a rejected choice cannot lose equipment.
 if(!previous.empty())state["stock"][previous]=state["stock"].value(previous,0)+1;
 if(!id.empty())state["stock"][id]=state["stock"].value(id,0)-1;
 state["personnel"][person]["loadout"]=std::move(next);return {true,"Soldier equipment updated"};
}
Result Engine::startGymTraining(std::size_t person){
 if(person>=state["personnel"].size()||state["personnel"][person]["role"]!="soldier")return {false,"Select a soldier"};if(!facility("gym"))return {false,"A completed gym is required"};auto& p=state["personnel"][person];if(p.contains("gymTraining"))return {false,"Training already active"};if(p.value("health",0)<=0)return {false,"Soldier is unfit for training"};bool improvable=false;for(auto key:{"STRENGTH","FITNESS","STAMINA","REACTIONS","TIME_UNITS"})if(p["attributes"].value(key,0)<100)improvable=true;if(!improvable)return {false,"Physical attributes are at maximum"};double now=state["hours"];p["gymTraining"]={{"started",now},{"ready",now+48.}};return {true,"Gym training started"};
}
Result Engine::cancelGymTraining(std::size_t person){if(person>=state["personnel"].size()||!state["personnel"][person].contains("gymTraining"))return {false,"No active training"};state["personnel"][person].erase("gymTraining");return {true,"Gym training cancelled"};}
void Engine::processGymTraining(double hours){if(!facility("gym"))return;for(auto& p:state["personnel"])if(p.contains("gymTraining")&&p["gymTraining"]["ready"].get<double>()<=hours){for(auto key:{"STRENGTH","FITNESS","STAMINA","REACTIONS","TIME_UNITS"})p["attributes"][key]=std::min(100,p["attributes"].value(key,0)+2);int total=0;for(const auto& value:p["attributes"])total+=value.get<int>();p["skill"]=total/int(p["attributes"].size());p.erase("gymTraining");log("GYM TRAINING COMPLETE: "+p["name"].get<std::string>());}}
Result Engine::renamePerson(std::size_t person,const std::string& name){if(person>=state["personnel"].size()||name.empty()||name.size()>80)return {false,"Invalid name"};state["personnel"][person]["name"]=name;return {true,"Name saved"};}
Result Engine::assignCrew(std::size_t craft,const std::string& role,int person){
 if(craft>=state["craft"].size()||!oneOf(role,{"pilot","navigator","technician"}))return {false,"Invalid aircraft or crew role"};
 if(state["craft"][craft].value("operationState",std::string("BASE"))=="DEPLOYED")return {false,"Transport is on an operation"};
 if(person< -1||person>=int(state["personnel"].size()))return {false,"Invalid crew member"};
 if(person>=0&&state["personnel"][person].value("role","")!=role)return {false,"Crew profession does not match the slot"};
 auto& c=state["craft"][craft];if(!c.contains("crew"))c["crew"]={{"pilot",-1},{"navigator",-1},{"technicians",Json::array()}};
 if(role=="technician"){if(person<0)c["crew"]["technicians"]=Json::array();else{auto& list=c["crew"]["technicians"];if(std::find(list.begin(),list.end(),person)!=list.end())return {true,"Technician already assigned"};if(list.size()>=6)return {false,"Maximum six technicians"};list.push_back(person);}}
 else c["crew"][role]=person;
 return {true,"Crew assignment saved"};
}
Json Engine::aircraftOptions(std::size_t ci,const std::string& slot)const{
 if(ci>=state["craft"].size())return Json::array();std::string type=state["craft"][ci]["type"];
 if(slot=="engine")return Json::array({type=="fighter"?"VECTORING ION DRIVE · MK I":type=="hovercraft"?"HOVER ENGINE · MK I":"TURBOFAN_MK_I"});
 if(type=="fighter"&&slot=="primary")return Json::array({"","ROTARY CANNON · 300 ROUNDS"});
 if(type=="fighter"&&slot=="secondary")return Json::array({"","AIR-TO-AIR MISSILES · 8 MISSILES"});
 if((type=="fighter"&&slot=="system")||(type!="fighter"&&slot=="accessory")){Json options=Json::array({"","AUX_RADAR","SHIELD","ELECTRONICS"});if(type=="hovercraft")options.push_back("MEDICAL_KIT");if(type=="fighter")options.push_back("FUEL_TANK");return options;}
 return Json::array();
}
std::string Engine::aircraftModule(std::size_t ci,const std::string& slot)const{
 auto options=aircraftOptions(ci,slot);if(options.empty())return "";const auto& craft=state["craft"][ci];
 if(craft.contains("modules")&&craft["modules"].contains(slot))return craft["modules"][slot];
 if(slot=="primary"||slot=="secondary")return options.back();return options[0];
}
Result Engine::setAircraftModule(std::size_t ci,const std::string& slot,const std::string& module){
 auto options=aircraftOptions(ci,slot);if(std::find(options.begin(),options.end(),module)==options.end())return {false,"Module is unavailable or requires research"};
 auto& craft=state["craft"][ci];if(craft.value("operationState",std::string("BASE"))=="DEPLOYED")return {false,"Transport is on an operation"};if(craft.contains("serviceReady"))return {false,"Aircraft is undergoing service"};
 if(!craft.contains("modules")){craft["modules"]=Json::object();for(const auto& key:{"engine","primary","secondary","system","accessory"})if(!aircraftOptions(ci,key).empty())craft["modules"][key]=aircraftModule(ci,key);}
 double fuelLitres=craft["type"]=="fighter"?fighterFuel(ci)["fuelLitres"].get<double>():0.;craft["modules"][slot]=module;if(craft["type"]=="fighter"&&slot=="system")craft["fuel"]=std::min(100.,fuelLitres/fighterFuel(ci)["capacityLitres"].get<double>()*100.);return {true,"Aircraft module selected"};
}
bool Engine::aircraftCanOperate(std::size_t ci,const std::string& mode)const{return ci<state["craft"].size()&&(mode=="AIR"||(mode=="UNDERWATER"&&state["craft"][ci]["type"]=="hovercraft"));}
Result Engine::setAircraftMode(std::size_t ci,const std::string& mode){if(!aircraftCanOperate(ci,mode))return {false,"Aircraft cannot operate in this environment"};auto& c=state["craft"][ci];if(c.value("operationState",std::string("BASE"))=="DEPLOYED"||c.contains("serviceReady"))return {false,"Transport is unavailable"};c["operationMode"]=mode;return {true,"Operation environment selected"};}
Result Engine::toggleTransportSoldier(std::size_t ci,std::size_t person){if(ci>=state["craft"].size()||state["craft"][ci]["type"]!="hovercraft"||person>=state["personnel"].size()||state["personnel"][person]["role"]!="soldier"||state["personnel"][person].value("dead",false))return {false,"Select a transport and soldier"};auto& c=state["craft"][ci];if(c.value("operationState",std::string("BASE"))=="DEPLOYED")return {false,"Transport is on an operation"};if(!c.contains("squad"))c["squad"]=Json::array();auto& list=c["squad"];auto found=std::find(list.begin(),list.end(),Json(person));if(found!=list.end()){list.erase(found);return {true,"Squad updated"};}for(const auto& other:state["craft"])if(other.contains("squad")&&std::find(other["squad"].begin(),other["squad"].end(),Json(person))!=other["squad"].end())return {false,"Soldier is assigned to another transport"};if(list.size()>=constants["amphibious"]["soldierCapacity"].get<std::size_t>())return {false,"Transport carries at most four soldiers"};list.push_back(person);return {true,"Squad updated"};}
Json Engine::transportReadiness(std::size_t ci)const{Json reasons=Json::array();if(ci>=state["craft"].size()||state["craft"][ci]["type"]!="hovercraft")return {{"ready",false},{"reasons",Json::array({{{"code","transport"}}})}};const auto& c=state["craft"][ci];auto mode=c.value("operationMode",std::string("AIR"));if(c.contains("serviceReady")||c.value("operationState",std::string("BASE"))=="DEPLOYED")reasons.push_back({{"code","busy"}});if(c["hull"].get<double>()<=30)reasons.push_back({{"code","hull"}});if(c["fuel"].get<double>()<20)reasons.push_back({{"code","fuel"}});auto crew=c.value("crew",Json::object());for(const auto& role:{"pilot","navigator"}){int person=crew.value(role,-1);if(person<0||person>=int(state["personnel"].size())||state["personnel"][person]["role"]!=role||state["personnel"][person].value("health",0)<=0)reasons.push_back({{"code",role}});}
 if(!c.contains("squad")||c["squad"].empty())reasons.push_back({{"code","squad"}});else for(const auto& index:c["squad"]){std::size_t p=index;auto loadout=soldierLoadout(p);Json missing=Json::array();if(state["personnel"][p].value("health",0)<=0||state["personnel"][p].contains("gymTraining"))missing.push_back("unfit");if(mode=="UNDERWATER"){if(loadout["divingSuit"]!="diving-suit")missing.push_back("diving-suit");if(loadout["rebreather"]!="rebreather")missing.push_back("rebreather");if(loadout["divingHood"]!="diving-hood")missing.push_back("diving-hood");}if(!missing.empty())reasons.push_back({{"code","soldier"},{"person",p},{"name",state["personnel"][p]["name"]},{"missing",missing}});}if(c.value("robotEnabled",false)){if(state["stock"].value("underwater-robot",0)<1)reasons.push_back({{"code","robot-stock"},{"item","underwater-robot"}});for(const auto& key:robotWeapons(ci)){const auto& design=constants["amphibious"]["robot"]["weapons"][key.get<std::string>()];auto research=design.value("requiresResearch",std::string(""));if(!research.empty()&&state["research"].value(research,0.)<1)reasons.push_back({{"code","research"}});for(const auto& id:{design["module"].get<std::string>(),design["ammo"].get<std::string>()})if(state["stock"].value(id,0)<1)reasons.push_back({{"code","robot-stock"},{"item",id}});}}return {{"ready",reasons.empty()},{"reasons",reasons},{"mode",mode}};
}
Result Engine::departTransport(std::size_t ci){auto readiness=transportReadiness(ci);if(!readiness["ready"].get<bool>())return {false,"Transport departure blocked"};auto& c=state["craft"][ci];if(c.value("robotEnabled",false)){state["stock"]["underwater-robot"]=state["stock"].value("underwater-robot",0)-1;c["robotAmmo"]=Json::object();c["robotHealth"]=constants["amphibious"]["robot"]["health"];c["robotTU"]=constants["amphibious"]["robot"]["timeUnits"];for(const auto& key:robotWeapons(ci)){std::string weapon=key;const auto& design=constants["amphibious"]["robot"]["weapons"][weapon];std::string ammo=design["ammo"],module=design["module"];int rounds=std::min(state["stock"].value(ammo,0),design["capacity"].get<int>());state["stock"][module]=state["stock"].value(module,0)-1;state["stock"][ammo]=state["stock"].value(ammo,0)-rounds;c["robotAmmo"][weapon]=rounds;}}

 c["operationState"]="DEPLOYED";c["departedAt"]=state["hours"];c["fuel"]=c["fuel"].get<double>()-20;c["missions"]=c["missions"].get<int>()+1;return {true,"Transport departed"};}
Result Engine::returnTransport(std::size_t ci){if(ci>=state["craft"].size()||state["craft"][ci].value("operationState",std::string("BASE"))!="DEPLOYED")return {false,"No active transport operation"};auto& c=state["craft"][ci];if(c.value("robotEnabled",false)){state["stock"]["underwater-robot"]=state["stock"].value("underwater-robot",0)+1;for(const auto& key:robotWeapons(ci)){std::string weapon=key;const auto& design=constants["amphibious"]["robot"]["weapons"][weapon];std::string ammo=design["ammo"],module=design["module"];state["stock"][module]=state["stock"].value(module,0)+1;state["stock"][ammo]=state["stock"].value(ammo,0)+c["robotAmmo"].value(weapon,0);}c.erase("robotAmmo");c.erase("robotTU");c.erase("robotHealth");}c["operationState"]="BASE";c.erase("departedAt");return {true,"Transport returned"};}
Json Engine::robotWeapons(std::size_t ci)const{if(ci>=state["craft"].size())return Json::array();const auto& c=state["craft"][ci];return c.value("robotWeapons",Json::array());}
Result Engine::setTransportRobot(std::size_t ci,bool enabled,const std::string& weapon){if(ci>=state["craft"].size()||state["craft"][ci]["type"]!="hovercraft")return {false,"Invalid underwater robot configuration"};auto& c=state["craft"][ci];if(c.value("operationState",std::string("BASE"))=="DEPLOYED")return {false,"Transport is on an operation"};if(weapon!="KEEP"&&weapon!="NONE"){if(!constants["amphibious"]["robot"]["weapons"].contains(weapon))return {false,"Invalid underwater robot configuration"};const auto& design=constants["amphibious"]["robot"]["weapons"][weapon];auto research=design.value("requiresResearch",std::string(""));if(!research.empty()&&state["research"].value(research,0.)<1)return {false,"Required research is incomplete"};c["robotWeapons"]=Json::array({weapon});}else if(weapon=="NONE"||!c.contains("robotWeapons"))c["robotWeapons"]=Json::array();c["robotEnabled"]=enabled;return {true,"Underwater robot configuration saved"};}
Result Engine::toggleRobotWeapon(std::size_t ci,const std::string& weapon){if(ci>=state["craft"].size()||state["craft"][ci]["type"]!="hovercraft"||!constants["amphibious"]["robot"]["weapons"].contains(weapon))return {false,"Invalid underwater robot configuration"};auto& c=state["craft"][ci];if(c.value("operationState",std::string("BASE"))=="DEPLOYED")return {false,"Transport is on an operation"};auto list=robotWeapons(ci);auto found=std::find(list.begin(),list.end(),Json(weapon));if(found!=list.end())list.erase(found);else{const auto& design=constants["amphibious"]["robot"]["weapons"][weapon];auto research=design.value("requiresResearch",std::string(""));if(!research.empty()&&state["research"].value(research,0.)<1)return {false,"Required research is incomplete"};for(std::size_t i=0;i<list.size();)if(constants["amphibious"]["robot"]["weapons"][list[i].get<std::string>()]["category"]==design["category"])list.erase(i);else ++i;list.push_back(weapon);}c["robotWeapons"]=list;return {true,"Underwater robot configuration saved"};}
Result Engine::setTransportTarget(std::size_t ci,std::size_t ui){if(ci>=state["craft"].size()||state["craft"][ci]["type"]!="hovercraft"||ui>=state["ufos"].size()||ufoContact(ui).is_null())return {false,"Select a detected UFO"};auto& c=state["craft"][ci];if(c.value("operationState",std::string("BASE"))=="DEPLOYED")return {false,"Transport is on an operation"};const auto& u=state["ufos"][ui];c["operationTarget"]={{"id",ufoContactId(u)},{"lat",u["lat"]},{"lon",u["lon"]}};return {true,"Operation target selected"};}
Result Engine::fireUnderwaterRobot(std::size_t ci,const std::string& selected,const std::string& shot){if(ci>=state["craft"].size())return {false,"Select a transport"};auto& c=state["craft"][ci];auto list=robotWeapons(ci);std::string key=selected.empty()&&!list.empty()?list[0].get<std::string>():selected;if(c.value("operationState",std::string("BASE"))!="DEPLOYED"||c.value("operationMode",std::string("AIR"))!="UNDERWATER"||!c.value("robotEnabled",false)||!c.contains("operationTarget")||std::find(list.begin(),list.end(),Json(key))==list.end())return {false,"Underwater robot requires an active underwater operation and target"};if(c["robotAmmo"].value(key,0)<1)return {false,"Underwater robot has no ammunition"};const auto& weapon=constants["amphibious"]["robot"]["weapons"][key];if(shot!="hip"&&shot!="aimed")return {false,"Invalid firing mode"};int cost=weapon["tu"][shot];if(c.value("robotTU",0)<cost)return {false,"Insufficient time units"};for(std::size_t i=0;i<state["ufos"].size();++i){auto& u=state["ufos"][i];if(ufoContactId(u)!=c["operationTarget"]["id"])continue;if(ufoEnvironmentAt(u,state["hours"])!="SEA"||ufoContact(i).is_null())return {false,"Target is not a detected underwater UFO"};if(distance(c["operationTarget"]["lat"],c["operationTarget"]["lon"],u["lat"],u["lon"])>weapon["rangeKm"].get<double>())return {false,"Underwater target outside weapon range"};c["robotAmmo"][key]=c["robotAmmo"][key].get<int>()-1;c["robotTU"]=c["robotTU"].get<int>()-cost;bool hit=int(rng()%100)<weapon["accuracy"][shot].get<int>();if(hit)u["hull"]=std::max(0.,u["hull"].get<double>()-weapon["damage"].get<double>());if(u["hull"].get<double>()<=0){recordUfoCrashSite(u);state["ufos"].erase(i);}return {true,hit?"Underwater robot hit target":"Underwater robot missed"};}return {false,"Operation target is no longer present"};}
Result Engine::nextRobotTurn(std::size_t ci){if(ci>=state["craft"].size()||state["craft"][ci].value("operationState",std::string("BASE"))!="DEPLOYED"||!state["craft"][ci].value("robotEnabled",false))return {false,"No active robot operation"};state["craft"][ci]["robotTU"]=constants["amphibious"]["robot"]["timeUnits"];advance(1./60.);return {true,"Next robot turn"};}
Json Engine::serviceQuote(std::size_t craft)const{
 if(craft>=state["craft"].size()||state["craft"][craft]["hull"].get<double>()<=0)return nullptr;const auto& c=state["craft"][craft];std::string type=c["type"];
 bool fighter=type=="fighter",awacs=type=="awacs";double range=awacs?30000:fighter?12000:25000;if(fighter)range*=fighterFuel(craft)["capacityLitres"].get<double>()/constants["fighterFuel"]["internalLitres"].get<double>();
 double fuelMissing=std::max(0.,100.-c["fuel"].get<double>()),hullMissing=std::max(0.,100.-c["hull"].get<double>());
 int ammoMissing=fighter?std::max(0,300-c["ammo"].get<int>()):0,missilesMissing=fighter?std::max(0,8-c["missiles"].get<int>()):0;
 long long cost=static_cast<long long>(std::ceil(fuelMissing/100.*range/1000.*(awacs?11500:fighter?7800:5400)))+ammoMissing*7LL+missilesMissing*10500LL+static_cast<long long>(std::ceil(hullMissing*(awacs?2400:fighter?1500:900)));
 int hours=int(std::ceil(fuelMissing/100.*(awacs?20:fighter?10:14)+ammoMissing/100.*3+missilesMissing+hullMissing*(awacs?.45:fighter?.35:.3)));
 return {{"cost",cost},{"hours",hours},{"needed",cost>0||hours>0}};
}
Result Engine::serviceAircraft(std::size_t craft){
 if(craft<state["craft"].size()&&state["craft"][craft].value("operationState",std::string("BASE"))=="DEPLOYED")return {false,"Transport is on an operation"};
 auto quote=serviceQuote(craft);if(quote.is_null())return {false,"Select an aircraft"};auto& c=state["craft"][craft];
 if(c.contains("serviceReady"))return {false,"Service already in progress"};if(!quote["needed"].get<bool>())return {false,"No service is required"};
 auto cost=quote["cost"].get<long long>();if(state["money"].get<long long>()<cost)return {false,"Insufficient funds for service"};
 state["money"]=state["money"].get<long long>()-cost;c["serviceReady"]=state["hours"].get<double>()+std::max(1,quote["hours"].get<int>());log("AIRCRAFT SERVICE STARTED");return {true,"Aircraft service started"};
}
void Engine::processAircraftService(double hours){removeDestroyedAircraft();for(auto& c:state["craft"])if(c.contains("serviceReady")&&c["serviceReady"].get<double>()<=hours){c["fuel"]=100.;c["hull"]=100.;c["ammo"]=c["type"]=="fighter"?300:0;c["missiles"]=c["type"]=="fighter"?8:0;c.erase("serviceReady");log("AIRCRAFT SERVICE COMPLETE");}}
Result Engine::validatePersonnelSave(const Json& campaign)const{
 const auto& people=campaign.at("personnel");
 for(const auto& p:people){if(p.contains("dead")&&(!p["dead"].is_boolean()||(p["dead"].get<bool>()&&(p.value("health",-1)!=0||p.value("salary",-1)!=0))))return {false,"Invalid deceased soldier"};
  if(!p.is_object()||!p.contains("role")||!p["role"].is_string()||!oneOf(p["role"],{"soldier","pilot","navigator","technician","scientist"})||!p.contains("name")||!p["name"].is_string()||p["name"].get<std::string>().empty())return {false,"Invalid personnel record"};
  for(const auto& key:{"skill","health","salary"})if(!p.contains(key)||!p[key].is_number_integer()||p[key].get<long long>()<0)return {false,"Invalid personnel value"};
  if(p["skill"].get<long long>()>100||p["health"].get<long long>()>100)return {false,"Invalid personnel attributes"};
  if(p.contains("attributes")){if(!p["attributes"].is_object())return {false,"Invalid attributes"};for(const auto& a:p["attributes"])if(!a.is_number()||!std::isfinite(a.get<double>())||a.get<double>()<0||a.get<double>()>100)return {false,"Invalid attribute value"};}
  if(p.contains("gymTraining")){const auto& t=p["gymTraining"];if(p["role"]!="soldier"||!t.is_object()||!t.contains("started")||!t.contains("ready")||!t["started"].is_number()||!t["ready"].is_number()||!std::isfinite(t["started"].get<double>())||!std::isfinite(t["ready"].get<double>())||t["started"].get<double>()<0||std::abs(t["ready"].get<double>()-t["started"].get<double>()-48.)>1e-8)return {false,"Invalid gym training"};}
  if(!p.contains("loadout"))continue;const auto& l=p["loadout"];
  if(!l.is_object()||!l.contains("armor")||!l["armor"].is_object()||!l.contains("beltSlots")||!l["beltSlots"].is_array()||l["beltSlots"].size()!=4||!l.contains("backpack")||!l["backpack"].is_array())return {false,"Invalid soldier loadout"};
  auto valid=[this](const Json& value,const std::string& slot){return value.is_string()&&soldierItemAllowed(slot,value.get<std::string>());};
  for(auto slot:{"primaryWeapon","secondaryWeapon","equipmentBelt","radiationSuit","medicalKit"})if(!l.contains(slot)||!valid(l[slot],slot))return {false,"Invalid equipment slot"};
  for(auto slot:{"divingSuit","rebreather","divingHood"})if(l.contains(slot)&&!valid(l[slot],slot))return {false,"Invalid underwater equipment"};
  for(auto slot:{"helmet","torso","arms","legs"})if(!l["armor"].contains(slot)||!valid(l["armor"][slot],slot))return {false,"Invalid armor slot"};
  for(const auto& id:l["beltSlots"])if(!valid(id,"beltSlots"))return {false,"Invalid belt item"};
  for(const auto& id:l["backpack"])if(!valid(id,"backpack")||id=="")return {false,"Invalid backpack item"};if(backpackPacking(l["backpack"]).is_null())return {false,"Invalid backpack geometry"};
 }
 std::set<int> transportedSoldiers;for(const auto& c:campaign.at("craft")){
  if(!c.is_object()||!c.contains("type")||!c["type"].is_string()||!oneOf(c["type"],{"fighter","hovercraft","awacs"}))return {false,"Invalid aircraft"};
  if(c.contains("operationMode")&&(!c["operationMode"].is_string()||(c["operationMode"]!="AIR"&&(c["operationMode"]!="UNDERWATER"||c["type"]!="hovercraft"))))return {false,"Invalid operation environment"};
  if(c.contains("operationState")&&c["operationState"]!="BASE"&&c["operationState"]!="DEPLOYED")return {false,"Invalid transport state"};
  if(c.contains("robotEnabled")&&!c["robotEnabled"].is_boolean())return {false,"Invalid robot assignment"};if(c.contains("robotWeapons")){if(!c["robotWeapons"].is_array()||c["robotWeapons"].size()>2)return {false,"Invalid robot weapons"};std::set<std::string> categories;for(const auto& key:c["robotWeapons"]){if(!key.is_string()||!constants["amphibious"]["robot"]["weapons"].contains(key.get<std::string>()))return {false,"Invalid robot weapon"};const auto& design=constants["amphibious"]["robot"]["weapons"][key.get<std::string>()];if(!categories.insert(design["category"]).second)return {false,"Duplicate robot weapon slot"};auto tech=design.value("requiresResearch",std::string(""));if(!tech.empty()&&campaign["research"].value(tech,0.)<1)return {false,"Robot weapon research incomplete"};}}
  if(c.contains("robotAmmo")){if(!c["robotAmmo"].is_object()||c["robotAmmo"].size()>2)return {false,"Invalid robot ammunition"};for(const auto& entry:c["robotAmmo"].items())if(!constants["amphibious"]["robot"]["weapons"].contains(entry.key())||!entry.value().is_number_integer()||entry.value().get<int>()<0||entry.value().get<int>()>constants["amphibious"]["robot"]["weapons"][entry.key()]["capacity"].get<int>())return {false,"Invalid robot ammunition"};}
  for(const auto& key:{"robotTU","robotHealth"})if(c.contains(key)&&(!c[key].is_number_integer()||c[key].get<int>()<0||c[key].get<int>()>(std::string(key)=="robotTU"?60:180)))return {false,"Invalid robot condition"};
  if(c.contains("operationTarget")){const auto& t=c["operationTarget"];if(!t.is_object()||!t.contains("id")||!t["id"].is_string()||!t.contains("lat")||!t.contains("lon")||!t["lat"].is_number()||!t["lon"].is_number()||!std::isfinite(t["lat"].get<double>())||!std::isfinite(t["lon"].get<double>())||std::abs(t["lat"].get<double>())>90||std::abs(t["lon"].get<double>())>180)return {false,"Invalid operation target"};}
  if(c.contains("squad")){if(c["type"]!="hovercraft"||!c["squad"].is_array()||c["squad"].size()>4)return {false,"Invalid transport squad"};for(const auto& person:c["squad"])if(!person.is_number_integer()||person.get<int>()<0||person.get<std::size_t>()>=people.size()||people[person.get<std::size_t>()]["role"]!="soldier"||!transportedSoldiers.insert(person.get<int>()).second)return {false,"Invalid transport soldier"};}
  for(auto key:{"fuel","hull"})if(!c.contains(key)||!c[key].is_number()||!std::isfinite(c[key].get<double>())||c[key].get<double>()<0||c[key].get<double>()>100)return {false,"Invalid aircraft condition"};
  for(auto key:{"ammo","missiles","missions"})if(!c.contains(key)||!c[key].is_number_integer()||c[key].get<long long>()<0)return {false,"Invalid aircraft value"};
  if(c["ammo"].get<long long>()>300||c["missiles"].get<long long>()>8)return {false,"Invalid aircraft ammunition"};
  if(c.contains("serviceReady")&&(!c["serviceReady"].is_number()||!std::isfinite(c["serviceReady"].get<double>())||c["serviceReady"].get<double>()<0))return {false,"Invalid service schedule"};
  if(c.contains("modules")){
   if(!c["modules"].is_object())return {false,"Invalid aircraft modules"};
   std::string type=c["type"];
   for(const auto& module:c["modules"].items()){
    if(!module.value().is_string())return {false,"Invalid aircraft module"};std::string id=module.value();bool valid=false;
    if(module.key()=="engine")valid=id==(type=="fighter"?"VECTORING ION DRIVE · MK I":type=="hovercraft"?"HOVER ENGINE · MK I":"TURBOFAN_MK_I");
    else if(type=="fighter"&&module.key()=="primary")valid=id.empty()||id=="ROTARY CANNON · 300 ROUNDS";
    else if(type=="fighter"&&module.key()=="secondary")valid=id.empty()||id=="AIR-TO-AIR MISSILES · 8 MISSILES";
    else if((type=="fighter"&&module.key()=="system")||(type!="fighter"&&module.key()=="accessory"))valid=id.empty()||id=="AUX_RADAR"||id=="SHIELD"||id=="ELECTRONICS"||(type=="fighter"&&id=="FUEL_TANK")||(type=="hovercraft"&&id=="MEDICAL_KIT");
    if(!valid)return {false,"Unavailable aircraft module"};
   }
  }
  if(c.contains("crew")){const auto& crew=c["crew"];if(!crew.is_object())return {false,"Invalid crew"};auto valid=[&](const Json& id,const std::string& role,bool empty){if(!id.is_number_integer())return false;long long i=id.get<long long>();return (empty&&i==-1)||(i>=0&&i<static_cast<long long>(people.size())&&people[std::size_t(i)]["role"]==role);};
   for(auto role:{"pilot","navigator"})if(!crew.contains(role)||!valid(crew[role],role,true))return {false,"Invalid crew assignment"};
   if(!crew.contains("technicians")||!crew["technicians"].is_array()||crew["technicians"].size()>6)return {false,"Invalid technicians"};std::set<int> unique;for(const auto& id:crew["technicians"]){if(!valid(id,"technician",false)||!unique.insert(id.get<int>()).second)return {false,"Invalid technician assignment"};}
  }
 }
 for(const auto& c:campaign["craft"])if(c.value("operationState",std::string("BASE"))=="DEPLOYED"){
  if(c["type"]!="hovercraft"||!c.contains("squad")||c["squad"].empty()||!c.contains("departedAt")||!c["departedAt"].is_number()||!std::isfinite(c["departedAt"].get<double>())||c["departedAt"].get<double>()<0||c["departedAt"].get<double>()>campaign["hours"].get<double>()||!c.contains("crew")||c["crew"].value("pilot",-1)<0||c["crew"].value("navigator",-1)<0)return {false,"Invalid deployed transport"};
  if(c.value("operationMode",std::string("AIR"))=="UNDERWATER")for(const auto& p:c["squad"]){const auto& soldier=people[p.get<std::size_t>()];if(!soldier.contains("loadout")||soldier["loadout"].value("divingSuit",std::string(""))!="diving-suit"||soldier["loadout"].value("rebreather",std::string(""))!="rebreather"||soldier["loadout"].value("divingHood",std::string(""))!="diving-hood")return {false,"Underwater deployment requires every soldier's diving equipment"};}
  if(c.value("robotEnabled",false)&&!c.contains("robotAmmo"))return {false,"Missing deployed robot ammunition"};
 }
 return {true,"Personnel and aircraft valid"};
}
Result Engine::damageAircraft(std::size_t index,double damage,const std::string& cause){if(index>=state["craft"].size()||!std::isfinite(damage)||damage<=0)return {false,"Invalid aircraft damage"};auto& c=state["craft"][index];c["hull"]=std::max(0.,c["hull"].get<double>()-damage);if(c["hull"].get<double>()<=0){c["lossCause"]=cause;removeDestroyedAircraft();return {true,"Aircraft destroyed and removed from fleet"};}return {true,"Aircraft damaged"};}
void Engine::removeDestroyedAircraft(){for(std::size_t index=0;index<state["craft"].size();){auto& c=state["craft"][index];if(c["hull"].get<double>()>0){++index;continue;}Json loss={{"craft",c},{"lostAt",state["hours"]},{"cause",c.value("lossCause",std::string("Hull destroyed"))},{"casualties",Json::array()},{"tankLost",false}};if(c["type"]=="hovercraft"&&c.value("operationState",std::string("BASE"))=="DEPLOYED"){for(const auto& p:c.value("squad",Json::array())){std::size_t person=p;if(person>=state["personnel"].size())continue;auto& soldier=state["personnel"][person];if(soldier.value("dead",false))continue;loss["casualties"].push_back({{"person",person},{"soldier",soldier}});soldier["dead"]=true;soldier["deadAt"]=state["hours"];soldier["health"]=0;soldier["salary"]=0;soldier["loadout"]=emptyLoadout();soldier.erase("gymTraining");}if(c.value("robotEnabled",false)){loss["tankLost"]=true;loss["craft"]["robotHealth"]=0;}}
 if(!state.contains("aircraftLosses"))state["aircraftLosses"]=Json::array();state["aircraftLosses"].push_back(loss);std::string name=c.value("name",c["type"].get<std::string>());state["craft"].erase(index);log("AIRCRAFT DESTROYED: "+name);}}

}
