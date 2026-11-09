 // Native gameset editor. Reference data is read-only; campaigns keep their own snapshot.
 Json editorSets=Json::array(),editorUndo=Json::array(),editorRedo=Json::array();
 int editorSet=0,newGameSet=0,editorScroll=0,editorRecordScroll=0;bool editorDirty=false;bool editorDeleting=false;
 std::string editorPath,editorFeedback,editorFilter,editorDeleteTitle,editorDeleteName,editorDeleteInfo;unsigned editorSerial=0;
 std::string pointerPart(std::string s){std::string out;for(char c:s)out+=c=='~'?"~0":c=='/'?"~1":std::string(1,c);return out;}
 std::string editorId(){return "custom_"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+"_"+std::to_string(++editorSerial);}
 bool editorReadOnly()const{return editorSet==0;}
 void editorCheckpoint(){editorUndo.push_back(custom);if(editorUndo.size()>40)editorUndo.erase(editorUndo.begin());editorRedo=Json::array();editorDirty=true;}
 void editorStore(){if(!editorSets.empty())editorSets[editorSet]=custom;}
 void editorSelect(int selected){editorStore();editorSet=selected;custom=editorSets.at(editorSet);editorPath.clear();editorFilter.clear();editorScroll=editorRecordScroll=index=scroll=0;editorUndo=editorRedo=Json::array();}
 void loadGamesetLibrary(){
  editorSets=Json::array({game.referenceGameset()});editorFeedback.clear();
  try{
   auto file=game.userDir/"gamesets.json";
   if(fs::exists(file)){auto library=readJson(file);if(!library.is_object()||library.value("schema","")!="uaa-native-gamesets-v1"||!library.contains("gamesets")||!library["gamesets"].is_array())throw std::runtime_error("Neplatná knihovna sad / Invalid gameset library");for(auto set:library["gamesets"]){auto valid=game.validateGameset(set);if(!valid.ok)throw std::runtime_error(valid.message);set["reference"]=false;editorSets.push_back(set);}}
   else if(fs::exists(game.userDir/"gameset-custom.json")){auto legacy=readJson(game.userDir/"gameset-custom.json");auto valid=game.validateGameset(legacy);if(valid.ok){legacy["reference"]=false;editorSets.push_back(legacy);}else editorFeedback=valid.message;}
  }catch(const std::exception& e){editorFeedback=e.what();}
  editorSet=newGameSet=0;custom=editorSets[0];
 }
 void saveGamesetLibrary(){try{editorStore();Json sets=Json::array();for(std::size_t i=1;i<editorSets.size();++i){auto valid=game.validateGameset(editorSets[i]);if(!valid.ok){editorFeedback=valid.message;return;}sets.push_back(editorSets[i]);}writeJson(game.userDir/"gamesets.json",{{"schema","uaa-native-gamesets-v1"},{"gamesets",sets}});editorDirty=false;editorFeedback=tr("Sady uloženy. Vyber je v NOVÉ HŘE.","Gamesets saved. Select one in NEW GAME.");}catch(const std::exception& e){editorFeedback=e.what();}}
 void copyGameset(){editorStore();Json copy=custom;copy["id"]=editorId();copy["reference"]=false;copy["name"]=nameOf(custom)+tr(" · kopie"," · copy");editorSets.push_back(copy);editorSelect(int(editorSets.size())-1);editorDirty=true;editorFeedback=tr("Upravuješ vlastní kopii. DUMMY je chráněný.","Editing your own copy. DUMMY is protected.");}
 void requestGamesetRename(){
  if(editorReadOnly()){editorFeedback=tr("Referenční DUMMY nelze přejmenovat.","The reference DUMMY cannot be renamed.");return;}
  input(custom.value("name",std::string("")),false,[this](const std::string& entered){
   auto first=entered.find_first_not_of(" \t\r\n"),last=entered.find_last_not_of(" \t\r\n");if(first==std::string::npos||entered.size()>160){editorFeedback=tr("Název nesmí být prázdný a má nejvýše 160 znaků.","Name must be nonempty and at most 160 characters.");return;}
   std::string name=entered.substr(first,last-first+1);if(!editorReadOnly())editorCheckpoint();else editorDirty=true;
   custom["name"]=name;if(english&&custom.contains("enName"))custom["enName"]=name;
   if(custom.contains("localized")&&custom["localized"].is_object()){auto lang=english?"en":"cz";if(custom["localized"].contains(lang)&&custom["localized"][lang].is_object())custom["localized"][lang]["name"]=name;}
   editorStore();saveGamesetLibrary();
  });
 }
 void requestGamesetDelete(){
  if(editorReadOnly()){editorFeedback=tr("Referenční DUMMY nelze smazat.","The reference DUMMY cannot be deleted.");return;}
  std::string id=custom["id"];editorDeleting=true;editorDeleteTitle=tr("SMAZAT KOPII SADY?","DELETE GAMESET COPY?");editorDeleteName=custom.value("name",std::string(""));editorDeleteInfo=tr("Potvrzením odstraníš tuto kopii z knihovny.","Confirm to remove this copy from the library.");
  input("",false,[this,id](const std::string&){
   if(editorReadOnly()||custom.value("id",std::string(""))!=id){editorFeedback=tr("Vybraná sada se změnila; smazání zrušeno.","Selected gameset changed; deletion cancelled.");return;}
   editorStore();Json candidate=editorSets;candidate.erase(candidate.begin()+editorSet);
   try{
    Json sets=Json::array();for(std::size_t i=1;i<candidate.size();++i){auto valid=game.validateGameset(candidate[i]);if(!valid.ok){editorFeedback=valid.message;return;}sets.push_back(candidate[i]);}
    writeJson(game.userDir/"gamesets.json",{{"schema","uaa-native-gamesets-v1"},{"gamesets",sets}});
    editorSets=std::move(candidate);editorSet=0;custom=editorSets[0];editorSelect(0);newGameSet=0;editorDirty=false;editorFeedback=tr("Kopie smazána.","Copy deleted.");
   }catch(const std::exception& e){editorFeedback=e.what();}
  });
 }
 bool commitEditorObject(Json candidate,const std::string& feedback){
  try{
   auto valid=game.validateGameset(candidate);if(!valid.ok){editorFeedback=valid.message;return false;}
   editorStore();auto sets=editorSets;sets[editorSet]=candidate;Json own=Json::array();for(std::size_t i=1;i<sets.size();++i){auto check=game.validateGameset(sets[i]);if(!check.ok){editorFeedback=check.message;return false;}own.push_back(sets[i]);}
   writeJson(game.userDir/"gamesets.json",{{"schema","uaa-native-gamesets-v1"},{"gamesets",own}});
   editorCheckpoint();custom=std::move(candidate);editorSets=std::move(sets);editorDirty=false;editorPath.clear();editorScroll=0;editorFilter.clear();editorFeedback=feedback;return true;
  }catch(const std::exception& e){editorFeedback=e.what();return false;}
 }
 void requestObjectRename(){
  if(editorReadOnly()){editorFeedback=tr("Objekty DUMMY nelze přejmenovat.","DUMMY objects cannot be renamed.");return;}
  if(!custom.contains(group)||!custom[group].is_array()||custom[group].empty())return;
  std::string category=group,id=custom[group][index]["id"],setId=custom["id"];
  input(nameOf(custom[group][index]),false,[this,category,id,setId](const std::string& entered){
   if(editorReadOnly()||custom["id"]!=setId)return;
   auto first=entered.find_first_not_of(" \t\r\n"),last=entered.find_last_not_of(" \t\r\n");if(first==std::string::npos||entered.size()>160){editorFeedback=tr("Zadej neprázdný název do 160 znaků.","Enter a nonempty name up to 160 characters.");return;}
   auto candidate=custom;std::string name=entered.substr(first,last-first+1);bool found=false;
   for(auto& object:candidate[category])if(object["id"]==id){object["name"]=name;if(english&&object.contains("enName"))object["enName"]=name;if(object.contains("localized")&&object["localized"].is_object()){auto lang=english?"en":"cz";if(object["localized"].contains(lang)&&object["localized"][lang].is_object())object["localized"][lang]["name"]=name;}found=true;break;}
   if(!found)return;
   if(candidate.contains("objectTranslations")&&candidate["objectTranslations"].is_object())for(auto& translations:candidate["objectTranslations"]){auto lang=english?"en":"cz";if(translations.is_object()&&translations.contains(id)&&translations[id].is_object()&&translations[id].contains(lang)&&translations[id][lang].is_object())translations[id][lang]["name"]=name;}
   commitEditorObject(std::move(candidate),tr("Objekt přejmenován a uložen.","Object renamed and saved."));
  });
 }
 void requestObjectDelete(){
  if(editorReadOnly()){editorFeedback=tr("Objekty DUMMY nelze smazat.","DUMMY objects cannot be deleted.");return;}
  if(!custom.contains(group)||!custom[group].is_array()||custom[group].empty())return;
  std::string category=group,id=custom[group][index]["id"],setId=custom["id"];editorDeleting=true;editorDeleteTitle=tr("SMAZAT OBJEKT ZE SADY?","DELETE OBJECT FROM GAMESET?");editorDeleteName=nameOf(custom[group][index]);editorDeleteInfo=tr("Smaže se pouze tento objekt z vlastní sady.","Only this object will be removed from your gameset.");
  input("",false,[this,category,id,setId](const std::string&){
   if(editorReadOnly()||custom["id"]!=setId)return;
   auto candidate=custom;auto& objects=candidate[category];auto found=std::find_if(objects.begin(),objects.end(),[&](const Json& object){return object["id"]==id;});if(found==objects.end())return;objects.erase(found);
   if(commitEditorObject(std::move(candidate),tr("Objekt smazán a sada uložena.","Object deleted and gameset saved."))){index=std::clamp(index,0,std::max(0,int(custom[category].size())-1));editorRecordScroll=std::min(editorRecordScroll,std::max(0,int(custom[category].size())-8));}
  });
 }
 void editorSetValue(const std::string& path,const std::string& entered){
  if(editorReadOnly())return;
  try{Json value;const auto& old=custom.at(Json::json_pointer(path));if(old.is_string())value=entered;else{value=Json::parse(entered);bool nullable=value.is_null()&&path.rfind("/finance/countries/",0)==0&&(path.substr(path.find_last_of('/')+1)=="approval"||path.substr(path.find_last_of('/')+1)=="contribution");if(old.is_boolean()&&!value.is_boolean())throw std::runtime_error("Očekáváno true / false");if(old.is_number_integer()&&!value.is_number_integer()&&!nullable)throw std::runtime_error("Očekáváno celé číslo / Expected integer");if(old.is_number()&&!nullable&&(!value.is_number()||value.get<double>()<0||value.get<double>()>10000000))throw std::runtime_error("Rozsah / Range: 0–10000000");if(!old.is_number()&&!old.is_boolean()&&!old.is_null())throw std::runtime_error("Otevři skupinu / Open the section");}
   editorCheckpoint();custom[Json::json_pointer(path)]=std::move(value);editorFeedback=tr("Hodnota změněna; ulož sadu.","Value changed; save the gameset.");
  }catch(const std::exception& e){editorFeedback=e.what();}
 }
 void editorEdit(const std::string& path){
  auto value=custom.at(Json::json_pointer(path));
  if(value.is_structured()){editorPath=path;editorScroll=0;return;}
  if(editorReadOnly()){editorFeedback=tr("DUMMY je pouze ke čtení. Použij KOPÍROVAT.","DUMMY is read-only. Use COPY.");return;}
  if(value.is_boolean()){editorCheckpoint();custom[Json::json_pointer(path)]=!value.get<bool>();return;}
  input(value.is_string()?value.get<std::string>():value.dump(),!value.is_string(),[this,path](const std::string& text){editorSetValue(path,text);});
 }
 void editorImport(const fs::path& file){
  try{if(fs::file_size(file)>128*1024*1024)throw std::runtime_error("Soubor je příliš velký / File too large");auto package=readJson(file);Json sets;
   if(package.is_object()&&package.value("schema","")=="uaa-gameset-package-v1"&&package.contains("gameset"))sets=Json::array({package["gameset"]});
   else if(package.is_object()&&package.contains("gamesets")&&package["gamesets"].is_array())sets=package["gamesets"];
   else sets=Json::array({package});
   if(sets.empty())throw std::runtime_error("Prázdný soubor / Empty package");
   std::vector<fs::path> importedAssets;
   std::function<void(Json&)> decode=[&](Json& node){
    if(node.is_string()&&node.get_ref<const std::string&>().rfind("data:image/",0)==0){
     auto url=node.get<std::string>();auto comma=url.find(',');if(comma==std::string::npos||url.substr(0,comma).find(";base64")==std::string::npos)throw std::runtime_error("Neplatný obrázek / Invalid image");
     const std::string alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::vector<unsigned char> bytes;unsigned acc=0;int bits=-8;for(std::size_t i=comma+1;i<url.size();++i){if(url[i]=='=')break;auto n=alphabet.find(url[i]);if(n==std::string::npos){if(std::isspace(static_cast<unsigned char>(url[i])))continue;throw std::runtime_error("Neplatný base64 obrázek / Invalid base64 image");}acc=(acc<<6)|unsigned(n);bits+=6;if(bits>=0){bytes.push_back((acc>>bits)&255);bits-=8;}}
     int w=0,h=0,channels=0;unsigned char* pixels=nullptr;void* webp=nullptr;bool webpPixels=false;
     if(url.substr(0,comma).find("image/webp")!=std::string::npos){webp=SDL_LoadObject("libwebp.so.7");if(!webp)webp=SDL_LoadObject("libwebp.dll");if(webp){auto fn=reinterpret_cast<unsigned char*(*)(const unsigned char*,std::size_t,int*,int*)>(SDL_LoadFunction(webp,"WebPDecodeRGBA"));if(fn)pixels=fn(bytes.data(),bytes.size(),&w,&h);webpPixels=true;}}
     else pixels=stbi_load_from_memory(bytes.data(),int(bytes.size()),&w,&h,&channels,4);
     if(!pixels||w<1||h<1||w>8192||h>8192){if(pixels){if(webpPixels){auto fn=reinterpret_cast<void(*)(void*)>(SDL_LoadFunction(webp,"WebPFree"));if(fn)fn(pixels);}else stbi_image_free(pixels);}if(webp)SDL_UnloadObject(webp);throw std::runtime_error("Obrázek nelze načíst; použij PNG/JPG / Cannot decode image; use PNG/JPG");}
     auto relative="user-assets/"+editorId()+".bmp";fs::create_directories(game.userDir/"user-assets");auto file=game.userDir/relative;auto surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels,w,h,32,w*4,SDL_PIXELFORMAT_RGBA32);int saved=surface?SDL_SaveBMP(surface,file.string().c_str()):-1;if(surface)SDL_FreeSurface(surface);if(webpPixels){auto fn=reinterpret_cast<void(*)(void*)>(SDL_LoadFunction(webp,"WebPFree"));if(fn)fn(pixels);SDL_UnloadObject(webp);}else stbi_image_free(pixels);if(saved)throw std::runtime_error(SDL_GetError());importedAssets.push_back(file);node=relative;
    }else if(node.is_structured())for(auto& child:node)decode(child);
   };
   try{for(auto& set:sets)decode(set);for(const auto& set:sets){auto checked=game.validateGameset(set);if(!checked.ok)throw std::runtime_error(checked.message);}}catch(...){for(const auto& file:importedAssets){std::error_code ec;fs::remove(file,ec);}throw;}
   for(const auto& set:sets){auto valid=game.validateGameset(set);if(!valid.ok)throw std::runtime_error(valid.message);}
   editorStore();for(auto set:sets){set["id"]=editorId();set["reference"]=false;editorSets.push_back(std::move(set));}editorSelect(int(editorSets.size())-1);editorDirty=true;editorFeedback=tr("Importováno jako vlastní kopie. Ulož sady.","Imported as your own copy. Save gamesets.");
  }catch(const std::exception& e){editorFeedback=e.what();}
 }
 void editorExport(){try{auto valid=game.validateGameset(custom);if(!valid.ok){editorFeedback=valid.message;return;}auto set=custom; // Embed user artwork so exported packages remain portable.
   std::function<void(Json&)> embed=[&](Json& node){if(node.is_string()&&node.get_ref<const std::string&>().rfind("user-assets/",0)==0){auto file=game.userDir/node.get<std::string>();std::ifstream f(file,std::ios::binary);if(!f)throw std::runtime_error("Chybí obrázek / Missing image");std::string bytes((std::istreambuf_iterator<char>(f)),{}),encoded;static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";unsigned acc=0;int bits=-6;for(unsigned char c:bytes){acc=(acc<<8)|c;bits+=8;while(bits>=0){encoded+=alphabet[(acc>>bits)&63];bits-=6;}}if(bits>-6)encoded+=alphabet[((acc<<8)>>(bits+8))&63];while(encoded.size()%4)encoded+='=';node="data:image/"+std::string(file.extension()==".png"?"png":file.extension()==".bmp"?"bmp":"jpeg")+";base64,"+encoded;}else if(node.is_structured())for(auto& child:node)embed(child);};embed(set);
   auto file=game.userDir/("UAA-gameset-"+std::to_string(editorSet)+".json");writeJson(file,{{"schema","uaa-gameset-package-v1"},{"gameset",set}});editorFeedback=tr("Export: ","Export: ")+file.string();
  }catch(const std::exception& e){editorFeedback=e.what();}}
 void editorImportImage(const std::string& path,const fs::path& source){
  if(editorReadOnly())return;try{int w,h,n;auto bytes=stbi_load(source.string().c_str(),&w,&h,&n,4);if(!bytes)throw std::runtime_error("Použij obrázek PNG/JPG / Use a PNG/JPG image");stbi_image_free(bytes);if(w>8192||h>8192)throw std::runtime_error("Maximálně 8192 × 8192");auto extension=source.extension().string();if(extension!=".png"&&extension!=".jpg"&&extension!=".jpeg")throw std::runtime_error("Použij příponu .png nebo .jpg");auto relative="user-assets/"+editorId()+extension;fs::create_directories(game.userDir/"user-assets");fs::copy_file(source,game.userDir/relative);editorCheckpoint();custom[Json::json_pointer(path)]=relative;editorFeedback=tr("Obrázek zkopírován do vlastní sady.","Image copied to your gameset.");}catch(const std::exception& e){editorFeedback=e.what();}}
 std::string editorLabel(const std::string& key){
  static const std::map<std::string,std::pair<std::string,std::string>> labels={
   {"name",{"Název","Name"}},{"enName",{"Anglický název","English name"}},{"description",{"Popis","Description"}},{"author",{"Autor","Author"}},{"version",{"Verze","Version"}},{"startingCount",{"Počáteční počet","Starting count"}},{"availableAfterDays",{"Dostupné po dnech","Available after days"}},{"combat",{"Bojové údaje","Combat data"}},{"values",{"Hodnoty / síla / dosah","Values / power / range"}},{"tu",{"Časové jednotky","Time units"}},{"curves",{"Křivky přesnosti","Accuracy curves"}},{"auto",{"Automat","Automatic"}},{"hip",{"Od boku","Hip fire"}},{"aimed",{"Zacíleně","Aimed fire"}},{"production",{"Výroba a ceny","Production and prices"}},{"productionHours",{"Výrobní hodiny","Production hours"}},{"productionCost",{"Výrobní cena","Production cost"}},{"marketPrice",{"Tržní cena","Market price"}},{"materials",{"Potřebné materiály","Required materials"}},{"researchHours",{"Hodiny výzkumu","Research hours"}},{"researchScientists",{"Počet vědců","Scientists"}},{"researchResult",{"Výsledek výzkumu","Research result"}},{"researchField",{"Obor výzkumu","Research field"}},{"unitStats",{"Vlastnosti jednotky","Unit statistics"}},{"ufoStats",{"Vlastnosti UFO","UFO statistics"}},{"specs",{"Technické údaje","Specifications"}},{"image",{"Obrázek","Image"}},{"localized",{"Překlady CZ/EN","CZ/EN translations"}},{"finance",{"Finance a státy","Finance and countries"}}};
  auto i=labels.find(key);return i==labels.end()?key:english?i->second.second:i->second.first;
 }
 int editorX(int x)const{return 16+(x-202)*992/812;}
 int editorY(int y)const{return 16+(y-62)*736/694+((y>=100&&y<647)?44:0);}
 int editorW(int w)const{return w*992/812;}
 int editorH(int h)const{return h*736/694;}
 SDL_Rect editorArea(SDL_Rect r)const{return {editorX(r.x),editorY(r.y),editorW(r.w),editorH(r.h)};}
 void editorTool(const std::string& icon,const std::string& title,int x,int y,int w,int h,std::function<void()> action){
  control("",x,y,w,h,std::move(action));
  int ix=x+10,iy=y+(h-18)/2;
  auto stroke=[&](int a,int b,int c,int d){line(ix+a,iy+b,ix+c,iy+d,bright);};
  auto outline=[&](int a,int b,int width,int height){stroke(a,b,a+width,b);stroke(a+width,b,a+width,b+height);stroke(a+width,b+height,a,b+height);stroke(a,b+height,a,b);};
  if(icon=="save"){outline(1,1,16,16);outline(5,1,8,5);outline(5,11,8,6);}
  else if(icon=="import"||icon=="export"){stroke(1,12,1,17);stroke(1,17,17,17);stroke(17,17,17,12);bool down=icon=="import";stroke(9,down?1:12,9,down?12:1);stroke(9,down?12:1,4,down?7:6);stroke(9,down?12:1,14,down?7:6);}
  else if(icon=="undo"||icon=="redo"){bool back=icon=="undo";auto arrow=[&](int a,int b,int c,int d){stroke(back?a:18-a,b,back?c:18-c,d);};arrow(2,6,12,6);arrow(12,6,16,10);arrow(16,10,16,15);arrow(2,6,7,1);arrow(2,6,7,11);}
  else if(icon=="check"){stroke(2,9,7,14);stroke(7,14,17,3);stroke(2,10,7,15);stroke(7,15,17,4);}
  else if(icon=="exit"){outline(1,1,8,16);stroke(5,9,18,9);stroke(18,9,13,4);stroke(18,9,13,14);}
  else if(icon=="add"||icon=="add-field"){if(icon=="add-field")outline(1,1,16,16);stroke(9,4,9,14);stroke(4,9,14,9);}
  else if(icon=="rename"){stroke(2,15,4,10);stroke(4,10,13,1);stroke(13,1,17,5);stroke(17,5,8,14);stroke(8,14,2,15);stroke(11,3,15,7);stroke(2,17,17,17);}
  else if(icon=="delete"){stroke(2,4,16,4);stroke(6,1,12,1);stroke(6,1,6,4);stroke(12,1,12,4);outline(4,4,10,13);stroke(7,7,7,14);stroke(11,7,11,14);}
  else if(icon=="remove-field"){outline(1,1,16,16);stroke(4,9,14,9);}
  else if(icon=="image"){outline(1,1,16,16);outline(11,4,2,2);stroke(2,14,7,8);stroke(7,8,11,13);stroke(11,13,14,10);stroke(14,10,17,14);}
  boundedText(title,{x+36,y+(h-10)/2,w-42,16},10,text);
 }
 void editor(){
  card(tr("EDITOR GAMESETU","GAMESET EDITOR"),editorX(202),editorY(62),editorW(812),editorH(694));
  control(tr("NOVÁ SADA","NEW SET"),813,editorY(71),177,editorH(25),[this]{editorStore();auto fresh=Json{{"id",editorId()},{"name",tr("Nová sada","New gameset")},{"author",""},{"version","1.0"},{"description",""},{"reference",false},{"localized",Json::object()},{"finance",{{"countries",Json::array()},{"notes",""}}}};for(const auto* key:{"missions","technologies","weapons","equipment","units","buildings","aliens","factions"})fresh[key]=Json::array();editorSets.push_back(fresh);editorSelect(int(editorSets.size())-1);editorDirty=true;editorFeedback=tr("Prázdná autorská sada. Pro hraní použij kopii DUMMY.","Empty authoring set. Use a DUMMY copy for gameplay.");});hits.back().name="gameset-empty";
  control("<",30,103,32,editorH(32),[this]{editorSelect((editorSet+int(editorSets.size())-1)%int(editorSets.size()));});hits.back().name="gameset-previous";
  control(std::to_string(editorSet+1)+" / "+std::to_string(editorSets.size())+" · "+nameOf(custom)+(editorReadOnly()?tr(" · CHRÁNĚNÝ"," · PROTECTED"):""),72,103,475,editorH(32),[this]{requestGamesetRename();});hits.back().name="gameset-select";
  control(">",557,103,32,editorH(32),[this]{editorSelect((editorSet+1)%int(editorSets.size()));});hits.back().name="gameset-next";
  control(tr("KOPÍROVAT","COPY"),602,103,188,editorH(32),[this]{copyGameset();});hits.back().name="gameset-copy";
  control(tr("SMAZAT","DELETE"),802,103,188,editorH(32),[this]{requestGamesetDelete();});hits.back().name="gameset-delete-set";

  const std::vector<std::pair<std::string,std::string>> tabs={{"_set",tr("SADA / PŘEKLADY","SET / LANGUAGES")},{"buildings",tr("BUDOVY","BUILDINGS")},{"units",tr("JEDNOTKY","UNITS")},{"weapons",tr("ZBRANĚ","WEAPONS")},{"equipment",tr("VYBAVENÍ","EQUIPMENT")},{"technologies",tr("VÝZKUM","RESEARCH")},{"aliens",tr("UFO / VETŘELCI","UFO / ALIENS")},{"missions",tr("MISE / UDÁLOSTI","MISSIONS / EVENTS")},{"factions",tr("FRAKCE","FACTIONS")},{"finance",tr("FINANCE / STÁTY","FINANCE / COUNTRIES")}};
  for(int i=0;i<int(tabs.size());++i){auto tab=tabs[i];control(tab.second,30+(i%5)*194,editorY(147+(i/5)*35),184,editorH(29),[this,id=tab.first]{group=id;editorPath.clear();editorFilter.clear();editorScroll=editorRecordScroll=index=0;},group==tab.first);hits.back().name="gameset-category:"+tab.first;}
  bool collection=custom.contains(group)&&custom[group].is_array();
  if(collection&&!custom[group].empty()){
   editorTool("rename",tr("PŘEJMENOVAT","RENAME"),654,editorY(225),164,editorH(28),[this]{requestObjectRename();});hits.back().name="gameset-object-rename";
   editorTool("delete",tr("SMAZAT OBJEKT","DELETE OBJECT"),828,editorY(225),162,editorH(28),[this]{requestObjectDelete();});hits.back().name="gameset-object-delete";
  }

  if(collection){auto& list=custom[group];control(tr("HLEDAT: ","FIND: ")+editorFilter,editorX(214),editorY(225),editorW(238),editorH(28),[this]{input(editorFilter,false,[this](const std::string& v){editorFilter=v;editorRecordScroll=0;});});hits.back().name="gameset-search";
   std::vector<int> visible;for(int i=0;i<int(list.size());++i)if(editorFilter.empty()||nameOf(list[i]).find(editorFilter)!=std::string::npos||list[i]["id"].get<std::string>().find(editorFilter)!=std::string::npos)visible.push_back(i);
   index=std::clamp(index,0,std::max(0,int(list.size())-1));listSlider(editorRecordScroll,int(visible.size()),8,editorX(214),editorY(261),editorW(238),editorH(280),"gameset-records");
   for(int row=0,n=editorRecordScroll;n<int(visible.size())&&row<8;++n,++row){int i=visible[n];control("",editorX(214),editorY(261+row*35),editorW(238),editorH(31),[this,i]{index=i;editorPath.clear();editorScroll=0;},index==i);boundedText(nameOf(list[i]),editorArea({223,267+row*35,218,20}),10,text);hits.back().name="gameset-record:"+std::to_string(i);}
   if(!list.empty()){
    bool chart=editorPath.find("/combat/curves/")!=std::string::npos;auto& r=list[index];
    if(chart&&r.contains("combat")){auto mode=editorPath.substr(editorPath.find_last_of('/')+1);auto& combat=r["combat"];if(combat.contains("curves")&&combat["curves"].contains(mode))combatCurve(combat["curves"][mode],combat.value("values",Json::object()).value("range",0.),editorLabel(mode),combat.value("tu",Json::object()).value(mode,0),editorX(220),editorY(547),editorW(226),editorH(96));}
    else image(r.value("image",""),editorX(220),editorY(548),editorW(226),editorH(92));
   }
  }else{boundedText(tr("Otevři skupinu a kliknutím uprav jednotlivá pole. Čísla, texty, přepínače, seznamy i křivky se upravují přímo zde.","Open a section and click a field to edit. Numbers, text, switches, lists and curves are edited here."),editorArea({220,265,226,150}),12,text);}
  std::string root=group=="_set"?"":collection?(custom[group].empty()?"": "/"+group+"/"+std::to_string(index)):"/"+group;
  if(editorPath.rfind(root,0)!=0)editorPath=root;if(editorPath.empty())editorPath=root;
  if(!collection||!custom[group].empty()){
   Json* node=nullptr;try{node=&custom.at(Json::json_pointer(editorPath));}catch(...){editorPath=root;node=&custom.at(Json::json_pointer(root));}
   if(!node->is_structured()){editorPath=root;node=&custom.at(Json::json_pointer(root));}
   control(tr("< ZPĚT","< BACK"),348,editorY(225),94,editorH(28),[this,root]{if(editorPath!=root){editorPath=editorPath.substr(0,editorPath.find_last_of('/'));if(editorPath.size()<root.size())editorPath=root;editorScroll=0;}});hits.back().name="gameset-back";
   boundedText(editorPath.empty()?tr("ÚDAJE SADY","SET DETAILS"):editorPath,SDL_Rect{454,editorY(231),190,editorH(20)},10,dim);
   std::vector<std::pair<std::string,std::string>> fields;
   for(auto it=node->begin();it!=node->end();++it){auto key=node->is_array()?std::to_string(it-node->begin()):it.key();if(group=="_set"&&editorPath.empty()&&(it.value().is_array()||key=="reference"||key=="finance"))continue;if(editorReadOnly()&&key=="reference")continue;fields.push_back({key,editorPath+"/"+pointerPart(key)});}
   listSlider(editorScroll,int(fields.size()),11,editorX(474),editorY(263),editorW(514),editorH(363),"gameset-fields");
   for(int row=0,n=editorScroll;n<int(fields.size())&&row<11;++n,++row){auto field=fields[n];const auto& value=custom.at(Json::json_pointer(field.second));int y=263+row*33;control("",editorX(474),editorY(y),editorW(514),editorH(29),[this,path=field.second]{editorEdit(path);});hits.back().name="gameset-field:"+field.second;
    std::string title=node->is_array()?"["+field.first+"]":editorLabel(field.first),preview=value.is_string()?value.get<std::string>():value.is_boolean()?(value.get<bool>()?tr("ANO","YES"):tr("NE","NO")):value.is_structured()?std::to_string(value.size())+tr(" položek >"," entries >"):value.dump();boundedText(title,editorArea({482,y+7,215,18}),10,dim);boundedText(preview,editorArea({704,y+7,274,18}),10,text);
   }
  }
  editorTool("save",tr("ULOŽIT","SAVE"),30,59,128,editorH(30),[this]{saveGamesetLibrary();});hits.back().name="gameset-save";
  editorTool("import",tr("IMPORT","IMPORT"),168,59,128,editorH(30),[this]{input("",false,[this](const std::string& file){editorImport(fs::u8path(file));});});hits.back().name="gameset-import";
  editorTool("export",tr("EXPORT","EXPORT"),306,59,128,editorH(30),[this]{editorExport();});hits.back().name="gameset-export";
  editorTool("undo",tr("ZPĚT KROK","UNDO"),444,59,128,editorH(30),[this]{if(editorReadOnly()||editorUndo.empty())return;editorRedo.push_back(custom);custom=editorUndo.back();editorUndo.erase(editorUndo.end()-1);editorPath.clear();editorDirty=true;});hits.back().name="gameset-undo";
  editorTool("redo",tr("ZNOVU","REDO"),582,59,128,editorH(30),[this]{if(editorReadOnly()||editorRedo.empty())return;editorUndo.push_back(custom);custom=editorRedo.back();editorRedo.erase(editorRedo.end()-1);editorPath.clear();editorDirty=true;});hits.back().name="gameset-redo";
  editorTool("check",tr("OVĚŘIT","VALIDATE"),720,59,128,editorH(30),[this]{editorFeedback=game.validateGameset(custom,true).message;});hits.back().name="gameset-validate";
  editorTool("exit",tr("MENU","MENU"),858,59,132,editorH(30),[this]{editorStore();go("menu");});hits.back().name="gameset-menu";
  boundedText((editorDirty?tr("NEULOŽENO · ","UNSAVED · "):"")+editorFeedback,editorArea({214,728,774,20}),10,bright);
 }
 void verifyGamesetEditorUI(){
  auto originalSets=editorSets,originalCustom=custom,originalUndo=editorUndo,originalRedo=editorRedo;auto originalGroup=group,originalPath=editorPath;int originalSet=editorSet,originalIndex=index,originalNewGame=newGameSet;bool originalDirty=editorDirty;auto originalSelected=game.selectedGameset;
  auto render=[this]{hits.clear();listScrollRegions.clear();layoutFrames.clear();rect(0,0,W,H,bg);editor();};
  auto click=[this](const std::string& id){auto hit=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.name==id;});if(hit==hits.end())throw std::runtime_error("Missing gameset UI action: "+id);auto action=hit->action;action();};
  group="weapons";editorSelect(0);render();auto pristineReference=game.referenceGameset();click("gameset-object-rename");if(editing)throw std::runtime_error("DUMMY object rename must be blocked");click("gameset-object-delete");if(editing)throw std::runtime_error("DUMMY object deletion must be blocked");click("gameset-select");if(editing||custom["name"]!=pristineReference["name"])throw std::runtime_error("DUMMY rename must be blocked");loadGamesetLibrary();if(custom!=pristineReference)throw std::runtime_error("DUMMY must keep its original reference data and name");group="weapons";render();click("gameset-delete-set");if(editing)throw std::runtime_error("Reference deletion must be blocked");click("gameset-copy");render();auto oldName=custom["name"];click("gameset-select");editText="Cancelled";finishInput(false);if(custom["name"]!=oldName)throw std::runtime_error("Cancelled rename changed the set");render();click("gameset-select");editText="My custom set";finishInput(true);loadGamesetLibrary();editorSelect(int(editorSets.size())-1);if(custom["name"]!="My custom set"||game.referenceGameset()!=pristineReference)throw std::runtime_error("Custom rename did not persist or changed DUMMY");group="weapons";render();auto deletionCount=editorSets.size();click("gameset-delete-set");modal();click("modal-cancel");if(editorSets.size()!=deletionCount)throw std::runtime_error("Cancelled deletion removed a set");render();std::string deleteId=custom["id"];click("gameset-delete-set");modal();if(std::any_of(hits.begin(),hits.end(),[](const Hit& h){return h.name=="modal-field";}))throw std::runtime_error("Delete confirmation must not ask for a technical ID");click("modal-confirm");if(editorSets.size()!=deletionCount-1||!editorReadOnly())throw std::runtime_error("Confirmed copy deletion failed");loadGamesetLibrary();if(editorSets.size()!=deletionCount-1)throw std::runtime_error("Deleted copy returned after library reload");group="weapons";render();auto reference=custom;editorSetValue("/weapons/0/name","Changed reference");if(custom!=reference)throw std::runtime_error("DUMMY protection failed");click("gameset-copy");render();if(editorReadOnly())throw std::runtime_error("Copy stayed read-only");
  auto objectId=custom["weapons"][0]["id"];auto objectName=custom["weapons"][0]["name"];click("gameset-object-rename");editText="Cancelled object name";finishInput(false);if(custom["weapons"][0]["name"]!=objectName)throw std::runtime_error("Cancelled object rename changed data");render();click("gameset-object-rename");editText="Renamed object";finishInput(true);if(custom["weapons"][0]["name"]!="Renamed object"||custom["weapons"][0]["id"]!=objectId)throw std::runtime_error("Object rename changed identity or failed");
  index=int(custom["weapons"].size())-1;auto objectCount=custom["weapons"].size();auto deletedObjectId=custom["weapons"][index]["id"];render();click("gameset-object-delete");modal();click("modal-cancel");if(custom["weapons"].size()!=objectCount)throw std::runtime_error("Cancelled object delete changed data");render();click("gameset-object-delete");modal();click("modal-confirm");if(custom["weapons"].size()!=objectCount-1)throw std::runtime_error("Object delete failed");
  auto objectUndo=editorUndo;std::string activeObjectSet=custom["id"];loadGamesetLibrary();for(int i=1;i<int(editorSets.size());++i)if(editorSets[i]["id"]==activeObjectSet){editorSelect(i);break;}if(custom["weapons"].size()!=objectCount-1||std::any_of(custom["weapons"].begin(),custom["weapons"].end(),[&](const Json& row){return row["id"]==deletedObjectId;}))throw std::runtime_error("Deleted object returned after reload");editorUndo=objectUndo;group="weapons";render();click("gameset-undo");if(custom["weapons"].size()!=objectCount)throw std::runtime_error("Object deletion cannot be undone");index=0;render();

  click("gameset-field:/weapons/0/name");if(!editing)throw std::runtime_error("Name editor failed");editText="UI custom pistol";finishInput(true);render();
  click("gameset-field:/weapons/0/production");render();click("gameset-field:/weapons/0/production/productionCost");editText="1234";finishInput(true);render();click("gameset-back");render();click("gameset-field:/weapons/0/combat");render();click("gameset-field:/weapons/0/combat/tu");render();click("gameset-field:/weapons/0/combat/tu/hip");editText="7";finishInput(true);
  if(custom["weapons"][0]["production"]["productionCost"]!=1234||custom["weapons"][0]["combat"]["tu"]["hip"]!=7)throw std::runtime_error("Nested field values lost");render();click("gameset-undo");if(custom["weapons"][0]["combat"]["tu"]["hip"]==7)throw std::runtime_error("Undo failed");render();click("gameset-redo");if(custom["weapons"][0]["combat"]["tu"]["hip"]!=7)throw std::runtime_error("Redo failed");
  for(const auto* category:{"aliens","missions","factions","finance","_set","technologies","buildings","units","equipment","weapons"}){render();click(std::string("gameset-category:")+category);render();}
  editorSetValue("/finance/countries/0/contribution","1234");editorSetValue("/finance/countries/0/contribution","null");if(!custom["finance"]["countries"][0]["contribution"].is_null())throw std::runtime_error("Cannot restore dynamic country contribution");
  editorPath.clear();index=0;render();click("gameset-save");if(editorDirty||!fs::exists(game.userDir/"gamesets.json"))throw std::runtime_error("Library save failed: "+editorFeedback);auto saved=custom;loadGamesetLibrary();editorSelect(int(editorSets.size())-1);if(custom!=saved)throw std::runtime_error("Saved library reload lost edits");group="weapons";
  editorExport();auto exported=game.userDir/("UAA-gameset-"+std::to_string(editorSet)+".json");if(!fs::exists(exported))throw std::runtime_error("Export failed: "+editorFeedback);editorImport(exported);if(custom["weapons"][0]["name"]!=saved["weapons"][0]["name"])throw std::runtime_error("Import roundtrip failed: "+editorFeedback);
  // Import a real PNG, export with embedded artwork, then import without external paths.
  editorImportImage("/weapons/0/image",game.dataDir/saved["weapons"][0]["image"].get<std::string>());if(custom["weapons"][0]["image"].get<std::string>().rfind("user-assets/",0)!=0)throw std::runtime_error("PNG import failed: "+editorFeedback);editorExport();exported=game.userDir/("UAA-gameset-"+std::to_string(editorSet)+".json");editorImport(exported);if(!game.validateGameset(custom,true).ok)throw std::runtime_error("Portable artwork import failed: "+editorFeedback);
  editorStore();newGameSet=editorSet;go("new-game");hits.clear();utilityMenu();auto start=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.name==tr("SPUSTIT NOVOU HRU","START NEW GAME");});if(start==hits.end())throw std::runtime_error("Missing new-game start button");auto action=start->action;action();if(page!="globe"||!placeBase)throw std::runtime_error("Edited gameset not playable: "+editorFeedback);page="editor";if(game.record("weapons","pistol")->at("production")["productionCost"]!=1234)throw std::runtime_error("Editor changes not active in game");
  editorSets=originalSets;custom=originalCustom;editorUndo=originalUndo;editorRedo=originalRedo;editorSet=originalSet;newGameSet=originalNewGame;group=originalGroup;index=originalIndex;editorPath=originalPath;editorScroll=editorRecordScroll=0;editorDirty=originalDirty;editorFeedback=tr("Ověřeno: editace, uložení, import/export a nová hra.","Verified: editing, save, import/export and new game.");game.selectedGameset=originalSelected;editing=false;render();
  std::cout<<"Gameset editor: protected DUMMY, copy, nested prices/TU, undo/redo, all categories, add/delete, library, portable artwork import/export and new game passed\n";
 }
