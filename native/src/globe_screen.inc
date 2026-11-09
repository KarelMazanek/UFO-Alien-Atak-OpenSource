 void zoomGlobe(double steps){globeZoom=std::clamp(globeZoom*std::pow(1.5,steps),.65,GlobeView::maxZoom);}
 GlobeView globeView()const{return {rotation,tilt,globeZoom};}
 bool project(double lon,double lat,int& x,int& y)const{double xx,yy;bool visible=globeView().project(lon,lat,xx,yy);x=int(std::round(xx));y=int(std::round(yy));return visible;}
 bool unproject(double x,double y,double& lon,double& lat)const{return globeView().unproject(x,y,lon,lat);}
 void loadGlobeAssets(){
  if(!globeCountries.is_null())return;
  globeCountries=readJson(game.dataDir/"globe/countries.json");globeCapitals=readJson(game.dataDir/"globe/capitals.json");
  for(const auto& f:globeCountries["features"]){const auto& g=f["geometry"];auto polygon=[this](const Json& polygon){for(const auto& ring:polygon){std::vector<GlobePoint> points;for(const auto& p:ring)points.push_back(GlobeView::unit(p[0],p[1]));globeRings.push_back(std::move(points));}};if(g["type"]=="Polygon")polygon(g["coordinates"]);else if(g["type"]=="MultiPolygon")for(const auto& p:g["coordinates"])polygon(p);}
  for(int lat=-90;lat<=90;lat+=2)for(int lon=-180;lon<=180;lon+=2)globeMesh.push_back(GlobeView::unit(lon,lat));
  int w,h,n,sourceW,sourceH;auto original=game.dataDir/"globe/surface.png";
  if(!stbi_info(original.string().c_str(),&sourceW,&sourceH,&n))throw std::runtime_error("Cannot inspect globe surface");
  // The renderer already caps the texture at 8192 pixels. Decode its identical
  // prepared sample to avoid a full-resolution allocation in 32-bit processes.
  auto prepared=game.dataDir/"globe/surface-render.png";
  auto path=fs::is_regular_file(prepared)?prepared:original;
  unsigned char* pixels=stbi_load(path.string().c_str(),&w,&h,&n,3);if(!pixels)throw std::runtime_error("Cannot decode globe surface");
  if(path==prepared&&(w!=std::min(sourceW,8192)||h!=std::max(1,int(double(sourceH)*w/sourceW)))){stbi_image_free(pixels);throw std::runtime_error("Invalid prepared globe surface dimensions");}
  // Keep the approved full-resolution file; bound the GPU copy for older cards and RAM use.
  SDL_RendererInfo info{};SDL_GetRendererInfo(renderer,&info);int limit=8192;
  if(info.max_texture_width>0)limit=std::min(limit,info.max_texture_width);
  if(info.max_texture_height>0)limit=std::min(limit,info.max_texture_height*2);
  int tw=std::min(w,limit),th=std::max(1,int(double(h)*tw/w));
  std::vector<unsigned char> rgb(std::size_t(tw)*th*3);
  for(int y=0;y<th;++y)for(int x=0;x<tw;++x){auto src=(std::size_t(y)*h/th*w+std::size_t(x)*w/tw)*3;auto dst=(std::size_t(y)*tw+x)*3;for(int c=0;c<3;++c)rgb[dst+c]=pixels[src+c];}
  stbi_image_free(pixels);
  globeSurface=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STATIC,tw,th);if(!globeSurface)throw std::runtime_error(SDL_GetError());
  if(SDL_UpdateTexture(globeSurface,nullptr,rgb.data(),tw*3)!=0)throw std::runtime_error(SDL_GetError());SDL_SetTextureScaleMode(globeSurface,SDL_ScaleModeLinear);
  std::cerr<<"Globe relief "<<sourceW<<"x"<<sourceH<<"; render texture "<<tw<<"x"<<th<<std::endl;
 }
 void drawGlobeSurface(){
  loadGlobeAssets();
  if(!globeCache){globeCache=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,1270,1432);if(!globeCache)throw std::runtime_error(SDL_GetError());SDL_SetTextureScaleMode(globeCache,SDL_ScaleModeLinear);}
  if(cachedRotation!=rotation||cachedTilt!=tilt||cachedZoom!=globeZoom){
   SDL_Texture* oldTarget=SDL_GetRenderTarget(renderer);SDL_Rect oldClip;bool clipped=SDL_RenderIsClipEnabled(renderer);SDL_RenderGetClipRect(renderer,&oldClip);int lw,lh;SDL_RenderGetLogicalSize(renderer,&lw,&lh);
   if(SDL_SetRenderTarget(renderer,globeCache)!=0)throw std::runtime_error(SDL_GetError());SDL_RenderSetLogicalSize(renderer,0,0);SDL_RenderSetViewport(renderer,nullptr);SDL_RenderSetScale(renderer,1,1);SDL_RenderSetClipRect(renderer,nullptr);color({2,4,2});SDL_RenderClear(renderer);

   constexpr double radians=3.14159265358979323846/180;double a=rotation*radians,t=tilt*radians,ca=std::cos(a),sa=std::sin(a),ct=std::cos(t),st=std::sin(t),r=267*globeZoom;
   auto rotate=[=](GlobePoint p){double x=p.x*ca+p.z*sa,z=p.z*ca-p.x*sa;return GlobePoint{x,p.y*ct-z*st,p.y*st+z*ct,p.u,p.v};};
   auto screen=[=](GlobePoint p){return SDL_FPoint{float((501-184+p.x*r)*2),float((410-52-p.y*r)*2)};};
   auto horizon=[](GlobePoint a,GlobePoint b){double t=a.z/(a.z-b.z);GlobePoint p{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,0,a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t};double length=std::hypot(p.x,p.y);if(length>0){p.x/=length;p.y/=length;}return p;};
   std::vector<GlobePoint> mesh;mesh.reserve(globeMesh.size());for(auto p:globeMesh)mesh.push_back(rotate(p));
   std::vector<SDL_Vertex> triangles;triangles.reserve(60000);
   auto vertex=[&](GlobePoint p){return SDL_Vertex{screen(p),{255,255,255,255},{float(p.u),float(p.v)}};};
   auto triangle=[&](GlobePoint a,GlobePoint b,GlobePoint c){
    std::array<GlobePoint,3> input{a,b,c};std::array<GlobePoint,5> output;int count=0;GlobePoint previous=c;
    for(auto current:input){if((previous.z>=0)!=(current.z>=0))output[count++]=horizon(previous,current);if(current.z>=0)output[count++]=current;previous=current;}
    for(int i=1;i+1<count;++i){triangles.push_back(vertex(output[0]));triangles.push_back(vertex(output[i]));triangles.push_back(vertex(output[i+1]));}
   };
   for(int y=0;y<90;++y)for(int x=0;x<180;++x){int i=y*181+x;triangle(mesh[i],mesh[i+1],mesh[i+181]);triangle(mesh[i+1],mesh[i+182],mesh[i+181]);}
   if(SDL_RenderGeometry(renderer,globeSurface,triangles.data(),int(triangles.size()),nullptr,0)!=0)throw std::runtime_error(SDL_GetError());
   // One physical pixel at 2x gives a half-logical-pixel boundary, like HTML's .45px stroke.
   color({76,117,76});std::vector<SDL_FPoint> strip;
   auto flush=[&]{if(strip.size()>1)SDL_RenderDrawLinesF(renderer,strip.data(),int(strip.size()));strip.clear();};
   for(const auto& ring:globeRings){if(ring.empty())continue;auto previous=rotate(ring[0]);for(std::size_t i=1;i<ring.size();++i){auto current=rotate(ring[i]);if(previous.z>=0&&current.z>=0){if(strip.empty())strip.push_back(screen(previous));strip.push_back(screen(current));}else if(previous.z>=0){if(strip.empty())strip.push_back(screen(previous));strip.push_back(screen(horizon(previous,current)));flush();}else if(current.z>=0){strip.push_back(screen(horizon(previous,current)));strip.push_back(screen(current));}previous=current;}flush();}
   color({82,134,82});std::vector<SDL_FPoint> rim;for(int i=0;i<=1440;++i){double a=i*2*3.14159265358979323846/1440;rim.push_back({float((317+std::cos(a)*r)*2),float((358+std::sin(a)*r)*2)});}SDL_RenderDrawLinesF(renderer,rim.data(),int(rim.size()));
   SDL_SetRenderTarget(renderer,oldTarget);SDL_RenderSetLogicalSize(renderer,lw,lh);SDL_RenderSetClipRect(renderer,clipped?&oldClip:nullptr);
   if(cachedRotation==1e99)std::cerr<<"Globe surface ready"<<std::endl;cachedRotation=rotation;cachedTilt=tilt;cachedZoom=globeZoom;
  }
  SDL_Rect destination{184,52,635,716};SDL_RenderCopy(renderer,globeCache,nullptr,&destination);
 }
 std::string capitalRole(const std::string& role)const{if(role=="temporary")return tr("dočasné sídlo vlády","temporary government seat");if(role=="government")return tr("sídlo vlády","seat of government");if(role=="administrative")return tr("správní centrum","administrative center");if(role=="executive")return tr("výkonné hlavní město","executive capital");if(role=="legislative")return tr("zákonodárné hlavní město","legislative capital");if(role=="judicial")return tr("soudní hlavní město","judicial capital");if(role=="de_facto")return tr("správní centrum de facto","de facto administrative center");if(role=="claimed")return tr("nárokované hlavní město","claimed capital");return tr("hlavní město","capital");}
 void drawCapitalCities(){
  SDL_Rect area{184,52,635,650};SDL_RenderSetClipRect(renderer,&area);std::vector<SDL_Rect> labels;for(const auto& b:game.state["bases"]){int x,y;if(project(b["lon"],b["lat"],x,y))labels.push_back({x+9,y-8,textWidth(b["name"],10),15});}const Json* hoveredCountry=nullptr;const Json* hoveredCity=nullptr;double nearest=18;
  if(globeZoom>=2.5)for(const auto& country:globeCapitals["countries"])for(const auto& city:country["cities"]){int x,y;if(!project(city["lon"],city["lat"],x,y)||x<188||x>814||y<56||y>697)continue;rect(x-2,y-2,5,5,bright);double dist=std::hypot(double(mx-x),double(my-y));if(dist<nearest){nearest=dist;hoveredCountry=&country;hoveredCity=&city;}if(globeZoom<1.35)continue;auto name=city[english?"name":"name_cs"].get<std::string>();int width=std::min(190,textWidth(name,10)+4);SDL_Rect r{std::clamp(x+7,190,812-width),std::clamp(y-14,58,680),width,16};bool collision=false;for(const auto& old:labels)if(SDL_HasIntersection(&old,&r)){collision=true;break;}if(!collision){labels.push_back(r);boundedText(name,r,10,bright);}}
  SDL_RenderSetClipRect(renderer,nullptr);
  if(hoveredCity){std::string info=(*hoveredCity)[english?"name":"name_cs"].get<std::string>()+" · "+(*hoveredCountry)[english?"country_en":"country_cs"].get<std::string>()+" · "+capitalRole((*hoveredCity)["role"]);auto note=(*hoveredCountry).value(english?"note_en":"note_cs",std::string(""));if(!note.empty())info+=" · "+note;int boxHeight=note.empty()?36:100;rect(195,100,610,boxHeight,panel);boundedText(info,{204,106,590,boxHeight-12},12,bright);}
  auto meters=globeView().scaleMeters();std::string measure=meters<1000?std::to_string(int(std::round(meters)))+" m":std::to_string(int(std::round(meters/1000)))+" km";line(205,679,365,679,bright);line(205,675,205,683,bright);line(365,675,365,683,bright);small(measure,205,657,bright,11);if(globeZoom>2)small(tr("Zvětšený globální podklad","Enlarged global basemap"),385,657,dim,10);
  button("−",704,63,42,30,[this]{zoomGlobe(-1);});hits.back().name="globe-zoom-out";button("+",751,63,42,30,[this]{zoomGlobe(1);});hits.back().name="globe-zoom-in";
  if(mx>=184&&mx<819&&my>=160&&my<650){double lon,lat;if(unproject(mx,my,lon,lat)){auto countryName=globeCountryAt(globeCountries,lon,lat);for(const auto& country:globeCapitals["countries"])if(country["country"]==countryName){std::string info=country[english?"country_en":"country_cs"].get<std::string>();for(const auto& city:country["cities"])info+=" · "+city[english?"name":"name_cs"].get<std::string>()+" ("+capitalRole(city["role"])+")";if(country["cities"].empty())info+=tr(" · bez hlavního města"," · no capital");boundedText(info,{205,724,594,42},10,text);break;}}}
 }
 void verifyCapitalCitiesUI(){loadGlobeAssets();if(globeCapitals["countries"].size()!=globeCountries["features"].size())throw std::runtime_error("Capital coverage lost");std::cout<<"Original strategic globe and capital coverage passed\n";}
 // Periodic multi-scale noise gives clouds soft, torn edges without image downloads.
 double cloudNoise(double x,double y,int period)const{
  auto hash=[period](int ix,int iy){ix=(ix%period+period)%period;unsigned n=unsigned(ix)*374761393u+unsigned(iy)*668265263u;n=(n^(n>>13))*1274126177u;return double(n^(n>>16))/4294967295.;};
  int ix=int(std::floor(x)),iy=int(std::floor(y));double fx=x-ix,fy=y-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);double a=hash(ix,iy)*(1-fx)+hash(ix+1,iy)*fx,b=hash(ix,iy+1)*(1-fx)+hash(ix+1,iy+1)*fx;return a*(1-fy)+b*fy;
 }
 void updateGlobeClouds(){
  double hour=game.state["hours"],time=std::floor(hour*2)/2.;Uint32 now=SDL_GetTicks();
  if(globeClouds&&(time==cloudHour||(now-cloudUpdated<250&&hour>=cloudHour)))return;
  constexpr int width=1440,height=720;constexpr double pi=3.14159265358979323846,rad=pi/180.;
  struct System{double lon,lat,radius,phase,strength,coslat,spin;};std::vector<System> systems;
  for(const auto& s:game.weatherSystems(time))systems.push_back({s["lon"],s["lat"],s["radius"],s["phase"],s["strength"],std::cos(s["lat"].get<double>()*rad),s["spin"]});
  std::vector<float> density(width*height);
  for(int y=0;y<height;++y){double lat=90-(y+.5)*180/height;for(int x=0;x<width;++x){double lon=-180+(x+.5)*360/width,nx=(lon+180-time*.13)/360*32,ny=(lat+90)/180*20;
   double coarse=cloudNoise(nx,ny,32),medium=cloudNoise(nx*3+13,ny*3+9,96),fine=cloudNoise(nx*9-7,ny*9+3,288),grain=cloudNoise(nx*24+21,ny*24,768);
   double turbulence=(coarse-.5)*.44+(medium-.5)*.34+(fine-.5)*.18+(grain-.5)*.08;
   double field=0.;for(int sign:{-1,1}){double front=sign*(42+8*std::sin(lon*rad*2+time*.025)+4*std::sin(lon*rad*5-time*.012));double distance=(lat-front+(coarse-.5)*8)/5.8;field=std::max(field,.64*std::exp(-distance*distance));}
   // Broad asymmetric cyclones: cloudy cores, two broken feeder arms and a dry slot.
   for(const auto& s:systems){double dx=std::remainder(lon-s.lon,360.)*s.coslat/s.radius,dy=(lat-s.lat)/s.radius,rr=dx*dx+dy*dy;if(rr>5)continue;double radius=std::sqrt(rr),angle=std::atan2(dy,dx),swirl=angle-s.spin*(3.9/(radius+.55))+s.phase*.25;
    double arm=std::pow(std::max(0.,.5+.5*std::cos(swirl*2+(coarse-.5)*1.8)),1.8);double core=std::exp(-rr*7)*.35;double asymmetric=.82+.18*std::sin(angle+s.phase);double storm=s.strength*std::exp(-rr*.85)*(.30+.66*arm)*asymmetric+core;field=std::max(field,storm);
   }
   // Small scattered clouds outside fronts, with large clear areas between systems.
   double scattered=std::max(0.,coarse-.59)*1.6*std::max(0.,medium-.28);field=std::max(field,scattered);
   density[y*width+x]=float(std::clamp((field+turbulence-.20)*1.65,0.,1.));
  }}
  std::vector<unsigned char> rgba(std::size_t(width)*height*4);
  for(int y=0;y<height;++y)for(int x=0;x<width;++x){int i=y*width+x;double d=density[i],gradient=density[std::max(0,y-1)*width+(x+width-1)%width]-d;int light=std::clamp(int(151+d*43+gradient*85),115,207);rgba[i*4]=Uint8(light-3);rgba[i*4+1]=Uint8(light);rgba[i*4+2]=Uint8(light-12);rgba[i*4+3]=Uint8(std::pow(d,1.2)*190);}
  // SDL's software triangle renderer can ignore linear sampling. Pre-filter the cloud
  // texture so magnified fronts remain soft on both software and hardware renderers.
  SDL_RendererInfo caps{};SDL_GetRendererInfo(renderer,&caps);int magnify=3;if(caps.max_texture_width>0)magnify=std::min(magnify,std::max(1,caps.max_texture_width/width));if(caps.max_texture_height>0)magnify=std::min(magnify,std::max(1,caps.max_texture_height/height));int textureWidth=width*magnify,textureHeight=height*magnify;std::vector<unsigned char> smooth(std::size_t(textureWidth)*textureHeight*4);
  for(int y=0;y<textureHeight;++y){double sy=(y+.5)/magnify-.5;int y0=int(std::floor(sy));double fy=sy-y0;int ya=std::clamp(y0,0,height-1),yb=std::clamp(y0+1,0,height-1);for(int x=0;x<textureWidth;++x){double sx=(x+.5)/magnify-.5;int x0=int(std::floor(sx));double fx=sx-x0;int xa=(x0+width)%width,xb=(xa+1)%width;for(int c=0;c<4;++c){double a=rgba[(ya*width+xa)*4+c]*(1-fx)+rgba[(ya*width+xb)*4+c]*fx,b=rgba[(yb*width+xa)*4+c]*(1-fx)+rgba[(yb*width+xb)*4+c]*fx;smooth[(std::size_t(y)*textureWidth+x)*4+c]=Uint8(a*(1-fy)+b*fy);}}}
  if(!globeClouds){globeClouds=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,textureWidth,textureHeight);if(!globeClouds)throw std::runtime_error(SDL_GetError());SDL_SetTextureBlendMode(globeClouds,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(globeClouds,SDL_ScaleModeLinear);}
  if(SDL_UpdateTexture(globeClouds,nullptr,smooth.data(),textureWidth*4)!=0)throw std::runtime_error(SDL_GetError());cloudHour=time;cloudUpdated=SDL_GetTicks();
 }
 void drawGlobeWeather(){
  if(!globeWeather)return;updateGlobeClouds();
  SDL_Rect clip{184,94,635,674};SDL_RenderSetClipRect(renderer,&clip);
  auto view=globeView();std::vector<GlobePoint> mesh;mesh.reserve(globeMesh.size());for(auto p:globeMesh)mesh.push_back(view.rotate(p));std::vector<SDL_Vertex> triangles;triangles.reserve(100000);
  auto horizon=[](GlobePoint a,GlobePoint b){double t=a.z/(a.z-b.z);GlobePoint p{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,0,a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t};double length=std::hypot(p.x,p.y);if(length>0){p.x/=length;p.y/=length;}return p;};
  auto vertex=[&](GlobePoint p){double rim=std::clamp(p.z*7.,0.,1.);return SDL_Vertex{{float(GlobeView::cx+p.x*GlobeView::radius*globeZoom),float(GlobeView::cy-p.y*GlobeView::radius*globeZoom)},{255,255,255,Uint8(255*rim)},{float(p.u),float(p.v)}};};
  auto triangle=[&](GlobePoint a,GlobePoint b,GlobePoint c){std::array<GlobePoint,3> input{a,b,c};std::array<GlobePoint,5> output;int count=0;auto previous=c;for(auto current:input){if((previous.z>=0)!=(current.z>=0))output[count++]=horizon(previous,current);if(current.z>=0)output[count++]=current;previous=current;}for(int i=1;i+1<count;++i){triangles.push_back(vertex(output[0]));triangles.push_back(vertex(output[i]));triangles.push_back(vertex(output[i+1]));}};
  for(int y=0;y<90;++y)for(int x=0;x<180;++x){int i=y*181+x;triangle(mesh[i],mesh[i+1],mesh[i+181]);triangle(mesh[i+1],mesh[i+182],mesh[i+181]);}
  if(SDL_RenderGeometry(renderer,globeClouds,triangles.data(),int(triangles.size()),nullptr,0)!=0)throw std::runtime_error(SDL_GetError());SDL_RenderSetClipRect(renderer,nullptr);
 }
 void globeWeatherControl(){
  control(globeWeather?tr("POČASÍ: ZAPNUTO","WEATHER: ON"):tr("POČASÍ: VYPNUTO","WEATHER: OFF"),205,63,224,30,[this]{globeWeather=!globeWeather;});hits.back().name="globe-weather-toggle";
  if(globeWeather){small(tr("HERNÍ POČASÍ · OBLAČNOST A VÍRY","GAME WEATHER · CLOUDS & VORTICES"),205,688,dim,10);
   double lon,lat;if(mx>=184&&mx<819&&my>=104&&my<635&&unproject(mx,my,lon,lat)){auto w=game.weatherAt(lon,lat,game.state["hours"]);rect(205,612,588,32,panel);small(tr("OBLAČNOST ","CLOUD COVER ")+std::to_string(w["cloud"].get<int>())+tr(" % · VÍTR "," % · WIND ")+std::to_string(int(w["wind"].get<double>()))+tr(" km/h · RIZIKO STARTU "," km/h · LAUNCH RISK ")+std::to_string(w["risk"].get<int>())+" %",214,622,bright,10);}
  }
 }
 void globe(){
  drawGlobeSurface();drawGlobeWeather();drawCapitalCities();globeWeatherControl();SDL_Rect mapClip{184,94,635,674};SDL_RenderSetClipRect(renderer,&mapClip);
  std::vector<SDL_Rect> contactLabels;auto contactLabel=[&](const std::string& text,int x,int y,Color shade){SDL_Rect chosen{std::clamp(x+9,190,619),std::clamp(y-12,60,672),194,30};for(int offset:{-12,20,-44,52,-76,84}){SDL_Rect candidate{chosen.x,std::clamp(y+offset,60,672),194,30};bool overlaps=false;for(const auto& used:contactLabels)if(SDL_HasIntersection(&candidate,&used))overlaps=true;if(!overlaps){chosen=candidate;break;}}contactLabels.push_back(chosen);boundedText(text,chosen,10,shade);};
  for(const auto& b:game.state["bases"]){int x,y;if(project(b["lon"],b["lat"],x,y)){rect(x-4,y-4,9,9,accent);contactLabels.push_back({x+9,y-8,textWidth(b["name"],10),15});label(b["name"],x+9,y-8,text,.4f);}}
  if(game.state.contains("tradeFlights"))for(std::size_t i=0;i<game.state["tradeFlights"].size();++i){auto f=game.freightStatus(i);if(f["delivered"].get<bool>()||f["status"]=="destroyed")continue;int x,y;if(project(f["lon"],f["lat"],x,y)){line(x-7,y,x+7,y,accent);line(x,y-7,x,y+7,bright);small(tr("NÁKLAD #","CARGO #")+std::to_string(i+1),x+10,y-12,bright,10);}}
  if(game.state.contains("radarOutposts"))for(const auto& site:game.state["radarOutposts"]){int x,y;if(!project(site["lon"],site["lat"],x,y)||x<190||x>812||y<100||y>697)continue;bool ready=site["ready"].get<double>()<=game.state["hours"].get<double>();Color shade=ready?bright:dim;line(x,y-6,x-6,y+5,shade);line(x-6,y+5,x+6,y+5,shade);line(x+6,y+5,x,y-6,shade);small(site["name"].get<std::string>()+(ready?tr(" · V PROVOZU"," · ONLINE"):tr(" · STAVBA"," · BUILDING")),x+9,y-8,shade,10);}
  for(std::size_t i=0;i<game.state["ufos"].size();++i){auto u=game.ufoContact(i);if(u.is_null())continue;int x,y;if(project(u["lon"],u["lat"],x,y)){if(u.value("kind",std::string(""))=="LANDED"){color(bright);SDL_Rect marker{x-6,y-6,13,13};SDL_RenderDrawRect(renderer,&marker);}else{line(x-6,y,x+6,y,bright);line(x,y-6,x,y+6,bright);}contactLabel(ufoContactLabel(u)+(u.value("source",std::string(""))=="RADAR"?" · RADAR":" · SAT"),x,y,bright);if(x>=194&&x<=809&&y>=104&&y<=758)hits.push_back({{x-10,y-10,20,20},[this,i]{beginInterceptionSelection(i);},"globe-intercept:"+std::to_string(i)});}}
  small(tr("+ VZDUCH · □ PŘISTÁLÉ · × TROSKY","+ AIRBORNE · □ LANDED · × WRECK"),205,708,dim,10);
  auto contacts=game.satelliteSurvey();for(const auto& pair:contacts["contacts"].items()){const auto& c=pair.value();if(c.value("kind",std::string(""))!="CRASH"||game.state["hours"].get<double>()-c["at"].get<double>()>72)continue;int x,y;if(project(c["lon"],c["lat"],x,y)){line(x-5,y-5,x+5,y+5,dim);line(x+5,y-5,x-5,y+5,dim);contactLabel(ufoContactLabel(c),x,y,dim);}}
  SDL_RenderSetClipRect(renderer,nullptr);globePlacementControls();
 }
 void chooseBaseLocation(){
  if(!placeBase&&!placeRadar)return;double lon,lat;if(!unproject(mx,my,lon,lat)){notice=tr("Klikni na povrch glóbu.","Click on the globe surface.");placementFeedback=notice;return;}loadGlobeAssets();auto country=globeCountryAt(globeCountries,lon,lat);
  if(country.empty()){notice=tr("Objekt lze umístit pouze na pevninu.","The facility can only be placed on land.");placementFeedback=notice;return;}
  if(placeRadar){auto built=game.createRadarOutpost(lat,lon);result(built);if(built.ok){placeRadar=false;placementFeedback=tr("Radarové stanoviště se staví · 10 dnů · dosah 2 500 km.","Radar station under construction · 10 days · range 2,500 km.");}else if(built.message=="A radar already exists here")placementFeedback=tr("Zde již radar stojí. Vyber vzdálenější místo.","A radar already exists here. Choose a more distant site.");else placementFeedback=localized(built.message);return;}
  for(const auto& b:game.state["bases"])if(std::hypot(lat-b["lat"].get<double>(),std::remainder(lon-b["lon"].get<double>(),360.)*std::cos(lat*3.14159265358979323846/180.))<3){placementFeedback=tr("Zde již báze stojí. Vyber vzdálenější místo.","A base already exists here. Choose a more distant site.");return;}
  std::cerr<<"Base dialog lon="<<lon<<" lat="<<lat<<" country="<<country<<std::endl;
  input("BASE "+std::to_string(game.state["bases"].size()+1),false,[this,lat,lon,country](const std::string& value){auto r=game.createBase(value,lat,lon);result(r);placementFeedback=localized(r.message);if(r.ok){std::cerr<<"Base created lon="<<lon<<" lat="<<lat<<" country="<<country<<std::endl;game.state["bases"].back()["country"]=country;placeBase=false;base=int(game.state["bases"].size())-1;go("bases");}});
 }
 bool handleGlobePointer(const SDL_Event& ev){
  if(page!="globe"||editing)return false;
  // SDL_RenderSetLogicalSize already transforms queued SDL mouse coordinates.
  if(ev.type==SDL_MOUSEMOTION){mx=float(ev.motion.x);my=float(ev.motion.y);if(globeGesture.pressed){auto view=globeView();globeGesture.motion(mx,my,view);rotation=view.rotation;tilt=view.tilt;return true;}return false;}
  if(ev.type!=SDL_MOUSEBUTTONDOWN&&ev.type!=SDL_MOUSEBUTTONUP)return false;
  if(ev.button.button!=SDL_BUTTON_LEFT)return false;mx=float(ev.button.x);my=float(ev.button.y);
  if(ev.type==SDL_MOUSEBUTTONDOWN&&my>=63&&my<93&&((mx>=704&&mx<793)||(mx>=205&&mx<429)))return false;
  if(ev.type==SDL_MOUSEBUTTONDOWN)for(const auto& hit:hits)if(hit.name.rfind("globe-intercept:",0)==0&&mx>=hit.r.x&&mx<hit.r.x+hit.r.w&&my>=hit.r.y&&my<hit.r.y+hit.r.h)return false;
  if(ev.type==SDL_MOUSEBUTTONDOWN)for(const auto& hit:hits)if(hit.name.rfind("placement-",0)==0&&mx>=hit.r.x&&mx<hit.r.x+hit.r.w&&my>=hit.r.y&&my<hit.r.y+hit.r.h)return false;
  bool inCanvas=mx>=184&&mx<819&&my>=52&&my<768;
  if(ev.type==SDL_MOUSEBUTTONDOWN&&inCanvas){if(SDL_getenv("UAA_TRACE_INPUT"))std::cerr<<"Globe down x="<<mx<<" y="<<my<<" rotation="<<rotation<<" tilt="<<tilt<<std::endl;globeGesture.down(mx,my);SDL_CaptureMouse(SDL_TRUE);return true;}
  if(ev.type==SDL_MOUSEBUTTONUP&&globeGesture.pressed){bool click=globeGesture.up();if(SDL_getenv("UAA_TRACE_INPUT"))std::cerr<<"Globe up click="<<click<<" rotation="<<rotation<<" tilt="<<tilt<<std::endl;SDL_CaptureMouse(SDL_FALSE);if(click&&inCanvas)chooseBaseLocation();return true;}return false;
 }
