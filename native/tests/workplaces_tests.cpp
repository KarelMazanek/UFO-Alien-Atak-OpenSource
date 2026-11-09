#include "engine.hpp"
#include <iostream>
using namespace uaa;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 check(argc==3,"Expected data and temporary directory");
 for(const auto& facility:{"research","workshop"}){
  Engine e(argv[1],fs::path(argv[2])/facility);e.createBase("Capacity",50,14);bool research=std::string(facility)=="research";
  int offset=int(e.state["personnel"].size());for(int i=0;i<30;++i)e.state["personnel"].push_back(e.makeCandidate(research?"scientist":"technician",0));
  if(research){check(e.startResearch("research_basic_lab").ok,"Start first research");check(e.startResearch("research_ballistics").ok,"Start second research");}
  auto assign=[&](int i,bool enabled){return research?e.assignScientist(i<15?"research_basic_lab":"research_ballistics",offset+i,enabled):e.assignTechnician("weapons",i<15?"pistol":"rifle",offset+i,enabled);};
  check(e.staffCapacity(facility)==25,"One completed facility provides 25 places");for(int i=0;i<25;++i)check(assign(i,true).ok,"Fill workplaces across multiple teams");auto before=e.state;check(!assign(25,true).ok&&before==e.state,"Worker 26 rejected without changing projects or teams");check(assign(0,true).ok&&e.assignedStaff(facility)==25,"Existing assignment remains idempotent at capacity");
  int object=-1;for(int i=0;i<int(e.state["bases"][0]["buildings"].size());++i)if(e.state["bases"][0]["buildings"][i]["id"]==facility)object=i;check(object>=0&&!e.canDemolish(0,object).ok,"Cannot remove occupied workplaces");
  check(assign(0,false).ok&&assign(25,true).ok,"Releasing a worker frees one place");
  auto building=e.state["bases"][0]["buildings"][object];building["ready"]=100.;e.state["bases"][0]["buildings"].push_back(building);check(e.staffCapacity(facility)==25&&!assign(26,true).ok,"Unfinished facility adds no workplaces");e.state["hours"]=100.;e.state["nextUfo"]=172.;check(e.staffCapacity(facility)==50&&assign(26,true).ok,"Completed second facility adds another 25 places");
  e.save(1);Engine loaded(argv[1],fs::path(argv[2])/facility);auto load=loaded.load(1);if(!load.ok)std::cerr<<facility<<": "<<load.message<<"\n";check(load.ok&&loaded.assignedStaff(facility)==26,"Staffing and facility capacity survive save/load");
 }
 std::cout<<"PASS: 25 workplaces per completed laboratory/workshop, shared teams, atomic rejection, release, construction, demolition and save/load\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
