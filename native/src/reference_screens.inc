 // Native screen geometry follows the measured HTML 1024x768 logical viewport.
 // Reference rectangles and screenshots live in verification/html-reference.
 void small(const std::string& value,int x,int y,Color c=text,int size=10){label(value,x,y,c,std::max(10,size)/20.f);}
 std::vector<std::string> wrappedLines(const std::string& value,int width,int size=10){
  size=std::max(10,size);std::vector<std::string> lines;std::istringstream paragraphs(localized(value));std::string paragraph;
  while(std::getline(paragraphs,paragraph)){std::istringstream words(paragraph);std::string word,row;while(words>>word){if(!row.empty()&&textWidth(row+" "+word,size)>width){lines.push_back(row);row.clear();}if(textWidth(word,size)>width){if(!row.empty()){lines.push_back(row);row.clear();}std::string part;for(std::size_t pos=0;pos<word.size();){std::size_t end=pos+1;while(end<word.size()&&(static_cast<unsigned char>(word[end])&0xc0)==0x80)++end;auto letter=word.substr(pos,end-pos);if(!part.empty()&&textWidth(part+letter,size)>width){lines.push_back(part);part.clear();}part+=letter;pos=end;}row=part;}else row+=(row.empty()?"":" ")+word;}lines.push_back(row);}
  if(lines.empty())lines.push_back("");return lines;
 }
 void wrap(const std::string& value,int x,int y,int width,int size=10,Color c=text){size=std::max(10,size);for(const auto& row:wrappedLines(value,width,size)){small(row,x,y,c,size);y+=size+5;}}
 void boundedText(const std::string& value,SDL_Rect area,int size,Color color){SDL_Rect previous,clip=area;bool clipped=SDL_RenderIsClipEnabled(renderer);SDL_RenderGetClipRect(renderer,&previous);if(clipped&&!SDL_IntersectRect(&area,&previous,&clip))return;SDL_RenderSetClipRect(renderer,&clip);int y=area.y;for(const auto& line:wrappedLines(value,area.w,size)){if(y+std::max(10,size)>area.y+area.h)break;small(line,area.x,y,color,size);y+=std::max(10,size)+5;}SDL_RenderSetClipRect(renderer,clipped?&previous:nullptr);}
 void control(const std::string& title,int x,int y,int w,int h,std::function<void()> action,bool active=false,int size=10){
  size=std::max(10,size);
  bool over=mx>=x&&my>=y&&mx<x+w&&my<y+h;rect(x,y,w,h,active?Color{22,37,22}:over?Color{18,32,18}:Color{8,24,13});color(active?accent:edge);SDL_Rect r{x,y,w,h};SDL_RenderDrawRect(renderer,&r);
  SDL_Rect previous,clip=r;bool clipped=SDL_RenderIsClipEnabled(renderer);SDL_RenderGetClipRect(renderer,&previous);if(clipped&&!SDL_IntersectRect(&r,&previous,&clip))return;SDL_RenderSetClipRect(renderer,&clip);small(title,x+8,y+(h-size)/2,active?bright:text,size);SDL_RenderSetClipRect(renderer,clipped?&previous:nullptr);hits.push_back({clip,std::move(action),title});
 }
 void card(const std::string& title,int x,int y,int w,int h){box(x,y,w,h);if(!title.empty()){small(title,x+14,y+14,bright,8);line(x+14,y+34,x+w-14,y+34);}}
 void metric(const std::string& title,const std::string& value,int x,int y,int w){small(title,x,y,dim,8);small(value,x+w-textWidth(value,9),y,text,9);line(x,y+22,x+w,y+22,{20,34,20});}
 std::string cash(long long n){bool negative=n<0;if(negative)n=-n;std::string v=std::to_string(n);for(int i=int(v.size())-3;i>0;i-=3)v.insert(std::size_t(i),",");return (negative?"-$ ":"$ ")+v;}
 std::string recordText(const Json& record,const std::string& field){
  std::string language=english?"en":"cz";const Json& set=page=="editor"?custom:game.gameset();
  auto translated=[&](const Json& value)->std::string{if(value.is_object()&&value.contains(language)&&value[language].is_object()&&value[language].contains(field)&&value[language][field].is_string())return value[language][field];return "";};
  if(record.contains("localized")){auto value=translated(record["localized"]);if(!value.empty())return value;}
  if(record.contains("id")&&record["id"]==set["id"]&&set.contains("localized")){auto value=translated(set["localized"]);if(!value.empty())return value;}
  if(set.contains("objectTranslations")&&set["objectTranslations"].is_object())for(const auto& category:set["objectTranslations"]){if(category.is_object()&&category.contains(record.value("id",""))){auto value=translated(category[record["id"].get<std::string>()]);if(!value.empty())return value;}}
  return field=="name"&&english?record.value("enName",record.value("name",record.value("id",""))):record.value(field,field=="name"?record.value("id",""):std::string(""));
 }
 std::string nameOf(const Json& record){return recordText(record,"name");}
 void smallMono(const std::string& s,int x,int y,Color c,int size){monospaced=true;small(s,x,y,c,size);monospaced=false;}
 void header(){
  box(0,0,W,page=="globe"?52:54);smallMono(game.date(),8,16,bright,16);smallMono(cash(game.state["money"].get<long long>()),548,16,bright,18);

  timeControls();
 }
 void timeControls(){
  const std::vector<int> seconds={0,5,60,300,1800,3600,86400,604800};int x=220;
  for(int i=0;i<8;++i){int w=i>5?36:32;SDL_Rect r{x,10,w,32};bool active=speed==seconds[i]/3600.;rect(x,10,w,32,active?Color{27,48,27}:Color{8,24,13});color(active?accent:edge);SDL_RenderDrawRect(renderer,&r);
   if(i==0){rect(x+14,22,1,8,text);rect(x+18,22,1,8,text);}else{int arrows=(i==2||i==4||i==5)?2:i==3?3:1;int xx=x+8;if(i>5){small(i==6?"D":"W",x+5,22,text,8);xx+=9;}for(int n=0;n<arrows;++n){line(xx,22,xx+3,26,text);line(xx+3,26,xx,30,text);xx+=6;}}
   hits.push_back({r,[this,sec=seconds[i]]{ufoAwaitingInterception=false;speed=sec/3600.;},"time-speed:"+std::to_string(seconds[i])});x+=w+(i==0?7:5);
  }
 }
 void sidebar(){
  bool globePage=page=="globe",collapsed=!globePage&&commandCollapsed;int left=collapsed?48:184;
  box(0,globePage?52:54,left,globePage?582:H-54);small("C O M M A N D",8,68,dim,8);
  control(tr("GLÓBUS","GLOBE"),globePage?14:8,globePage?90:86,globePage?155:collapsed?30:124,globePage?29:38,[this]{go("globe");});
  if(!globePage)control(collapsed?">":"<",collapsed?8:136,collapsed?128:86,38,38,[this]{commandCollapsed=!commandCollapsed;});
  if(collapsed){control(tr("VÝZK.","RES."),4,180,40,52,[this]{commandCollapsed=false;go("research");},page=="research");control(tr("VÝR.","PROD."),4,240,40,52,[this]{commandCollapsed=false;go("production");},page=="production");return;}
  small(tr("ZÁKLADNY","BASES"),16,137,dim,8);
  const std::vector<std::tuple<std::string,std::string,std::string>> nav={
   {"BASE","ZÁKLADNA","bases"},{"LOCATIONS","LOKACE","locations"},{"AIRCRAFT","LETADLA","aircraft"},{"SOLDIERS","VOJÁCI","soldiers"},{"RESEARCH","VÝZKUM","research"},{"MANUFACTURE","VÝROBA","production"},{"STORAGE","SKLAD","storage"},{"WORKFORCE","PRACOVNÍ TRH","personnel"},{"FINANCE","FINANCE","finance"},{"UFOs","UFO","ufos"},{"GRAPHS","GRAFY","graphs"},{"UFOPEDIA","UFOPEDIE","catalog"},{"WIKI","WIKI","wiki"}};
  int y=globePage?151:149;for(const auto& [en,cz,p]:nav){control(tr(cz,en),globePage?14:8,y,globePage?155:166,29,[this,p]{if(p=="bases")openBaseMenu();else go(p);},page==p||(p=="production"&&page=="workshop")||(p=="research"&&page=="laboratory"));y+=36;}
 }
 SDL_Rect menuBounds()const{return {0,16,1024,735};}
 SDL_Rect menuRow(int i)const{auto r=menuBounds();return {r.x+555*r.w/1513,r.y+(280+71*i-(i>1?1:0))*r.h/1039,371*r.w/1513,53*r.h/1039};}
 void menu(){
  imageStretch("ui/main-menu.png",menuBounds());
  const std::vector<std::function<void()>> actions={
   [this]{go("new-game");},[this]{go("load");},[this]{go("options");},[this]{go("editor");},[this]{go("authors");},[this]{go("script");},[this]{go("map");},[this]{go("exit");}};
  const std::vector<std::string> englishMenu={"NEW GAME","LOAD GAME","SETTINGS","GAMESET","AUTHORS","SCRIPT","MAP EDITOR","EXIT"};
  for(int i=0;i<8;++i){auto r=menuRow(i);if(english||i==3){rect(r.x+45,r.y+4,r.w-50,r.h-8,panel);if(i==3){label("GAMESET",r.x+66,r.y+(r.h-18)/2,{116,225,145},.9f,true);label("GAMESET",r.x+67,r.y+(r.h-18)/2,{116,225,145},.9f,true);}else small(englishMenu[i],r.x+48,r.y+(r.h-14)/2,bright,14);}if(mx>=r.x&&my>=r.y&&mx<r.x+r.w&&my<r.y+r.h){rect(r.x+3,r.y+3,r.w-6,r.h-6,{20,122,45,45});color(accent);SDL_RenderDrawRect(renderer,&r);}hits.push_back({r,actions[i]});}
 }
 void options(){
  imageStretch("ui/settings.png",{20,33,984,703});box(158,276,640,286);
  const std::vector<std::string> titles={"HUDBA","ZVUKY","ROZLIŠENÍ","REŽIM ZOBRAZENÍ","RYCHLOST HRY","OBTÍŽNOST"};
  for(int i=0;i<6;++i){int y=282+int(i*46.2);box(166,y,624,42);small(titles[i],179,y+11,text,16);
   if(i<2){double value=i==0?(musicEnabled?musicVolume:0.):soundVolume;rect(442,y+17,279,6,{28,50,28});rect(442,y+17,int(value*279),6,accent);rect(442+int(value*273),y+7,6,26,bright);small(std::to_string(int(value*100))+"%",744,y+11,text,13);hits.push_back({{442,y+3,279,36},[this,i]{double value=std::clamp((mx-442)/279.,0.,1.);if(i==0){musicVolume=value;musicEnabled=value>0;musicFailed=false;}else soundVolume=value;},i==0?"music-volume":"sound-volume"});continue;}
   const std::vector<std::string> paces={"Pomalá","Normální","Rychlá"},difficulties={"Lehká","Normální","Těžká"};int ww,hh;SDL_GetWindowSize(window,&ww,&hh);
   std::string value=i==2?std::to_string(ww)+" × "+std::to_string(hh):i==3?(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP?"CELÁ OBRAZOVKA":"OKNO"):i==4?paces[paceIndex]:difficulties[difficultyIndex];
   control("<",442,y+3,37,33,[this,i]{changeOption(i,-1);});hits.back().name="option:"+std::to_string(i)+":-1";control(value,486,y+3,251,33,[this,i]{changeOption(i,1);},false,13);control(">",744,y+3,37,33,[this,i]{changeOption(i,1);});hits.back().name="option:"+std::to_string(i)+":1";
  }
  rect(202,585,620,70,{0,23,12});control("ZPĚT DO MENU",220,592,275,52,[this]{go("menu");});control("ULOŽIT NASTAVENÍ",515,592,285,52,[this]{if(saveUiOptions())notice=tr("Nastavení uložena.","Settings saved.");});
 }
 void utilityMenu(){
  if(page=="options"){options();return;}
  if(page=="new-game"){
   menu();hits.clear();rect(0,0,W,H,{0,0,0,205});card(tr("NOVÁ HRA","NEW GAME"),235,240,554,250);
   small(tr("HERNÍ SADA","GAMESET"),260,300,dim,11);control(editorSets.at(newGameSet).value("name","DUMMY")+" >",260,330,504,38,[this]{newGameSet=(newGameSet+1)%editorSets.size();});hits.back().name="new-game-gameset";
   control("ČESKY",260,380,200,32,[this]{english=false;languageChosen=true;saveUiOptions();},!english);hits.back().name="language-czech";control("ENGLISH",474,380,290,32,[this]{english=true;languageChosen=true;saveUiOptions();},english);hits.back().name="language-english";
   control(tr("ZRUŠIT","CANCEL"),260,425,200,38,[this]{go("menu");});control(tr("SPUSTIT NOVOU HRU","START NEW GAME"),474,425,290,38,[this]{std::cerr<<"New game: preparing first base"<<std::endl;editorStore();auto started=game.startNewGame(editorSets.at(newGameSet));if(!started.ok){editorFeedback=started.message;notice=started.message;return;}ufoAlertOpen=ufoAlertVisible=ufoAwaitingInterception=false;go("globe");placeBase=true;});boundedText(editorFeedback,{260,470,504,60},10,bright);return;
  }
  card(page=="load"?tr("NAČÍST / ULOŽIT HRU","LOAD / SAVE GAME"):page=="authors"?tr("AUTOŘI","AUTHORS"):page=="exit"?tr("UKONČIT HRU?","EXIT GAME?"):tr("SKRIPT","SCRIPT"),242,90,540,580);
  if(page=="load")for(int i=1;i<=5;++i){control(tr("NAČÍST SLOT ","LOAD SLOT ")+std::to_string(i),270,160+(i-1)*64,300,44,[this,i]{auto r=game.load(i);result(r);if(r.ok){ufoAlertOpen=ufoAlertVisible=ufoAwaitingInterception=false;go("globe");}});control(tr("ULOŽIT","SAVE"),584,160+(i-1)*64,170,44,[this,i]{try{game.save(i);notice=tr("Hra uložena.","Game saved.");}catch(const std::exception& e){notice=e.what();}});}
  if(page=="authors"){wrap("Karel Mazánek alias Mazil (Sovička). UFO Alien Atak.",270,180,480,14);wrap(tr("Vlastní zdrojový kód: open source, licence MIT.","Project source code: open source, MIT license."),270,250,480,12);wrap("Hudba / Music: © Karel Keslly Mazánek",270,310,480,14);wrap(tr("Hudba není součástí licence MIT. Všechna práva k hudbě vyhrazena.","Music is excluded from the MIT license. All rights reserved."),270,360,480,12);}
  if(page=="script")wrap(tr("Skriptovací prostředí zatím není převedeno do C++.","The scripting environment has not yet been ported to C++."),270,180,480,14);
  if(page=="exit")control(tr("UKONČIT","EXIT"),270,260,484,42,[this]{running=false;});
  control(tr("ZPĚT DO MENU","BACK TO MENU"),270,590,484,42,[this]{go("menu");});
 }
 void systemPanel(){
  box(819,52,205,368);small("S Y S T E M",833,72,dim,8);box(833,90,176,57);small(tr("POHOTOVOST","ALERT LEVEL"),841,99,dim,10);int level=game.alertLevel();Color color=alertColor(level);small(level==2?tr("KRITICKÁ","CRITICAL"):level==1?tr("ZVÝŠENÁ","ELEVATED"):tr("KLID","NORMAL"),919,99,color,10);rect(841,126,160,12,color);
  metric("BASES",std::to_string(game.state["bases"].size()),833,164,176);metric("SELECTED",game.state["bases"].empty()?"NONE":std::to_string(base+1),833,192,176);
  small("G L O B A L  D E F E N C E",833,236,dim,8);int radars=0,defences=0;for(const auto& b:game.state["bases"])for(const auto& item:b["buildings"]){std::string id=item["id"];radars+=id.find("radar")!=std::string::npos;defences+=id.find("defen")!=std::string::npos;}
  metric("RADAR",std::to_string(radars),833,260,176);metric("RADAR OUTPOSTS",std::to_string(game.state.value("radarOutposts",Json::array()).size()),833,284,176);metric("SATELLITES","—",833,308,176);metric("DEFENCE",std::to_string(defences),833,332,176);
  box(833,355,176,49);small("SELECTED BASE",842,363,dim,8);small(game.state["bases"].empty()?"NONE":game.state["bases"][std::min(base,int(game.state["bases"].size())-1)]["name"].get<std::string>(),842,385,text,8);
 }
 void geoscape(){
  rect(184,52,635,716,{2,4,2});SDL_Rect canvasClip{184,52,635,716};SDL_RenderSetClipRect(renderer,&canvasClip);globe();SDL_RenderSetClipRect(renderer,nullptr);systemPanel();
  if(game.state["bases"].empty())placeBase=true;
  if(placeBase){card("",295,65,410,62);small(tr("UMÍSTI PRVNÍ ZÁKLADNU","PLACE YOUR FIRST BASE"),311,78,bright,10);small(tr("Tažením otáčej · kliknutím vyber pevninu","Drag to rotate · click on land to select a location"),311,100,text,10);}

  small(tr("TAŽENÍM OTÁČEJ · KOLEČKEM PŘIBLIŽUJ","DRAG TO ROTATE · SCROLL TO ZOOM"),345,740,dim,8);
 }
 void bases(){
  auto& list=game.state["bases"];if(list.empty()){control("GLOBE",20,80,180,35,[this]{go("globe");placeBase=true;});wrap(tr("Nejdříve založ základnu na glóbu.","Create your first base on the globe."),250,150,500,14);return;}
  base=std::clamp(base,0,int(list.size())-1);auto& b=list[base];
  box(12,66,220,690);box(762,66,250,690);
  baseSummary(b);
  small(tr("PATRA BÁZE","BASE LEVELS"),774,80,dim,10);
  control("LEVEL "+std::to_string(floor),774,103,108,29,[this]{int count=game.state["bases"][base]["floors"];floor=floor<=-count?0:floor-1;});control("+ FLOOR",892,103,108,29,[this]{result(game.addFloor(base));});
  small("C O M M A N D",23,80,dim,8);int y=108;for(auto [title,p]:std::vector<std::pair<std::string,std::string>>{{"GLOBE","globe"},{"BASE","bases"},{"LOCATIONS","locations"},{"AIRCRAFT","aircraft"},{"SOLDIERS","soldiers"},{"RESEARCH","research"},{"MANUFACTURE","production"},{"STORAGE","storage"},{"WORKFORCE","personnel"},{"FINANCE","finance"},{"UFOs","ufos"},{"GRAPHS","graphs"},{"UFOPEDIA","catalog"},{"WIKI","wiki"}}){control(title,23,y,198,29,[this,p]{if(p=="bases")openBaseMenu();else go(p);});y+=36;}
  control(b["name"].get<std::string>()+" >",23,655,198,29,[this]{base=(base+1)%int(game.state["bases"].size());});
  control(tr("INFORMACE A POPLATKY","INFORMATION & FEES"),23,699,198,29,[this]{go("base-fees");});
  small("CONSTRUCTION",774,147,dim,8);int row=0;for(int i=scroll;i<int(game.constants["BASE_OBJECTS"].size())&&row<10;++i,++row){auto obj=game.constants["BASE_OBJECTS"][i];std::string id=obj["key"];auto r=game.record("buildings",id);control((r?nameOf(*r):id)+"  "+cash(game.buildingPrice(id)),774,170+row*48,226,42,[this,id]{building=id;demolishMode=false;auto [w,h]=game.footprint(id);bool fits=false;for(int y=0;y<10&&!fits;++y)for(int x=0;x<10;++x)if(game.canBuild(base,id,x,y,floor).ok){fits=true;break;}baseFeedback=std::to_string(w)+" × "+std::to_string(h)+" · "+(fits?tr("NA TOMTO PATŘE LZE UMÍSTIT","CAN BE PLACED ON THIS LEVEL"):tr("NA TOMTO PATŘE NELZE UMÍSTIT","CANNOT BE PLACED ON THIS LEVEL"));},building==id,9);}
  listSlider(scroll,int(game.constants["BASE_OBJECTS"].size()),10,774,170,226,474,"construction");
  const bool iso=baseIsometric&&floor==0;
  control("2D",258,700,105,29,[this]{baseIsometric=false;},!iso);
  if(floor==0)control("3D",373,700,105,29,[this]{baseIsometric=true;},iso);
  else small(tr("Podzemí · pouze 2D","Underground · 2D only"),380,709,dim,10);
  control(tr("MAZAT OBJEKTY","REMOVE BUILDINGS"),490,700,245,29,[this]{demolishMode=!demolishMode;baseFeedback.clear();},demolishMode);hits.back().name="base-demolish-mode";
  drawBaseWorkspace(b,iso);
  SDL_Rect hint{258,738,485,17};SDL_RenderSetClipRect(renderer,&hint);small(baseFeedback,258,739,demolishMode?bright:dim,10);SDL_RenderSetClipRect(renderer,nullptr);

 }
 Json orderedRecords(){
  Json list=page=="editor"?custom.at(group):(page=="catalog"||page=="combat-data")?game.ufopediaRecords(group):game.gameset().at(group);
  if(group=="buildings"){Json ordered=Json::array();for(const auto& obj:game.constants["BASE_OBJECTS"])for(const auto& r:list)if(r["id"]==obj["key"])ordered.push_back(r);for(const auto& r:list){bool seen=false;for(const auto& o:ordered)seen=seen||r["id"]==o["id"];if(!seen)ordered.push_back(r);}return ordered;}return list;
 }
 void verifyResearchProductionNavigationUI(){auto oldState=game.state;auto oldPage=page,oldGroup=group;bool oldCollapsed=commandCollapsed;auto render=[this]{hits.clear();listScrollRegions.clear();rect(0,0,W,H,bg);renderScreen();};auto click=[this](const std::string& name){auto found=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.name==name;});if(found==hits.end())throw std::runtime_error("Missing research/manufacture navigation: "+name);auto action=found->action;action();};for(const auto& origin:{"globe","aircraft","catalog"}){page=origin;commandCollapsed=false;group="tank-equipment";render();click(tr("VÝZKUM","RESEARCH"));render();if(page!="research"||std::none_of(hits.begin(),hits.end(),[](const Hit& h){return h.name.find("research-project:")==0;}))throw std::runtime_error("Research projects disappeared from navigation");page=origin;group="tank-equipment";render();click(tr("VÝROBA","MANUFACTURE"));render();if(page!="production"||group!="weapons"||std::none_of(hits.begin(),hits.end(),[](const Hit& h){return h.name.find("production-design:")==0;}))throw std::runtime_error("Manufacturing designs disappeared from navigation");}page="catalog";commandCollapsed=true;render();click(tr("VÝZK.","RES."));if(page!="research"||commandCollapsed)throw std::runtime_error("Research inaccessible from collapsed menu");game.state=oldState;page=oldPage;group=oldGroup;commandCollapsed=oldCollapsed;render();std::cout<<"Research/manufacture navigation: visible projects/designs from globe, aircraft, Ufopedia and collapsed menu passed\n";}
 void verifyUfopediaCategoriesUI(){auto savedPage=page,savedGroup=group;int savedIndex=index,savedScroll=scroll;for(const auto& category:std::vector<std::pair<std::string,std::string>>{{"aircraft-equipment",tr("LETECKÁ VÝBAVA","AIRCRAFT EQUIPMENT")},{"tank-equipment",tr("TANKOVÁ VÝBAVA","TANK EQUIPMENT")},{"personnel",tr("PERSONÁL","PERSONNEL")}}){hits.clear();rect(0,0,W,H,bg);renderScreen();auto found=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.name==category.second;});if(found==hits.end())throw std::runtime_error("Missing Ufopedia category tab");auto action=found->action;action();if(group!=category.first||orderedRecords().empty())throw std::runtime_error("Ufopedia category navigation failed");hits.clear();rect(0,0,W,H,bg);renderScreen();auto surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch);SDL_SaveBMP(surface,(game.userDir/("ufopedia-category-"+group+".bmp")).string().c_str());SDL_FreeSurface(surface);}go("editor");hits.clear();rect(0,0,W,H,bg);renderScreen();if(!custom.contains(group)||orderedRecords().empty())throw std::runtime_error("Virtual Ufopedia category leaked into raw editor");page=savedPage;group=savedGroup;index=savedIndex;scroll=savedScroll;hits.clear();listScrollRegions.clear();rect(0,0,W,H,bg);renderScreen();std::cout<<"Ufopedia category navigation and safe editor return passed\n";}
 void combatCurve(const Json& points,double range,const std::string& title,int tu,int x,int y,int w,int h){
  // Geometry, grid, scales and point filtering follow combatGraphSvg in DUMMY.
  box(x,y,w,h);small(title+" · TU "+std::to_string(tu),x+8,y+8,bright,10);
  int gx=x+28,gy=y+31,gw=w-38,gh=h-57;double maxX=std::max(1.,range);std::vector<std::pair<double,double>> valid;
  if(points.is_array())for(const auto& p:points)if(p.is_array()&&p.size()==2&&p[0].is_number()&&p[1].is_number()&&std::isfinite(p[0].get<double>())&&std::isfinite(p[1].get<double>())){valid.emplace_back(p[0].get<double>(),p[1].get<double>());maxX=std::max(maxX,p[0].get<double>());}
  std::sort(valid.begin(),valid.end());for(int value:{0,25,50,75,100})line(gx,gy+gh-value*gh/100,gx+gw,gy+gh-value*gh/100,{41,77,52});
  for(int value:{0,50,100})small(std::to_string(value),x+2,gy+gh-value*gh/100-4,dim,10);
  small("0 m",gx,y+h-18,dim,10);std::string maxLabel=std::to_string(int(maxX))+" m";small(maxLabel,gx+gw-textWidth(maxLabel,10),y+h-18,dim,10);
  SDL_Rect clip{gx-2,gy-2,gw+4,gh+4};SDL_RenderSetClipRect(renderer,&clip);int px=0,py=0;bool first=true;
  for(const auto& p:valid){int xx=gx+int(p.first/maxX*gw),yy=gy+gh-int(std::clamp(p.second,0.,100.)/100*gh);if(!first)line(px,py,xx,yy,accent);rect(xx-2,yy-2,4,4,bright);px=xx;py=yy;first=false;}
  SDL_RenderSetClipRect(renderer,nullptr);if(tu==0&&valid.empty())small(tr("NEDOSTUPNÉ","UNAVAILABLE"),gx+4,gy+gh/2,dim,10);
 }
 void manufactureScreen(){
  if(group!="weapons"&&group!="equipment"&&group!="units")group="weapons";
  card(tr("VÝROBA","MANUFACTURE"),202,62,812,694);int tx=215;
  for(const auto& tab:std::vector<std::pair<std::string,std::string>>{{"weapons",tr("ZBRANĚ","WEAPONS")},{"equipment",tr("VYBAVENÍ","EQUIPMENT")},{"units",tr("LETADLA A VOZIDLA","AIRCRAFT & VEHICLES")}}){control(tab.second,tx,103,250,30,[this,id=tab.first]{group=id;index=scroll=0;quantity=1;},group==tab.first);tx+=260;}
  Json records=Json::array();for(const auto& r:game.gameset()[group])if(r.contains("production"))records.push_back(r);if(records.empty())return;index=std::clamp(index,0,int(records.size())-1);auto item=records[index];std::string id=item["id"];
  for(int row=0,i=scroll;i<int(records.size())&&row<7;++i,++row){int yy=145+row*80;control("",215,yy,238,74,[this,i]{index=i;quantity=1;},i==index);hits.back().name="production-design:"+records[i]["id"].get<std::string>();image(records[i].value("image",""),221,yy+5,82,62);wrap(nameOf(records[i]),310,yy+17,132,11);}
  listSlider(scroll,int(records.size()),7,215,145,238,554,"production-designs");
  control(tr("FRONTA DÍLNY","WORKSHOP QUEUE"),600,70,180,24,[this]{go("workshop");});
  control(tr("PRACOVNÍ A VÝZKUMNÁ FRONTA","WORK & RESEARCH QUEUES"),790,70,206,24,[this]{go("queues");});hits.back().name="open-queues";
  int remainingUnits=0;for(const auto& order:game.state["production"])if(order.value("group",group)==group&&order["id"]==id)++remainingUnits;small(tr("ZBÝVÁ VYROBIT: ","REMAINING TO PRODUCE: ")+std::to_string(remainingUnits),705,146,bright,12);
  auto quote=game.manufactureQuote(group,id,quantity);small(nameOf(item),470,146,bright,13);image(item.value("image",""),470,175,220,135);
  const auto& prod=item["production"];metric(tr("HODIN / KUS","HOURS / UNIT"),std::to_string(prod.value("productionHours",1)),705,178,290);
  metric(tr("CENA / KUS","COST / UNIT"),cash(prod.value("productionCost",0LL)),705,207,290);
  metric(tr("TRŽNÍ KURZ","MARKET PRICE"),cash(prod.value("marketPrice",0LL)),705,236,290);
  small(tr("SUROVINY · SKLAD / POTŘEBA","MATERIALS · STOCK / REQUIRED"),705,268,dim,10);int yy=291;
  for(const auto& material:quote["materials"]){small(material["id"].get<std::string>()+": "+std::to_string(material["have"].get<int>())+" / "+std::to_string(material["required"].get<int>()),705,yy,material["missing"].get<int>()?bright:dim,10);yy+=17;}
  auto selectedProgress=game.productionProgress(group,id);double batchProgress=selectedProgress["progress"];std::string batchState=selectedProgress["remaining"].get<int>()==0?(selectedProgress["total"].get<int>()?tr("DOKONČENO","COMPLETED"):tr("ŽÁDNÁ ZAKÁZKA","NO ORDER")):game.productionTeam(group,id).empty()?tr("ČEKÁ NA TECHNIKY","WAITING FOR TECHNICIANS"):speed==0?tr("ČAS POZASTAVEN","TIME PAUSED"):tr("VÝROBA PROBÍHÁ","IN PRODUCTION");
  small(tr("POSTUP: ","PROGRESS: ")+std::to_string(int(batchProgress*100))+"% · "+tr("HOTOVO ","DONE ")+std::to_string(selectedProgress["completed"].get<int>())+" / "+std::to_string(selectedProgress["total"].get<int>()),470,319,bright,11);box(470,337,220,18);rect(473,340,int(batchProgress*214),12,accent);small(batchState,470,358,dim,9);
  if(item.contains("combat")&&item["combat"].value("kind",std::string(""))=="weapon"&&item["combat"].contains("curves")){
   const auto& combat=item["combat"];int x=470;for(const auto& mode:std::vector<std::pair<std::string,std::string>>{{"auto",tr("AUTOMAT","AUTO")},{"hip",tr("OD BOKU","HIP")},{"aimed",tr("ZACÍLENÍ","AIMED")}}){combatCurve(combat["curves"].value(mode.first,Json::array()),combat.value("values",Json::object()).value("range",0.),mode.second,combat.value("tu",Json::object()).value(mode.first,0),x,370,172,150);x+=177;}
  }else{
   card(tr("VÝROBNÍ RECEPTURA","PRODUCTION RECIPE"),470,370,526,150);int yy=411;for(const auto& m:quote["materials"]){small(m["id"].get<std::string>()+" · "+std::to_string(m["required"].get<int>())+tr(" jednotek"," units"),485,yy,text,11);yy+=21;}
  }
  small(tr("MNOŽSTVÍ NOVÉ ZAKÁZKY","NEW ORDER QUANTITY"),470,523,dim,9);
  control("-",470,535,35,30,[this]{quantity=std::max(1,quantity-1);});control(std::to_string(quantity),512,535,64,30,[this]{input(std::to_string(quantity),true,[this](const std::string& value){try{std::size_t used=0;int n=std::stoi(value,&used);if(used!=value.size()||n<1||n>100)throw std::runtime_error("range");quantity=n;}catch(...){notice=tr("Zadej 1–100 kusů.","Enter 1–100 units.");}});});control("+",583,535,35,30,[this]{quantity=std::min(100,quantity+1);});
  control(tr("ZADAT VÝROBU","QUEUE PRODUCTION")+" · "+cash(quote["cost"]),628,535,368,30,[this,id]{result(game.manufacture(group,id,quantity));});hits.back().name="production-start";
  rect(470,576,526,7,{23,39,23});int productionThumb=470+(quantity-1)*525/99;rect(470,576,productionThumb-470,7,accent);rect(productionThumb-3,570,6,20,bright);hits.push_back({{470,568,526,24},[this]{quantityAt(mx);quantityDragging=true;SDL_CaptureMouse(SDL_TRUE);},"production-quantity-slider"});
  small(remainingUnits?batchState:productionStatus(quote),470,738,dim,10);
  control(tr("SKLAD / SUROVINY >","STORAGE / MATERIALS >"),470,597,175,25,[this]{go("storage");index=int(game.gameset()["weapons"].size()+game.gameset()["equipment"].size())+(game.record("units","underwater-robot")?1:0);scroll=index;});
  control(tr("TECHNICI: ","TECHNICIANS: ")+std::to_string(game.productionTeam(group,id).size())+" >",655,597,104,25,[this,id]{productionStaffGroup=group;productionStaffId=id;technicianScroll=0;go("production-staff");});hits.back().name="production-team";
  control(tr("PŘIŘADIT VOLNÉ","ASSIGN FREE"),769,597,111,25,[this,id]{for(std::size_t i=0;i<game.state["personnel"].size();++i)if(game.state["personnel"][i]["role"]=="technician"&&game.technicianProject(i).empty())game.assignTechnician(group,id,i,true);});hits.back().name="production-assign-all";
  control(tr("ZRUŠIT VŠE","CLEAR ALL"),890,597,106,25,[this,id]{auto team=game.productionTeam(group,id);for(const auto& person:team)game.assignTechnician(group,id,person.get<std::size_t>(),false);});hits.back().name="production-clear-all";
  auto schedule=visibleProductionQueue();productionScroll=std::clamp(productionScroll,0,std::max(0,int(schedule.size())-2));small(tr("FRONTA VÝROBY · ","WORKSHOP QUEUE · ")+std::to_string(schedule.size())+tr(" POLOŽEK"," ITEMS"),470,635,bright,10);
  for(int row=0,n=productionScroll;n<int(schedule.size())&&row<2;++n,++row){const auto& batch=schedule[n];std::string g=batch["group"],itemId=batch["id"];auto design=game.record(g,itemId);auto progress=game.productionProgress(g,itemId);int y=653+row*32;small(design?nameOf(*design):itemId,470,y,text,10);small(std::to_string(progress["remaining"].get<int>())+tr(" ks · "," units · ")+productionEta(batch),690,y,bright,10);box(470,y+12,526,12);rect(472,y+14,int(progress["progress"].get<double>()*522),8,accent);}
  if(schedule.empty())small(tr("ŽÁDNÉ ZAKÁZKY","NO ACTIVE ORDERS"),470,660,dim,10);listSlider(productionScroll,int(schedule.size()),2,470,653,526,56,"production-orders");small(tr("MÍSTA V DÍLNÁCH: ","WORKSHOP PLACES: ")+std::to_string(game.assignedStaff("workshop"))+" / "+std::to_string(game.staffCapacity("workshop"))+tr(" · 25 TECHNIKŮ NA DÍLNU"," · 25 TECHNICIANS PER WORKSHOP"),470,720,dim,10);

 }
 Json visibleProductionQueue(){auto jobs=game.productionSchedule();std::set<std::string> seen;for(const auto& job:jobs)seen.insert(job["group"].get<std::string>()+":"+job["id"].get<std::string>());if(game.state.contains("productionFinished"))for(const auto& done:game.state["productionFinished"].items())if(done.value().get<int>()>0&&!seen.count(done.key())){auto split=done.key().find(':');if(split!=std::string::npos)jobs.push_back({{"group",done.key().substr(0,split)},{"id",done.key().substr(split+1)},{"finishInHours",0.},{"completed",true}});}return jobs;}
 std::string itemTime(const Json& row,bool done,const std::string& waiting){if(done)return tr("DOKONČENO · 0 h","COMPLETED · 0 h");if(row.is_null()||row["finishInHours"].is_null())return waiting;int minutes=int(std::ceil(row["finishInHours"].get<double>()*60-1e-7));return tr("ZBÝVÁ ","LEFT ")+std::to_string(minutes/60)+" h "+std::to_string(minutes%60)+" min";}
 std::string productionEta(const Json& batch){if(batch.value("completed",false))return tr("DOKONČENO · 0 h","COMPLETED · 0 h");if(batch["finishInHours"].is_null())return tr("ČEKÁ NA TECHNIKY","WAITING FOR TECHNICIANS");int minutes=int(std::ceil(batch["finishInHours"].get<double>()*60-1e-7));return tr("DO KONCE ","FINISH IN ")+std::to_string(minutes/60)+" h "+std::to_string(minutes%60)+" min";}
 void facilityQueue(){
  bool lab=page=="laboratory";Json items;if(lab){auto times=game.researchSchedule();items=times;for(const auto& project:game.state["research"].items())if(project.value().get<double>()>=1)items.push_back({{"id",project.key()},{"finishInHours",0.},{"completed",true}});}else items=visibleProductionQueue();
  card(lab?tr("LABORATOŘ · VÝZKUMNÁ FRONTA","LABORATORY · RESEARCH QUEUE"):tr("DÍLNA · VÝROBNÍ FRONTA","WORKSHOP · PRODUCTION QUEUE"),202,62,812,694);
  control(speed>0?tr("POZASTAVIT ČAS","PAUSE TIME"):tr("SPUSTIT ČAS","START TIME"),600,70,190,26,[this]{speed=speed>0?0.:1.;});hits.back().name="queue-time-toggle";
  control(lab?tr("PŘIDAT PROJEKT","ADD PROJECT"):tr("PŘIDAT VÝROBU","ADD PRODUCTION"),800,70,196,26,[this,lab]{go(lab?"research":"production");if(!lab)group="weapons";});hits.back().name="queue-add";
  small(tr("POLOŽKA","ITEM"),232,134,dim,11);small(lab?tr("VĚDCI","SCIENTISTS"):tr("HOTOVO / CELKEM","DONE / TOTAL"),530,134,dim,10);small(tr("DOKONČENÍ","PROGRESS"),650,134,dim,10);small(tr("ČAS DO KONCE","TIME TO FINISH"),790,134,dim,10);
  int& offset=lab?researchQueueScroll:workQueueScroll;offset=std::clamp(offset,0,std::max(0,int(items.size())-8));
  for(int row=0,n=offset;n<int(items.size())&&row<8;++n,++row){auto item=items[n];std::string id=item["id"],g=lab?"technologies":item["group"].get<std::string>();auto design=game.record(g,id);auto progress=lab?game.state["research"].value(id,0.):game.productionProgress(g,id)["progress"].get<double>();bool done=progress>=1.;int y=160+row*60;control("",220,y,774,54,[this,lab,g,id]{go(lab?"research":"production");group=g;int n=0;for(const auto& candidate:game.gameset()[g])if(lab?game.researchAvailable(candidate["id"]):candidate.contains("production")){if(candidate["id"]==id){index=n;scroll=std::max(0,n-3);break;}++n;}});hits.back().name=std::string(lab?"laboratory-item:":"workshop-item:")+id;SDL_Rect clip{232,y+6,280,22};SDL_RenderSetClipRect(renderer,&clip);wrap(design?nameOf(*design):id,232,y+6,280,10);SDL_RenderSetClipRect(renderer,nullptr);
   std::string count=lab?std::to_string(done?0:game.researchTeam(id).size()):std::to_string(game.productionProgress(g,id)["completed"].get<int>())+" / "+std::to_string(game.productionProgress(g,id)["total"].get<int>());small(count,530,y+8,bright,12);small(std::to_string(int(progress*100))+"%",650,y+8,bright,12);small(itemTime(item,done,lab?tr("ČEKÁ NA VĚDCE","WAITING FOR SCIENTISTS"):tr("ČEKÁ NA TECHNIKY","WAITING FOR TECHNICIANS")),790,y+8,bright,10);small(done?tr("DOKONČENO","COMPLETED"):item["finishInHours"].is_null()?tr("ČEKÁ NA TÝM","WAITING FOR TEAM"):speed==0?tr("ČAS POZASTAVEN","TIME PAUSED"):tr("VE FRONTĚ / PROBÍHÁ","QUEUED / IN PROGRESS"),232,y+33,dim,9);rect(530,y+34,450,7,edge);rect(530,y+34,int(progress*450),7,accent);
  }
  listSlider(offset,int(items.size()),8,220,160,774,474,lab?"laboratory-queue":"workshop-queue");if(items.empty())small(lab?tr("FRONTA JE PRÁZDNÁ · PŘIDEJ PROJEKT","QUEUE EMPTY · ADD A PROJECT"):tr("FRONTA JE PRÁZDNÁ · PŘIDEJ VÝROBU","QUEUE EMPTY · ADD PRODUCTION"),232,180,dim,12);
  wrap(tr("Každá položka má vlastní čas a dokončení. Dokončené položky zůstávají v přehledu. Časy jsou v herních hodinách a zahrnují čekání na tým.","Each item has its own completion and remaining time. Completed items remain listed. Game-hour estimates include waiting for a team."),220,665,774,11,dim);
  control(tr("SPOLEČNÝ PŘEHLED FRONT","ALL QUEUES"),220,715,774,27,[this]{go("queues");});
 }
 void workQueues(){
  card(tr("PRACOVNÍ A VÝZKUMNÁ FRONTA","WORK & RESEARCH QUEUES"),202,62,812,694);Json jobs=visibleProductionQueue();auto research=game.researchQueue();for(const auto& done:game.state["research"].items())if(done.value().get<double>()>=1)research.push_back(done.key());auto researchTimes=game.researchSchedule();
  small(tr("VÝROBA · ","PRODUCTION · ")+std::to_string(jobs.size()),220,115,bright,12);small(tr("VÝZKUM · ","RESEARCH · ")+std::to_string(research.size()),620,115,bright,12);
  workQueueScroll=std::clamp(workQueueScroll,0,std::max(0,int(jobs.size())-7));researchQueueScroll=std::clamp(researchQueueScroll,0,std::max(0,int(research.size())-7));
  for(int row=0,n=workQueueScroll;n<int(jobs.size())&&row<7;++n,++row){std::string g=jobs[n]["group"],id=jobs[n]["id"];auto design=game.record(g,id);auto progress=game.productionProgress(g,id);int y=150+row*70;control("",220,y,374,64,[this,g,id]{go("production");group=g;int n=0;for(const auto& r:game.gameset()[g])if(r.contains("production")){if(r["id"]==id){index=n;scroll=std::max(0,n-3);break;}++n;}});hits.back().name="queued-production:"+id;small(std::to_string(n+1)+" · "+(design?nameOf(*design):id),230,y+9,text,10);small(tr("ZBÝVÁ ","REMAINING ")+std::to_string(progress["remaining"].get<int>())+" · "+std::to_string(int(progress["progress"].get<double>()*100))+"% · "+productionEta(jobs[n]),230,y+29,dim,10);rect(230,y+49,354,6,edge);rect(230,y+49,int(progress["progress"].get<double>()*354),6,accent);}
  for(int row=0,n=researchQueueScroll;n<int(research.size())&&row<7;++n,++row){std::string id=research[n];auto design=game.record("technologies",id);double progress=game.state["research"].value(id,0.);int y=150+row*70;control("",620,y,374,64,[this,id]{go("research");int n=0;for(const auto& r:game.gameset()["technologies"])if(game.researchAvailable(r["id"])){if(r["id"]==id){index=n;scroll=std::max(0,n-3);break;}++n;}});hits.back().name="queued-research:"+id;small(std::to_string(n+1)+" · "+(design?nameOf(*design):id),630,y+9,text,10);small(std::to_string(int(progress*100))+"% · "+researchItemTime(id,researchTimes),630,y+29,dim,10);rect(630,y+49,354,6,edge);rect(630,y+49,int(progress*354),6,accent);}
  listSlider(workQueueScroll,int(jobs.size()),7,220,150,374,484,"work-queue");listSlider(researchQueueScroll,int(research.size()),7,620,150,374,484,"research-queue");
  if(jobs.empty())small(tr("ŽÁDNÉ VÝROBNÍ ZAKÁZKY","NO PRODUCTION ORDERS"),230,165,dim,11);if(research.empty())small(tr("ŽÁDNÉ VÝZKUMNÉ PROJEKTY","NO RESEARCH PROJECTS"),630,165,dim,11);
  wrap(tr("Uvolněný tým pokračuje na první čekající položce. Kliknutím otevři detail.","Released teams continue with the first waiting item. Click an item to open details."),220,654,774,11,dim);
  control(tr("VÝROBA","MANUFACTURE"),220,705,374,32,[this]{go("production");});control(tr("VÝZKUM","RESEARCH"),620,705,374,32,[this]{go("research");});
 }
 void productionStaffScreen(){
  const auto* design=game.record(productionStaffGroup,productionStaffId);if(!design)return;auto team=game.productionTeam(productionStaffGroup,productionStaffId);std::string key=productionStaffGroup+":"+productionStaffId;
  card(tr("VÝROBA · PŘIŘAZENÍ TECHNIKŮ","PRODUCTION · TECHNICIAN ASSIGNMENT"),202,62,812,694);small(nameOf(*design),220,110,bright,14);
  control(tr("PŘIŘADIT VOLNÉ","ASSIGN FREE"),220,147,374,32,[this]{for(std::size_t i=0;i<game.state["personnel"].size();++i)if(game.state["personnel"][i]["role"]=="technician"&&game.technicianProject(i).empty())game.assignTechnician(productionStaffGroup,productionStaffId,i,true);});hits.back().name="production-assign-available";
  control(tr("ZRUŠIT VŠE","CLEAR ALL"),610,147,384,32,[this]{for(const auto& i:game.productionTeam(productionStaffGroup,productionStaffId))game.assignTechnician(productionStaffGroup,productionStaffId,i.get<std::size_t>(),false);});
  small(tr("PŘIŘAZENO: ","ASSIGNED: ")+std::to_string(team.size())+tr(" · MÍSTA V DÍLNÁCH: "," · WORKSHOP PLACES: ")+std::to_string(game.assignedStaff("workshop"))+" / "+std::to_string(game.staffCapacity("workshop"))+tr(" · 25 NA DÍLNU"," · 25 PER WORKSHOP"),220,193,dim,11);
  std::vector<int> people;for(int i=0;i<int(game.state["personnel"].size());++i)if(game.state["personnel"][i]["role"]=="technician")people.push_back(i);
  for(int row=0,n=technicianScroll;n<int(people.size())&&row<10;++n,++row){int person=people[n];const auto& p=game.state["personnel"][person];bool selected=std::find(team.begin(),team.end(),Json(person))!=team.end();std::string busy=game.technicianProject(person);
   control(p["name"].get<std::string>()+" · "+(selected?tr("PŘIŘAZEN","ASSIGNED"):busy.empty()?tr("VOLNÝ","AVAILABLE"):tr("JINÁ ZAKÁZKA","OTHER ORDER")),220,225+row*42,774,34,[this,person,selected]{game.assignTechnician(productionStaffGroup,productionStaffId,person,!selected);},selected);hits.back().name="production-technician:"+std::to_string(person);
  }
  listSlider(technicianScroll,int(people.size()),10,220,225,774,412,"technicians");
  control(tr("ZPĚT DO VÝROBY","BACK TO PRODUCTION"),220,705,774,34,[this]{go("production");group=productionStaffGroup;int n=0;for(const auto& r:game.gameset()[group])if(r.contains("production")){if(r["id"]==productionStaffId){index=n;scroll=std::max(0,n-3);break;}++n;}});
 }
 std::string productionStatus(const Json& quote){
  std::string reason=quote["reason"];if(reason=="Ready to manufacture")return tr("Připraveno k výrobě.","Ready to manufacture.");if(reason=="Missing materials: buy or deliver the displayed requirements")return tr("Chybí suroviny · doplň uvedené množství ve skladu.",reason);if(reason=="A workshop is required")return tr("Chybí dokončená dílna.",reason);if(reason=="Item is not yet available")return tr("Návrh zatím není dostupný.",reason);if(reason=="Insufficient funds for the whole order")return tr("Nedostatek peněz na celou zakázku.",reason);return reason;
 }
 std::string researchStatus(const Json& quote){
  std::string reason=quote["reason"];
  if(reason=="Research complete")return tr("Výzkum dokončen.","Research complete.");
  if(reason=="A laboratory is required")return tr("Je potřeba dokončená laboratoř.","A completed laboratory is required.");
  if(reason=="Assign scientists to this project")return tr("Vyber vědce pro tento projekt.","Select scientists for this project.");
  if(reason=="Research in progress")return tr("Výzkum probíhá s přiřazeným týmem.","Research uses the assigned team.");
  if(reason=="Ready to start research")return tr("Vybraný projekt lze zahájit.","Selected project is ready to start.");
  if(reason.find("Complete prerequisite research: ")==0){auto id=reason.substr(32);auto required=game.record("technologies",id);return tr("Nejdříve dokonči: ","First complete: ")+(required?nameOf(*required):id);}
  if(reason.find("Required specimen or wreck: ")==0)return tr("Na základně chybí požadovaný exemplář nebo vrak.","The required specimen or wreck has not arrived.");
  return tr("Projekt zatím není dostupný.","Project is not yet available.");
 }
 std::string researchItemTime(const std::string& id,const Json& schedule){Json row;for(const auto& plan:schedule)if(plan["id"]==id)row=plan;return itemTime(row,game.state["research"].value(id,0.)>=1,game.state["research"].contains(id)?tr("ČEKÁ NA VĚDCE","WAITING FOR SCIENTISTS"):tr("NEZAŘAZENO","NOT QUEUED"));}
 void researchScreen(){
  group="technologies";Json records=Json::array();for(const auto& project:game.gameset()["technologies"])if(game.researchAvailable(project["id"]))records.push_back(project);if(records.empty()){card(tr("VÝZKUM","RESEARCH"),202,62,812,694);small(tr("ŽÁDNÉ DOSTUPNÉ PROJEKTY","NO AVAILABLE PROJECTS"),220,112,dim,12);return;}scroll=std::clamp(scroll,0,std::max(0,int(records.size())-1));index=std::clamp(index,0,int(records.size())-1);const auto& project=records[index];std::string id=project["id"];
  card(tr("VÝZKUM","RESEARCH"),202,62,812,694);
  control(tr("FRONTA LABORATOŘE","LABORATORY QUEUE"),600,70,180,24,[this]{go("laboratory");});
  control(tr("PRACOVNÍ A VÝZKUMNÁ FRONTA","WORK & RESEARCH QUEUES"),790,70,206,24,[this]{go("queues");});hits.back().name="open-queues";
  for(int row=0,i=scroll;i<int(records.size())&&row<7;++i,++row){int yy=113+row*82;control("",215,yy,250,76,[this,i]{index=i;researchStaffScroll=0;researchFeedback.clear();},index==i);hits.back().name="research-project:"+records[i]["id"].get<std::string>();image(records[i].value("image",""),221,yy+5,66,63);wrap(nameOf(records[i]),294,yy+13,158,10);double progress=game.state["research"].value(records[i]["id"].get<std::string>(),0.);if(game.state["research"].contains(records[i]["id"].get<std::string>()))small(std::to_string(int(progress*100))+"%",294,yy+58,dim,10);}
  listSlider(scroll,int(records.size()),7,215,113,250,568,"research-projects");
  auto quote=game.researchQuote(id);auto team=game.researchTeam(id);double progress=quote["progress"],hours=quote["hours"];int referenceCount=quote["scientists"],assigned=int(team.size());
  wrap(nameOf(project),480,112,512,14);image(progress>=1?project.value("researchResultImage",project.value("image","")):project.value("image",""),480,156,220,160);
  metric(tr("HODINY · REFERENČNÍ TÝM","HOURS · REFERENCE TEAM"),std::to_string(int(hours)),712,161,282);
  metric(tr("REFERENČNÍ POČET VĚDCŮ","REFERENCE SCIENTISTS"),std::to_string(referenceCount),712,194,282);
  metric(tr("PŘIŘAZENO VĚDCŮ","ASSIGNED SCIENTISTS"),std::to_string(assigned),712,227,282);
  metric(tr("ZBÝVAJÍCÍ ČAS","REMAINING TIME"),progress>=1?"0 h":assigned?std::to_string(int(std::ceil((1-progress)*hours*std::max(1,referenceCount)/assigned)))+" h":"—",712,260,282);
  rect(480,325,514,10,{23,39,23});rect(480,325,int(progress*514),10,accent);small(std::to_string(int(progress*100))+"%",480,341,bright,11);
  SDL_Rect descriptionClip{480,365,514,48};SDL_RenderSetClipRect(renderer,&descriptionClip);
  wrap(progress>=1?project.value("researchResult",project.value("description","")):project.value("description",""),480,365,514,11);
  SDL_RenderSetClipRect(renderer,nullptr);
  small(tr("MÍSTA V LABORATOŘÍCH: ","LABORATORY PLACES: ")+std::to_string(game.assignedStaff("research"))+" / "+std::to_string(game.staffCapacity("research"))+" · "+researchStatus(quote),480,419,dim,10);
  control(quote["started"].get<bool>()?tr("PROJEKT VE FRONTĚ","PROJECT QUEUED"):tr("ZAŘADIT DO FRONTY","QUEUE RESEARCH"),480,446,160,30,[this,id]{auto outcome=game.startResearch(id);researchFeedback=outcome.ok?"":outcome.message;});hits.back().name="research-start";
  control(tr("PŘIŘADIT VOLNÉ","ASSIGN FREE"),650,446,165,30,[this,id]{auto start=game.startResearch(id);if(!start.ok){researchFeedback=start.message;return;}for(std::size_t i=0;i<game.state["personnel"].size();++i)if(game.state["personnel"][i]["role"]=="scientist"&&game.scientistProject(i).empty())game.assignScientist(id,i,true);researchFeedback.clear();});hits.back().name="research-assign-available";
  control(tr("ZRUŠIT VŠE","CLEAR ALL"),825,446,169,30,[this,id]{auto team=game.researchTeam(id);for(const auto& person:team)game.assignScientist(id,person.get<std::size_t>(),false);researchFeedback.clear();});hits.back().name="research-clear-all";
  std::vector<int> scientists;for(int i=0;i<int(game.state["personnel"].size());++i)if(game.state["personnel"][i]["role"]=="scientist")scientists.push_back(i);
  for(int row=0,n=researchStaffScroll;n<int(scientists.size())&&row<7;++n,++row){int person=scientists[n];const auto& scientist=game.state["personnel"][person];bool selected=std::find(team.begin(),team.end(),Json(person))!=team.end();std::string busy=game.scientistProject(person);int skill=scientist.contains("attributes")?scientist["attributes"].value("RESEARCH",scientist.value("skill",50)):scientist.value("skill",50);
   std::string status=selected?tr("PŘIŘAZEN","ASSIGNED"):busy.empty()?tr("VOLNÝ","AVAILABLE"):tr("JINÝ PROJEKT","OTHER PROJECT");
   control(scientist["name"].get<std::string>()+" · "+tr("VÝZKUM ","RESEARCH ")+std::to_string(skill)+" · "+status,480,488+row*29,514,25,[this,id,person,selected]{auto start=game.startResearch(id);if(!start.ok){researchFeedback=start.message;return;}auto outcome=game.assignScientist(id,person,!selected);researchFeedback=outcome.ok?"":outcome.message;},selected);hits.back().name="research-scientist:"+std::to_string(person);
  }
  if(scientists.empty())small(tr("Najmi vědce na pracovním trhu.","Recruit scientists in Workforce."),480,500,dim,11);
  if(!researchFeedback.empty())small(researchFeedback.find("capacity reached")!=std::string::npos?tr("Laboratoře jsou plné · 25 vědců na laboratoř.","Laboratories full · 25 scientists per laboratory."):tr("Změna není možná · ověř stav a přiřazení projektu.","Cannot change this assignment · check project status and staffing."),480,696,dim,10);
  listSlider(researchStaffScroll,int(scientists.size()),7,480,488,514,199,"scientists");
  small(speed==0?tr("ČAS POZASTAVEN · OVLÁDÁNÍ NAHOŘE","TIME PAUSED · CONTROLS ABOVE"):tr("VÝZKUM PROBÍHÁ V HERNÍM ČASE","RESEARCH USES GAME TIME"),578,725,dim,10);
 }
 void catalogue(){
  bool edit=page=="editor";
  card(tr("UFOPEDIE / TECHNICKÁ DATABÁZE","UFOPEDIA / TECHNICAL DATABASE"),202,62,814,698);
  int xx=213;auto tabs=std::vector<std::pair<std::string,std::string>>{{"buildings",tr("BUDOVY","FACILITIES")},{"units",tr("LETADLA A TANKY","AIRCRAFT & TANKS")},{"weapons",tr("PĚCHOTNÍ ZBRANĚ","PERSONNEL WEAPONS")},{"equipment",tr("OSTATNÍ VYBAVENÍ","OTHER EQUIPMENT")}};
  if(edit)tabs.push_back({"technologies",tr("VÝZKUM","RESEARCH")});else{tabs.push_back({"aircraft-equipment",tr("LETECKÁ VÝBAVA","AIRCRAFT EQUIPMENT")});tabs.push_back({"tank-equipment",tr("TANKOVÁ VÝBAVA","TANK EQUIPMENT")});tabs.push_back({"personnel",tr("PERSONÁL","PERSONNEL")});}
  for(const auto& [id,title]:tabs){int w=textWidth(title,8)+20;control(title,xx,106,w,31,[this,id]{group=id;index=scroll=0;},group==id);xx+=w+7;}
  auto list=orderedRecords();if(list.empty())return;index=std::clamp(index,0,int(list.size())-1);auto r=list[index];
  int rowHeight=80;for(const auto& item:list)rowHeight=std::max(rowHeight,int(wrappedLines(nameOf(item),110,10).size())*15+12);int visible=std::max(1,560/rowHeight);std::string listKey=group+":"+r["id"].get<std::string>()+nameOf(r)+(english?":en":":cs");if(catalogueListKey!=listKey){catalogueListKey=listKey;if(index<scroll)scroll=index;else if(index>=scroll+visible)scroll=index-visible+1;}scroll=std::clamp(scroll,0,std::max(0,int(list.size())-visible));
  for(int row=0,i=scroll;i<int(list.size())&&row<visible;++i,++row){int y=146+row*rowHeight;control("",213,y,190,rowHeight-6,[this,i]{index=i;},index==i);image(list[i].value("image",""),219,y+8,58,58);boundedText(nameOf(list[i]),{285,y+6,110,rowHeight-18},10,text);}
  listSlider(scroll,int(list.size()),visible,213,146,190,554,"catalog");
  auto titleLines=wrappedLines(nameOf(r),328,10);int titleHeight=std::min(140,std::max(33,int(titleLines.size())*15+12));box(413,146,352,titleHeight);boundedText(nameOf(r),{425,152,328,titleHeight-12},10,bright);int imageTop=146+titleHeight+8;box(413,imageTop,352,706-imageTop);image(r.value("image",""),425,imageTop+12,328,682-imageTop);
  std::vector<std::pair<std::string,std::string>> rows;
  if(r.contains("specs"))for(const auto& spec:r["specs"])rows.push_back({spec[0],spec[1]});
  if(r.contains("vehicleStats")){const auto& stats=r["vehicleStats"];rows.push_back({tr("ŽIVOT","HEALTH"),stats["health"].dump()});rows.push_back({tr("ČASOVÉ JEDNOTKY","TIME UNITS"),stats["timeUnits"].dump()});rows.push_back({tr("ODBOKU · PŘESNOST / TU","HIP · ACCURACY / TU"),stats["hipAccuracy"].dump()+" % / "+stats["hipTU"].dump()});rows.push_back({tr("ZACÍLENĚ · PŘESNOST / TU","AIMED · ACCURACY / TU"),stats["aimedAccuracy"].dump()+" % / "+stats["aimedTU"].dump()});}
  if(r.contains("combat")){const auto& combat=r["combat"];if(combat.contains("values"))for(const auto& value:combat["values"].items())rows.push_back({value.key(),value.value().dump()});if(combat.contains("tu"))for(const auto& mode:combat["tu"].items())rows.push_back({mode.key()+" · TU",mode.value()==0?"—":mode.value().dump()});}
  if(group=="buildings"){std::string id=r["id"];auto [w,h]=game.footprint(id);rows.push_back({tr("ROZMĚRY","DIMENSIONS"),std::to_string(w)+" × "+std::to_string(h)});auto capacities=readJson(game.dataDir/"facility-capacities.json");if(capacities.contains(id))rows.push_back({tr("KAPACITA JEDNOHO OBJEKTU","CAPACITY PER FACILITY"),capacities[id][english?"en":"cs"]});}
  struct DetailText{std::string value;int x,y;Color color;};std::vector<DetailText> details;std::vector<int> dividers;int contentY=0;
  auto paragraph=[&](const std::string& value,Color color){for(const auto& line:wrappedLines(value,201,10)){details.push_back({line,787,contentY,color});contentY+=15;}};
  paragraph(tr("POPIS","DESCRIPTION"),bright);contentY+=9;dividers.push_back(contentY);contentY+=12;paragraph(recordText(r,"description"),text);contentY+=20;paragraph(tr("TECHNICKÉ ÚDAJE","TECHNICAL DATA"),bright);contentY+=9;dividers.push_back(contentY);contentY+=12;
  for(const auto& row:rows){if(row.first.find('\n')==std::string::npos&&row.second.find('\n')==std::string::npos&&textWidth(row.first,10)+textWidth(row.second,10)+12<=201){details.push_back({row.first,787,contentY,dim});details.push_back({row.second,988-textWidth(row.second,10),contentY,text});contentY+=15;}else{paragraph(row.first,dim);contentY+=3;paragraph(row.second,text);}contentY+=8;dividers.push_back(contentY);contentY+=10;}
  std::string detailKey=group+":"+r["id"].get<std::string>()+(english?":en":":cs");if(catalogueDetailKey!=detailKey){catalogueDetailKey=detailKey;catalogueDetailScroll=0;}
  bool curves=r.contains("combat")&&r["combat"].contains("curves");int viewportHeight=edit?378:curves?476:530;box(775,146,229,560);listSlider(catalogueDetailScroll,(contentY+14)/15,viewportHeight/15,787,160,201,viewportHeight,"catalogue-details");SDL_Rect detailClip{787,160,201,viewportHeight},previousClip;bool previousClipped=SDL_RenderIsClipEnabled(renderer);SDL_RenderGetClipRect(renderer,&previousClip);SDL_RenderSetClipRect(renderer,&detailClip);
  for(const auto& entry:details)small(entry.value,entry.x,160+entry.y-catalogueDetailScroll*15,entry.color,10);for(int divider:dividers)line(787,160+divider-catalogueDetailScroll*15,988,160+divider-catalogueDetailScroll*15,edge);SDL_RenderSetClipRect(renderer,previousClipped?&previousClip:nullptr);
  if(curves)control(tr("PŘESNOST STŘELBY >","ACCURACY CURVES >"),787,660,205,30,[this,selected=index]{go("combat-data");index=selected;});
  if(edit){control(tr("UPRAVIT NÁZEV","EDIT NAME"),425,650,158,38,[this,id=r["id"].get<std::string>()]{input("",false,[this,id](const std::string& v){for(auto& r:custom[group])if(r["id"]==id)r["name"]=v;});});control(tr("ULOŽIT DUMMY KOPII","SAVE DUMMY COPY"),590,650,163,38,[this]{writeJson(game.userDir/"gameset-custom.json",custom);notice="Saved custom gameset";});}
  if(edit)control(tr("UPRAVIT ČÍSLO","EDIT NUMBER"),787,560,205,35,[this,id=r["id"].get<std::string>()]{std::string field=group=="technologies"?"researchHours":"availableAfterDays";int value=0;for(const auto& item:custom[group])if(item["id"]==id)value=item.value(field,0);input(std::to_string(value),true,[this,id,field](const std::string& entered){try{std::size_t used=0;int value=std::stoi(entered,&used);if(used!=entered.size()||value<0||value>10000000)throw std::runtime_error("range");for(auto& item:custom[group])if(item["id"]==id)item[field]=value;notice="Custom copy updated";}catch(...){notice="Invalid number";}});});
  box(413,714,352,34);std::string categoryName=group;for(const auto& tab:tabs)if(tab.first==group)categoryName=tab.second;boundedText(tr("DATABÁZE OBJEKTŮ · ","OBJECT DATABASE · ")+categoryName,{425,722,328,20},10,dim);
 }
 void storageCapacityBar(double used,int capacity,int x,int y,int width){
  rect(x,y,width,8,{23,39,23});if(capacity>0){rect(x,y,int(std::clamp(used/capacity,0.,1.)*width),8,accent);if(capacity<=80)for(int i=0;i<capacity;++i){int left=x+i*width/capacity,right=x+(i+1)*width/capacity;rect(right-2,y,2,8,bg);color(edge);SDL_Rect mark{left,y,std::max(1,right-left-2),8};SDL_RenderDrawRect(renderer,&mark);}}
  else{color(edge);SDL_Rect empty{x,y,width,8};SDL_RenderDrawRect(renderer,&empty);}
 }
 std::string storageCapacityLabel(double used,int capacity){std::ostringstream value;value<<std::fixed<<std::setprecision(2)<<used<<" / "<<capacity<<tr(" PALET"," PALLETS");if(capacity>0)value<<" · "<<std::setprecision(0)<<used/capacity*100<<"%";if(used>capacity)value<<tr(" · PŘEKROČENO"," · OVER CAPACITY");return value.str();}
 void storage(){
  const auto capacity=game.storageCapacity();box(194,60,820,64);line(611,68,611,116);
  small(tr("ZAPLNĚNÍ SKLADU","WAREHOUSE LOAD"),205,68,dim,10);small(tr("40 PALET / BUDOVA","40 PALLETS / FACILITY"),425,68,dim,10);
  small(storageCapacityLabel(capacity["used"],capacity["capacity"]),205,86,text,11);storageCapacityBar(capacity["used"],capacity["capacity"],205,106,390);
  small(tr("ZAPLNĚNÍ URANOVÉHO SKLADU","URANIUM STORAGE LOAD"),626,68,dim,10);small(storageCapacityLabel(capacity["uraniumUsed"],capacity["uraniumCapacity"]),626,86,text,11);storageCapacityBar(capacity["uraniumUsed"],capacity["uraniumCapacity"],626,106,372);
  card("WEAPONS & EQUIPMENT",204,130,488,618);card("",702,130,312,618);
  wrap("Select an item to view details.",219,183,458,9,dim);base=std::clamp(base,0,std::max(0,int(game.state["bases"].size())-1));if(!game.state["bases"].empty())control(tr("ZÁKLADNA: ","BASE: ")+game.state["bases"][base]["name"].get<std::string>()+(game.tradeAirport(base,game.state["hours"])?tr(" · LETIŠTĚ ONLINE"," · AIRPORT ONLINE"):tr(" · CHYBÍ LETIŠTĚ / SKLAD"," · AIRPORT / STORAGE REQUIRED")),219,202,440,28,[this]{if(!game.state["bases"].empty())base=(base+1)%int(game.state["bases"].size());});
  Json items=Json::array();for(const auto& g:{"weapons","equipment"})for(const auto& r:game.gameset()[g]){Json item=r;item["nativeGroup"]=g;items.push_back(item);}
  if(const auto* robot=game.record("units","underwater-robot")){Json item=*robot;item["nativeGroup"]="units";items.push_back(item);}
  Json ordered=Json::array();for(const auto& id:{"pistol","pistol-ammo","rifle","rifle-ammo","machine-gun","machine-gun-ammo","rocket-launcher","rocket-ammo","incendiary-rocket"})for(const auto& item:items)if(item["id"]==id)ordered.push_back(item);
  for(const auto& item:items){bool seen=false;for(const auto& r:ordered)seen=seen||r["id"]==item["id"];if(!seen)ordered.push_back(item);}items=std::move(ordered);
  for(const auto& material:{"iron","plastics","electronics","glass"})items.push_back({{"id",material},{"name",material},{"description","Raw manufacturing material."},{"nativeGroup","materials"},{"production",{{"marketPrice",std::string(material)=="electronics"?200:50}}}});
  if(items.empty())return;index=std::clamp(index,0,int(items.size())-1);auto selected=items[index];std::string id=selected["id"],itemGroup=selected["nativeGroup"];
  listSlider(scroll,int(items.size()),5,219,242,440,390,"storage");
  small("WEAPONS",219,222,dim,8);SDL_Rect listClip{219,242,458,392};SDL_RenderSetClipRect(renderer,&listClip);
  for(int row=0,i=scroll;i<int(items.size())&&row<5;++i,++row){int y=242+row*78;bool ammo=items[i]["id"].get<std::string>().find("ammo")!=std::string::npos;int x=ammo?237:219;control("",x,y,ammo?422:440,74,[this,i]{index=i;quantity=1;},index==i);hits.back().name="storage-item:"+items[i]["id"].get<std::string>();image(items[i].value("image",""),x+9,y+7,98,58);small((ammo?"• ":"")+nameOf(items[i]),x+119,y+25,text,9);small("IN STOCK · "+std::to_string(game.state["stock"].value(items[i]["id"].get<std::string>(),0)),x+119,y+58,dim,8);}
  SDL_RenderSetClipRect(renderer,nullptr);
  small(itemGroup,717,146,dim,8);small(nameOf(selected),717,163,bright,10);line(717,179,999,179);wrap(selected.value("description",""),717,190,282,10);
  card(tr("STÁTNÍ OBCHOD · NÁKLADNÍ LETY","COUNTRY TRADE · CARGO FLIGHTS"),717,243,282,111);std::string partner=game.state.value("tradePartner",std::string("Germany"));control(localized(partner)+" v",727,273,262,29,[this]{freightPartnerPicker=true;});hits.back().name="freight-country-open";control(tr("LETY ZÁSOBOVÁNÍ >","CARGO FLIGHTS >"),727,315,262,27,[this]{go("freight");});hits.back().name="freight-open";
  int sy=370;if(selected.contains("specs"))for(const auto& spec:selected["specs"]){metric(spec[0],spec[1],717,sy,282);sy+=30;if(sy>500)break;}
  metric("IN STORAGE",std::to_string(game.state["stock"].value(id,0)),717,533,282);metric("PURCHASE PRICE",cash(selected.value("production",Json::object()).value("marketPrice",0)),717,565,282);
  metric(tr("NAKUPOVANÉ MNOŽSTVÍ","PURCHASE QUANTITY"),std::to_string(quantity),717,607,282);
  box(219,650,458,89);small(tr("MNOŽSTVÍ","QUANTITY"),229,665,dim,10);
  control(std::to_string(quantity),306,657,58,27,[this]{input(std::to_string(quantity),true,[this](const std::string& value){try{std::size_t used=0;int n=std::stoi(value,&used);if(used==value.size()&&n>=1&&n<=100)quantity=n;}catch(...){}});});hits.back().name="storage-quantity-number";
  rect(379,667,287,7,{23,39,23});int thumb=379+(quantity-1)*286/99;rect(379,667,thumb-379,7,accent);rect(thumb-3,659,6,23,bright);hits.push_back({{379,655,287,30},[this]{quantityAt(mx);quantityDragging=true;SDL_CaptureMouse(SDL_TRUE);},"storage-quantity-slider"});
  control("-",229,695,35,30,[this]{quantity=std::max(1,quantity-1);});control("1",271,695,40,30,[this]{quantity=1;});control("10",318,695,40,30,[this]{quantity=10;});control("100",365,695,47,30,[this]{quantity=100;});hits.back().name="storage-quantity-100";
  control("BUY",419,695,115,30,[this,id,itemGroup]{freightFeedback=localized(game.trade(itemGroup,id,true,quantity,base,game.state.value("tradePartner",std::string("Germany"))).message);});control("SELL",541,695,125,30,[this,id,itemGroup]{freightFeedback=localized(game.trade(itemGroup,id,false,quantity,base,game.state.value("tradePartner",std::string("Germany"))).message);});
  small(tr("DÉLKA LETU: ","FLIGHT TIME: ")+std::to_string(int(game.tradeFlightHours(partner,base)))+" h",717,645,dim,10);wrap(freightFeedback,717,673,280,11,bright);if(freightPartnerPicker)freightCountryPicker();
 }
 void finance(){
  card("FINANCIAL STATUS",200,70,397,500);card("PERSONNEL COSTS",611,70,397,500);
  long long payroll=0;for(const auto& p:game.state["personnel"])payroll+=p["salary"].get<long long>();
  metric("AVAILABLE FUNDS",cash(game.state["money"].get<long long>()),215,122,367);metric("TOTAL MONTHLY PAYROLL",cash(payroll),215,149,367);metric("PERSONNEL",std::to_string(game.state["personnel"].size()),215,176,367);
  small("WEEKLY CASHFLOW · CURRENT PERIOD",215,238,dim,8);long long total=0;auto support=game.countrySupport();for(const auto& r:support)total+=r["contribution"].get<long long>();metric("STATE SUPPORT · WEEKLY",cash(total),215,269,367);
  wrap(tr("Hodnoty odpovídají nativní kampani. Úplné účetnictví původní HTML hry ještě není převedeno.","Values reflect the native campaign. The complete HTML accounting model has not yet been ported."),215,325,367,10,dim);
  wrap("Total personnel payroll includes all hired staff.",626,117,367,9,dim);metric("MONTHLY PAYROLL",cash(payroll),626,166,367);int y=205;
  for(const auto& role:{"soldier","technician","scientist","pilot","navigator"}){long long n=0;for(const auto& p:game.state["personnel"])if(p["role"]==role)n+=p["salary"].get<long long>();metric(role,cash(n),626,y,367);y+=27;}
  int height=countriesOpen?174:77;card("STATE SUPPORT & APPROVAL",200,584,808,height);control(tr("PODPORA PODLE STÁTŮ","SUPPORT BY COUNTRY"),213,619,782,31,[this]{countriesOpen=!countriesOpen;scroll=0;});
  if(countriesOpen){for(int row=0,i=scroll;i<int(support.size())&&row<3;++i,++row){auto r=support[i];int yy=659+row*28;small(r["name"],217,yy,text,9);small(std::to_string(r["approval"].get<int>())+"%",710,yy,text,9);small(cash(r["contribution"].get<long long>()),800,yy,text,9);}}
 }
 void personnel(){
  game.ensureWorkforce();bool soldiers=page=="soldiers";if(soldiers)role="soldier";
  card(tr("PERSONÁL","PERSONNEL"),200,70,300,682);card(tr("PRACOVNÍ TRH","WORKFORCE MARKET"),510,70,498,682);
  int x=215;for(const auto& id:{"soldier","technician","scientist","pilot","navigator"}){if(soldiers&&std::string(id)!="soldier")continue;control(id,x,114,145,30,[this,id]{role=id;index=scroll=0;},role==id);x+=155;}
  small(tr("NAJATÝ PERSONÁL","HIRED PERSONNEL"),215,165,dim,10);int row=0,seen=0,count=0;
  for(int i=0;i<int(game.state["personnel"].size());++i){const auto& person=game.state["personnel"][i];if(person["role"]!=role||person.value("dead",false))continue;++count;if(seen++<scroll||row>=10)continue;int yy=195+row++*44;
   control(person["name"],215,yy,270,36,[this,i]{if(game.state["personnel"][i]["role"]=="soldier"){selectedSoldier=i;go("soldier-profile");}else{go("staff-profile");index=i;}});if(role=="soldier")hits.back().name="PROFILE";
  }
  listSlider(scroll,count,10,215,195,270,432,"personnel");
  std::vector<int> offers;for(int i=0;i<int(game.state["offers"].size());++i)if(game.state["offers"][i]["role"]==role)offers.push_back(i);
  if(offers.empty()){small(tr("ŽÁDNÍ KANDIDÁTI","NO CANDIDATES"),525,180,dim,12);return;}index=std::clamp(index,0,int(offers.size())-1);
  for(int i=0;i<int(offers.size());++i){const auto& offer=game.state["offers"][offers[i]];control(offer["tier"].get<std::string>()+" · "+offer["name"].get<std::string>(),525,160+i*34,468,29,[this,i]{index=i;},index==i);}
  const auto& candidate=game.state["offers"][offers[index]];int yy=312;
  metric(tr("MĚSÍČNÍ PLAT","MONTHLY SALARY"),cash(candidate["salary"]),525,yy,468);yy+=30;
  for(const auto& attribute:candidate["attributes"].items()){metric(attribute.key(),attribute.value().dump()+" / 100",525,yy,468);yy+=25;}
  control(tr("NAJMOUT · ","HIRE · ")+cash(candidate["role"]=="soldier"?candidate.value("quality",42)*1000LL:0),525,704,468,32,[this,id=offers[index]]{result(game.hireCandidate(id));});
 }
 void staffProfile(){
  auto& people=game.state["personnel"];if(people.empty())return;index=std::clamp(index,0,int(people.size())-1);const auto& person=people[index];
  card(person["name"],210,75,795,675);int y=145;metric("ROLE",person["role"],230,y,750);y+=35;metric("SALARY",cash(person["salary"]),230,y,750);y+=40;
  if(person.contains("attributes"))for(const auto& a:person["attributes"].items()){metric(a.key(),a.value().dump(),230,y,750);y+=30;}
  control(tr("ZPĚT","BACK"),230,685,750,36,[this]{go("personnel");});
 }
 std::string aircraftImage(const std::string& type){auto design=game.record("units",type);if(design){auto path=design->value("image",std::string(""));if(!path.empty()&&fs::exists(game.dataDir/path))return path;}const std::map<std::string,std::string> originals={{"fighter","assets/87a8a5bec7c68900451d2d3d5c0fa8af1ef0f616b1e1e184b6e484a8a6cadf8f.png"},{"hovercraft","assets/c44845499d829d3d5e36878fac7e7b8b75fa73708e7d20acf2ccdd88900d37cb.png"},{"awacs","assets/3abe0bc577204d7ecd9ce51685a3a015c04243e5127b035eecdf8b77e6b67ceb.png"}};auto found=originals.find(type);if(found==originals.end()||!fs::exists(game.dataDir/found->second))throw std::runtime_error("Missing aircraft artwork: "+type);return found->second;}
 void aircraft(){
  card("",196,66,462,690);card("HANGAR CAPACITY · MAXIMUM",670,66,342,690);
  auto& fleet=game.state["craft"];if(fleet.empty()){small("NO AIRCRAFT",210,100,dim,12);boundedText(aircraftFeedback,{210,140,430,60},11,bright);return;}activeCraft=std::clamp(activeCraft,0,int(fleet.size())-1);auto& craft=fleet[activeCraft];std::string type=craft["type"];
  small("SELECT AIRCRAFT",205,75,dim,8);control(craft.value("name",type+" · "+std::to_string(activeCraft+1))+"  v",205,87,397,23,[this]{activeCraft=(activeCraft+1)%int(game.state["craft"].size());});
  control("NAME",608,89,41,21,[this]{input(game.state["craft"][activeCraft].value("name",game.state["craft"][activeCraft]["type"].get<std::string>()),false,[this](const std::string& value){if(value.empty()||value.size()>80){notice="Invalid aircraft name";return;}game.state["craft"][activeCraft]["name"]=value;});});
  small(type=="hovercraft"?tr("OBOJŽIVELNÍK · LET / POD VODOU","AMPHIBIOUS · FLIGHT / UNDERWATER"):type,205,118,bright,10);line(205,134,648,134);card("CREW ASSIGNMENT · WORKFORCE MARKET",205,142,444,202);
  auto crew=craft.value("crew",Json{{"pilot",-1},{"navigator",-1},{"technicians",Json::array()}});
  int pilot=crew.value("pilot",-1);std::string pilotName=pilot<0?"SELECT PILOT":game.state["personnel"][pilot]["name"].get<std::string>();control("PILOT · "+pilotName,220,177,414,28,[this]{crewRole="pilot";equipmentScroll=0;});
  if(type=="hovercraft"){int nav=crew.value("navigator",-1);control("NAVIGATOR · "+(nav<0?std::string("SELECT"):game.state["personnel"][nav]["name"].get<std::string>()),220,211,414,28,[this]{crewRole="navigator";equipmentScroll=0;});}
  else if(type=="awacs")control("TECHNICIANS · "+std::to_string(crew["technicians"].size())+" / 6",220,211,414,28,[this]{crewRole="technician";equipmentScroll=0;});
  small("AIRCRAFT SERVICE & OPERATING COSTS",220,245,dim,10);
  small("FUEL "+std::to_string(int(craft["fuel"].get<double>()))+"% · HULL "+std::to_string(int(craft["hull"].get<double>()))+"%",220,266,text,10);if(type=="fighter"){auto f=game.fighterFuel(activeCraft);boundedText(tr("BOJOVÝ DOSAH: ","COMBAT RADIUS: ")+std::to_string(int(f["combatRadiusKm"].get<double>()))+" km",{220,335,414,16},10,dim);}
  if(type=="fighter")small("CANNON "+std::to_string(craft["ammo"].get<int>())+" · MISSILES "+std::to_string(craft["missiles"].get<int>()),220,286,text,10);
  auto quote=game.serviceQuote(activeCraft);std::string service=craft.contains("serviceReady")?"SERVICE IN PROGRESS":quote["needed"].get<bool>()?"SERVICE · "+cash(quote["cost"])+" · "+std::to_string(quote["hours"].get<int>())+" H":"READY · NO SERVICE REQUIRED";
  control(service,220,306,414,23,[this]{result(game.serviceAircraft(activeCraft));});
  box(316,352,222,222);small("HULL INTEGRITY",326,362,dim,8);rect(326,385,int(craft["hull"].get<double>()*1.9),5,accent);
  image(aircraftImage(type),326,405,202,150);hits.push_back({{326,405,202,150},[this]{go("aircraft-modules");},"aircraft-image-loadout"});
  card("AVAILABLE AIRCRAFT",685,132,312,205);int yy=170;for(std::size_t i=0;i<fleet.size()&&i<5;++i){control(fleet[i]["type"].get<std::string>()+" · "+std::to_string(i+1),694,yy,294,28,[this,i]{activeCraft=int(i);},activeCraft==int(i));yy+=34;}
  card(tr("KONTAKTY UFO","UFO CONTACTS"),685,351,312,200);int visibleContacts=0;for(std::size_t i=0;i<game.state["ufos"].size()&&visibleContacts<4;++i){auto contact=game.ufoContact(i);if(contact.is_null())continue;control(ufoContactLabel(contact)+(contact.value("source",std::string(""))=="RADAR"?" · RADAR":" · SAT"),697,394+visibleContacts++*33,288,27,[this,i]{beginInterceptionSelection(i);});}if(!visibleContacts)small(tr("ŽÁDNÉ ZACHYCENÉ KONTAKTY","NO DETECTED CONTACTS"),697,397,dim,9);
  boundedText(aircraftFeedback,{220,705,414,35},10,bright);
  if(type=="hovercraft"){control(tr("POSÁDKA A OPERACE OBOJŽIVELNÍKA >","AMPHIBIOUS SQUAD & OPERATIONS >"),220,661,414,32,[this]{operationFeedback.clear();go("amphibious");});hits.back().name="amphibious-open";}
  control(tr("VÝZBROJ A MODULY >","LOADOUT & MODULES >"),220,610,414,38,[this]{go("aircraft-modules");});
 }
 void wiki(){
  card(tr("HERNÍ PŘÍRUČKA · WIKI","OPERATIONS GUIDE · GAME WIKI"),204,64,810,694);auto articles=readJson(game.dataDir/(english?"ui/manual.json":"ui/manual-cs.json"));if(articles.empty())return;index=std::clamp(index,0,int(articles.size())-1);wikiTopicScroll=std::clamp(wikiTopicScroll,0,std::max(0,int(articles.size())-8));
  for(int row=0,i=wikiTopicScroll;i<int(articles.size())&&row<8;++i,++row){int y=118+row*65;control("",220,y,214,59,[this,i]{index=i;wikiArticleScroll=0;},index==i);hits.back().name="wiki-topic:"+std::to_string(i);SDL_Rect clip{228,y+5,198,49};SDL_RenderSetClipRect(renderer,&clip);wrap(articles[i].value("title",""),228,y+8,198,10);SDL_RenderSetClipRect(renderer,nullptr);}listSlider(wikiTopicScroll,int(articles.size()),8,220,118,214,514,"wiki-topics");SDL_Rect article{460,116,530,615};SDL_RenderSetClipRect(renderer,&article);wrap(articles[index].value("title",""),460,120,526,14);SDL_RenderSetClipRect(renderer,nullptr);std::string body=articles[index].value("text","");int lines=1;std::istringstream words(localized(body));std::string row,word;while(words>>word){if(!row.empty()&&textWidth(row+" "+word,12)>526){++lines;row=word;}else row+=(row.empty()?"":" ")+word;}wikiArticleScroll=std::clamp(wikiArticleScroll,0,std::max(0,lines-30));SDL_Rect bodyClip{460,180,526,530};SDL_RenderSetClipRect(renderer,&bodyClip);wrap(body,460,180-wikiArticleScroll*17,526,12);SDL_RenderSetClipRect(renderer,nullptr);listSlider(wikiArticleScroll,lines,30,460,180,526,530,"wiki-article");
 }

 void mapEditor(){
  card("UAA MAPEDITOR 0.1.4",242,140,540,390);wrap(tr("Editor map se otevře v samostatném okně. Mapy ukládej do složky uvedené níže.","The map editor opens in a separate window. Save maps in the folder shown below."),267,215,490,14);
  boundedText(mapEditorFeedback,{267,268,490,44},10,bright);
  boundedText(tr("MAPY OPERACÍ: ","OPERATION MAPS: ")+(game.dataDir.parent_path()/"maps").string(),{267,381,490,50},10,dim);
  control(tr("SPUSTIT EDITOR MAP","LAUNCH MAP EDITOR"),267,320,490,48,[this]{try{auto launched=launchMapEditor(game.dataDir.parent_path());mapEditorFeedback=launched.ok?tr("Editor map byl spuštěn.","Map editor launched."):tr("Editor map se nepodařilo spustit: ","Unable to launch map editor: ")+localized(launched.message);}catch(const std::exception& e){mapEditorFeedback=e.what();}});hits.back().name="mapeditor-launch";control(tr("ZPĚT DO MENU","BACK TO MENU"),267,446,490,38,[this]{go("menu");});
 }
 void unavailable(){card(tr("ROZPRACOVANÁ OBRAZOVKA","SCREEN IN DEVELOPMENT"),242,140,540,390);wrap(tr("Tato část původní HTML hry zatím není převedena. Zobrazení není ověřenou náhradou originálu.","This part of the original HTML game has not yet been ported. This view is not a verified replacement for the original."),267,215,490,14,dim);control("GLOBE",267,440,490,38,[this]{go("globe");});}
 void locations(){
  game.ensureLocations();card(tr("LOKACE · KOSMICKÁ STŘEDISKA","LOCATIONS · SPACE CENTERS"),202,65,812,690);
  int y=115;for(const auto& item:std::vector<std::pair<std::string,std::string>>{{"nasa",tr("NASA · HOUSTON A FLORIDA","NASA · HOUSTON & FLORIDA")},{"esa","ESA · KOUROU"},{"roscosmos",tr("ROSKOSMOS · BAJKONUR","ROSCOSMOS · BAIKONUR")}}){control("",220,y,772,80,[this,key=item.first]{location=key;go("space-location");});hits.back().name="location:"+item.first;small(item.second,232,y+12,bright,13);long long price=item.first=="roscosmos"?300000:400000;small(tr("PRONÁJEM NA 30 DNŮ: ","30-DAY LEASE: ")+cash(price),232,y+46,dim,11);std::string status=game.spaceLease(item.first)?tr("AKTIVNÍ · ZBÝVÁ ","ACTIVE · LEFT ")+std::to_string(int(std::ceil((game.state["leases"][item.first].get<double>()-game.state["hours"].get<double>())/24)))+tr(" DNŮ"," DAYS"):tr("BEZ PRONÁJMU","NOT LEASED");small(status,690,y+46,bright,11);y+=94;}
  small(tr("ZÁKLADNOVÉ SATELITNÍ STANICE","BASE SATELLITE STATIONS"),220,405,bright,12);y=440;
  for(int b=0;b<int(game.state["bases"].size())&&y<680;++b){bool station=false;for(const auto& facility:game.state["bases"][b]["buildings"])if(facility["id"]=="satellite"&&facility["ready"].get<double>()<=game.state["hours"].get<double>())station=true;
   if(station){control(game.state["bases"][b]["name"].get<std::string>(),220,y,772,34,[this,b]{base=b;location="esa";spaceVisual="orbit";go("space-visual");});y+=44;}}
  if(y==440)small(tr("ŽÁDNÁ STANICE · POSTAV JI NA ZÁKLADNĚ","NO STATIONS · BUILD ONE AT A BASE"),220,y,dim,11);
 }
 void spaceModel(const std::string& type,int x,int y,int w,int h){
  SDL_Rect clip{x,y,w,h};if(type=="iss"||type=="kourou")modelViewports.push_back({clip,type});SDL_RenderSetClipRect(renderer,&clip);
  if(type=="florida"||type=="baikonur"){
   bool narrow=w<600;int availableWidth=narrow?w-16:w-240,availableHeight=narrow?h-104:h-16;double scale=std::min(availableWidth/440.,availableHeight/700.);int iw=int(440*scale),ih=int(700*scale),ix=x+(availableWidth-iw)/2,iy=y+(availableHeight-ih)/2;image("space/"+type+"-detailed.png",ix,iy,iw,ih);
   std::vector<std::string> labels={tr("KRYT NÁKLADU","PAYLOAD FAIRING"),tr("NÁDRŽE A TRUP","TANKS AND BODY"),tr("POHONNÁ SEKCE","PROPULSION SECTION"),tr("OBSLUŽNÁ VĚŽ","SERVICE TOWER"),tr("STARTOVACÍ STŮL","LAUNCH TABLE")};int n=0;for(const auto& p:std::vector<std::pair<int,int>>{{220,135},{220,300},{220,470},{335,260},{220,615}}){int yy=narrow?y+h-88+(n/2)*26:y+28+n*std::max(36,(h-60)/5),px=ix+int(p.first*scale),py=iy+int(p.second*scale),lx=narrow?x+8+(n%2)*(w/2):x+w-230;if(!narrow)line(px,py,lx-8,yy+7,dim);rect(px-9,py-8,18,16,panel);small(std::to_string(n+1),px-5,py-5,bright,10);small(std::to_string(n+1)+" · "+labels[n],lx,yy,bright,narrow?9:11);++n;}
  }
  else{
   if(!spaceMeshes.count(type)){spaceMeshes[type]=readJson(game.dataDir/("space/"+type+".json"));auto detail=readJson(game.dataDir/"space/surface-details.json");if(detail.contains(type))for(const auto& poly:detail[type])spaceMeshes[type].push_back(poly);}
   SDL_Rect model=type=="iss"?SDL_Rect{x+8,y+8,w-16,h-158}:SDL_Rect{x+8,y+8,w-270,h-16};
   double fit=1,centerX=0,centerY=0;
   if(type=="kourou"){double loX=1e9,loY=1e9,hiX=-1e9,hiY=-1e9;for(const auto& poly:spaceMeshes[type])for(const auto& pt:poly["points"]){auto q=projectSpacePoint(type,pt[0],pt[1],pt[2],model);loX=std::min(loX,q[0]);hiX=std::max(hiX,q[0]);loY=std::min(loY,q[1]);hiY=std::max(hiY,q[1]);}fit=std::min((model.w-32)/(hiX-loX),(model.h-32)/(hiY-loY))*spaceCamera(type).zoom/1.5;centerX=(loX+hiX)/2;centerY=(loY+hiY)/2;}
   auto project=[&](double a,double b,double c){auto q=projectSpacePoint(type,a,b,c,model);if(type=="kourou"){q[0]=model.x+model.w*.5+(q[0]-centerX)*fit;q[1]=model.y+model.h*.5+(q[1]-centerY)*fit;}return q;};
   struct Face{std::vector<SDL_Vertex> vertices;double depth;};std::vector<Face> faces;
   for(const auto& poly:spaceMeshes[type]){std::string c=poly["color"];int r=120,g=150,b=120;if(c[0]=='#'){unsigned n=std::stoul(c.substr(1),nullptr,16);r=n>>16;g=(n>>8)&255;b=n&255;}else std::sscanf(c.c_str(),"rgb(%d,%d,%d)",&r,&g,&b);
    if(type=="iss"&&r<=25&&g<=40){r=50;g=67;b=43;}
    const auto& pts=poly["points"];double ux=pts[1][0].get<double>()-pts[0][0].get<double>(),uy=pts[1][1].get<double>()-pts[0][1].get<double>(),uz=pts[1][2].get<double>()-pts[0][2].get<double>(),vx=pts[2][0].get<double>()-pts[0][0].get<double>(),vy=pts[2][1].get<double>()-pts[0][1].get<double>(),vz=pts[2][2].get<double>()-pts[0][2].get<double>();double nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx,len=std::sqrt(nx*nx+ny*ny+nz*nz);auto& cam=spaceCamera(type);double rx=cos(cam.yaw)*nx-sin(cam.yaw)*(type=="iss"?ny:nz),q=sin(cam.yaw)*nx+cos(cam.yaw)*(type=="iss"?ny:nz),ry=cos(cam.pitch)*(type=="iss"?q:ny)-sin(cam.pitch)*(type=="iss"?nz:q);double light=len>1e-9?std::abs((-.45*rx+.7*ry+.55*nz)/len):.5;double shade=.9+.4*std::clamp(light,0.,1.);r=std::clamp(int(r*shade),0,215);g=std::clamp(int(g*shade),0,225);b=std::clamp(int(b*shade),0,215);
    Face f;f.depth=0;for(const auto& pt:pts){auto projected=project(pt[0],pt[1],pt[2]);f.vertices.push_back({{float(projected[0]),float(projected[1])},{Uint8(r),Uint8(g),Uint8(b),255},{0,0}});f.depth+=projected[2];}f.depth/=f.vertices.size();faces.push_back(std::move(f));
   }
   std::stable_sort(faces.begin(),faces.end(),[&](const Face&a,const Face&b){return type=="iss"?a.depth<b.depth:a.depth>b.depth;});for(const auto& f:faces){std::vector<int> indices;for(int i=1;i+1<int(f.vertices.size());++i){indices.push_back(0);indices.push_back(i);indices.push_back(i+1);}SDL_RenderGeometry(renderer,nullptr,f.vertices.data(),int(f.vertices.size()),indices.data(),int(indices.size()));double area=0;for(size_t i=0;i<f.vertices.size();++i){auto a=f.vertices[i].position,b=f.vertices[(i+1)%f.vertices.size()].position;area+=a.x*b.y-b.x*a.y;}float minX=1e9,minY=1e9,maxX=-1e9,maxY=-1e9;for(const auto& vertex:f.vertices){minX=std::min(minX,vertex.position.x);maxX=std::max(maxX,vertex.position.x);minY=std::min(minY,vertex.position.y);maxY=std::max(maxY,vertex.position.y);}double aspect=std::max(maxX-minX,maxY-minY)/std::max(1.f,std::min(maxX-minX,maxY-minY));if(std::abs(area)>120&&aspect<8){for(size_t i=0;i<f.vertices.size();++i){auto a=f.vertices[i].position,b=f.vertices[(i+1)%f.vertices.size()].position;line(int(a.x),int(a.y),int(b.x),int(b.y),Color{12,22,12,110});}}}
   struct Part{double x,y,z;std::string name,detail;};std::vector<Part> parts;
   if(type=="iss")parts={{-5.5,0,.72,"ZVEZDA",tr("řízení a podpora života","control and life support")},{-3.8,0,.61,"ZARJA",tr("zásoby a pohon","storage and propulsion")},{-7.7,0,.68,"NAUKA",tr("výzkumný modul","research module")},{1.15,0,.77,"DESTINY",tr("americká laboratoř","US laboratory")},{4.8,0,.72,"KIBO",tr("japonská laboratoř","Japanese laboratory")},{6.8,0,.58,"COLUMBUS",tr("evropská laboratoř","European laboratory")},{.9,-1.9,.42,"CUPOLA",tr("pozorovací stanoviště","observation deck")},{4.5,3.7,1.5,"CANADARM2",tr("robotické rameno","robotic arm")},{.48,-6.7,4.5,tr("SOLÁRNÍ PANELY","SOLAR ARRAYS"),tr("výroba elektřiny","electrical generation")},{0,5,-.65,tr("PŘÍHRADOVÝ NOSNÍK","INTEGRATED TRUSS"),tr("nosná konstrukce","structural backbone")},{-.95,-3.1,0,tr("RADIÁTORY","RADIATORS"),tr("odvod tepla","heat rejection")},{8.1,.25,.3,tr("DOKOVACÍ UZEL","DOCKING PORT"),tr("připojení lodí","visiting spacecraft")}};
   else parts={{0,60,0,tr("AERODYNAMICKÝ KRYT","PAYLOAD FAIRING"),tr("chrání vynášený náklad","protects the payload")},{0,42,0,tr("HORNÍ STUPEŇ","UPPER STAGE"),tr("dosažení cílové dráhy","orbital insertion")},{0,25,0,tr("HLAVNÍ STUPEŇ","CORE STAGE"),tr("kryogenní nádrže","cryogenic tanks")},{4.15,12,0,tr("POMOCNÉ MOTORY · 4×","BOOSTERS · 4×"),tr("tuhé pohonné látky","solid propellant")},{0,1,0,tr("HLAVNÍ MOTOR","MAIN ENGINE"),tr("tryska a pohonná jednotka","nozzle and propulsion")},{-18,60,-1,tr("OBSLUŽNÁ VĚŽ","SERVICE GANTRY"),tr("přístup a přípojky","access and connections")},{3,2.7,0,tr("STARTOVACÍ STŮL","LAUNCH TABLE"),tr("podpěry a odvod spalin","supports and flame trench")}};
   int number=0;for(const auto& part:parts){int n=++number;auto p=project(part.x,part.y,part.z);int lx=type=="iss"?x+8+((n-1)%2)*(w/2):x+w-250,ly=type=="iss"?y+h-138+((n-1)/2)*22:y+28+(n-1)*74;
    if(p[0]>=model.x&&p[0]<=model.x+model.w&&p[1]>=model.y&&p[1]<=model.y+model.h){if(type!="iss")line(int(p[0]),int(p[1]),lx-8,ly+7,Color{74,94,70,150});int pinX=int(p[0])+(type=="iss"?0:28);if(type!="iss")line(int(p[0]),int(p[1]),pinX-10,int(p[1]),bright);rect(pinX-10,int(p[1])-9,22,18,Color{5,12,5,235});small(std::to_string(n),pinX-6,int(p[1])-6,bright,10);}
    small(std::to_string(n)+" · "+part.name+(type=="iss"?" · "+part.detail:""),lx,ly,bright,type=="iss"?9:12);if(type!="iss")wrap(part.detail,lx,ly+23,238,11,dim);
   }
  }SDL_RenderSetClipRect(renderer,nullptr);
 }
 void verifySatelliteSurveyUI(){
  auto render=[this]{hits.clear();modelViewports.clear();listScrollRegions.clear();layoutFrames.clear();rect(0,0,W,H,bg);renderScreen();};
  auto saved=game.state;auto oldPage=page,oldLocation=location,oldVisual=spaceVisual,oldFilter=orbitFilter;bool oldWeather=weatherOverlay;int oldMX=mx,oldMY=my;
  game.state["hours"]=96.;game.state["satelliteScans"]={{"EARTH",Json::object()},{"WATER",Json::object()},{"AIR",Json::object()}};game.state["ufoSites"]=Json::array();game.state["ufos"]=Json::array();game.state["satellites"]=Json::array({{{"type","AIR"},{"altitude",980},{"fuel",60},{"ready",0.},{"launchTime",0.},{"number",1}}});game.state["ufos"].push_back({{"type","UNMANNED_DRONE"},{"lat",30.},{"lon",80.},{"spawned",93.},{"expires",800.},{"environment","AIR"}});
  game.recordSatelliteScan(45,10,0,"EARTH",1100,1);game.recordSatelliteScan(-15,30,84,"EARTH",1100,1);game.recordSatelliteScan(-20,-140,48,"WATER",1100,1);game.recordSatelliteScan(30,80,94,"AIR",1100,1);location="esa";spaceVisual="orbit";page="space-visual";weatherOverlay=false;mx=10;my=10;
  auto mapPixels=[this]{SDL_Rect area{220,195,772,386};std::vector<Uint32> pixels(area.w*area.h);SDL_RenderReadPixels(renderer,&area,SDL_PIXELFORMAT_ARGB8888,pixels.data(),area.w*4);return pixels;};std::vector<Uint32> all;
  for(const std::string filter:{"ALL","EARTH","WATER","AIR"}){render();auto found=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.name=="survey-filter:"+filter;});if(found==hits.end())throw std::runtime_error("Survey filter missing");found->action();render();if(orbitFilter!=filter)throw std::runtime_error("Survey filter failed");if(filter=="ALL")all=mapPixels();else if(mapPixels()==all)throw std::runtime_error("Survey layer does not change map pixels");SDL_Surface* shot=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,shot->format->format,shot->pixels,shot->pitch);SDL_SaveBMP(shot,(game.userDir/("survey-"+filter+".bmp")).string().c_str());SDL_FreeSurface(shot);}
  orbitFilter="EARTH";render();auto before=mapPixels();game.state["hours"]=97.;render();if(mapPixels()!=before)throw std::runtime_error("Satellite orbit lines or instantaneous positions remain on the survey map");
  game.state=saved;page=oldPage;location=oldLocation;spaceVisual=oldVisual;orbitFilter=oldFilter;weatherOverlay=oldWeather;mx=oldMX;my=oldMY;render();std::cout<<"Satellite survey UI: age layers, actual UFO observation, all sensor filters and no orbital paths passed\n";
 }
 Color surveyAgeColor(double hours){return hours<=6?Color{111,152,99,170}:hours<=24?Color{74,112,68,160}:hours<=72?Color{103,111,70,150}:Color{62,72,60,145};}
 void satelliteSurveyLegend(int x,int y){int n=0;for(const auto& item:std::vector<std::pair<double,std::string>>{{0,tr("DO 6 H","UP TO 6 H")},{12,"6–24 h"},{48,"1–3 d"},{96,tr("STARŠÍ","OLDER")},{-1,tr("BEZ SNÍMKU","UNSCANNED")}}){int left=x+n++*153;auto color=item.first<0?Color{14,23,14,255}:surveyAgeColor(item.first);color.a=255;rect(left,y,18,12,color);small(item.second,left+25,y,dim,10);}}
 void orbitMap(int x,int y,int w,int h){
  loadGlobeAssets();box(x,y,w,h);SDL_Rect surface{x+1,y+1,w-2,h-2};SDL_RenderCopy(renderer,globeSurface,nullptr,&surface);auto point=[&](double lon,double lat){return std::pair<int,int>{x+int((lon+180)/360*w),y+int((90-lat)/180*h)};};
  for(int lon=-150;lon<180;lon+=30){auto p=point(lon,0);line(p.first,y,p.first,y+h,edge);}for(int lat=-60;lat<=60;lat+=30){auto p=point(0,lat);line(x,p.second,x+w,p.second,edge);}
  if(weatherOverlay)for(int row=0;row<12;++row)for(int col=0;col<24;++col){double lon=-180+(col+.5)*15,lat=90-(row+.5)*15;int risk=game.weatherAt(lon,lat,game.state["hours"])["risk"];rect(x+col*w/24+1,y+row*h/12+1,std::max(1,w/24-1),std::max(1,h/12-1),risk>=85?Color{125,131,112,100}:risk>=60?Color{79,92,68,75}:Color{12,29,15,0});}
  auto ring=[&](const Json& r){bool first=true;std::pair<int,int> prev;for(const auto& p:r){auto q=point(p[0],p[1]);if(!first&&abs(q.first-prev.first)<w/2)line(prev.first,prev.second,q.first,q.second,Color{59,87,54});first=false;prev=q;}};
  for(const auto& feature:globeCountries["features"]){const auto& geo=feature["geometry"];if(geo["type"]=="Polygon")for(const auto& r:geo["coordinates"])ring(r);else if(geo["type"]=="MultiPolygon")for(const auto& p:geo["coordinates"])for(const auto& r:p)ring(r);}
  SDL_Rect clip{x+1,y+1,w-2,h-2};SDL_RenderSetClipRect(renderer,&clip);
  // ISS ground track uses the period and inclination from the original HTML.
  auto track=[&](double period,double inclination,double phase,const std::string& label){std::pair<int,int> prev;for(int i=-600;i<=600;++i){double hours=game.state["hours"].get<double>()+i*period/100,u=hours/period*2*M_PI+phase,lat=asin(sin(inclination)*sin(u))*180/M_PI,lon=(atan2(cos(inclination)*sin(u),cos(u))-hours/23.9344*2*M_PI)*180/M_PI;lon=fmod(fmod(lon+180,360)+360,360)-180;auto q=point(lon,lat);if(i>-600&&abs(q.first-prev.first)<w/2)line(prev.first,prev.second,q.first,q.second,std::abs(i)<=100?bright:i<0?Color{125,151,112}:Color{92,116,81});if(i==0){rect(q.first-3,q.second-3,6,6,bright);small(label,q.first+7,q.second-12,bright,10);}prev=q;}};
  if(location!="esa")track(92.68/60,51.6*M_PI/180,0,"ISS");
  int visible=0;for(const auto& sat:game.state["satellites"])if(orbitFilter=="ALL"||sat["type"]==orbitFilter)++visible;
  if(location=="esa"){
   rect(x+1,y+1,w-2,h-2,Color{3,9,3,105});auto survey=game.satelliteSurvey(orbitFilter);double now=game.state["hours"];
   for(const auto& cell:survey["cells"]){double age=std::max(0.,now-cell["at"].get<double>());int col=cell["col"],row=cell["row"],left=x+col*w/72,top=y+(35-row)*h/36,right=x+(col+1)*w/72,bottom=y+(36-row)*h/36;rect(left,top,std::max(1,right-left),std::max(1,bottom-top),surveyAgeColor(age));}
   int shown=0;for(const auto& contact:survey["contacts"]){double age=std::max(0.,now-contact["at"].get<double>());if(age>72||shown++>=16)continue;auto p=point(contact["lon"],contact["lat"]);auto color=surveyAgeColor(age);color.a=255;if(contact.value("kind",std::string(""))=="CRASH"){line(p.first-5,p.second-5,p.first+5,p.second+5,color);line(p.first+5,p.second-5,p.first-5,p.second+5,color);}else if(contact.value("kind",std::string(""))=="LANDED"){this->color(color);SDL_Rect marker{p.first-5,p.second-5,11,11};SDL_RenderDrawRect(renderer,&marker);}else{line(p.first-5,p.second,p.first,p.second-5,color);line(p.first,p.second-5,p.first+5,p.second,color);line(p.first+5,p.second,p.first,p.second+5,color);line(p.first,p.second+5,p.first-5,p.second,color);}small(ufoContactLabel(contact)+(contact.value("source",std::string(""))=="RADAR"?" · R":" · S")+" · "+std::to_string(int(age))+" h",std::min(x+w-170,p.first+8),std::clamp(p.second-12,y+3,y+h-14),bright,10);}
  }
  if(weatherOverlay)for(const auto& site:std::vector<std::pair<std::string,std::pair<double,double>>>{{"florida",{-80.6042,28.6084}},{"baikonur",{63.4,45.6}},{"kourou",{-52.768,5.236}}}){auto p=point(site.second.first,site.second.second);rect(p.first-3,p.second-3,6,6,bright);small((site.first=="florida"?"FLORIDA":site.first=="baikonur"?"BAJKONUR":"KOUROU")+std::string(" · ")+std::to_string(game.launchWeather(site.first,game.state["hours"])["risk"].get<int>())+" %",p.first+7,p.second-12,bright,10);}
  SDL_RenderSetClipRect(renderer,nullptr);if(location=="esa"&&!visible){rect(x+8,y+h-23,290,18,panel);small(tr("ŽÁDNÉ SATELITY V TOMTO FILTRU","NO SATELLITES IN THIS FILTER"),x+15,y+h-19,dim,10);}
 }
 void spaceVisualScreen(){
  game.ensureLocations();card(location=="esa"?tr("SATELITNÍ PRŮZKUM · STÁŘÍ SNÍMKŮ A KONTAKTY UFO","SATELLITE SURVEY · IMAGE AGE & UFO CONTACTS"):tr("MAPA PŘELETŮ A VÝŠEK","GROUND TRACKS & ALTITUDE"),202,65,812,690);control(weatherOverlay?tr("POČASÍ: ZAPNUTO","WEATHER: ON"):tr("POČASÍ: VYPNUTO","WEATHER: OFF"),820,74,172,25,[this]{weatherOverlay=!weatherOverlay;});
  int xx=220;for(const auto& item:std::vector<std::pair<std::string,std::string>>{{"orbit",location=="esa"?tr("PRŮZKUM A KONTAKTY","SURVEY & CONTACTS"):tr("DRÁHY A VÝŠKA","ORBITS & ALTITUDE")},{"iss","ISS"},{"ramp",tr("ODPALOVACÍ RAMPA","LAUNCH RAMP")}}){control(item.second,xx,112,250,32,[this,key=item.first]{if(key=="iss"){openSpaceModel("iss");return;}if(key=="ramp"&&location=="esa"){openSpaceModel("kourou");return;}spaceVisual=key;},spaceVisual==item.first);hits.back().name="space-view:"+item.first;xx+=260;}
  if(spaceVisual=="orbit"){
   int x=220;for(const auto& type:{"ALL","EARTH","WATER","AIR"}){control(type==std::string("ALL")?tr("VŠE","ALL"):type==std::string("EARTH")?tr("ZEMĚ","EARTH"):type==std::string("WATER")?tr("VODA","WATER"):tr("VZDUCH","AIR"),x,156,185,28,[this,type]{orbitFilter=type;},orbitFilter==type);hits.back().name=std::string("survey-filter:")+type;x+=195;}orbitMap(220,195,772,386);
   if(location=="esa"){
    small(tr("STÁŘÍ SNÍMKU · SVĚTLEJŠÍ ZELENÁ = NOVĚJŠÍ · OLIVOVÁ / ŠEDÁ = STARŠÍ","IMAGE AGE · LIGHT GREEN = RECENT · OLIVE / GREY = OLDER"),230,590,dim,10);satelliteSurveyLegend(220,615);auto survey=game.satelliteSurvey(orbitFilter);int contacts=0;for(const auto& c:survey["contacts"])if(game.state["hours"].get<double>()-c["at"].get<double>()<=72)++contacts;
    small(tr("NASNÍMANÝCH BUNĚK: ","IMAGED CELLS: ")+std::to_string(survey["cells"].size())+tr(" / 2592 · SAT/RADAR DO 72 H: "," / 2592 · SAT/RADAR WITHIN 72 H: ")+std::to_string(contacts),220,644,bright,11);
    std::string detail=tr("NAJEĎ NA MAPU · UFO OZNAČUJE POZOROVÁNÍ V ČASE SNÍMKU","HOVER THE MAP · UFO MARKS AN OBSERVATION AT THE IMAGE TIME");if(mx>=220&&mx<992&&my>=195&&my<581){int col=(mx-220)*72/772,row=35-(my-195)*36/386;std::string key=std::to_string(row)+":"+std::to_string(col);if(survey["cells"].contains(key)){const auto& c=survey["cells"][key];detail=tr("SNÍMEK PŘED ","IMAGE AGE ")+std::to_string(int(std::max(0.,game.state["hours"].get<double>()-c["at"].get<double>())))+" h · "+localized(c["channel"])+" · SAT-"+std::to_string(c["satellite"].get<int>())+(c["contacts"].empty()?tr(" · BEZ ZÁCHYTU V TOMTO SNÍMKU"," · NO CONTACT IN THIS IMAGE"):tr(" · ZÁCHYT UFO"," · UFO OBSERVED"));}else detail=tr("OBLAST DOSUD NENÍ NASNÍMANÁ V TOMTO FILTRU","AREA NOT YET IMAGED IN THIS FILTER");}small(detail,220,674,dim,10);
   }else{
    small(tr("STOPY NAVAZUJÍCÍCH PŘELETŮ ISS","SUCCESSIVE ISS GROUND TRACKS"),230,588,dim,10);
   small(tr("VÝŠKA OBĚŽNÉ DRÁHY · KM","ORBIT ALTITUDE · KM"),220,605,bright,11);double maxAltitude=1000;for(std::size_t i=0;i<game.state["satellites"].size();++i)maxAltitude=std::max(maxAltitude,game.satelliteStatus(i)["currentAltitude"].get<double>());line(260,625,980,625);line(260,650,980,650);small(std::to_string(int(maxAltitude)),220,621,dim,10);small("0",235,646,dim,10);
   int n=0,total=int(game.state["satellites"].size())+1;for(std::size_t i=0;i<game.state["satellites"].size();++i){auto sat=game.satelliteStatus(i);if(orbitFilter!="ALL"&&sat["type"]!=orbitFilter)continue;int x=285+n++*650/std::max(1,total),top=650-int(sat["currentAltitude"].get<double>()/maxAltitude*25);line(x,650,x,top,accent);rect(x-3,top-3,6,6,bright);small("#"+std::to_string(i+1),x-8,665,dim,10);}x=285+n*650/std::max(1,total);double issAltitude=game.state["iss"]["altitude"];int top=650-int(issAltitude/maxAltitude*25);line(x,650,x,top,accent);small("ISS · "+std::to_string(int(issAltitude)),x-20,665,dim,10);
   }
  }else{
   std::string type=spaceVisual=="iss"?"iss":location=="esa"?"kourou":location=="nasa"?"florida":"baikonur";spaceModel(type,220,155,772,470);
   small(spaceVisual=="iss"?tr("ISS · MODEL Z PŮVODNÍHO HTML","ISS · ORIGINAL HTML MODEL"):location=="esa"?"ARIANE 64 · KOUROU · ELA-4":location=="nasa"?"FLORIDA · LC 39B":tr("BAJKONUR · NÁKLADNÍ NOSIČ","BAIKONUR · CARGO LAUNCHER"),230,635,bright,12);
   if(type=="iss"||type=="kourou"){spaceModelControls(type,600,660,392);small(tr("TAŽENÍM OTÁČEJ · KOLEČKEM PŘIBLIŽUJ · DVOJKLIK RESET","DRAG TO ORBIT · WHEEL TO ZOOM · DOUBLE CLICK RESET"),230,649,dim,10);}

  }control(tr("ZPĚT DO STŘEDISKA","BACK TO CONTROL CENTER"),220,710,772,28,[this]{go("space-location");});
 }
 void spaceLocation(){
  game.ensureSpaceBase(location);bool active=game.spaceLease(location);long long cost=location=="roscosmos"?300000:400000;
  card(location=="esa"?tr("ESA · ŘÍZENÍ SATELITŮ","ESA · SATELLITE CONTROL"):location=="nasa"?tr("NASA · HOUSTON · ŘÍZENÍ LETŮ","NASA · HOUSTON · FLIGHT CONTROL"):tr("ROSKOSMOS · ŘÍZENÍ MISÍ","ROSCOSMOS · MISSION CONTROL"),202,65,812,690);
  control(location=="esa"?tr("MAPA PRŮZKUMU >","SURVEY MAP >"):tr("MAPA PŘELETŮ >","GROUND TRACK MAP >"),640,74,170,25,[this]{spaceVisual="orbit";go("space-visual");});
  control(weatherOverlay?tr("POČASÍ: ZAPNUTO","WEATHER: ON"):tr("POČASÍ: VYPNUTO","WEATHER: OFF"),820,74,172,25,[this]{weatherOverlay=!weatherOverlay;});orbitMap(220,110,772,240);auto weather=game.launchWeather(location=="nasa"?"florida":location=="esa"?"kourou":"baikonur",game.state["hours"]);rect(228,326,756,20,panel);small(tr("POČASÍ NA RAMPĚ: ","LAUNCH-SITE WEATHER: ")+(weather["safe"].get<bool>()?tr("VHODNÉ PRO START","SUITABLE FOR LAUNCH"):tr("NEVHODNÉ · ODLOŽENÍ STARTU","UNSUITABLE · LAUNCH DELAY"))+tr(" · RIZIKO "," · RISK ")+std::to_string(weather["risk"].get<int>())+" %",235,331,bright,10);
  if(location=="esa"){int x=220;for(const auto& type:{"ALL","EARTH","WATER","AIR"}){control(type==std::string("ALL")?tr("VŠE","ALL"):type==std::string("EARTH")?tr("ZEMĚ","EARTH"):type==std::string("WATER")?tr("VODA","WATER"):tr("VZDUCH","AIR"),x,360,115,26,[this,type]{orbitFilter=type;},orbitFilter==type);x+=123;}}
  else if(location=="nasa"){int capacity=game.astronautCapacity();small(tr("POSÁDKA HOUSTONU: ","HOUSTON CREW: ")+std::to_string(game.state["astronauts"].size())+" / "+std::to_string(capacity)+tr(" MÍST · 4 NA OBYTNOU BUŇKU"," PLACES · 4 PER RESIDENTIAL CELL"),220,365,bright,11);rect(220,384,772,5,edge);if(capacity)rect(220,384,int(std::min(1.,double(game.state["astronauts"].size())/capacity)*772),5,accent);}
  control(tr("PRONAJMOUT / PRODLOUŽIT · ","RENT / EXTEND · ")+cash(cost),220,395,280,28,[this]{spaceResult(game.rentLocation(location));});small(active?tr("PRONÁJEM: ","LEASE: ")+std::to_string(int(std::ceil((game.state["leases"][location].get<double>()-game.state["hours"].get<double>())/24)))+tr(" DNŮ"," DAYS"):tr("KOMPLEX NENÍ PRONAJATÝ","COMPLEX NOT LEASED"),520,404,dim,11);
  if(location=="nasa"){
   small(tr("HOUSTON · POZEMNÍ STŘEDISKO","HOUSTON · REFERENCE LAYOUT"),220,431,bright,11);spaceBaseView();
   control(tr("FLORIDA · RAMPA PRO POSÁDKU","FLORIDA · CREWED LAUNCH RAMP"),680,440,312,28,[this]{go("launch-ramp");});
   control(tr("ASTRONAUTI · NÁBOR A POSÁDKA >","ASTRONAUTS · RECRUITMENT & CREW >"),680,478,312,28,[this]{spaceFeedback.clear();go("astronauts");});
   control(tr("POSTAVIT OBYTNOU BUŇKU · ","BUILD RESIDENTIAL CELL · ")+cash(game.constants["costs"].value("residential",240000LL)),680,516,312,28,[this]{spaceResult(game.buildSpaceHousing());});
   control(tr("PRONAJMOUT ISS · 30 DNŮ · ","RENT ISS · 30 DAYS · ")+cash(400000),680,554,312,28,[this]{spaceResult(game.rentISS(1));});
   control(tr("OTEVŘÍT ISS V NOVÉM OKNĚ","OPEN ISS IN NEW WINDOW"),680,592,312,28,[this]{openSpaceModel("iss");});hits.back().name="open-iss-window";
   small(game.state.value("issLease",0.)>game.state["hours"].get<double>()?tr("ISS · AKTIVNÍ PRONÁJEM","ISS · ACTIVE LEASE"):tr("ISS · BEZ PRONÁJMU","ISS · NO LEASE"),680,635,dim,11);
   small(tr("STÁLÁ POSÁDKA ISS: 7 OSOB","PERMANENT ISS CREW: 7"),680,657,bright,11);
   small(tr("DORUČENÝ NÁKLAD: ","DELIVERED CARGO: ")+std::to_string(game.state.value("issCargo",0)),680,677,dim,11);
  }else{
   small(location=="esa"?tr("ESA · ŘÍDICÍ BÁZE PAŘÍŽ","ESA · PARIS CONTROL BASE"):tr("ROSKOSMOS · KOROLJOV · ŘÍDICÍ BÁZE","ROSCOSMOS · KOROLEV CONTROL BASE"),220,431,bright,11);spaceBaseView();
   control(location=="esa"?tr("KOUROU · RAMPA PRO SATELITY","KOUROU · SATELLITE LAUNCH RAMP"):tr("BAJKONUR · NÁKLADNÍ RAMPA","BAIKONUR · CARGO LAUNCH RAMP"),680,440,312,28,[this]{go("launch-ramp");});
   if(location=="esa"){control(tr("ARIANE 64 · NOVÉ OKNO","ARIANE 64 · NEW WINDOW"),680,478,312,28,[this]{openSpaceModel("kourou");});hits.back().name="open-rocket-window";}
   control(tr("MAPA A GRAF VÝŠEK >","MAP & ALTITUDE CHART >"),680,516,312,28,[this]{spaceVisual="orbit";go("space-visual");});
   small(location=="esa"?tr("2 OBYTNÉ BUŇKY · 6 SKLADŮ","2 RESIDENTIAL CELLS · 6 WAREHOUSES"):tr("2 OBYTNÉ BUŇKY · 4 SKLADY","2 RESIDENTIAL CELLS · 4 WAREHOUSES"),680,563,dim,10);
   if(location=="esa"){int y=593;for(int row=0,i=satelliteScroll;i<int(game.state["satellites"].size())&&row<3;++i,++row){auto sat=game.satelliteStatus(i);small("#"+std::to_string(i+1)+" · "+sat["type"].get<std::string>()+" · "+std::to_string(int(sat["currentAltitude"].get<double>()))+" km",680,y,text,11);y+=26;}listSlider(satelliteScroll,int(game.state["satellites"].size()),3,680,593,302,75,"satellite-fleet");if(game.state["satellites"].empty())small(tr("ŽÁDNÉ SATELITY NA DRÁZE","NO SATELLITES IN ORBIT"),680,593,dim,10);}
   else small(tr("ŘÍZENÍ NÁKLADNÍCH LETŮ K ISS","ISS CARGO FLIGHT CONTROL"),680,603,dim,11);
  }
  small(spaceFeedback.empty()&&location=="nasa"&&game.issStatus()["emergency"].get<bool>()?tr("ISS · NOUZOVÝ PROVOZ · OTEVŘI PROVOZ A ZÁSOBY VE OKNĚ ISS","ISS · EMERGENCY MODE · OPEN OPERATIONS & SUPPLIES IN ISS WINDOW"):spaceFeedback,220,692,bright,11);control(tr("ZPĚT NA LOKACE","BACK TO LOCATIONS"),220,720,220,28,[this]{go("locations");});
 }
 void launchRamp(){
  game.ensureLocations();card(location=="esa"?tr("KOUROU · PŘÍPRAVA STARTU","KOUROU · LAUNCH CONFIGURATION"):location=="nasa"?tr("FLORIDA · PILOTOVANÁ RAMPA","FLORIDA · CREWED LAUNCH RAMP"):tr("BAJKONUR · NÁKLADNÍ RAMPA","BAIKONUR · CARGO LAUNCH RAMP"),202,65,812,690);
  if(location=="esa"){
   int x=220;for(const auto& type:{"EARTH","WATER","AIR"}){control(type==std::string("EARTH")?tr("ZEMĚ","EARTH"):type==std::string("WATER")?tr("VODA","WATER"):tr("VZDUCH","AIR"),x,120,116,34,[this,type]{scanType=type;},scanType==type);x+=126;}
   auto quote=game.satelliteQuote(launchFuel);metric(tr("PALIVO · TUNY","FUEL · TONNES"),std::to_string(launchFuel),220,185,370);
   control("-",220,222,75,30,[this]{launchFuel=std::max(0,launchFuel-5);});control("+",305,222,75,30,[this]{launchFuel=std::min(100,launchFuel+5);});
   metric(tr("VÝŠKA OBĚŽNÉ DRÁHY","ORBIT ALTITUDE"),std::to_string(quote["altitude"].get<int>())+" km",220,275,370);
   metric(tr("SÍLA SKENOVÁNÍ","SCAN STRENGTH"),std::to_string(quote["strength"].get<int>())+" / 10",220,311,370);
   metric(tr("POKRYTÍ","COVERAGE"),std::to_string(quote["coverage"].get<int>())+" km",220,347,370);
   control(tr("POČET RAKET: ","ROCKET COUNT: ")+std::to_string(launchCount),220,390,370,34,[this]{launchCount=launchCount%5+1;});
   control(tr("NAPLÁNOVAT START · ","SCHEDULE LAUNCH · ")+cash(quote["cost"].get<long long>()*launchCount),220,440,370,38,[this]{spaceResult(game.launchSatellite(scanType,launchFuel,launchCount));});
   small(tr("NAKLÁDÁNÍ 12 H · PŘÍPRAVA 24 H","LOADING 12 H · PREPARATION 24 H"),220,495,dim,12);
   int y=535;for(const auto& launch:game.state["launchQueue"]){if(y>655)break;small(launch["type"].get<std::string>()+" · "+std::to_string(int(std::ceil(launch["ready"].get<double>()-game.state["hours"].get<double>())))+" h",220,y,text,12);y+=28;}
  }else{
   control(tr("VELIKOST NÁKLADU: ","CARGO SIZE: ")+std::to_string(cargoUnits),220,120,370,34,[this]{cargoUnits=cargoUnits%3+1;});
   int fuel=40+cargoUnits*20;metric(tr("PALIVO · TUNY","FUEL · TONNES"),std::to_string(fuel),220,180,370);metric(tr("VÝŠKA OBĚŽNÉ DRÁHY","ORBIT ALTITUDE"),"420 km",220,215,370);
   if(location=="nasa")control(tr("VYBRAT POSÁDKU · ","SELECT CREW · ")+std::to_string(game.state["flightCrew"].size())+" / 8",220,260,370,34,[this]{go("astronauts");});
   else control(tr("POČET LODÍ: ","SHIP COUNT: ")+std::to_string(launchCount),220,260,370,34,[this]{launchCount=launchCount%5+1;});
   long long price=(location=="nasa"?1500000+(cargoUnits==1?100000:cargoUnits==2?250000:400000)+cargoUnits*500000LL:650000LL)+std::max(0,fuel-60)*12000LL;price*=location=="nasa"?1:launchCount;
   control(tr("NAPLÁNOVAT LET · ","SCHEDULE FLIGHT · ")+cash(price),220,315,370,38,[this]{spaceResult(game.launchTransport(location,cargoUnits,location=="nasa"?1:launchCount));});
   small(tr("PŘÍPRAVA 36 H · PŘELET ","PREPARATION 36 H · TRANSFER ")+std::string(location=="nasa"?"18 h":"48 h"),220,380,dim,12);
   int y=425;for(const auto& launch:game.state["launchQueue"]){if(y>655)break;small(launch["type"].get<std::string>()+" · "+std::to_string(int(std::ceil(launch["ready"].get<double>()-game.state["hours"].get<double>())))+" h",220,y,text,12);y+=30;}
  }
  if(location=="esa"){card("ARIANE 64 · KOUROU · ELA-4",610,115,382,160);wrap(tr("Interaktivní model se otevře v samostatném okně. Tažení otáčí, kolečko mění přiblížení.","The interactive model opens in a separate window. Drag to orbit, wheel to zoom."),625,160,350,12);control(tr("OTEVŘÍT RAKETU V NOVÉM OKNĚ","OPEN ROCKET IN NEW WINDOW"),625,225,352,28,[this]{openSpaceModel("kourou");});hits.back().name="open-rocket-window";}
  else{box(610,115,382,537);spaceModel(location=="nasa"?"florida":"baikonur",620,125,362,440);small(location=="nasa"?"FLORIDA · LC 39B":"BAJKONUR",625,638,bright,12);}small(spaceFeedback,220,690,bright,11);
  control(tr("ZPĚT DO STŘEDISKA","BACK TO CONTROL CENTER"),220,714,772,28,[this]{go("space-location");});
 }
 void astronauts(){
  game.ensureHouston();card(tr("HOUSTON · ASTRONAUTI A POSÁDKA","HOUSTON · ASTRONAUTS & CREW"),202,65,812,690);
  int x=220;for(const auto& profession:{"commander","navigator","technician","researcher"}){control(profession,x,115,185,32,[this,profession]{spaceRole=profession;index=0;},spaceRole==profession);x+=195;}
  std::vector<int> candidates;for(int i=0;i<int(game.state["spaceOffers"].size());++i)if(game.state["spaceOffers"][i]["role"]==spaceRole)candidates.push_back(i);
  if(candidates.empty()){small("NO ASTRONAUT OFFERS",220,180,dim,12);control("BACK",220,700,772,34,[this]{go("space-location");});return;}
  index=std::clamp(index,0,int(candidates.size())-1);for(int i=0;i<int(candidates.size());++i){const auto& offer=game.state["spaceOffers"][candidates[i]];control(offer["tier"].get<std::string>()+" · "+offer["name"].get<std::string>(),220,170+i*34,374,29,[this,i]{index=i;},i==index);}
  const auto& selected=game.state["spaceOffers"][candidates[index]];int y=395;metric(tr("MĚSÍČNÍ PLAT","MONTHLY SALARY"),cash(selected["salary"]),220,y,374);y+=35;
  for(const auto& a:selected["attributes"].items()){metric(a.key(),a.value().dump(),220,y,374);y+=30;}
  control(tr("NAJMOUT ASTRONAUTA","HIRE ASTRONAUT"),220,585,374,35,[this,chosen=candidates[index]]{spaceResult(game.hireAstronaut(chosen));});
  int capacity=game.astronautCapacity();
  small(tr("UBYTOVÁNÍ: ","ACCOMMODATION: ")+std::to_string(game.state["astronauts"].size())+" / "+std::to_string(capacity),220,640,dim,12);
  small(tr("POSÁDKA · VELITEL, NAVIGÁTOR, TECHNIK","CREW · COMMANDER, NAVIGATOR, TECHNICIAN"),615,165,bright,10);
  int row=0;for(int i=scroll;i<int(game.state["astronauts"].size())&&row<11;++i,++row){const auto& person=game.state["astronauts"][i];bool chosen=std::find(game.state["flightCrew"].begin(),game.state["flightCrew"].end(),Json(i))!=game.state["flightCrew"].end();control(person["name"].get<std::string>()+" · "+person["role"].get<std::string>(),615,195+row*36,377,30,[this,i]{spaceResult(game.toggleFlightCrew(i));},chosen);}
  listSlider(scroll,int(game.state["astronauts"].size()),11,615,195,377,390,"astronauts");
  small(spaceFeedback,220,675,bright,11);
  control(tr("ZPĚT NA RAMPU","BACK TO LAUNCH RAMP"),220,700,772,34,[this]{go("launch-ramp");});
 }
 void aircraftModules(){
  const auto& fleet=game.state["craft"];if(fleet.empty())return;activeCraft=std::clamp(activeCraft,0,int(fleet.size())-1);const auto& craft=fleet[activeCraft];
  card(craft.value("name",craft["type"].get<std::string>())+" · "+tr("VÝZBROJ A MODULY","LOADOUT & MODULES"),202,65,812,690);
  std::string type=craft["type"];image(aircraftImage(type),220,155,550,470);int planeX=220;for(std::size_t i=0;i<fleet.size()&&i<3;++i){control(fleet[i]["type"].get<std::string>()+" · "+std::to_string(i+1),planeX,113,176,28,[this,i]{activeCraft=int(i);aircraftFeedback.clear();},activeCraft==int(i),10);hits.back().name="loadout-aircraft:"+std::to_string(i);planeX+=184;}
  if(game.aircraftOptions(activeCraft,aircraftSlot).empty())aircraftSlot="engine";
  struct ModulePoint{std::string slot;int x,y;};std::vector<ModulePoint> points=type=="fighter"?std::vector<ModulePoint>{{"engine",590,260},{"primary",310,470},{"secondary",620,440},{"system",470,365}}:std::vector<ModulePoint>{{"engine",565,300},{"accessory",425,430}};
  for(const auto& point:points){if(game.aircraftOptions(activeCraft,point.slot).empty())continue;std::string title=point.slot=="engine"?tr("MOTOR","ENGINE"):point.slot=="primary"?tr("KANÓN","CANNON"):point.slot=="secondary"?tr("RAKETY","MISSILES"):point.slot=="system"?tr("SYSTÉM","SYSTEM"):tr("VYBAVENÍ","ACCESSORY");control(title,point.x,point.y,104,28,[this,slot=point.slot]{aircraftSlot=slot;},aircraftSlot==point.slot,10);hits.back().name="aircraft-slot:"+point.slot;}
  if(type=="fighter"){auto f=game.fighterFuel(activeCraft);small(tr("NÁDRŽ: ","FUEL TANK: ")+std::to_string(int(f["fuelLitres"].get<double>()))+" / "+std::to_string(int(f["capacityLitres"].get<double>()))+" l · "+tr("DOLET PLNÁ: ","FULL RANGE: ")+std::to_string(int(f["rangeKm"].get<double>()))+" km",220,660,bright,11);}
  small(tr("KLIKNI NA MODUL V OBRÁZKU LETADLA","SELECT A MODULE ON THE AIRCRAFT"),220,635,dim,11);
  card(tr("VÝBĚR MODULU","MODULE SELECTION"),788,123,208,493);small(aircraftSlot,800,163,bright,12);int y=196;std::string selected=game.aircraftModule(activeCraft,aircraftSlot);
  for(const auto& value:game.aircraftOptions(activeCraft,aircraftSlot)){std::string module=value;control("",800,y,184,54,[this,slot=aircraftSlot,module]{auto r=game.setAircraftModule(activeCraft,slot,module);aircraftFeedback=r.ok?tr("MODUL PŘIŘAZEN","MODULE ASSIGNED"):tr("NELZE PŘIŘADIT · DOKONČI SERVIS LETADLA","CANNOT ASSIGN · FINISH AIRCRAFT SERVICE");},module==selected);hits.back().name=module.empty()?"module-empty":module;wrap(module.empty()?tr("PRÁZDNÉ","EMPTY"):module,808,y+10,168,10);y+=62;}
  boundedText(aircraftFeedback.empty()?tr("VYBER SLOT V OBRÁZKU A MODUL VPRAVO","SELECT AN IMAGE SLOT AND A MODULE ON THE RIGHT"):aircraftFeedback,{220,681,772,16},10,bright);
  control(tr("ZPĚT DO HANGÁRU","BACK TO HANGAR"),220,700,772,34,[this]{go("aircraft");});
 }
 void baseSummary(const Json& b){
  card(b["name"].get<std::string>()+" · "+tr("INFORMACE O BÁZI","BASE INFORMATION"),244,66,506,150);
  std::map<std::string,int> counts;int occupied=0,staff=0,soldiers=0;
  for(const auto& item:b["buildings"]){auto id=item["id"].get<std::string>();auto [w,h]=game.footprint(id);if(item["floor"]==floor)occupied+=w*h;if(item["ready"].get<double>()<=game.state["hours"].get<double>())++counts[id];}
  for(const auto& person:game.state["personnel"])if(!person.value("dead",false)){if(person["role"]=="soldier")++soldiers;else ++staff;}
  int output=counts["power"]*150,consumption=output-game.energy(base),scale=std::max({150,output,consumption});
  small(tr("ENERGIE BÁZE","BASE ENERGY"),258,112,bright,10);
  for(int row=0;row<3;++row){int value=row==0?output:row==1?consumption:game.energy(base),yy=140+row*22;small(row==0?tr("VÝROBA","OUTPUT"):row==1?tr("SPOTŘEBA","USE"):tr("VOLNÁ","AVAILABLE"),258,yy,dim,10);rect(333,yy+1,142,8,{23,39,23});rect(333,yy+1,std::clamp(value*142/scale,0,142),8,accent);small(std::to_string(value)+" EU",488,yy,text,10);}
  small(tr("STAV BÁZE","BASE STATUS"),550,112,dim,10);
  small(tr("POLOHA: ","LOCATION: ")+b.value("country",std::to_string(b["lat"].get<double>()).substr(0,5)+", "+std::to_string(b["lon"].get<double>()).substr(0,5)),550,130,text,10);
  small("RADAR: "+std::string(counts["radar"]+counts["radarLarge"]?"ONLINE":"OFFLINE"),550,143,text,10);
  small(tr("OBRANA: ","DEFENCE: ")+(counts["defence"]?"ONLINE":"OFFLINE"),550,157,text,10);
  small(tr("PERSONÁL: ","STAFF: ")+std::to_string(staff)+" / "+std::to_string(counts["quarters"]*50),550,171,text,10);
  small(tr("VOJÁCI: ","SOLDIERS: ")+std::to_string(soldiers)+" / "+std::to_string(counts["soldierBarracks"]*20),550,185,text,10);
  small(tr("VOLNÁ POLE: ","FREE CELLS: ")+std::to_string(100-occupied),550,199,text,10);
 }
 void baseFees(){
  if(game.state["bases"].empty())return;const auto& b=game.state["bases"][std::clamp(base,0,int(game.state["bases"].size())-1)];
  card(tr("ZÁKLADNA · POPLATKY A KAPACITY","BASE · FEES & CAPACITIES"),202,65,812,690);
  long long payroll=0;for(const auto& person:game.state["personnel"])payroll+=person["salary"].get<long long>();if(game.state.contains("astronauts"))for(const auto& person:game.state["astronauts"])payroll+=person["salary"].get<long long>();
  metric(tr("MĚSÍČNÍ MZDY · ORGANIZACE","MONTHLY PAYROLL · ORGANIZATION"),cash(payroll),220,120,772);
  long long rent=0;for(const auto& key:{"nasa","esa","roscosmos"})if(game.spaceLease(key))rent+=std::string(key)=="roscosmos"?300000:400000;
  metric(tr("PRONÁJMY STŘEDISEK · 30 DNŮ","SPACE COMPLEX LEASES · 30 DAYS"),cash(rent),220,160,772);
  metric(tr("ISS · 30 DNŮ","ISS · 30 DAYS"),game.spaceLease("nasa")&&game.state.value("issLease",0.)>game.state["hours"].get<double>()?cash(400000):cash(0),220,200,772);
  std::map<std::string,int> counts;for(const auto& item:b["buildings"])if(item["ready"].get<double>()<=game.state["hours"].get<double>())++counts[item["id"].get<std::string>()];
  int y=265;for(const auto& entry:std::vector<std::pair<std::string,std::string>>{{"hangar",tr("HANGÁRY","HANGARS")},{"storage",tr("SKLADY · 40 PALET / BUDOVA","WAREHOUSES · 40 PALLETS / FACILITY")},{"workshop",tr("DÍLNY · 25 TECHNIKŮ / BUDOVA","WORKSHOPS · 25 TECHNICIANS / FACILITY")},{"research",tr("LABORATOŘE · 25 VĚDCŮ / BUDOVA","LABORATORIES · 25 SCIENTISTS / FACILITY")},{"quarters",tr("UBYTOVNY · 50 MÍST / BUDOVA","QUARTERS · 50 PLACES / FACILITY")},{"soldierBarracks",tr("KASÁRNA · 20 MÍST / BUDOVA","BARRACKS · 20 PLACES / FACILITY")},{"satellite",tr("SATELITNÍ STANICE","SATELLITE STATIONS")}}){metric(entry.second,std::to_string(counts[entry.first]),220,y,772);y+=36;}
  control(tr("ZPĚT NA INFORMACE","BACK TO INFORMATION"),220,695,772,35,[this]{go("base-info");});
 }
 void combatData(){
  const auto records=orderedRecords();if(records.empty())return;index=std::clamp(index,0,int(records.size())-1);const auto& item=records[index];
  card("",202,65,812,690);boundedText(nameOf(item)+" · "+tr("PŘESNOST A SÍLA","ACCURACY & POWER"),{216,77,780,36},10,bright);
  if(!item.contains("combat"))return;const auto& combat=item["combat"];int y=120;
  if(combat.contains("values"))for(const auto& value:combat["values"].items()){metric(value.key(),value.value().dump(),220,y,772);y+=27;}
  if(combat.contains("curves")){int column=0;for(const auto& mode:std::vector<std::pair<std::string,std::string>>{{"auto",tr("AUTOMAT","AUTO")},{"hip",tr("OD BOKU","HIP")},{"aimed",tr("ZACÍLENÍ","AIMED")}}){
    int x=220+column++*258;auto points=combat["curves"].value(mode.first,Json::array());combatCurve(points,combat.value("values",Json::object()).value("range",0.),mode.second,combat.value("tu",Json::object()).value(mode.first,0),x,320,250,200);
    small(tr("VZDÁLENOST · ÚČINNOST","DISTANCE · EFFECTIVENESS"),x+8,540,dim,10);int yy=565;for(const auto& point:points){small(point[0].dump()+" m · "+point[1].dump()+"%",x+8,yy,text,11);yy+=23;if(yy>660)break;}
   }}
  control(tr("ZPĚT DO UFOPEDIE","BACK TO UFOPEDIA"),220,695,772,35,[this,savedGroup=group,selected=index]{go("catalog");group=savedGroup;index=selected;});
 }
 void renderScreen(){
  bool alone=page=="menu"||page=="load"||page=="new-game"||page=="options"||page=="authors"||page=="script"||page=="exit"||page=="map";
  if(!alone&&page!="editor"){header();if(page!="bases")sidebar();}
  if(page=="menu")menu();else if(page=="map")mapEditor();else if(alone)utilityMenu();else if(page=="globe")geoscape();else if(page=="bases"||page=="base-info")bases();else if(page=="production")manufactureScreen();else if(page=="workshop"||page=="laboratory")facilityQueue();else if(page=="queues")workQueues();else if(page=="production-staff")productionStaffScreen();else if(page=="research")researchScreen();else if(page=="catalog")catalogue();else if(page=="personnel"||page=="soldiers")personnel();else if(page=="soldier-profile"||page=="soldier-equipment")soldierView();else if(page=="aircraft"||page=="ufos")aircraft();else if(page=="amphibious")amphibiousScreen();else if(page=="finance")finance();else if(page=="graphs"){}else if(page=="astronauts")astronauts();else if(page=="locations")locations();else if(page=="space-location")spaceLocation();else if(page=="space-visual")spaceVisualScreen();else if(page=="launch-ramp")launchRamp();else if(page=="aircraft-modules")aircraftModules();else if(page=="base-fees")baseFees();else if(page=="combat-data")combatData();else if(page=="staff-profile")staffProfile();else if(page=="editor")editor();else if(page=="storage")storage();else if(page=="freight")freightScreen();else if(page=="wiki")wiki();else unavailable();
  if(!equipmentSlot.empty())equipmentPicker();
  if(!crewRole.empty())aircraftCrewPicker();
 }
