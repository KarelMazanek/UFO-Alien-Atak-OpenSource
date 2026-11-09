#include "engine.hpp"
#include <iostream>
#include <cmath>
using namespace uaa;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 check(argc==3,"Expected data and temporary directory");Engine e(argv[1],argv[2]);e.createBase("Storage",50,14);
 check(e.storageCapacity()["used"].get<double>()<=160&&e.state["stock"]["machine-gun"]==0&&e.state["stock"]["fighter-engine"]==1,"Balanced initial reserves fit in starting warehouses");
 for(const auto& id:{"light-tank","tank-rocket-pods","tank-rocket","tank-main-gun","tank-main-gun-round","tank-machine-gun-rounds"})check(e.state["stock"][id]==0,"New game starts without tank, tank weapons or ammunition");
 check(std::abs(e.storageUnitLitres("pistol")-2.16)<1e-10,"HTML item dimensions convert cubic centimetres to litres");check(e.storageUnitLitres("uranium")==1.,"HTML uranium unit volume");
 for(std::size_t i=0;i<e.state["personnel"].size();++i)if(e.state["personnel"][i]["role"]=="soldier")e.equipSoldier(i,"primaryWeapon","");
 e.state["stock"]={{"pistol",1200},{"uranium",1200}};auto load=e.storageCapacity();check(load["capacity"]==160&&load["uraniumCapacity"]==0,"Four initial warehouses provide 160 pallets; uranium separate");check(std::abs(load["used"].get<double>()-2.16)<1e-10&&std::abs(load["uraniumUsed"].get<double>()-1.)<1e-10,"HTML 1200 litres per pallet and no uranium double count");
 check(e.trade("weapons","pistol",true).ok,"Buy stored item");e.advance(e.state["tradeFlights"].back()["arrival"].get<double>()-e.state["hours"].get<double>());auto after=e.storageCapacity();check(std::abs(after["used"].get<double>()-load["used"].get<double>()-.0018)<1e-10,"Purchase updates fill amount");check(e.trade("weapons","pistol",false).ok,"Sell stored item");check(std::abs(e.storageCapacity()["used"].get<double>()-2.16)<1e-10,"Sale updates fill amount");
 int soldier=-1;for(int i=0;i<int(e.state["personnel"].size());++i)if(e.state["personnel"][i]["role"]=="soldier"){soldier=i;break;}check(e.equipSoldier(soldier,"primaryWeapon","pistol").ok,"Equip from stock");check(std::abs(e.storageCapacity()["used"].get<double>()-(2.16-.0018))<1e-10,"Assigned equipment is not counted as stored");check(e.equipSoldier(soldier,"primaryWeapon","").ok,"Return stored equipment");
 check(e.build(0,"storage",0,8,0).ok,"Build another warehouse");e.state["bases"][0]["buildings"].push_back({{"id","uraniumStorage"},{"x",8},{"y",8},{"floor",0},{"ready",72.}});check(e.storageCapacity()["capacity"]==160&&e.storageCapacity()["uraniumCapacity"]==0,"Unfinished construction adds no capacity");e.advance(72);load=e.storageCapacity();check(load["capacity"]==200&&load["uraniumCapacity"]==10,"New warehouses add the correct separate capacities");
 e.state["stock"]["pistol"]=200000;check(e.storageCapacity()["used"].get<double>()>200,"Over-capacity inventory is reported truthfully rather than clamped");e.save(1);Engine restored(argv[1],argv[2]);check(restored.load(1).ok&&restored.storageCapacity()==e.storageCapacity(),"Storage fill is derived correctly after reload");
 auto before=e.state;e.state["money"]=64999;auto poor=e.state;check(!e.trade("weapons","pistol",true,100).ok&&e.state==poor,"Bulk purchase is all-or-nothing when funds are insufficient");e.state=before;int pistols=e.state["stock"]["pistol"];check(e.trade("weapons","pistol",true,100).ok&&e.state["stock"]["pistol"]==pistols&&e.state["tradeFlights"].back()["quantity"]==100,"Book one hundred units without instant stock delivery");
 std::cout<<"PASS: HTML volumes, pallet capacities, separate uranium, purchase/sale/equipment updates, construction, overflow and reload\n";return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}}
