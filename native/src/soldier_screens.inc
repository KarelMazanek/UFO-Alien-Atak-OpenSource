 void portrait(int person,int x,int y,int w,int h){
  const std::string path="ui/soldier-portraits-ten-bright.png";image(path,-10000,-10000,1,1);auto it=textures.find(path);if(it==textures.end()||!it->second)return;
  const auto& p=game.state["personnel"][person];int type=std::clamp(p.value("portraitType",person%5),0,4);bool female=p.value("portraitGender",std::string("male"))=="female";
  SDL_Rect src{int(type*1983./5)+4,female?4:400,int(1983./5)-8,int(793./2)-8},dst{x,y,w,h};SDL_RenderCopy(renderer,it->second,&src,&dst);
 }
 void soldierFigure(int person,int x,int y,int w,int h){
  // Same 240 × 360 coordinate system as soldierFieldSoldierGraphic in HTML.
  double scale=std::min(w/240.,h/360.);int left=x+int((w-240*scale)/2),top=y+int((h-360*scale)/2);
  imageStretch("ui/soldier-armor-bright.png",{left+int(17.6*scale),top+int(45*scale),int(204.8*scale),int(307.2*scale)});
  const auto& personData=game.state["personnel"][person];int kind=std::clamp(personData.value("portraitType",person%5),0,4);bool female=personData.value("portraitGender",std::string("male"))=="female",bald=!female&&kind==1;
  auto polygon=[&](const std::string& path,const std::vector<SDL_FPoint>& shape,float centerX,float centerY,float originX,float originY,float sx,float sy,float tx,float ty,float tw,float th){
   image(path,-10000,-10000,1,1);auto texture=textures.find(path);if(texture==textures.end()||!texture->second)return;
   int iw,ih;SDL_QueryTexture(texture->second,nullptr,nullptr,&iw,&ih);std::vector<SDL_Vertex> vertices;std::vector<int> indices;
   auto add=[&](float px,float py){vertices.push_back({{float(left+(originX+px*sx)*scale),float(top+(originY+py*sy)*scale)},{255,255,255,255},{(tx+px*tw)/iw,(ty+py*th)/ih}});};
   add(centerX,centerY);for(const auto& point:shape)add(point.x,point.y);for(int i=0;i<int(shape.size());++i){indices.push_back(0);indices.push_back(i+1);indices.push_back((i+1)%shape.size()+1);}SDL_RenderGeometry(renderer,texture->second,vertices.data(),int(vertices.size()),indices.data(),int(indices.size()));
  };
  if(bald)imageStretch("ui/soldier-bald-head-neck-realistic.png",{left+int(94*scale),top+int(5*scale),int(52*scale),int(62*scale)});
  else{
   rect(left+int(112*scale),top+int(49*scale),std::max(1,int(16*scale)),std::max(1,int(20*scale)),{101,116,81});
   polygon("ui/soldier-portraits-ten-bright.png",{{30,46},{40,25},{75,8},{115,4},{160,9},{198,25},{218,48},{224,125},{218,203},{204,247},{164,286},{161,326},{80,326},{78,286},{43,260},{22,207},{16,130}},120,165,97,8,46.f/240,52.f/330,kind*1983.f/5+78,(female?0.f:793.f/2)+12,1,1);
  }
  // The front collar overlaps the textured neck exactly as in the HTML SVG.
  polygon("ui/soldier-armor-bright.png",{{103,61},{112,64},{120,65},{129,64},{137,61},{135,77},{105,77}},120,70,0,0,1,1,-17.6f*5,-45.f*5,5,5);

 }
 int soldierStat(const Json& p,const std::string& key)const{
  if(key=="HEALTH")return p.value("health",100);int fallback=key=="MEDICAL"?20:p.value("skill",50);return p.contains("attributes")?p["attributes"].value(key,fallback):fallback;
 }
 void selectEquipment(const std::string& slot,int i=0){equipmentSlot=slot;equipmentIndex=i;equipmentScroll=0;notice.clear();}
 std::string itemName(const std::string& id){if(id.empty())return tr("PRÁZDNÉ","EMPTY");const auto* r=game.record("weapons",id);if(!r)r=game.record("equipment",id);return r?nameOf(*r):id;}
 void equippedControl(const std::string& slot,const std::string& id,int x,int y,int w,int h,int i=0){
  control("",x,y,w,h,[this,slot,i]{selectEquipment(slot,i);});hits.back().name="equip:"+slot+":"+std::to_string(i);
  SDL_Rect clip{x,y,w,h};SDL_RenderSetClipRect(renderer,&clip);
  if(!id.empty()){const auto* r=game.record("weapons",id);if(!r)r=game.record("equipment",id);
   if(w<70){if(r)image(r->value("image",""),x+2,y+2,w-4,h-4);}
   else{if(r)image(r->value("image",""),x+4,y+3,std::min(50,w/3),h-6);wrap(itemName(id),x+std::min(56,w/3+6),y+8,w-std::min(60,w/3+10),10);}
  }else if(w>=70)small(tr("PRÁZDNÉ","EMPTY"),x+8,y+(h-10)/2,dim,10);
  SDL_RenderSetClipRect(renderer,nullptr);
 }
 void soldierView(){
  auto& people=game.state["personnel"];
  if(selectedSoldier<0||selectedSoldier>=int(people.size())||people[selectedSoldier].value("role","")!="soldier"||people[selectedSoldier].value("dead",false)){selectedSoldier=-1;for(int i=0;i<int(people.size());++i)if(people[i].value("role","")=="soldier"&&!people[i].value("dead",false)){selectedSoldier=i;break;}}
  if(selectedSoldier<0){control("BACK TO SOLDIERS",207,163,329,29,[this]{go("soldiers");});return;}
  auto& person=people[selectedSoldier];auto loadout=game.soldierLoadout(selectedSoldier);
  std::vector<int> soldiers;for(int i=0;i<int(people.size());++i)if(people[i].value("role","")=="soldier"&&!people[i].value("dead",false))soldiers.push_back(i);int selectedPosition=int(std::find(soldiers.begin(),soldiers.end(),selectedSoldier)-soldiers.begin());if(lastStripSoldier!=selectedSoldier){if(selectedPosition<soldierStripScroll)soldierStripScroll=selectedPosition;if(selectedPosition>=soldierStripScroll+10)soldierStripScroll=selectedPosition-9;lastStripSoldier=selectedSoldier;}soldierStripScroll=std::clamp(soldierStripScroll,0,std::max(0,int(soldiers.size())-10));
  for(int row=0,n=soldierStripScroll;n<int(soldiers.size())&&row<10;++n,++row){int i=soldiers[n],xx=192+row*80;control("",xx,62,76,70,[this,i]{selectedSoldier=i;equipmentSlot.clear();},selectedSoldier==i);hits.back().name="soldier-strip:"+std::to_string(i);portrait(i,xx+17,65,42,46);SDL_Rect clip{xx,62,76,70};SDL_RenderSetClipRect(renderer,&clip);small(people[i]["name"],xx+3,115,text,10);SDL_RenderSetClipRect(renderer,nullptr);}
  listSlider(soldierStripScroll,int(soldiers.size()),10,192,62,802,70,"soldier-strip");
  if(page=="soldier-profile"){
   card("",192,140,359,524);card("COMBAT ATTRIBUTES",559,140,457,566);
   control(tr("ZPĚT NA VOJÁKY","BACK TO SOLDIERS"),207,163,329,29,[this]{go("soldiers");});
   box(210,209,80,100);small("CURRENT RANK",216,216,dim,10);small(person.value("rank","Private (PV1)"),216,237,text,10);
   control(tr("VYBAVIT","EQUIP"),214,278,70,23,[this]{go("soldier-equipment");});
   small("SOLDIER NAME",307,210,dim,10);control(person["name"],307,225,229,30,[this]{input(game.state["personnel"][selectedSoldier]["name"],false,[this](const std::string& name){result(game.renamePerson(selectedSoldier,name));});});
   small("MISSIONS · "+std::to_string(person.value("missions",0)),307,276,text,10);small("HEALTH · "+std::to_string(person.value("health",100)),307,299,text,10);soldierFigure(selectedSoldier,207,341,329,308);
   const std::vector<std::pair<std::string,std::string>> stats={{"TIME_UNITS","TIME UNITS"},{"STAMINA","STAMINA"},{"HEALTH","HEALTH"},{"REACTIONS","REACTIONS"},{"BRAVERY","BRAVERY"},{"MARKSMANSHIP","FIRING ACCURACY"},{"THROWING_ACCURACY","THROWING ACCURACY"},{"STRENGTH","STRENGTH"},{"COMMAND","COMMAND"},{"MEDICAL","MEDICAL"}};
   int yy=185;for(const auto& [key,title]:stats){int value=std::clamp(soldierStat(person,key),0,100);small(title,574,yy,dim,10);small(std::to_string(value)+" / 100",725,yy,text,10);rect(792,yy+4,208,6,{23,39,23});rect(792,yy+4,value*208/100,6,accent);yy+=17;}
   card("AGILITY RADAR",574,357,426,334);constexpr double pi=3.141592653589793;const std::vector<std::string> radar={"MARKSMANSHIP","FITNESS","REACTIONS","COMMAND","BRAVERY","STRENGTH"};
   for(int ring=1;ring<=4;++ring)for(int i=0;i<6;++i){double a=-pi/2+i*pi/3,b=a+pi/3;int r=ring*24;line(787+int(std::cos(a)*r),530+int(std::sin(a)*r),787+int(std::cos(b)*r),530+int(std::sin(b)*r));}
   for(int i=0;i<6;++i){double a=-pi/2+i*pi/3,b=a+pi/3;int r=std::clamp(soldierStat(person,radar[i]),0,100),r2=std::clamp(soldierStat(person,radar[(i+1)%6]),0,100);line(787+int(std::cos(a)*r),530+int(std::sin(a)*r),787+int(std::cos(b)*r2),530+int(std::sin(b)*r2),bright);}
   card(tr("TĚLOCVIČNA · FYZICKÝ TRÉNINK","GYM · PHYSICAL TRAINING"),207,669,329,86);
   bool training=person.contains("gymTraining");control(training?tr("ZRUŠIT TRÉNINK","CANCEL TRAINING"):tr("TRÉNOVAT · +2 / 48 h","TRAIN · +2 / 48 h"),217,696,309,25,[this,training]{auto r=training?game.cancelGymTraining(selectedSoldier):game.startGymTraining(selectedSoldier);gymFeedback=r.ok?"":r.message;});hits.back().name="soldier-gym";
   if(training){double remaining=std::max(0.,person["gymTraining"]["ready"].get<double>()-game.state["hours"].get<double>());small(tr("ZBÝVÁ ","REMAINING ")+std::to_string(int(std::ceil(remaining)))+" h",217,729,bright,10);rect(370,733,155,5,edge);rect(370,733,int((48-std::min(48.,remaining))/48*155),5,accent);}
   else small(!game.facility("gym")?tr("Nejdříve postav tělocvičnu.","Build a gym first."):gymFeedback.empty()?tr("SÍLA · KONDICE · VÝDRŽ · REAKCE · ČJ","STRENGTH · FITNESS · STAMINA · REACTIONS · TU"):tr("Trénink nelze zahájit · ověř stav vojáka.","Cannot train · check soldier status."),217,729,dim,9);
   return;
  }
  card("SOLDIER EQUIPMENT",192,140,824,664);small(person["name"],207,170,bright,12);small(person.value("rank","Private (PV1)"),207,195,dim,10);control(tr("ZPĚT NA PROFIL","BACK TO PROFILE"),899,168,102,29,[this]{go("soldier-profile");});
  card("PRIMARY WEAPON",207,221,232,75);equippedControl("primaryWeapon",loadout["primaryWeapon"],216,247,214,40);
  card("SECONDARY WEAPON",207,304,232,72);equippedControl("secondaryWeapon",loadout["secondaryWeapon"],216,330,214,38);
  card("BACKPACK · 5 × 4",207,385,232,246);equippedControl("backpack","",216,411,214,38);
  auto cells=game.backpackPacking(loadout["backpack"]);if(!cells.is_null())for(int y=0;y<4;++y)for(int x=0;x<5;++x){int item=cells[y*5+x];int sx=216+x*43,sy=474+y*29;control(item<0?"":std::to_string(item+1),sx,sy,40,23,[this,item]{if(item>=0)result(game.equipSoldier(selectedSoldier,"backpack","",item));},item>=0);}
  wrap(tr("Kliknutím na obsah jej vrátíš do skladu.","Click packed contents to return them to storage."),216,601,214,10,dim);
  box(449,221,311,473);soldierFigure(selectedSoldier,459,232,291,374);small(person["name"],459,620,text,10);
  box(769,221,232,75);small("COMBAT LOAD",779,230,dim,10);std::ostringstream loadLabel;loadLabel<<std::fixed<<std::setprecision(1)<<game.loadoutWeight(loadout)<<" / "<<game.soldierCapacity(selectedSoldier)<<" kg";small(loadLabel.str(),779,250,text,10);
  card("ARMOR",769,305,232,165);small("HELMET",779,328,dim,10);small("TORSO",889,328,dim,10);equippedControl("helmet",loadout["armor"]["helmet"],779,344,102,48);equippedControl("torso",loadout["armor"]["torso"],889,344,102,48);small("ARMS",779,397,dim,10);small("LEGS",889,397,dim,10);equippedControl("arms",loadout["armor"]["arms"],779,412,102,48);equippedControl("legs",loadout["armor"]["legs"],889,412,102,48);
  box(769,478,232,121);wrap("EQUIPMENT BELT",778,487,65,10,dim);equippedControl("equipmentBelt",loadout["equipmentBelt"],848,487,144,40);small("4 SLOTS · SIZE 1 ONLY",778,540,dim,10);for(int i=0;i<4;++i)equippedControl("beltSlots",loadout["beltSlots"][i],778+i*38,559,32,32,i);
  card(tr("PODVODNÍ VÝBAVA","UNDERWATER EQUIPMENT"),207,640,232,122);equippedControl("divingSuit",loadout["divingSuit"],216,663,214,27);equippedControl("rebreather",loadout["rebreather"],216,697,214,27);equippedControl("divingHood",loadout["divingHood"],216,731,214,27);
  card("RADIATION OVER-SUIT",769,608,232,73);equippedControl("radiationSuit",loadout["radiationSuit"],778,634,214,38);
  card("MEDICAL EQUIPMENT",769,689,232,73);equippedControl("medicalKit",loadout["medicalKit"],778,715,214,38);
 }
 void equipmentPicker(){
  rect(0,0,W,H,{0,0,0,210});hits.push_back({{0,0,W,H},[]{},"equipment-overlay"});card(equipmentSlot,252,120,520,550);
  control(tr("ZAVŘÍT","CLOSE"),660,135,95,29,[this]{equipmentSlot.clear();});
  control(tr("ODEBRAT / PRÁZDNÉ","REMOVE / EMPTY"),269,180,486,34,[this]{if(equipmentSlot=="backpack"){equipmentSlot.clear();return;}auto r=game.equipSoldier(selectedSoldier,equipmentSlot,"",equipmentIndex);result(r);if(r.ok)equipmentSlot.clear();});
  std::vector<Json> choices;for(const auto& g:{"weapons","equipment"})for(const auto& r:game.gameset()[g])if(game.soldierItemAllowed(equipmentSlot,r["id"])&&game.state["stock"].value(r["id"].get<std::string>(),0)>0)choices.push_back(r);
  for(int row=0,i=equipmentScroll;i<int(choices.size())&&row<7;++i,++row){auto r=choices[i];std::string id=r["id"];int y=225+row*49;control("",269,y,486,43,[this,id]{auto r=game.equipSoldier(selectedSoldier,equipmentSlot,id,equipmentIndex);result(r);if(r.ok)equipmentSlot.clear();});hits.back().name="choose:"+id;image(r.value("image",""),274,y+3,58,37);small(nameOf(r),343,y+8,text,10);small("IN STOCK · "+std::to_string(game.state["stock"].value(id,0))+" · SIZE "+std::to_string(game.soldierItemSize(id)),343,y+24,dim,10);}
  listSlider(equipmentScroll,int(choices.size()),7,269,225,486,337,"soldier-equipment");
 }

 void aircraftCrewPicker(){
  rect(0,0,W,H,{0,0,0,210});hits.push_back({{0,0,W,H},[]{},"crew-overlay"});card("ASSIGN "+crewRole,252,140,520,490);
  control("CLOSE",660,153,95,29,[this]{crewRole.clear();});control("REMOVE ASSIGNMENT",269,200,486,34,[this]{auto r=game.assignCrew(activeCraft,crewRole,-1);result(r);if(r.ok)crewRole.clear();});
  int row=0,seen=0,total=0;for(const auto& p:game.state["personnel"])if(p["role"]==crewRole)++total;for(int i=0;i<int(game.state["personnel"].size())&&row<6;++i){const auto& p=game.state["personnel"][i];if(p["role"]!=crewRole)continue;if(seen++<equipmentScroll)continue;control(p["name"].get<std::string>()+" · SKILL "+std::to_string(p["skill"].get<int>()),269,250+row*47,486,39,[this,i]{auto r=game.assignCrew(activeCraft,crewRole,i);result(r);if(r.ok)crewRole.clear();});++row;}
  if(row==0)control("WORKFORCE MARKET",269,300,486,39,[this]{go("personnel");});
  listSlider(equipmentScroll,total,6,269,250,486,274,"aircraft-crew");
 }
