#include "engine.hpp"
#include <iostream>
#include <set>
using namespace uaa;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 check(argc==3,"Expected data and temporary directory");Engine e(argv[1],argv[2]);e.createBase("Parity",50,14);
 std::set<std::string> soldiers,scientists,technicians;
 for(const auto& p:e.state["personnel"]){auto role=p["role"].get<std::string>();check(p.contains("attributes"),"Individual attributes required");
  if(role=="soldier"){soldiers.insert(p["attributes"].dump());for(const auto& a:p["attributes"])check(a.get<int>()>=20&&a.get<int>()<=50,"HTML soldier attribute range");check(p["health"]==p["attributes"]["HEALTH"],"Soldier current/max health starts at individual value");}
  if(role=="scientist")scientists.insert(p["attributes"].dump());if(role=="technician")technicians.insert(p["attributes"].dump());
 }
 check(soldiers.size()>1&&scientists.size()>1&&technicians.size()>1,"Starting staff must not share identical attributes");
 int offer=-1;for(int i=0;i<int(e.state["offers"].size());++i)if(e.state["offers"][i]["role"]=="scientist"){offer=i;break;}
 auto person=e.state["offers"][offer];auto money=e.state["money"];check(e.hireCandidate(offer).ok,"Hire selected scientist");check(e.state["personnel"].back()==person,"Hire preserves displayed candidate stats and salary");check(e.state["money"]==money,"Civilian staff has no HTML recruitment fee");
 person=e.state["offers"][0];check(e.hireCandidate(0).ok,"Recruit displayed soldier");check(e.state["money"].get<long long>()==money.get<long long>()-42000,"ENTRY soldier costs quality times 1000");
 check(e.energy(0)==75,"Power output 150 EU, starting consumption 75 EU including three additional warehouses");
 auto before=e.state;check(!e.setAircraftModule(0,"engine","MK_II").ok,"Unavailable engine blocked");check(e.state==before,"Invalid module does not mutate campaign");
 check(e.setAircraftModule(0,"system","SHIELD").ok,"Install fighter shield");check(e.aircraftModule(0,"primary")=="ROTARY CANNON · 300 ROUNDS","Choosing optional module preserves default cannon");check(e.setAircraftModule(0,"primary","").ok,"Remove nose cannon");check(e.aircraftModule(0,"primary").empty(),"Empty hardpoint persists");check(!e.setAircraftModule(2,"accessory","MEDICAL_KIT").ok,"AWACS rejects hovercraft-only accessory");
 before=e.state;check(!e.launchSatellite("AIR",60,1).ok,"ESA lease required");check(e.state==before,"Failed launch cannot deduct money");
 money=e.state["money"];check(e.rentLocation("esa").ok,"Rent ESA");check(e.state["money"].get<long long>()==money.get<long long>()-400000,"HTML ESA lease price");check(e.spaceLease("esa"),"Lease active");check(e.rentLocation("esa").ok&&e.state["leases"]["esa"]==1440.,"Extension adds 30 days to paid term");
 auto quote=e.satelliteQuote(60);check(quote["altitude"]==980&&quote["strength"]==5&&quote["coverage"]==900&&quote["cost"]==500000,"HTML fuel/altitude/scan/price formulas");
 before=e.state;check(!e.launchSatellite("UNKNOWN",60,1).ok&&!e.launchSatellite("AIR",101,1).ok&&!e.launchSatellite("AIR",60,6).ok,"Invalid launch parameters rejected");check(e.state==before,"Invalid launch is atomic");
 money=e.state["money"];check(e.launchSatellite("AIR",60,2).ok,"Schedule two payloads");check(e.state["money"].get<long long>()==money.get<long long>()-1000000,"Launch charged once for all rockets");e.advance(35);check(e.state["satellites"].empty(),"Loading and preparation take 36 hours");e.advance(1);check(e.state["satellites"].size()==1,"First satellite reaches orbit at 36 hours");e.advance(36);check(e.state["satellites"].size()==2&&e.state["launchQueue"].empty(),"Second launch follows next 36-hour cycle");
 e.save(1);Engine loaded(argv[1],argv[2]);check(loaded.load(1).ok,"New personnel, modules and space state loads");check(loaded.aircraftModule(0,"system")=="SHIELD"&&loaded.state["satellites"].size()==2,"Modules and satellites survive reload");
 auto invalid=readJson(e.userDir/"save-1.json");invalid["offers"][0]["attributes"]="bad";writeJson(e.userDir/"save-2.json",invalid);before=loaded.state;check(!loaded.load(2).ok&&loaded.state==before,"Bad candidate cannot replace campaign");
 invalid=readJson(e.userDir/"save-1.json");invalid["craft"][0]["modules"]["engine"]="MK_III";writeJson(e.userDir/"save-2.json",invalid);check(!loaded.load(2).ok&&loaded.state==before,"Bad module cannot replace campaign");
 e.advance(96);check(e.state["offers"].size()==20,"Weekly workforce market replenishes all professions and tiers");
 e.state["money"]=100000000LL;check(e.rentLocation("nasa").ok,"Houston lease");e.ensureHouston();check(e.astronautCapacity()==8,"Reference Houston starts with two housing cells for eight astronauts");check(e.buildSpaceHousing().ok,"Build residential cell");e.advance(72);
 for(int offer:{0,6,12})check(e.hireAstronaut(offer).ok,"Hire commander, navigator, technician");
 before=e.state;check(!e.launchTransport("nasa",1,1).ok&&e.state==before,"Crewed flight needs selected required roles");for(int i=0;i<3;++i)check(e.toggleFlightCrew(i).ok,"Select required crew");
 money=e.state["money"];check(e.launchTransport("nasa",1,1).ok,"Schedule crewed flight");check(e.state["money"].get<long long>()==money.get<long long>()-2100000,"HTML small crew capsule price includes payload");
 check(e.rentLocation("roscosmos").ok,"Roscosmos lease");money=e.state["money"];check(e.launchTransport("roscosmos",2,1).ok,"Schedule medium cargo ship");check(e.state["money"].get<long long>()==money.get<long long>()-890000,"HTML cargo ship plus 20 tonnes extra fuel");
 e.save(3);check(loaded.load(3).ok,"Pending mixed satellite/transport queue loads");e.advance(54);check(e.state.value("issCargo",0)==1,"Crewed flight arrives after 36 h preparation and 18 h transit");e.advance(66);check(e.state.value("issCargo",0)==3,"Sequential cargo ship arrives after 48 h transit");e.advance(1);check(e.state.value("issCargo",0)==3,"ISS cargo is not delivered twice");check(e.rentISS(3).ok,"ISS three-month prepayment");
 e.save(4);check(loaded.load(4).ok&&loaded.state["transports"].size()==2&&loaded.state["flightCrew"].size()==3,"Astronaut crew, ISS lease and delivered flights survive reload");
 invalid=readJson(e.userDir/"save-4.json");for(auto& person:invalid["personnel"]){person.erase("attributes");person["skill"]=50;person["health"]=100;}writeJson(e.userDir/"save-5.json",invalid);check(loaded.load(5).ok,"Legacy saves with shared skill values load");std::set<std::string> migrated;for(const auto& person:loaded.state["personnel"]){check(person.contains("attributes"),"Legacy staff gains role-specific attributes");if(person["role"]=="soldier")migrated.insert(person["attributes"].dump());}check(migrated.size()>1,"Legacy soldiers gain varied attributes");
 std::cout<<"PASS: HTML staff ranges, selected offers, hiring prices, power output, aircraft modules, ESA leases, launch queue, orbital formulas, weekly refresh and validated saves\n";return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}}
