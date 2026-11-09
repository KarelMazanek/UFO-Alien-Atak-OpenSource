 struct BaseViewport {SDL_Rect area;int ox,oy,tw,th,flatX,flatY,cell;};
 BaseViewport baseViewport()const{return {{244,230,506,455},497,310,23,12,282,235,43};}
 // Both views share the engine footprint and placement coordinates.
 void drawBaseWorkspace(const Json& b,bool iso){
  if(baseSprites.is_null())baseSprites=readJson(game.dataDir/"base/sprites.json");
  auto viewport=baseViewport();auto area=viewport.area;SDL_RenderSetClipRect(renderer,&area);
  int ox=viewport.ox,oy=viewport.oy,tw=viewport.tw,th=viewport.th,flatX=viewport.flatX,flatY=viewport.flatY,cell=viewport.cell;
  auto point=[=](int x,int y){return std::pair{ox+(x-y)*tw,oy+(x+y)*th};};
  std::vector<Json> items;bool occupied[10][10]={};
  for(const auto& item:b["buildings"]){
   if(item["floor"]!=floor)continue;
   items.push_back(item);auto [w,h]=game.footprint(item["id"]);
   int bx=item["x"],by=item["y"];
   for(int y=by;y<by+h;++y)for(int x=bx;x<bx+w;++x)if(x>=0&&x<10&&y>=0&&y<10)occupied[y][x]=true;
  }
  for(int y=0;y<10;++y)for(int x=0;x<10;++x){
   if(iso){
    if(occupied[y][x])continue;
    auto [sx,sy]=point(x,y);line(sx,sy,sx+tw,sy+th);line(sx,sy,sx-tw,sy+th);line(sx+tw,sy+th,sx,sy+2*th);line(sx-tw,sy+th,sx,sy+2*th);
   }else{
    rect(flatX+x*cell,flatY+y*cell,cell-1,cell-1,(x+y)%2?Color{11,24,11}:Color{10,22,10});
    color(edge);SDL_Rect tile{flatX+x*cell,flatY+y*cell,cell,cell};SDL_RenderDrawRect(renderer,&tile);
   }
  }
  std::sort(items.begin(),items.end(),[](const Json& a,const Json& b){int ax=a["x"],ay=a["y"],bx=b["x"],by=b["y"];return ax+ay==bx+by?ax<bx:ax+ay<bx+by;});
  for(const auto& item:items){
   std::string id=item["id"];int x=item["x"],y=item["y"];auto [w,h]=game.footprint(id);auto record=game.record("buildings",id);
   if(iso){
    auto [sx,sy]=point(x,y);int width=(w+h)*tw;
    if(baseSprites.contains(id)){
     const auto& sprite=baseSprites[id];int height=int(std::round(double(width)*sprite["height"].get<int>()/sprite["width"].get<int>()));
     // Preserve original sprite aspect and anchor its bottom to the near footprint vertex.
     image(sprite["image"],sx-h*tw,sy+(w+h)*th-height,width,height);
    }else{
     auto [a,c]=point(x+w,y);auto [d,e]=point(x+w,y+h);auto [f,g]=point(x,y+h);
     line(sx,sy,a,c,accent);line(a,c,d,e,accent);line(d,e,f,g,accent);line(f,g,sx,sy,accent);
     small(record?nameOf(*record):id,sx-h*tw+3,sy+8,text,10);
    }
   }else{
    int px=flatX+x*cell,py=flatY+y*cell;
    rect(px+2,py+2,w*cell-4,h*cell-4,{16,35,16});color(accent);SDL_Rect footprint{px+2,py+2,w*cell-4,h*cell-4};SDL_RenderDrawRect(renderer,&footprint);
    wrap(id=="runway"?tr("Letiště – vzletová dráha 6 × 4","Runway 6 × 4"):(record?nameOf(*record):id),px+5,py+7,w*cell-10,10);
   }
  }
  auto cellFromPoint=[&](float px,float py){if(iso){double a=(px-ox)/tw,b=(py-oy)/th;return std::pair{int(std::floor((a+b)/2)),int(std::floor((b-a)/2))};}return std::pair{int(std::floor((px-flatX)/cell)),int(std::floor((py-flatY)/cell))};};
  if(mx>=area.x&&mx<area.x+area.w&&my>=area.y&&my<area.y+area.h){auto [cx,cy]=cellFromPoint(mx,my);if(cx>=0&&cx<10&&cy>=0&&cy<10){if(!demolishMode){auto [w,h]=game.footprint(building);auto allowed=game.canBuild(base,building,cx,cy,floor);Color preview=allowed.ok?bright:dim;if(iso){auto [a,b]=point(cx,cy);auto [c,d]=point(cx+w,cy);auto [e,f]=point(cx+w,cy+h);auto [g,k]=point(cx,cy+h);line(a,b,c,d,preview);line(c,d,e,f,preview);line(e,f,g,k,preview);line(g,k,a,b,preview);if(!allowed.ok){line(a,b,e,f,preview);line(c,d,g,k,preview);}}else{SDL_Rect shape{flatX+cx*cell,flatY+cy*cell,w*cell,h*cell};color(preview);SDL_RenderDrawRect(renderer,&shape);SDL_Rect inner{shape.x+1,shape.y+1,shape.w-2,shape.h-2};SDL_RenderDrawRect(renderer,&inner);if(!allowed.ok){line(shape.x,shape.y,shape.x+shape.w,shape.y+shape.h,preview);line(shape.x+shape.w,shape.y,shape.x,shape.y+shape.h,preview);}}baseFeedback=(allowed.ok?tr("LZE UMÍSTIT · ","FITS · "):tr("NELZE UMÍSTIT · ","DOES NOT FIT · "))+std::to_string(w)+" × "+std::to_string(h)+(allowed.ok?"":" · "+localized(allowed.message));}else baseFeedback=tr("Klikni na objekt k odstranění · vyžaduje potvrzení.","Click a building to remove · confirmation required.");}}
  SDL_RenderSetClipRect(renderer,nullptr);
  hits.push_back({area,[this,iso,viewport]{
   int x,y;if(iso){double a=(mx-viewport.ox)/viewport.tw,b=(my-viewport.oy)/viewport.th;x=int(std::floor((a+b)/2));y=int(std::floor((b-a)/2));}
   else{x=int(std::floor((mx-viewport.flatX)/viewport.cell));y=int(std::floor((my-viewport.flatY)/viewport.cell));}
   if(x>=0&&x<10&&y>=0&&y<10){if(demolishMode){const auto& objects=game.state["bases"][base]["buildings"];for(int i=int(objects.size())-1;i>=0;--i){const auto& item=objects[i];if(item["floor"]!=floor)continue;auto [w,h]=game.footprint(item["id"]);int bx=item["x"],by=item["y"];if(x>=bx&&x<bx+w&&y>=by&&y<by+h){requestDemolition(i);break;}}}else{auto outcome=game.build(base,building,x,y,floor);baseFeedback=outcome.message;}}
  },"base-workspace"});
 }
