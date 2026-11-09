#include "engine.hpp"
#include <cmath>
#include <set>
#include <functional>
namespace uaa {
Result Engine::validateGameset(const Json& g,bool playable)const{
 try{
  if(!g.is_object()||!g.contains("id")||!g["id"].is_string()||g["id"].get<std::string>().empty()||!g.contains("name")||!g["name"].is_string()||g["name"].get<std::string>().empty())return {false,"Sada musí mít ID a název / Gameset needs an ID and name"};
  std::function<void(const Json&,int)> safe=[&](const Json& v,int depth){if(depth>16)throw std::runtime_error("Příliš hluboká data / Data too deep");if(v.is_string()&&v.get_ref<const std::string&>().size()>65536)throw std::runtime_error("Příliš dlouhý text / Text too long");if(v.is_number()&&(!std::isfinite(v.get<double>())||v.get<double>()<0||v.get<double>()>10000000))throw std::runtime_error("Číslo musí být 0–10000000 / Number must be 0–10000000");if(v.is_structured()){if(v.size()>10000)throw std::runtime_error("Příliš mnoho položek / Too many entries");for(const auto& child:v)safe(child,depth+1);}};safe(g,0);
  std::function<void(const Json&,const Json&,const std::string&)> shape=[&](const Json& v,const Json& model,const std::string& path){
   bool same=model.is_number()?v.is_number()&&(!model.is_number_integer()||v.is_number_integer()):model.type()==v.type();
   if(!same)throw std::runtime_error("Nesprávný typ / Invalid type: "+path);
   if(model.is_object())for(const auto& field:model.items()){if(!v.contains(field.key()))throw std::runtime_error("Chybí pole / Missing field: "+path+"/"+field.key());shape(v[field.key()],field.value(),path+"/"+field.key());}
   if(model.is_array()&&!model.empty())for(std::size_t i=0;i<v.size();++i)shape(v[i],model[std::min(i,model.size()-1)],path+"/"+std::to_string(i));
  };
  for(const auto* key:{"missions","technologies","weapons","equipment","units","buildings","aliens","factions"}){
   if(!g.contains(key)||!g[key].is_array())return {false,std::string("Chybí seznam / Missing collection: ")+key};std::set<std::string> ids;
   for(const auto& item:g[key]){
    if(!item.is_object()||!item.contains("id")||!item["id"].is_string()||item["id"].get<std::string>().empty()||!item.contains("name")||!item["name"].is_string())return {false,std::string("Chybí ID nebo název položky / Missing record ID or name: ")+key};
    std::string id=item["id"];if(!ids.insert(id).second)return {false,"Duplicitní ID / Duplicate ID: "+id};
    for(const auto& baseline:referenceGameset()[key])if(baseline["id"]==id)shape(item,baseline,std::string(key)+"/"+id);
    for(const auto* text:{"description","enName","group","requiresResearch","requiresCorpse","requiresLiveAlien","requiresUfoWreck"})if(item.contains(text)&&!item[text].is_string())return {false,std::string("Textové pole / Text field: ")+text};
    if(item.contains("specs")){if(!item["specs"].is_array())return {false,"Neplatné technické údaje / Invalid specifications"};for(const auto& row:item["specs"])if(!row.is_array()||row.size()!=2||!row[0].is_string()||!row[1].is_string())return {false,"Technické údaje: dva texty na řádek / Specifications: two strings per row"};}
    for(const auto* stats:{"unitStats","ufoStats","vehicleStats"})if(item.contains(stats)){if(!item[stats].is_object())return {false,"Neplatné vlastnosti / Invalid statistics"};for(const auto& value:item[stats])if(!value.is_number()&&!value.is_array())return {false,"Vlastnost musí být číslo / Statistic must be numeric"};}
    // Validate optional numeric structures too, including newly created records.
    for(const auto* count:{"startingCount","availableAfterDays","researchHours","researchScientists"})if(item.contains(count)&&!item[count].is_number_integer())return {false,std::string("Celé číslo / Integer required: ")+count};
    if(item.contains("combat")){
     const auto& combat=item["combat"];if(!combat.is_object())return {false,"Neplatné bojové údaje / Invalid combat data"};
     for(const auto* field:{"values","tu","curves"})if(combat.contains(field)&&!combat[field].is_object())return {false,"Neplatná sekce boje / Invalid combat section"};
     for(const auto* field:{"values","tu"})if(combat.contains(field))for(const auto& v:combat[field])if(!v.is_number())return {false,"Bojová hodnota musí být číslo / Combat value must be numeric"};
     if(combat.contains("curves"))for(const auto& curve:combat["curves"]){if(!curve.is_array())return {false,"Křivka musí být seznam / Curve must be an array"};double last=-1;for(const auto& point:curve){if(!point.is_array()||point.size()!=2||!point[0].is_number()||!point[1].is_number()||point[0].get<double>()<last||point[1].get<double>()>100)return {false,"Křivka: vzestupná vzdálenost a přesnost 0–100 / Curve: increasing distance, accuracy 0–100"};last=point[0].get<double>();}}
    }
    if(item.contains("production")){const auto& p=item["production"];if(!p.is_object())return {false,"Neplatná výroba / Invalid production"};for(const auto* field:{"productionHours","productionCost","marketPrice"})if(!p.contains(field)||!p[field].is_number_integer())return {false,"Výroba potřebuje celočíselný čas a ceny / Production requires integer time and prices"};if(!p.contains("materials")||!p["materials"].is_object())return {false,"Chybí materiály / Missing materials"};for(const auto& m:p["materials"])if(!m.is_number_integer())return {false,"Materiál musí být celé číslo / Material must be an integer"};}
    for(const auto& field:item.items())if(field.key().find("Image")!=std::string::npos||field.key()=="image"){if(!field.value().is_string())return {false,"Obrázek musí být cesta / Image must be a path"};std::string path=field.value();if(!path.empty()&&(fs::path(path).is_absolute()||path.find("..")!=std::string::npos||path.find(':')!=std::string::npos))return {false,"Použij import obrázku PNG/JPG / Use PNG/JPG image import"};}
   }
   if(playable)for(const auto& baseline:referenceGameset()[key])if(!ids.count(baseline["id"].get<std::string>()))return {false,"Pro hru chybí základní položka / Missing base record for gameplay: "+baseline["id"].get<std::string>()};
  }
  if(g.contains("finance")){const auto& finance=g["finance"];if(!finance.is_object()||!finance.contains("countries")||!finance["countries"].is_array())return {false,"Neplatné finance / Invalid finance"};std::set<std::string> names;for(const auto& c:finance["countries"]){if(!c.is_object()||!c.contains("name")||!c["name"].is_string()||!names.insert(c["name"].get<std::string>()).second||!c.contains("tier")||!c["tier"].is_number_integer()||c["tier"].get<int>()<1||c["tier"].get<int>()>5)return {false,"Stát potřebuje jedinečný název a třídu 1–5 / Country needs unique name and tier 1–5"};for(const auto* field:{"contribution","approval"})if(c.contains(field)&&!c[field].is_null()&&!c[field].is_number_integer())return {false,"Finance: celé číslo nebo null / Finance: integer or null"};if(c.contains("approval")&&!c["approval"].is_null()&&c["approval"].get<int>()>100)return {false,"Spokojenost musí být 0–100 / Approval must be 0–100"};}}
  return {true,"Sada je platná / Gameset is valid"};
 }catch(const std::exception& e){return {false,e.what()};}
}
long long Engine::buildingPrice(const std::string& id)const{
 long long price=0;for(const auto& b:constants["BASE_OBJECTS"])if(b["key"]==id)price=b.value("cost",0LL);
 if(selectedGameset.is_object())if(const auto* item=record("buildings",id))if(item->contains("production"))for(const auto& base:referenceGameset()["buildings"])if(base["id"]==id&&base.contains("production")&&(*item)["production"]["productionCost"]!=base["production"]["productionCost"])return (*item)["production"]["productionCost"].get<long long>();
 return price;
}
double Engine::buildingHours(const std::string& id)const{
 if(selectedGameset.is_object())if(const auto* item=record("buildings",id))if(item->contains("production"))for(const auto& base:referenceGameset()["buildings"])if(base["id"]==id&&base.contains("production")&&(*item)["production"]["productionHours"]!=base["production"]["productionHours"])return (*item)["production"]["productionHours"].get<double>();
 return 72.;
}
Result Engine::startNewGame(const Json& candidate){
 auto valid=validateGameset(candidate,true);if(!valid.ok)return valid;
 auto previousSet=selectedGameset,previousState=state;auto previousRng=rng;
 try{selectedGameset=candidate==referenceGameset()?Json():candidate;newGame();return {true,"Nová hra / New game"};}
 catch(const std::exception& e){selectedGameset=std::move(previousSet);state=std::move(previousState);rng=previousRng;return {false,e.what()};}
}
}
