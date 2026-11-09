#include "engine.hpp"
#include "map_editor.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
using namespace uaa;
void require(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char**argv){
 try{
  require(argc==3,"Expected data directory and test directory");fs::path temp=argv[2];fs::create_directories(temp);Engine e(argv[1],temp);
  require(e.date()=="2042-01-01 00:00:00","Original HTML calendar start");
  e.state["hours"]=24.*31;require(e.date()=="2042-02-01 00:00:00","Calendar month rollover");
  e.state["hours"]=24.*(365+365+59);require(e.date()=="2044-02-29 00:00:00","Leap day");
  e.state["hours"]=0;e.newGame();
  require(e.gameset()["technologies"].size()==56+readJson(e.dataDir/"amphibious.json")["research"].size(),"Research extraction count");
  require(e.gameset()["aliens"].size()==33,"Alien extraction count");
  for(const auto& group:{"technologies","weapons","equipment","buildings","aliens","units"})for(const auto& r:e.gameset()[group])if(r.contains("image"))require(fs::is_regular_file(e.dataDir/r["image"].get<std::string>()),"Missing image: "+r["id"].get<std::string>());
  require(!e.createBase("",0,0),"Reject empty name");require(!e.createBase("X",91,0),"Reject latitude");
  require(bool(e.createBase("Praha",50.08,14.43)),"Create initial base");require(e.state["money"]==6500000,"First base is free as in HTML");
  auto [rw,rh]=e.footprint("runway");require(rw==6&&rh==4,"Runway orientation");auto [aw,ah]=e.footprint("awacsPad");require(aw==6&&ah==2,"AWACS footprint");
  for(const auto& a:e.state["bases"][0]["buildings"])for(const auto& b:e.state["bases"][0]["buildings"]){if(a==b)continue;auto [w,h]=e.footprint(a["id"]);auto [bw,bh]=e.footprint(b["id"]);int x=a["x"],y=a["y"],bx=b["x"],by=b["y"];require(!(x<bx+bw&&x+w>bx&&y<by+bh&&y+h>by),"Starting layout collision");}
  auto balance=e.state["money"];require(!e.build(0,"storage",0,0,0),"Reject overlapping building");require(e.state["money"]==balance,"Failed construction must not debit money");
  e.state["bases"][0]["floors"]=1;
  for(const auto& id:{"power","runway","hangar","radar","radarLarge","defence"})require(!e.build(0,id,0,0,-1),std::string("Reject underground ")+id);
  require(bool(e.build(0,"storage",0,8,0)),"Place storage");require(e.state["money"].get<long long>()==balance.get<long long>()-150000,"Construction debit");
  require(bool(e.startResearch("research_basic_lab")),"Start basic research");require(!e.startResearch("research_autopsy_sectoid"),"Autopsy requires corpse");for(std::size_t i=0;i<e.state["personnel"].size();++i)if(e.state["personnel"][i]["role"]=="scientist")require(e.assignScientist("research_basic_lab",i,true).ok,"Assign starter scientist");e.advance(72);require(e.state["research"]["research_basic_lab"]==1.0,"Research completes");
  e.state["stock"]["iron"]=0;e.state["stock"]["plastics"]=0;require(!e.manufacture("weapons","pistol"),"Materials required");
  for(const auto& id:{"iron","plastics"})require(bool(e.trade("materials",id,true)),"Purchase material");e.advance(e.state["tradeFlights"].back()["arrival"].get<double>()-e.state["hours"].get<double>());require(bool(e.manufacture("weapons","pistol")),"Queue pistol");require(e.assignTechnician("weapons","pistol",6,true).ok,"Assign production technician");int before=e.state["stock"]["pistol"];e.advance(18);require(e.state["stock"]["pistol"]==before+1,"Production completes");
  auto saved=e.state;e.save(1);e.advance(10);require(bool(e.load(1)),"Load save");auto restored=e.state;restored.erase("rng");require(restored==saved,"Lossless save/load state");
  Engine second(argv[1],temp);require(bool(second.load(1)),"Second instance loads");e.advance(240);second.advance(240);require(e.state==second.state,"Random state survives save/load");
  writeJson(temp/"save-2.json",Json{{"schema","wrong"}});auto unchanged=e.state;require(!e.load(2),"Reject invalid save");require(e.state==unchanged,"Invalid save is transactional");
  require(!launchMapEditor(temp/"missing-installation"),"Missing real map editor must fail visibly");
  require(std::abs(Engine::distance(0,0,0,90)-10007.543398)<.01,"Great-circle distance");
  Engine demolition(argv[1],temp/"demolition");demolition.createBase("Removal",50,14);auto beforePreview=demolition.state;require(demolition.canBuild(0,"storage",0,8,0).ok&&!demolition.canBuild(0,"gym",9,9,0).ok&&!demolition.canBuild(0,"storage",0,0,0).ok,"Placement preview checks free space, edges and collisions");require(demolition.state==beforePreview,"Placement preview is read-only");int store=-1,command=-1,power=-1;for(int i=0;i<int(demolition.state["bases"][0]["buildings"].size());++i){std::string id=demolition.state["bases"][0]["buildings"][i]["id"];if(id=="storage")store=i;if(id=="command")command=i;if(id=="power")power=i;}require(!demolition.canDemolish(0,command).ok&&!demolition.canDemolish(0,power).ok,"Protected command center and required power cannot be demolished");auto money=demolition.state["money"].get<long long>();auto refund=demolition.demolitionRefund("storage");require(demolition.demolish(0,store).ok&&demolition.state["money"]==money+refund,"Confirmed storage demolition returns reference ten percent refund");
  std::cout<<"PASS: source data, assets, base geometry, surface restrictions, research, trade, manufacture, transactional saves, RNG roundtrip, editor module gate\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
