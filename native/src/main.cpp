#define SDL_MAIN_HANDLED
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "engine.hpp"
#include "map_editor.hpp"
#include "globe.hpp"
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
using namespace uaa;
namespace {
struct Color{Uint8 r,g,b,a=255;};
constexpr Color bg{5,8,5},panel{8,16,8},edge{53,85,53},text{185,217,185},bright{210,232,210},dim{111,157,111},accent{116,169,116};
struct Hit {SDL_Rect r;std::function<void()> action;std::string name;};
class App {
 SDL_Window* window=nullptr;SDL_Renderer* renderer=nullptr;
 std::vector<Hit> hits;std::vector<SDL_Rect> layoutFrames;fs::path layoutReport;std::map<std::string,SDL_Texture*> textures;
 stbtt_fontinfo font{},monoFont{};std::vector<unsigned char> fontBytes,monoBytes;bool monospaced=false;
 struct Glyph {SDL_Texture* texture;int w,h,x,y,advance;};std::map<int,Glyph> glyphs;
 Engine game;
 bool baseMenuOpen=false,placeRadar=false;int baseMenuScroll=0;std::string placementFeedback;
 bool running=true,english=false,placeBase=false,editing=false,editNumber=false,showHelp=false;
 std::string page="menu",group="buildings",selection,building="storage",notice,editText,editKey;
 int catalogueDetailScroll=0;std::string catalogueDetailKey,catalogueListKey,mapEditorFeedback;
 Json custom;int index=0,scroll=0,base=0,floor=0,saveSlot=1;double rotation=20,tilt=15,speed=0,globeZoom=1;bool globeWeather=true;
 GlobeGesture globeGesture;Json globeCountries,globeCapitals;SDL_Texture* globeClouds=nullptr;double cloudHour=-1e99;Uint32 cloudUpdated=0;SDL_Texture* globeSurface=nullptr;SDL_Texture* globeCache=nullptr;double cachedRotation=1e99,cachedTilt=0,cachedZoom=0;std::vector<std::vector<GlobePoint>> globeRings;std::vector<GlobePoint> globeMesh;
 std::string role="soldier";int activeCraft=0;float mx=0,my=0;std::function<void(const std::string&)> commit;
 static constexpr int W=1024,H=768;
 bool baseIsometric=true,demolishMode=false,demolitionPending=false,demolitionVisible=false;int demolitionBase=-1,demolitionIndex=-1;double demolitionResumeSpeed=0;Json demolitionTarget;std::string baseFeedback;Json baseSprites;int wikiTopicScroll=0,wikiArticleScroll=0;
 bool countriesOpen=false,commandCollapsed=false;int countryTier=0,quantity=1,productionScroll=0,researchStaffScroll=0;std::string subMenu;
 std::string location="esa",scanType="EARTH",spaceRole="commander";int launchFuel=60,launchCount=1,cargoUnits=1;
 std::string spaceVisual="orbit",orbitFilter="ALL";struct SpaceCamera{double yaw,pitch,zoom,distance;};SpaceCamera issCamera{.68,.34,1,27},rocketCamera{.34,.36,1.5,27};
 struct ModelViewport{SDL_Rect rect;std::string type;};std::vector<ModelViewport> modelViewports;std::string draggedModel;float modelLastX=0,modelLastY=0,modelStartX=0,modelStartY=0;double modelStartYaw=0,modelStartPitch=0;bool modelDragging=false;std::map<SDL_FingerID,SDL_FPoint> modelTouches;double modelPinchStart=0,modelPinchDistance=27;
 SDL_Window* spaceWindow=nullptr;SDL_Renderer* spaceRenderer=nullptr;std::string popupModel;bool popupCloseRequested=false,popupISSOperations=false,popupISSLog=false,weatherOverlay=false;float popupMX=0,popupMY=0;
 std::vector<Hit> popupHits;std::vector<ModelViewport> popupViewports;std::vector<SDL_Rect> popupFrames;std::map<int,Glyph> popupGlyphs;std::map<std::string,SDL_Texture*> popupTextures;
 bool interceptionPicker=false;int interceptionScroll=0;std::string interceptionTarget;
 bool ufoAlertOpen=false,ufoAlertVisible=false,ufoAwaitingInterception=false;double ufoResumeSpeed=0;int ufoAlertScroll=0;Json ufoAlertContacts=Json::array();
 bool freightPartnerPicker=false;int freightPartnerScroll=0,freightScroll=0;std::string freightFeedback;
 bool operationWarning=false;int operationWarningScroll=0,operationSquadScroll=0;Json operationWarningReport=Json::array();std::string operationFeedback,robotFiringMode="aimed";
 std::string spaceFeedback;int satelliteScroll=0;
 double modelZoom=1;std::map<std::string,Json> spaceMeshes;
 int workQueueScroll=0,researchQueueScroll=0;
 double resumeProductionSpeed=0;bool productionCompleted=false,productionCompletionVisible=false;
 struct ListScrollRegion{SDL_Rect area;int* value;int maximum;};std::vector<ListScrollRegion> listScrollRegions;int* listScrollValue=nullptr;int listScrollMaximum=0,listScrollTop=0,listScrollTravel=0,listScrollThumb=0;
 void listScrollAt(int y){if(listScrollValue)*listScrollValue=std::clamp(int(std::round(double(y-listScrollTop-listScrollThumb/2)*listScrollMaximum/std::max(1,listScrollTravel))),0,listScrollMaximum);}
 void listSlider(int& value,int total,int visible,int x,int y,int w,int h,const std::string& id){int maximum=std::max(0,total-visible);value=std::clamp(value,0,maximum);int thumb=maximum?std::max(18,h*visible/std::max(1,total)):h;thumb=std::min(h,thumb);int travel=h-thumb,position=maximum?value*travel/maximum:0,barX=x+w+6;rect(barX,y,3,h,edge);rect(barX,y+position,3,thumb,maximum?accent:dim);listScrollRegions.push_back({{x,y,w,h},&value,maximum});if(maximum)hits.push_back({{barX-4,y,11,h},[this,target=&value,maximum,y,travel,thumb]{listScrollValue=target;listScrollMaximum=maximum;listScrollTop=y;listScrollTravel=travel;listScrollThumb=thumb;listScrollAt(int(my));SDL_CaptureMouse(SDL_TRUE);},"list-scroll:"+id});}
 bool listScrollPointer(const SDL_Event& event){if(event.type==SDL_MOUSEWHEEL){for(auto it=listScrollRegions.rbegin();it!=listScrollRegions.rend();++it)if(mx>=it->area.x&&mx<it->area.x+it->area.w+13&&my>=it->area.y&&my<it->area.y+it->area.h){*it->value=std::clamp(*it->value-event.wheel.y*3,0,it->maximum);return true;}}if(!listScrollValue)return false;if(event.type==SDL_MOUSEMOTION){listScrollAt(event.motion.y);return true;}if((event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT)||(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST)){if(event.type==SDL_MOUSEBUTTONUP)listScrollAt(event.button.y);listScrollValue=nullptr;SDL_CaptureMouse(SDL_FALSE);return true;}return false;}

 bool quantityDragging=false;int technicianScroll=0;std::string productionStaffGroup="weapons",productionStaffId="pistol";
 int soldierStripScroll=0,lastStripSoldier=-1;
 int selectedSoldier=-1,equipmentIndex=0,equipmentScroll=0;std::string equipmentSlot,crewRole,researchFeedback,aircraftSlot="system",gymFeedback,aircraftFeedback;
 SDL_AudioDeviceID musicDevice=0;drmp3 musicDecoder{};bool musicOpened=false,musicEnabled=true,musicActivated=false,musicFailed=false;int musicTrack=-1,paceIndex=1,difficultyIndex=1,windowResolution=0;double musicVolume=.35,soundVolume=1;bool fullscreenRequested=false;
 bool languageChosen=false;std::vector<std::pair<std::string,std::string>> czechPhrases,englishPhrases;
 void loadTranslations(){auto data=readJson(game.dataDir/"ui-translations.json");for(const auto& entry:data.items()){czechPhrases.push_back({entry.key(),entry.value().get<std::string>()});englishPhrases.push_back({entry.value().get<std::string>(),entry.key()});}auto longest=[](const auto& a,const auto& b){return a.first.size()>b.first.size();};std::stable_sort(czechPhrases.begin(),czechPhrases.end(),longest);std::stable_sort(englishPhrases.begin(),englishPhrases.end(),longest);}
 std::string localized(const std::string& text)const{const auto& phrases=english?englishPhrases:czechPhrases;std::string out;auto word=[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c>=128;};for(std::size_t i=0;i<text.size();){bool found=false;for(const auto& phrase:phrases){const auto& key=phrase.first;if(text.compare(i,key.size(),key)==0&&(i==0||!word(text[i-1])||!word(key.front()))&&(i+key.size()==text.size()||!word(text[i+key.size()])||!word(key.back()))){out+=phrase.second;i+=key.size();found=true;break;}}if(!found)out+=text[i++];}return out;}
 std::string tr(const std::string& cz,const std::string& en)const{return english?en:cz;}
 void color(Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);}
 void rect(int x,int y,int w,int h,Color c){color(c);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
 void line(int x,int y,int xx,int yy,Color c=edge){color(c);SDL_RenderDrawLine(renderer,x,y,xx,yy);}
 static std::vector<int> unicode(const std::string& s){std::vector<int> out;for(std::size_t i=0;i<s.size();){unsigned char c=s[i++];int n=c,extra=0;if(c>=0xf0){n=c&7;extra=3;}else if(c>=0xe0){n=c&15;extra=2;}else if(c>=0xc0){n=c&31;extra=1;}for(int j=0;j<extra&&i<s.size();j++)n=(n<<6)|(s[i++]&63);out.push_back(n);}return out;}
 Glyph& glyph(int cp,int pixels){int key=(monospaced?100000000:0)+pixels*0x110000+cp;const auto* selectedFont=monospaced?&monoFont:&font;auto found=glyphs.find(key);if(found!=glyphs.end())return found->second;float scale=stbtt_ScaleForMappingEmToPixels(selectedFont,float(pixels));int w,h,x,y,advance,bearing;unsigned char* bitmap=stbtt_GetCodepointBitmap(selectedFont,0,scale,cp,&w,&h,&x,&y);stbtt_GetCodepointHMetrics(selectedFont,cp,&advance,&bearing);SDL_Texture* texture=nullptr;if(w&&h){std::vector<Uint32> rgba(std::size_t(w)*h);for(std::size_t i=0;i<rgba.size();++i)rgba[i]=0xffffff00u|bitmap[i];texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,w,h);SDL_UpdateTexture(texture,nullptr,rgba.data(),w*4);SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);}stbtt_FreeBitmap(bitmap,nullptr);return glyphs.emplace(key,Glyph{texture,w,h,x,y,int(std::round(advance*scale))}).first->second;}
 int textWidth(const std::string& input,int pixels){auto s=localized(input);pixels=std::max(10,pixels);int width=0;for(int cp:unicode(s))width+=glyph(cp,pixels).advance;return width;}
 void label(const std::string& input,int x,int y,Color c=text,float scale=1,bool literal=false){auto s=literal?input:localized(input);int pixels=std::max(10,int(std::round(20*scale))),ascent,descent,gap;const auto* selectedFont=monospaced?&monoFont:&font;stbtt_GetFontVMetrics(selectedFont,&ascent,&descent,&gap);int baseline=int(std::round(ascent*stbtt_ScaleForMappingEmToPixels(selectedFont,float(pixels))));int cursor=x;for(int cp:unicode(s)){if(cp=='\n'){cursor=x;y+=pixels+4;continue;}auto& g=glyph(cp,pixels);if(g.texture){SDL_SetTextureColorMod(g.texture,c.r,c.g,c.b);SDL_Rect dst{cursor+g.x,y+baseline+g.y,g.w,g.h};SDL_RenderCopy(renderer,g.texture,nullptr,&dst);}cursor+=g.advance;}}
 void paragraph(const std::string& s,int x,int y,int width,Color c=text){std::string current;std::istringstream words(s);std::string word;int max=std::max(10,width/10);while(words>>word){if(int(unicode(current+word).size())>max){label(current,x,y,c);y+=25;current.clear();}current+=word+" ";}label(current,x,y,c);}
 void box(int x,int y,int w,int h){layoutFrames.push_back({x,y,w,h});rect(x,y,w,h,panel);color(edge);SDL_Rect r{x,y,w,h};SDL_RenderDrawRect(renderer,&r);}
 void button(const std::string& s,int x,int y,int w,int h,std::function<void()> f,bool active=false){bool over=mx>=x&&my>=y&&mx<x+w&&my<y+h;rect(x,y,w,h,active?Color{41,54,34}:over?Color{31,43,34}:panel);color(active?accent:edge);SDL_Rect r{x,y,w,h};SDL_RenderDrawRect(renderer,&r);label(s,x+10,y+(h-24)/2,active?bright:text);hits.push_back({r,std::move(f)});}
 void result(const Result& r){notice=r.message;}
 void image(const std::string& path,int x,int y,int w,int h){if(path.empty())return;auto it=textures.find(path);if(it==textures.end()){int iw,ih,n;auto file=(path.rfind("user-assets/",0)==0?game.userDir:game.dataDir)/path;if(file.extension()==".bmp"){auto surface=SDL_LoadBMP(file.string().c_str());SDL_Texture* t=surface?SDL_CreateTextureFromSurface(renderer,surface):nullptr;if(surface)SDL_FreeSurface(surface);it=textures.emplace(path,t).first;}else{unsigned char* bytes=stbi_load(file.string().c_str(),&iw,&ih,&n,4);SDL_Texture* t=nullptr;if(bytes){t=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,iw,ih);SDL_UpdateTexture(t,nullptr,bytes,iw*4);SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);stbi_image_free(bytes);}it=textures.emplace(path,t).first;}}if(!it->second)return;int iw,ih;SDL_QueryTexture(it->second,nullptr,nullptr,&iw,&ih);double scale=std::min(double(w)/iw,double(h)/ih);SDL_Rect dst{x+(w-int(iw*scale))/2,y+(h-int(ih*scale))/2,int(iw*scale),int(ih*scale)};SDL_RenderCopy(renderer,it->second,nullptr,&dst);}
 void imageStretch(const std::string& path,SDL_Rect dst){layoutFrames.push_back(dst);image(path,-10000,-10000,1,1);auto t=textures.find(path);if(t==textures.end()||!t->second)throw std::runtime_error("Missing original interface artwork: "+path);SDL_RenderCopy(renderer,t->second,nullptr,&dst);}
 void input(std::string value,bool number,std::function<void(const std::string&)> f){editing=true;editNumber=number;editText=std::move(value);commit=std::move(f);SDL_StartTextInput();}
 void go(const std::string& p){if(p=="map")mapEditorFeedback.clear();baseMenuOpen=false;if(p!="globe"){placeBase=false;placeRadar=false;placementFeedback.clear();}globeGesture.cancel();if(window)SDL_CaptureMouse(SDL_FALSE);quantityDragging=false;listScrollValue=nullptr;listScrollRegions.clear();modelDragging=false;modelTouches.clear();draggedModel.clear();modelViewports.clear();page=p=="base-info"?"bases":p;scroll=0;index=0;selection.clear();subMenu.clear();equipmentSlot.clear();crewRole.clear();if(p=="menu"||p=="load"||p=="new-game"||p=="options"||p=="exit")speed=0;if(p=="catalog"||(p=="editor"&&!custom.contains(group)&&group!="_set"))group="buildings";if(p=="production"&&group!="weapons"&&group!="equipment"&&group!="units")group="weapons";if(p=="wiki")wikiTopicScroll=wikiArticleScroll=0;}

std::string ufoContactLabel(const Json& contact){if(contact.value("kind",std::string(""))=="CRASH")return tr("SESTŘELENÉ UFO · TROSKY","SHOT-DOWN UFO · WRECK");std::string environment=contact.value("environment",std::string("AIR"));return environment=="LAND"?tr("PŘISTÁLÉ UFO · ZEMĚ","LANDED UFO · LAND"):environment=="SEA"?tr("PŘISTÁLÉ UFO · MOŘE","LANDED UFO · SEA"):tr("UFO VE VZDUCHU","AIRBORNE UFO");}
#include "globe_screen.inc"
#include "base_menu.inc"

 // HTML v0.3.0: 52px top row, 184px command panel, 205px system panel.

#include "base_screen.inc"
#include "space_view.inc"
#include "iss_screen.inc"
#include "space_popup.inc"
#include "gameset_editor.inc"
#include "reference_screens.inc"
#include "air_alerts.inc"
#include "amphibious_screen.inc"
#include "freight_screen.inc"
#include "soldier_screens.inc"
#include "audio_settings.inc"
 void finishInput(bool accept){auto value=editText;editing=false;editorDeleting=false;SDL_StopTextInput();if(accept)commit(value);}
 void modal(){
  if(page=="editor"){
   if(editorDeleting){
    hits.clear();rect(0,0,W,H,{0,0,0,210});box(220,245,584,258);
    small(editorDeleteTitle,242,266,bright,15);
    boundedText(editorDeleteName,{242,310,540,62},13,text);
    small(editorDeleteInfo,242,382,dim,11);
    control(tr("SMAZAT","DELETE"),242,439,260,42,[this]{finishInput(true);});hits.back().name="modal-confirm";
    control(tr("ZRUŠIT","CANCEL"),522,439,260,42,[this]{finishInput(false);});hits.back().name="modal-cancel";return;
   }
   hits.clear();rect(0,0,W,H,{0,0,0,210});box(180,190,664,388);
   small(tr("HODNOTA · Ctrl+A vymaže · Ctrl+V vloží","VALUE · Ctrl+A clears · Ctrl+V pastes"),200,210,bright,12);
   control("",200,245,624,260,[]{SDL_StartTextInput();});hits.back().name="modal-field";
   auto lines=wrappedLines(editText+"_",600,12);int first=std::max(0,int(lines.size())-14);for(int i=first;i<int(lines.size());++i)small(lines[i],212,255+(i-first)*17,text,12);
   control(tr("POTVRDIT · ENTER","CONFIRM · ENTER"),200,525,302,35,[this]{finishInput(true);});hits.back().name="modal-confirm";
   control(tr("ZRUŠIT · ESC","CANCEL · ESC"),522,525,302,35,[this]{finishInput(false);});hits.back().name="modal-cancel";return;
  }
  hits.clear();rect(0,0,W,H,{0,0,0,210});box(220,260,584,210);label(tr("Zadej hodnotu a potvrď","Enter a value and confirm"),240,280,bright);
  control("",240,325,544,45,[]{SDL_StartTextInput();});hits.back().name="modal-field";label(editText+"_",250,335);
  control(tr("POTVRDIT · ENTER","CONFIRM · ENTER"),240,407,262,38,[this]{finishInput(true);});hits.back().name="modal-confirm";
  control(tr("ZRUŠIT · ESC","CANCEL · ESC"),522,407,262,38,[this]{finishInput(false);});hits.back().name="modal-cancel";
 }
 bool modalPointer(const SDL_Event& event){if(event.type!=SDL_MOUSEBUTTONDOWN||event.button.button!=SDL_BUTTON_LEFT)return false;mx=event.button.x;my=event.button.y;auto targets=hits;for(auto it=targets.rbegin();it!=targets.rend();++it)if(it->name.rfind("modal-",0)==0&&mx>=it->r.x&&my>=it->r.y&&mx<it->r.x+it->r.w&&my<it->r.y+it->r.h){it->action();break;}return true;}
 void requestDemolition(int object){auto allowed=game.canDemolish(base,object);if(!allowed.ok){baseFeedback=allowed.message;return;}demolitionBase=base;demolitionIndex=object;demolitionTarget=game.state["bases"][base]["buildings"][object];demolitionPending=true;demolitionVisible=false;demolitionResumeSpeed=speed;speed=0;}
 void finishDemolition(bool accept){if(accept&&demolitionBase>=0&&demolitionBase<int(game.state["bases"].size())){auto& objects=game.state["bases"][demolitionBase]["buildings"];if(demolitionIndex>=0&&demolitionIndex<int(objects.size())&&objects[demolitionIndex]==demolitionTarget)baseFeedback=game.demolish(demolitionBase,demolitionIndex).message;else baseFeedback=tr("Objekt se změnil, vyber jej znovu.","Building changed; select it again.");}demolitionPending=demolitionVisible=false;speed=demolitionResumeSpeed;}
 void demolitionDialog(){demolitionVisible=true;hits.clear();box(262,252,560,242);std::string id=demolitionTarget["id"];auto object=game.record("buildings",id);small(tr("POTVRDIT ODSTRANĚNÍ OBJEKTU","CONFIRM BUILDING REMOVAL"),282,273,bright,15);wrap(tr("Opravdu odstranit: ","Remove building: ")+(object?nameOf(*object):id)+"?",282,315,520,13);small(tr("VRÁCENÍ 10 % CENY: ","REFUND 10%: ")+cash(game.demolitionRefund(id)),282,370,dim,12);control(tr("ODSTRANIT","REMOVE"),282,427,245,40,[this]{finishDemolition(true);});hits.back().name="demolish-confirm";control(tr("ZRUŠIT","CANCEL"),547,427,255,40,[this]{finishDemolition(false);});hits.back().name="demolish-cancel";}
 bool demolitionPointer(const SDL_Event& event){if(!demolitionPending)return false;if(!demolitionVisible)return true;if(event.type==SDL_KEYDOWN){if(event.key.keysym.sym==SDLK_ESCAPE)finishDemolition(false);if(event.key.keysym.sym==SDLK_RETURN)finishDemolition(true);}if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT&&event.button.y>=427&&event.button.y<467){if(event.button.x>=282&&event.button.x<527)finishDemolition(true);else if(event.button.x>=547&&event.button.x<802)finishDemolition(false);}return true;}
 void advanceGame(double hours){auto queued=game.state["production"].size();bool alreadyPending=game.state.value("pendingProductionCompletions",0)>0;double previousSpeed=speed;game.advanceUntilUfoContact(hours);pollUfoAlerts();if(game.state["production"].size()<queued){productionCompleted=true;productionCompletionVisible=false;if(!alreadyPending)resumeProductionSpeed=previousSpeed;speed=0;}}
 void acknowledgeProduction(){productionCompleted=false;productionCompletionVisible=false;game.state["pendingProductionCompletions"]=0;speed=(ufoAlertOpen||ufoAwaitingInterception)?0:resumeProductionSpeed;resumeProductionSpeed=0;}
 void productionCompletionDialog(){productionCompleted=game.state.value("pendingProductionCompletions",0)>0;if(!productionCompleted)return;productionCompletionVisible=true;hits.clear();box(312,295,520,160);small(tr("Výroba dokončena","Production completed"),342,329,bright,20);control(tr("POTVRDIT","OK"),342,388,460,40,[this]{acknowledgeProduction();});hits.back().name="production-completed-confirm";}
 bool productionCompletionPointer(const SDL_Event& event){productionCompleted=game.state.value("pendingProductionCompletions",0)>0;if(!productionCompleted)return false;if(!productionCompletionVisible)return true;if((event.type==SDL_KEYDOWN&&(event.key.keysym.sym==SDLK_RETURN||event.key.keysym.sym==SDLK_ESCAPE))||(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT&&event.button.x>=342&&event.button.x<802&&event.button.y>=388&&event.button.y<428)){acknowledgeProduction();}return true;}

 void quantityAt(float x){const int left=page=="production"?470:379, width=page=="production"?525:286;quantity=std::clamp(1+int(std::round((x-left)*99./width)),1,100);}
 bool quantityPointer(const SDL_Event& event){if(!quantityDragging)return false;if(event.type==SDL_MOUSEMOTION){quantityAt(event.motion.x);return true;}if((event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT)||(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST)){if(event.type==SDL_MOUSEBUTTONUP)quantityAt(event.button.x);quantityDragging=false;SDL_CaptureMouse(SDL_FALSE);return true;}return false;}
 void verifyScreenActions(){
  for(const auto& frame:layoutFrames)if(frame.x==300&&frame.y==H-35&&frame.w==650&&frame.h==27)throw std::runtime_error("Removed footer overlay must not be rendered on any page");
  auto originalState=game.state;auto originalPage=page,originalGroup=group;auto originalNotice=notice;bool originalCountries=countriesOpen;int originalSoldier=selectedSoldier,originalPace=paceIndex;double originalVolume=musicVolume;bool originalMusic=musicEnabled;double originalRotation=rotation,originalTilt=tilt,originalZoom=globeZoom,originalSpeed=speed;bool originalPlacement=placeBase;auto originalLocation=location,originalSpaceRole=spaceRole;int originalIndex=index,originalQuantity=quantity;
  auto click=[this](const std::string& name){auto it=std::find_if(hits.begin(),hits.end(),[&](const Hit& h){return h.name==name;});if(it==hits.end())throw std::runtime_error("Missing screen action: "+name);float x=it->r.x+it->r.w/2.f,y=it->r.y+it->r.h/2.f;mx=x;my=y;auto copy=hits;for(auto h=copy.rbegin();h!=copy.rend();++h)if(x>=h->r.x&&x<h->r.x+h->r.w&&y>=h->r.y&&y<h->r.y+h->r.h){auto action=h->action;action();return;}throw std::runtime_error("Unreachable screen action: "+name);};
  if(page=="editor"){
   for(const auto& hit:hits)if(hit.name.rfind("time-speed:",0)==0)throw std::runtime_error("Game time controls leaked into editor");
   for(const auto& frame:layoutFrames)if(frame.x==0||frame.y==0)throw std::runtime_error("Game sidebar or header leaked into editor");
   verifyGamesetEditorUI();
  }
  if(page=="globe"){verifyAirAlertUI();verifyInterceptionSelectionUI();verifyBaseMenuUI();hits.clear();listScrollRegions.clear();renderScreen();}
  if(page=="aircraft"){SDL_Rect picture{326,405,202,150};std::vector<Uint32> pixels(202*150);if(SDL_RenderReadPixels(renderer,&picture,SDL_PIXELFORMAT_ARGB8888,pixels.data(),202*4)!=0||std::set<Uint32>(pixels.begin(),pixels.end()).size()<20)throw std::runtime_error("Aircraft artwork is hidden or blank in hangar");std::cout<<"Hangar aircraft image is visibly rendered\n";}
  if(!english&&(localized("BUY")!="KOUPIT"||localized("PRIMARY WEAPON")!="HLAVNÍ ZBRAŇ"||localized("IN STOCK · 100")!="VE SKLADU · 100"))throw std::runtime_error("Default Czech captions are incomplete");
  if(!english&&(localized("AUTOŘI")!="AUTOŘI"||localized("Autoři")!="Autoři"||localized("AUTO")!="AUTOMAT"))throw std::runtime_error("Czech word boundaries corrupt author captions");
  if(page=="new-game"){bool original=english;click("language-english");if(!english||!readJson(game.userDir/"ui-options.json").value("languageChosen",false))throw std::runtime_error("English startup language selection failed");click("language-czech");if(english)throw std::runtime_error("Czech startup language selection failed");english=original;std::cout<<"Startup Czech/English language choice passed\n";}
  if(page=="finance"){click(tr("PODPORA PODLE STÁTŮ","SUPPORT BY COUNTRY"));if(countriesOpen==originalCountries)throw std::runtime_error("Country support disclosure did not toggle");std::cout<<"Finance disclosure click passed\n";}
  if(page=="storage"){auto fill=game.storageCapacity()["used"].get<double>();auto money=game.state["money"].get<long long>();int stock=game.state["stock"].value("pistol",0);click("BUY");if(game.state["stock"].value("pistol",0)!=stock)throw std::runtime_error("Purchase teleported before arrival");game.advance(game.state["tradeFlights"].back()["arrival"].get<double>()-game.state["hours"].get<double>());if(game.state["stock"].value("pistol",0)!=stock+1||game.state["money"].get<long long>()>=money)throw std::runtime_error("Storage BUY did not update inventory and funds");if(game.storageCapacity()["used"].get<double>()<=fill)throw std::runtime_error("Warehouse fill did not update after purchase");click("SELL");if(std::abs(game.storageCapacity()["used"].get<double>()-fill)>1e-9)throw std::runtime_error("Warehouse fill did not update after sale");if(game.state["stock"].value("pistol",0)!=stock)throw std::runtime_error("Storage SELL did not update inventory");hits.clear();renderScreen();click("storage-quantity-slider");SDL_Event drag{};drag.type=SDL_MOUSEMOTION;drag.motion.x=666;quantityPointer(drag);drag.type=SDL_MOUSEBUTTONUP;drag.button.button=SDL_BUTTON_LEFT;drag.button.x=666;quantityPointer(drag);if(quantity!=100||quantityDragging)throw std::runtime_error("Quantity slider drag failed");click("BUY");if(game.state["stock"].value("pistol",0)!=stock)throw std::runtime_error("Purchase teleported before aircraft arrival");game.advance(game.state["tradeFlights"].back()["arrival"].get<double>()-game.state["hours"].get<double>());if(game.state["stock"].value("pistol",0)!=stock+100)throw std::runtime_error("100-unit purchase failed");click("SELL");if(game.state["stock"].value("pistol",0)!=stock)throw std::runtime_error("100-unit sale failed");quantity=1;scroll=100000;renderScreen();click("storage-item:glass");renderScreen();auto glassHit=std::find_if(hits.begin(),hits.end(),[](const Hit& hit){return hit.name=="storage-item:glass";});if(glassHit==hits.end()||glassHit->r.y+glassHit->r.h>641)throw std::runtime_error("Glass row is covered by purchase footer");click("storage-quantity-100");int glass=game.state["stock"].value("glass",0);click("BUY");game.advance(game.state["tradeFlights"].back()["arrival"].get<double>()-game.state["hours"].get<double>());if(game.state["stock"]["glass"]!=glass+100)throw std::runtime_error("Glass bulk purchase failed");index=scroll=0;quantity=1;std::cout<<"Storage slider, 100-unit purchase/sale and fill updates passed\n";}
  if(page=="freight")verifyFreightUI();if(page=="amphibious")verifyAmphibiousUI();
  if(page=="catalog"){
   verifyUfopediaCategoriesUI();click(tr("PĚCHOTNÍ ZBRANĚ","PERSONNEL WEAPONS"));if(group!="weapons")throw std::runtime_error("Ufopedia category click failed");std::cout<<"Ufopedia category click passed\n";}
  auto render=[this]{hits.clear();modelViewports.clear();listScrollRegions.clear();layoutFrames.clear();rect(0,0,W,H,bg);renderScreen();};
  for(const auto& hit:hits)if(hit.name=="^"||hit.name=="v")throw std::runtime_error("List arrows must be replaced by sliders");
  auto activeSlider=std::find_if(listScrollRegions.begin(),listScrollRegions.end(),[](const ListScrollRegion& region){return region.maximum>0;});if(activeSlider!=listScrollRegions.end()){auto region=*activeSlider;int saved=*region.value;auto sliderHit=std::find_if(hits.begin(),hits.end(),[&](const Hit& hit){return hit.name.find("list-scroll:")==0&&hit.r.x==region.area.x+region.area.w+2;});if(sliderHit==hits.end())throw std::runtime_error("List slider is missing");click(sliderHit->name);SDL_Event drag{};drag.type=SDL_MOUSEMOTION;drag.motion.y=10000;listScrollPointer(drag);if(*region.value!=region.maximum)throw std::runtime_error("List slider does not reach last page");drag.motion.y=-10000;listScrollPointer(drag);if(*region.value!=0)throw std::runtime_error("List slider does not reach first page");drag.type=SDL_MOUSEBUTTONUP;drag.button.button=SDL_BUTTON_LEFT;drag.button.y=-10000;listScrollPointer(drag);if(listScrollValue)throw std::runtime_error("List slider mouse capture not released");mx=region.area.x+1;my=region.area.y+1;SDL_Event wheel{};wheel.type=SDL_MOUSEWHEEL;wheel.wheel.y=-1;listScrollPointer(wheel);if(*region.value!=std::min(3,region.maximum))throw std::runtime_error("List-specific mouse wheel failed");*region.value=saved;render();std::cout<<"List slider drag, limits, release and wheel passed\n";}

  if(page=="catalog"){
   verifyUfopediaCategoriesUI();
   if(!english){auto translations=readJson(game.dataDir/"ui-translations.json");Json audit=Json::array();for(const auto& category:{"buildings","units","weapons","equipment"})for(const auto& record:game.gameset()[category]){auto checkText=[&](const std::string& source){bool ascii=true;int letters=0;for(unsigned char c:source){ascii=ascii&&c<128;letters+=((c>='A'&&c<='Z')||(c>='a'&&c<='z'));}if(ascii&&letters>=3&&!translations.contains(source))throw std::runtime_error("Missing complete Czech Ufopedia translation: "+source);audit.push_back({{"id",record["id"]},{"source",source},{"czech",localized(source)}});};checkText(record.value("name",""));if(!record.value("description",std::string("")).empty())checkText(record["description"]);for(const auto& pair:record.value("specs",Json::array())){checkText(pair[0]);checkText(pair[1]);}for(const auto& field:{"values","tu"})if(record.contains("combat")&&record["combat"].contains(field))for(const auto& item:record["combat"][field].items())checkText(item.key());}writeJson(game.userDir/"ufopedia-czech-audit.json",audit);std::cout<<"Complete Czech translation coverage checked for all Ufopedia records\n";}
   for(const auto& value:{std::string("Integrated main cannon and secondary machine gun · buy ammunition only"),std::string("Přílišdlouhýnerozdělenýřetězecproověřenízalamováníčeskéhotextubezpřetečení"),std::string("první řádek\ndruhý řádek")}){auto lines=wrappedLines(value,201,10);for(const auto& line:lines)if(textWidth(line,10)>201)throw std::runtime_error("Ufopedia wrapped text exceeds its table cell");if(lines.size()<2)throw std::runtime_error("Ufopedia long text or explicit newline did not wrap");}
   for(const auto& entry:std::vector<std::pair<std::string,std::string>>{{"equipment","diving-hood"},{"equipment","rebreather"},{"equipment","diving-suit"},{"equipment","compressed-air-cannon"},{"equipment","underwater-torpedo-launcher"},{"equipment","air-round"},{"equipment","underwater-torpedo"},{"equipment","compressed-air-cannon-mk2"},{"equipment","guided-torpedo-launcher"},{"units","underwater-robot"},{"units","light-tank"},{"units","pilot"},{"units","recruit"},{"units","heavy-soldier"},{"equipment","tank-main-gun-round"},{"equipment","radiation-suit"},{"weapons","pistol-ammo"}}){group=game.ufopediaCategory(entry.second,entry.first);auto records=orderedRecords();auto found=std::find_if(records.begin(),records.end(),[&](const Json& r){return r["id"]==entry.second;});if(found==records.end())throw std::runtime_error("Missing Ufopedia entry: "+entry.second);index=int(found-records.begin());scroll=std::max(0,index-6);render();SDL_Surface* snap=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,snap->format->format,snap->pixels,snap->pitch);SDL_SaveBMP(snap,(game.userDir/("ufopedia-"+entry.second+".bmp")).string().c_str());SDL_FreeSurface(snap);if(entry.second=="light-tank"){auto region=std::find_if(listScrollRegions.begin(),listScrollRegions.end(),[this](const ListScrollRegion& r){return r.value==&catalogueDetailScroll;});if(region==listScrollRegions.end()||region->maximum<=0)throw std::runtime_error("Long Ufopedia table must be scrollable");int maximum=region->maximum;click("list-scroll:catalogue-details");SDL_Event drag{};drag.type=SDL_MOUSEMOTION;drag.motion.y=10000;listScrollPointer(drag);drag.type=SDL_MOUSEBUTTONUP;drag.button.button=SDL_BUTTON_LEFT;drag.button.y=10000;listScrollPointer(drag);if(catalogueDetailScroll!=maximum||listScrollValue)throw std::runtime_error("Ufopedia table slider failed to reach the final row");render();snap=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,snap->format->format,snap->pixels,snap->pitch);SDL_SaveBMP(snap,(game.userDir/"ufopedia-light-tank-bottom.bmp").string().c_str());SDL_FreeSurface(snap);}}group="weapons";scroll=0;
   index=2;render();click(tr("PŘESNOST STŘELBY >","ACCURACY CURVES >"));if(page!="combat-data"||index!=2||group!="weapons")throw std::runtime_error("Weapon detail lost selected record");render();click(tr("ZPĚT DO UFOPEDIE","BACK TO UFOPEDIA"));if(page!="catalog"||index!=2||group!="weapons")throw std::runtime_error("Ufopedia back navigation lost selection");std::cout<<"Weapon accuracy detail and return selection passed\n";
  }
  if(page=="personnel"){
   role="scientist";index=2;render();std::vector<int> offers;for(int i=0;i<int(game.state["offers"].size());++i)if(game.state["offers"][i]["role"]==role)offers.push_back(i);auto candidate=game.state["offers"][offers[index]];
   click(tr("NAJMOUT · ","HIRE · ")+cash(0));if(game.state["personnel"].back()!=candidate)throw std::runtime_error("Hiring did not preserve selected candidate");render();
   std::cout<<"Workforce selected-candidate hire click passed\n";
  }
  if(page=="research"){render();for(const auto& hit:hits)if(hit.name=="research-project:research_alien_biology"||hit.name=="research-project:research_alien_materials"||hit.name=="research-project:research_intel_sectoid"||hit.name=="research-project:research_autopsy_sectoid")throw std::runtime_error("Undiscovered alien project visible");
   game.state["wrecks"]["ufo-craft"]=1;render();click("research-project:research_alien_materials");render();click("research-start");render();int scientist=-1;for(int i=0;i<int(game.state["personnel"].size());++i)if(game.state["personnel"][i]["role"]=="scientist"){scientist=i;break;}
   if(scientist<0)throw std::runtime_error("No scientist for UI regression");click("research-scientist:"+std::to_string(scientist));if(game.researchTeam("research_alien_materials").size()!=1)throw std::runtime_error("Selected project did not receive chosen scientist");
   render();click("research-project:research_basic_lab");render();click("research-assign-available");if(game.researchTeam("research_basic_lab").size()!=9||game.researchTeam("research_alien_materials").size()!=1)throw std::runtime_error("Assign available scientists stole another team");
   game.advance(72);if(game.state["research"]["research_basic_lab"].get<double>()<.89||game.state["research"]["research_alien_materials"].get<double>()<=0)throw std::runtime_error("Assigned teams failed to advance");
   render();click("research-project:research_alien_materials");render();click("research-start");if(game.researchTeam("research_alien_materials").size()!=1)throw std::runtime_error("Reopening research reset team");
   render();auto progressBeforeClear=game.state["research"]["research_alien_materials"];click("research-clear-all");if(!game.researchTeam("research_alien_materials").empty()||game.state["research"]["research_alien_materials"]!=progressBeforeClear)throw std::runtime_error("Clear research team changed progress");
   std::cout<<"Research clicks: select second project, assign scientist, independent second team, progress and reselection passed\n";
  }
  if(page=="workshop"||page=="laboratory"){bool lab=page=="laboratory";if(lab)game.startResearch("research_basic_lab");else game.manufacture("weapons","pistol",2);render();click(lab?"laboratory-item:research_basic_lab":"workshop-item:pistol");if(page!=(lab?"research":"production"))throw std::runtime_error("Facility queue detail navigation failed");go(lab?"laboratory":"workshop");if(lab){for(std::size_t i=0;i<game.state["personnel"].size();++i)if(game.state["personnel"][i]["role"]=="scientist")game.assignScientist("research_basic_lab",i,true);}else game.assignTechnician("weapons","pistol",6,true);double half=lab?game.researchQuote("research_basic_lab")["hours"].get<double>()/2:18.;
   auto capture=[this,lab](const std::string& suffix){SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);if(!surface)throw std::runtime_error(SDL_GetError());int error=SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch);if(!error)error=SDL_SaveBMP(surface,(game.userDir/(std::string(lab?"laboratory-front-":"workshop-front-")+suffix+".bmp")).string().c_str());SDL_FreeSurface(surface);if(error)throw std::runtime_error(SDL_GetError());};game.advance(half);render();capture("active");game.advance(half);game.state["pendingProductionCompletions"]=0;render();auto rowName=std::string(lab?"laboratory-item:research_basic_lab":"workshop-item:pistol");if(std::none_of(hits.begin(),hits.end(),[&](const Hit& hit){return hit.name==rowName;}))throw std::runtime_error("Completed item disappeared from facility queue");capture("completed");std::cout<<"Facility queue rows, active ETA and retained completion passed\n";}

  if(page=="queues"){game.startResearch("research_basic_lab");game.manufacture("weapons","pistol",1);render();click("queued-production:pistol");if(page!="production")throw std::runtime_error("Production queue detail navigation failed");render();click("open-queues");render();click("queued-research:research_basic_lab");if(page!="research")throw std::runtime_error("Research queue detail navigation failed");go("queues");render();std::cout<<"Work and research queue navigation passed\n";}
  if(page=="production"){
   render();click("production-quantity-slider");SDL_Event quantityDrag{};quantityDrag.type=SDL_MOUSEMOTION;quantityDrag.motion.x=995;quantityPointer(quantityDrag);if(quantity!=100)throw std::runtime_error("Production slider maximum failed");quantityDrag.type=SDL_MOUSEMOTION;quantityDrag.motion.x=470;quantityPointer(quantityDrag);if(quantity!=1)throw std::runtime_error("Production slider minimum failed");quantityDrag.type=SDL_MOUSEBUTTONUP;quantityDrag.button.button=SDL_BUTTON_LEFT;quantityDrag.button.x=475;quantityPointer(quantityDrag);if(quantity!=2||quantityDragging)throw std::runtime_error("Production slider release failed");
   group="weapons";index=0;quantity=2;game.state["stock"]["iron"]=0;game.state["stock"]["plastics"]=0;render();auto count=game.state["production"].size();
   click("production-start");if(game.state["production"].size()!=count)throw std::runtime_error("Production accepted absent materials");
   click(tr("SKLAD / SUROVINY >","STORAGE / MATERIALS >"));render();click("BUY");game.advance(game.state["tradeFlights"].back()["arrival"].get<double>()-game.state["hours"].get<double>());if(game.state["stock"]["iron"]!=2)throw std::runtime_error("Iron purchase from production failed");
   click("storage-item:plastics");quantity=2;render();click("BUY");game.advance(game.state["tradeFlights"].back()["arrival"].get<double>()-game.state["hours"].get<double>());if(game.state["stock"]["plastics"]!=2)throw std::runtime_error("Plastics purchase failed");
   click(tr("VÝROBA","MANUFACTURE"));render();if(page=="workshop"){click("queue-add");render();}click("production-start");
   if(game.state["production"].size()!=count+2||game.state["stock"]["iron"]!=0||game.state["stock"]["plastics"]!=0)throw std::runtime_error("Production quantity click failed");
   render();click("production-assign-all");if(game.productionTeam("weapons","pistol").size()!=10)throw std::runtime_error("Direct assign-all technicians failed");render();auto queuedBeforeClear=game.state["production"];click("production-clear-all");if(!game.productionTeam("weapons","pistol").empty()||game.state["production"]!=queuedBeforeClear)throw std::runtime_error("Clear production team changed queued work");click("production-assign-all");render();click("production-team");render();click("production-assign-available");if(game.productionTeam("weapons","pistol").size()!=10)throw std::runtime_error("Technician assignment UI failed");click(tr("ZPĚT DO VÝROBY","BACK TO PRODUCTION"));
   int stock=game.state["stock"].value("pistol",0);render();click("time-speed:3600");go("workshop");if(speed!=1.)throw std::runtime_error("Navigating to queue paused production time");go("production");group="weapons";if(speed!=1.)throw std::runtime_error("Navigating to item paused production time");advanceGame(9);render();{SDL_Surface* preview=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);if(!preview)throw std::runtime_error(SDL_GetError());int captured=SDL_RenderReadPixels(renderer,nullptr,preview->format->format,preview->pixels,preview->pitch);if(!captured)captured=SDL_SaveBMP(preview,(game.userDir/"production-progress.bmp").string().c_str());SDL_FreeSurface(preview);if(captured)throw std::runtime_error(SDL_GetError());}if(std::abs(game.productionProgress("weapons","pistol")["progress"].get<double>()-.5)>1e-9)throw std::runtime_error("Visible batch progress is not fifty percent");if(game.state["production"][0]["remaining"]!=9.)throw std::runtime_error("Halfway production progress incorrect");if(game.state["stock"].value("pistol",0)!=stock)throw std::runtime_error("Production finished too early");advanceGame(9);if(!productionCompleted)throw std::runtime_error("Production completion announcement missing");productionCompletionDialog();click("production-completed-confirm");if(productionCompleted||speed!=1.)throw std::runtime_error("Completion confirmation did not resume production time");if(game.state["stock"].value("pistol",0)!=stock+2)throw std::runtime_error("Manufactured items failed to reach storage");
   std::cout<<"Fresh-stock workflow: missing materials, storage purchases, two-unit order and timed delivery passed\n";
  }
  if(page=="graphs"){
   if(std::any_of(layoutFrames.begin(),layoutFrames.end(),[](const SDL_Rect& frame){return frame.x>=194&&frame.y>=60;}))throw std::runtime_error("Graphs menu must have an empty content area");
   std::cout<<"Graphs menu content remains empty\n";
  }
  if(page=="aircraft-modules"){
   for(int craft=0;craft<int(game.state["craft"].size())&&craft<3;++craft){activeCraft=craft;render();auto artwork=aircraftImage(game.state["craft"][craft]["type"]);if(!textures.count(artwork)||!textures.at(artwork))throw std::runtime_error("Aircraft image did not render");std::string slot=game.state["craft"][craft]["type"]=="fighter"?"system":"accessory";click("aircraft-slot:"+slot);render();click("SHIELD");if(game.aircraftModule(craft,slot)!="SHIELD")throw std::runtime_error("Aircraft module assignment failed");}activeCraft=0;aircraftSlot="system";render();double litres=game.fighterFuel(0)["fuelLitres"];click("FUEL_TANK");render();if(game.aircraftModule(0,"system")!="FUEL_TANK"||std::abs(game.fighterFuel(0)["fuelLitres"].get<double>()-litres)>1e-6)throw std::runtime_error("External fuel tank UI assignment created fuel or failed");SDL_Surface* fuelShot=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,fuelShot->format->format,fuelShot->pixels,fuelShot->pitch);SDL_SaveBMP(fuelShot,(game.userDir/"fighter-external-tank.bmp").string().c_str());SDL_FreeSurface(fuelShot);std::cout<<"All aircraft images, direct module assignments and external tank passed\n";
  }

  if(page=="catalog"){
   verifyUfopediaCategoriesUI();
   auto capacities=readJson(game.dataDir/"facility-capacities.json");group="buildings";for(const auto& object:orderedRecords())if(!capacities.contains(object["id"].get<std::string>()))throw std::runtime_error("Missing facility capacity documentation");std::cout<<"Ufopedia capacity documentation covers all facilities\n";
   auto facilities=orderedRecords();for(int i=0;i<int(facilities.size());++i)if(facilities[i]["id"]=="research"||facilities[i]["id"]=="workshop"||facilities[i]["id"]=="quarters"){index=i;render();SDL_Surface* shot=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,shot->format->format,shot->pixels,shot->pitch);SDL_SaveBMP(shot,(game.userDir/("capacity-"+facilities[i]["id"].get<std::string>()+".bmp")).string().c_str());SDL_FreeSurface(shot);}index=0;render();
  }
  if(page=="space-visual"){
   verifySpaceGestures();verifySpacePopup();verifyISSOperationsUI();verifySatelliteSurveyUI();
   game.ensureLocations();game.state["satellites"].push_back({{"type","EARTH"},{"altitude",980},{"fuel",60},{"launchTime",game.state["hours"]},{"number",1}});
   for(const auto& center:{"esa","nasa","roscosmos"}){location=center;for(const auto& view:{"orbit","iss","ramp"}){spaceVisual=view;render();SDL_Rect area{240,200,720,360};std::vector<Uint32> pixels(area.w*area.h);SDL_RenderReadPixels(renderer,&area,SDL_PIXELFORMAT_ARGB8888,pixels.data(),area.w*4);if(std::set<Uint32>(pixels.begin(),pixels.end()).size()<8)throw std::runtime_error("Space visual is blank: "+location+" / "+view);SDL_Surface* shot=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,shot->format->format,shot->pixels,shot->pitch);SDL_SaveBMP(shot,(game.userDir/("space-"+location+"-"+view+".bmp")).string().c_str());SDL_FreeSurface(shot);}}
   location="esa";spaceVisual="orbit";render();std::cout<<"All centers: orbit map, ISS and ramp artwork visibly rendered\n";
  }
  if(page=="space-location"){
   for(const std::string center:{"nasa","esa","roscosmos"}){location=center;render();SDL_Rect area{235,450,410,218};std::vector<Uint32> pixels(area.w*area.h);SDL_RenderReadPixels(renderer,&area,SDL_PIXELFORMAT_ARGB8888,pixels.data(),area.w*4);if(std::set<Uint32>(pixels.begin(),pixels.end()).size()<20)throw std::runtime_error("Control base is missing: "+center);SDL_Surface* shot=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,shot->format->format,shot->pixels,shot->pitch);SDL_SaveBMP(shot,(game.userDir/("control-"+center+".bmp")).string().c_str());SDL_FreeSurface(shot);if(center!="roscosmos"){click(center=="nasa"?"open-iss-window":"open-rocket-window");if(!spaceWindow||page!="space-location")throw std::runtime_error("Control center model button did not open a separate window");closeSpaceModel();}}
   location="nasa";render();click(tr("PRONAJMOUT / PRODLOUŽIT · ","RENT / EXTEND · ")+cash(400000));if(!game.spaceLease("nasa"))throw std::runtime_error("Houston lease click failed");
   render();click(tr("POSTAVIT OBYTNOU BUŇKU · ","BUILD RESIDENTIAL CELL · ")+cash(game.constants["costs"].value("residential",240000LL)));if(game.state["spaceHousing"].size()!=3)throw std::runtime_error("Houston housing click failed");
   game.advance(72);render();click(tr("ASTRONAUTI · NÁBOR A POSÁDKA >","ASTRONAUTS · RECRUITMENT & CREW >"));if(page!="astronauts")throw std::runtime_error("Astronaut market navigation failed");render();click(tr("NAJMOUT ASTRONAUTA","HIRE ASTRONAUT"));if(game.state["astronauts"].size()!=1)throw std::runtime_error("Astronaut hire click failed");
   render();click(game.state["astronauts"][0]["name"].get<std::string>()+" · "+game.state["astronauts"][0]["role"].get<std::string>());if(game.state["flightCrew"].size()!=1)throw std::runtime_error("New recruit cannot be assigned to crew");std::cout<<"Houston HTML base, eight initial places, recruitment, crew assignment, ESA/Roscosmos bases and popup navigation passed\n";
  }
  if(page=="launch-ramp"){
   location="esa";game.rentLocation("esa");render();auto count=game.state["launchQueue"].size();click(tr("NAPLÁNOVAT START · ","SCHEDULE LAUNCH · ")+cash(game.satelliteQuote(launchFuel)["cost"].get<long long>()*launchCount));if(game.state["launchQueue"].size()!=count+launchCount)throw std::runtime_error("Satellite scheduling click failed");
   location="roscosmos";game.rentLocation(location);render();if(!game.launchTransport(location,1,1).ok)throw std::runtime_error("Baikonur flight scheduling failed");std::cout<<"ESA launch click and Baikonur configuration passed\n";
  }
  if(page=="soldier-profile"){for(int i=0;i<12;++i)game.state["personnel"].push_back(game.makeCandidate("soldier",0));render();click("list-scroll:soldier-strip");SDL_Event drag{};drag.type=SDL_MOUSEMOTION;drag.motion.y=10000;listScrollPointer(drag);drag.type=SDL_MOUSEBUTTONUP;drag.button.button=SDL_BUTTON_LEFT;drag.button.y=10000;listScrollPointer(drag);render();int last=int(game.state["personnel"].size())-1;click("soldier-strip:"+std::to_string(last));if(selectedSoldier!=last)throw std::runtime_error("Last soldier is unreachable in top strip");render();go("soldier-equipment");render();if(std::none_of(hits.begin(),hits.end(),[&](const Hit& hit){return hit.name=="soldier-strip:"+std::to_string(last);}))throw std::runtime_error("Selected soldier lost when changing profile/equipment");go("soldier-profile");render();std::cout<<"Top soldier strip: twenty soldiers, last selection and equipment navigation passed\n";}
  if(page=="soldiers"){
   click("PROFILE");if(page!="soldier-profile")throw std::runtime_error("Soldier profile did not open");render();click(tr("VYBAVIT","EQUIP"));if(page!="soldier-equipment")throw std::runtime_error("Equipment screen did not open");render();
   int stock=game.state["stock"].value("pistol",0);click("equip:primaryWeapon:0");if(equipmentSlot!="primaryWeapon")throw std::runtime_error("Equipment picker did not open");render();click("choose:pistol");if(!equipmentSlot.empty()||game.state["stock"].value("pistol",0)!=stock-1)throw std::runtime_error("Picker failed to equip from storage");render();click("equip:primaryWeapon:0");render();click(tr("ODEBRAT / PRÁZDNÉ","REMOVE / EMPTY"));if(game.state["stock"].value("pistol",0)!=stock)throw std::runtime_error("Picker did not return equipment");std::cout<<"Soldier profile/equipment navigation and transactional picker clicks passed\n";
  }
  if(page=="aircraft"){
   click("PILOT · SELECT PILOT");if(crewRole!="pilot")throw std::runtime_error("Crew picker did not open");render();int pilot=-1;for(int i=0;i<int(game.state["personnel"].size());++i)if(game.state["personnel"][i]["role"]=="pilot"){pilot=i;break;}if(pilot<0)throw std::runtime_error("Missing test pilot");auto p=game.state["personnel"][pilot];click(p["name"].get<std::string>()+" · SKILL "+std::to_string(p["skill"].get<int>()));if(game.state["craft"][activeCraft]["crew"]["pilot"]!=pilot||!crewRole.empty())throw std::runtime_error("Crew picker did not assign pilot");std::cout<<"Crew picker clicks passed\n";verifyAmphibiousUI();verifyFleetLossUI();
  }
  if(page=="globe"){
   verifyResearchProductionNavigationUI();verifyCapitalCitiesUI();
   auto saveWeather=[this](const char* name){SDL_Surface* shot=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,shot->format->format,shot->pixels,shot->pitch);SDL_SaveBMP(shot,(game.userDir/name).string().c_str());SDL_FreeSurface(shot);};
   bool priorWeather=globeWeather;globeWeather=true;mx=0;my=0;render();saveWeather("globe-weather.bmp");click("globe-weather-toggle");if(globeWeather)throw std::runtime_error("Cannot hide globe weather");render();saveWeather("globe-weather-off.bmp");click("globe-weather-toggle");if(!globeWeather)throw std::runtime_error("Cannot restore globe weather");globeWeather=priorWeather;std::cout<<"Globe weather overlay: render, hide and restore passed\n";
   globeZoom=1;for(int i=0;i<10;++i){render();click("globe-zoom-in");}if(std::abs(globeView().scaleMeters()-1000000.)>.001)throw std::runtime_error("Zoom button must stop at 1000 km");
   render();SDL_Surface* zoomShot=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,nullptr,zoomShot->format->format,zoomShot->pixels,zoomShot->pitch);SDL_SaveBMP(zoomShot,(game.userDir/"globe-1000km.bmp").string().c_str());SDL_FreeSurface(zoomShot);
   click("globe-zoom-out");if(globeZoom>=GlobeView::maxZoom)throw std::runtime_error("Zoom out button failed");zoomGlobe(100);if(std::abs(globeView().scaleMeters()-1000000.)>.001)throw std::runtime_error("Wheel zoom must stop at 1000 km");zoomGlobe(-100);if(globeZoom!=.65)throw std::runtime_error("Zoom lower bound failed");std::cout<<"Globe buttons and shared wheel zoom: 1000 km maximum and zoom-out passed\n";
   game.newGame();placeBase=true;
   auto pointer=[this](Uint32 type,float x,float y){SDL_Event ev{};ev.type=type;int wx=int(x),wy=int(y);if(type==SDL_MOUSEMOTION){ev.motion.x=wx;ev.motion.y=wy;}else{ev.button.button=SDL_BUTTON_LEFT;ev.button.x=wx;ev.button.y=wy;}return handleGlobePointer(ev);};
   for(auto size:std::vector<std::pair<int,int>>{{1024,768},{1280,800},{1600,900}}){
    SDL_SetWindowSize(window,size.first,size.second);SDL_RenderPresent(renderer);SDL_PumpEvents();
    double oldRotation=rotation,oldTilt=tilt;globeZoom=size.first==1024?.65:size.first==1280?1.:GlobeView::maxZoom;pointer(SDL_MOUSEBUTTONDOWN,501,410);if(editing)throw std::runtime_error("Mouse down must not open base dialog");pointer(SDL_MOUSEMOTION,581,450);pointer(SDL_MOUSEBUTTONUP,581,450);
    if(rotation==oldRotation||tilt==oldTilt||editing||!game.state["bases"].empty())throw std::runtime_error("First-base dragging did not rotate or accidentally placed a base");render();
   }
   SDL_SetWindowSize(window,W,H);SDL_RenderPresent(renderer);SDL_PumpEvents();globeZoom=1;rotation=30;tilt=0;pointer(SDL_MOUSEBUTTONDOWN,501,410);pointer(SDL_MOUSEBUTTONUP,501,410);if(editing||!game.state["bases"].empty())throw std::runtime_error("Ocean accepted as base site");
   pointer(SDL_MOUSEBUTTONDOWN,190,700);pointer(SDL_MOUSEBUTTONUP,190,700);if(editing)throw std::runtime_error("Outside sphere accepted as base site");
   rotation=-14.4378;tilt=50.0755;pointer(SDL_MOUSEBUTTONDOWN,501,410);pointer(SDL_MOUSEMOTION,503,412);pointer(SDL_MOUSEBUTTONUP,501,410);if(!editing)throw std::runtime_error("Land click must open naming dialog");editText="GLOBE CLICK TEST";modal();SDL_Event confirm{};confirm.type=SDL_MOUSEBUTTONDOWN;confirm.button.button=SDL_BUTTON_LEFT;confirm.button.x=371;confirm.button.y=426;modalPointer(confirm);if(editing)throw std::runtime_error("Mouse confirm did not close base dialog");
   if(game.state["bases"].size()!=1||game.state["bases"][0].value("country","")!="Czechia"||std::abs(game.state["bases"][0]["lon"].get<double>()-14.4378)>.001||std::abs(game.state["bases"][0]["lat"].get<double>()-50.0755)>.001||placeBase)throw std::runtime_error("Clicked land coordinates were not preserved in first base");
   auto baseCount=game.state["bases"].size();input("CANCEL TEST",false,[this](const std::string& value){game.createBase(value,50,14);});modal();SDL_Event cancel{};cancel.type=SDL_MOUSEBUTTONDOWN;cancel.button.button=SDL_BUTTON_LEFT;cancel.button.x=653;cancel.button.y=426;modalPointer(cancel);if(editing||game.state["bases"].size()!=baseCount)throw std::runtime_error("Mouse cancel created a base");
   std::cout<<"Globe SDL input: first-base drag at 3 window sizes, ocean/space rejection, jitter click and saved Czech coordinates passed\n";
  }
  if(page=="bases"){
   const bool savedIso=baseIsometric;const int savedFloor=floor;
   auto snapshot=[this](const std::string& name){SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);if(!surface)throw std::runtime_error(SDL_GetError());int result=SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch);if(!result)result=SDL_SaveBMP(surface,(game.userDir/name).string().c_str());SDL_FreeSurface(surface);if(result)throw std::runtime_error(SDL_GetError());};
   floor=0;baseIsometric=true;render();snapshot("base-surface-3d.bmp");click("2D");render();if(baseIsometric)throw std::runtime_error("2D base toggle failed");snapshot("base-surface-2d.bmp");
   auto place=[this](float x,float y){mx=x;my=y;auto hit=std::find_if(hits.begin(),hits.end(),[](const Hit& h){return h.name=="base-workspace";});if(hit==hits.end())throw std::runtime_error("Missing base placement area");auto action=hit->action;action();};
   building="storage";game.state["money"]=100000000;
   auto view=baseViewport();place(view.flatX+8*view.cell+view.cell/2,view.flatY+8*view.cell+view.cell/2);
   const auto& placed=game.state["bases"][base]["buildings"].back();if(placed["x"]!=8||placed["y"]!=8)throw std::runtime_error("2D placement coordinates failed");
   click("3D");render();if(!baseIsometric)throw std::runtime_error("3D surface toggle failed");
   game.state["bases"][base]["floors"]=1;click("LEVEL 0");render();if(floor!=-1)throw std::runtime_error("Floor selector failed");if(std::any_of(hits.begin(),hits.end(),[](const Hit& h){return h.name=="3D";}))throw std::runtime_error("Underground must not offer 3D");snapshot("base-underground-2d.bmp");
   place(view.flatX+2*view.cell+view.cell/2,view.flatY+2*view.cell+view.cell/2);const auto& underground=game.state["bases"][base]["buildings"].back();if(underground["x"]!=2||underground["y"]!=2||underground["floor"]!=-1)throw std::runtime_error("Underground did not use 2D placement");
   floor=0;baseIsometric=false;demolishMode=false;building="gym";mx=view.flatX+9*view.cell+view.cell/2;my=view.flatY+9*view.cell+view.cell/2;render();snapshot("base-preview-invalid.bmp");if(game.canBuild(base,"gym",9,9,0).ok)throw std::runtime_error("Outside footprint preview incorrectly valid");building="storage";mx=view.flatX+7*view.cell+view.cell/2;my=view.flatY+9*view.cell+view.cell/2;render();snapshot("base-preview-valid.bmp");click("base-demolish-mode");render();auto beforeRemoval=game.state;place(view.flatX+9*view.cell+view.cell/2,view.flatY+view.cell/2);if(!demolitionPending||game.state!=beforeRemoval)throw std::runtime_error("Demolition must request confirmation before mutation");demolitionDialog();snapshot("base-demolition-confirm.bmp");click("demolish-cancel");if(game.state!=beforeRemoval)throw std::runtime_error("Cancelled demolition changed campaign");render();place(view.flatX+9*view.cell+view.cell/2,view.flatY+view.cell/2);demolitionDialog();auto objectCount=game.state["bases"][base]["buildings"].size();click("demolish-confirm");if(demolitionPending||game.state["bases"][base]["buildings"].size()!=objectCount-1)throw std::runtime_error("Confirmed demolition did not remove exactly one object");demolishMode=false;floor=savedFloor;baseIsometric=savedIso;

   std::cout<<"Base views: surface 2D/3D toggles, 2D placement, underground forced 2D and placement passed\n";
  }
  if(page=="wiki"){auto articles=readJson(game.dataDir/(english?"ui/manual.json":"ui/manual-cs.json"));if(!english&&articles[0]["title"].get<std::string>().find("GLÓBUS")!=0)throw std::runtime_error("Wiki did not select Czech manual");wikiTopicScroll=std::max(0,int(articles.size())-8);render();click("wiki-topic:"+std::to_string(articles.size()-1));render();if(index!=int(articles.size())-1)throw std::runtime_error("Last wiki chapter is unreachable");std::cout<<"Czech wiki source and all chapters passed\n";}
  if(page=="locations"){click("location:nasa");if(page!="space-location"||location!="nasa")throw std::runtime_error("Location overview navigation failed");go("locations");render();std::cout<<"Localized space-center overview and navigation passed\n";}
  if(page=="options"){click("music-volume");if(!musicEnabled||musicVolume<.49||musicVolume>.51)throw std::runtime_error("Music slider failed");click("option:4:1");if(paceIndex!=(originalPace+1)%3)throw std::runtime_error("Settings arrow failed");std::cout<<"Settings slider and arrow clicks passed\n";}
  game.state=std::move(originalState);page=originalPage;group=originalGroup;notice=originalNotice;countriesOpen=originalCountries;selectedSoldier=originalSoldier;paceIndex=originalPace;musicVolume=originalVolume;musicEnabled=originalMusic;equipmentSlot.clear();crewRole.clear();speed=originalSpeed;ufoAlertOpen=ufoAlertVisible=ufoAwaitingInterception=false;ufoAlertContacts=Json::array();interceptionPicker=false;resumeProductionSpeed=0;productionCompleted=false;productionCompletionVisible=false;rotation=originalRotation;tilt=originalTilt;globeZoom=originalZoom;placeBase=originalPlacement;location=originalLocation;spaceRole=originalSpaceRole;index=originalIndex;quantity=originalQuantity;if(page=="wiki")wikiTopicScroll=wikiArticleScroll=0;render();
 }
public:
 App(fs::path data,fs::path user):game(std::move(data),std::move(user)){
  loadUiOptions();
  loadGamesetLibrary();

 }
 ~App(){closeSpaceModel();if(globeClouds)SDL_DestroyTexture(globeClouds);if(globeSurface)SDL_DestroyTexture(globeSurface);if(globeCache)SDL_DestroyTexture(globeCache);if(musicDevice)SDL_CloseAudioDevice(musicDevice);if(musicOpened)drmp3_uninit(&musicDecoder);for(auto [_,t]:textures)if(t)SDL_DestroyTexture(t);for(auto [_,g]:glyphs)if(g.texture)SDL_DestroyTexture(g.texture);if(renderer)SDL_DestroyRenderer(renderer);if(window)SDL_DestroyWindow(window);SDL_Quit();}
 int run(bool smoke,const fs::path& screenshot,const std::string& initial,bool uiTest,const fs::path& report,bool audioTest){
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
#ifndef _WIN32
  // Match the validated portable renderer; explicit SDL_RENDER_DRIVER can override it.
  SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER,"software",SDL_HINT_DEFAULT);
#endif
  std::cerr<<"UAA "<<Version<<" starting"<<std::endl;SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0)throw std::runtime_error(SDL_GetError());
  window=SDL_CreateWindow("UFO Alien Atak - native C++ development",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);if(!window)throw std::runtime_error(SDL_GetError());
  changeOption(2,0);
  if(fullscreenRequested)SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
  renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);if(!renderer)throw std::runtime_error(SDL_GetError());
  SDL_RendererInfo rendererInfo{};SDL_GetRendererInfo(renderer,&rendererInfo);std::cerr<<"Renderer: "<<(rendererInfo.name?rendererInfo.name:"unknown")<<std::endl;
  SDL_RenderSetLogicalSize(renderer,W,H);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
  std::ifstream f(game.dataDir/"fonts/LiberationSans-Regular.ttf",std::ios::binary);fontBytes.assign(std::istreambuf_iterator<char>(f),{});if(fontBytes.empty()||!stbtt_InitFont(&font,fontBytes.data(),0))throw std::runtime_error("Font unavailable");
  std::ifstream mf(game.dataDir/"fonts/LiberationMono-Bold.ttf",std::ios::binary);monoBytes.assign(std::istreambuf_iterator<char>(mf),{});if(monoBytes.empty()||!stbtt_InitFont(&monoFont,monoBytes.data(),0))throw std::runtime_error("Monospace font unavailable");
  if(audioTest){
   auto previousPage=page;double previousVolume=musicVolume;musicActivated=true;
   page="menu";updateMusic();if(musicDevice||musicOpened)throw std::runtime_error("Menu must not start music");
   page="globe";updateMusic();if(musicFailed||!musicDevice||musicTrack!=0||SDL_GetQueuedAudioSize(musicDevice)==0)throw std::runtime_error("Original ambient MP3 playback failed");
   game.state["ufos"].push_back(Json::object());updateMusic();if(musicFailed||musicTrack!=1||SDL_GetQueuedAudioSize(musicDevice)==0)throw std::runtime_error("Original alert MP3 playback failed");game.state["ufos"].clear();
   for(const auto& silentPage:{"menu","load","new-game","options","authors","script","exit","map","editor"}){
    page=silentPage;updateMusic();if(SDL_GetAudioDeviceStatus(musicDevice)!=SDL_AUDIO_PAUSED||SDL_GetQueuedAudioSize(musicDevice)!=0||!musicEnabled||musicVolume!=previousVolume)throw std::runtime_error("Music outside gameplay: "+page);
    page="globe";updateMusic();if(musicFailed||SDL_GetAudioDeviceStatus(musicDevice)!=SDL_AUDIO_PLAYING||SDL_GetQueuedAudioSize(musicDevice)==0)throw std::runtime_error("Music must resume on return to game");
   }
   musicEnabled=false;updateMusic();if(SDL_GetAudioDeviceStatus(musicDevice)!=SDL_AUDIO_PAUSED)throw std::runtime_error("Music disable failed");page=previousPage;
   std::cout<<"Ambient/alert playback, silent menu pages and editors, preserved settings and gameplay resumption passed\n";
  }
  if(uiTest){
   const std::vector<std::string> expected={"new-game","load","options","editor","authors","script","map","exit"};
   for(auto size:std::vector<std::pair<int,int>>{{1280,800},{1024,768},{1600,900}}){
    SDL_SetWindowSize(window,size.first,size.second);SDL_RenderPresent(renderer);SDL_PumpEvents();
    for(int row=0;row<8;++row){
     page="menu";hits.clear();menu();auto target=menuRow(row);
     int dragX,dragY;float wx,wy,lx,ly;SDL_RenderLogicalToWindow(renderer,target.x+target.w/2.f,target.y+target.h/2.f,&dragX,&dragY);wx=float(dragX);wy=float(dragY);
     SDL_RenderWindowToLogical(renderer,int(wx),int(wy),&lx,&ly);
     auto match=hits.end();for(auto it=hits.begin();it!=hits.end();++it)if(lx>=it->r.x&&lx<it->r.x+it->r.w&&ly>=it->r.y&&ly<it->r.y+it->r.h){if(match!=hits.end())throw std::runtime_error("Overlapping original-menu hit regions");match=it;}
     if(match==hits.end())throw std::runtime_error("Menu button unreachable after resizing");auto action=match->action;action();
     if(page!=expected[row])throw std::runtime_error("Wrong original-menu action");running=true;
    }
   }
   SDL_SetWindowSize(window,W,H);SDL_RenderPresent(renderer);SDL_PumpEvents();
   std::cout<<"Original menu: 24 resized click checks passed\n";
  }
  if(uiTest&&initial!="menu"&&initial!="new-game"&&initial!="options"&&initial!="load"&&initial!="authors"&&initial!="script"){auto created=game.createBase("LAYOUT TEST",50,14);if(!created.ok)throw std::runtime_error(created.message);}
  page=initial=="base-info"?"bases":initial;layoutReport=report;notice=uiTest?"Footer suppression regression test":"";auto last=SDL_GetTicks64();int frames=0;bool loggedReady=false;
  while(running){auto now=SDL_GetTicks64();double dt=std::min(0.25,double(now-last)/1000);last=now;auto fleetSize=game.state["craft"].size();game.removeDestroyedAircraft();if(game.state["craft"].size()!=fleetSize){hits.clear();crewRole.clear();equipmentSlot.clear();activeCraft=std::clamp(activeCraft,0,std::max(0,int(game.state["craft"].size())-1));}pollUfoAlerts();if(speed>0&&!ufoAlertOpen)advanceGame(dt*speed);
   SDL_Event ev;while(SDL_PollEvent(&ev)){
    if(ev.type==SDL_MOUSEBUTTONDOWN||ev.type==SDL_KEYDOWN)musicActivated=true;
    if(ev.type==SDL_QUIT)running=false;
    if(ufoAlertPointer(ev))continue;
    if(interceptionPickerPointer(ev))continue;
    if(baseMenuPointer(ev))continue;
    if(spaceWindowEvent(ev))continue;
    if(ev.type==SDL_MOUSEMOTION){mx=float(ev.motion.x);my=float(ev.motion.y);}
    if(freightPartnerPicker&&ev.type==SDL_KEYDOWN&&ev.key.keysym.sym==SDLK_ESCAPE){freightPartnerPicker=false;continue;}
    if(operationWarningPointer(ev))continue;
    if(demolitionPointer(ev))continue;
    if(productionCompletionPointer(ev))continue;
    if(editing){if(modalPointer(ev))continue;if(ev.type==SDL_TEXTINPUT&&editText.size()<(page=="editor"?65536u:120u))editText+=ev.text.text;if(ev.type==SDL_KEYDOWN){if(ev.key.keysym.sym==SDLK_v&&(ev.key.keysym.mod&KMOD_CTRL)){char* pasted=SDL_GetClipboardText();if(pasted){std::string value=pasted;SDL_free(pasted);if(editText.size()+value.size()<=(page=="editor"?65536u:120u))editText+=value;}}else if(ev.key.keysym.sym==SDLK_a&&(ev.key.keysym.mod&KMOD_CTRL)){editText.clear();}else if(ev.key.keysym.sym==SDLK_ESCAPE){finishInput(false);}else if(ev.key.keysym.sym==SDLK_BACKSPACE&&!editText.empty()){std::size_t pos=editText.size()-1;while(pos>0&&(static_cast<unsigned char>(editText[pos])&0xc0)==0x80)--pos;editText.erase(pos);}else if(ev.key.keysym.sym==SDLK_RETURN){finishInput(true);}}continue;}
    if(ev.type==SDL_WINDOWEVENT&&ev.window.event==SDL_WINDOWEVENT_FOCUS_LOST){globeGesture.cancel();SDL_CaptureMouse(SDL_FALSE);}
    if(spaceModelPointer(ev))continue;
    if(listScrollPointer(ev))continue;
    if(quantityPointer(ev))continue;
    if(handleGlobePointer(ev))continue;
    if(ev.type==SDL_KEYDOWN&&ev.key.keysym.sym==SDLK_ESCAPE)go("menu");
    if(ev.type==SDL_MOUSEWHEEL){if(page=="globe"&&mx>=184&&mx<819&&my>=52&&my<768)zoomGlobe(ev.wheel.y);else scroll=std::max(0,scroll-ev.wheel.y*3);}
    if(ev.type==SDL_MOUSEBUTTONDOWN&&ev.button.button==SDL_BUTTON_LEFT){mx=float(ev.button.x);my=float(ev.button.y);
     auto oldHits=hits;for(auto it=oldHits.rbegin();it!=oldHits.rend();++it)if(mx>=it->r.x&&my>=it->r.y&&mx<it->r.x+it->r.w&&my<it->r.y+it->r.h){it->action();break;}}
   }
   pollUfoAlerts();updateMusic();
   hits.clear();modelViewports.clear();listScrollRegions.clear();layoutFrames.clear();rect(0,0,W,H,bg);renderScreen();if(editing)modal();if(game.state.value("pendingProductionCompletions",0)>0)productionCompletionDialog();if(demolitionPending)demolitionDialog();if(operationWarning)operationWarningDialog();if(baseMenuOpen&&!ufoAlertOpen)baseMenuDialog();if(interceptionPicker&&!ufoAlertOpen)interceptionPickerDialog();if(ufoAlertOpen)ufoAlertDialog();
   if(frames==2&&!layoutReport.empty()){Json j={{"page",page},{"logicalWidth",W},{"logicalHeight",H},{"minimumTextPixels",10},{"frames",Json::array()},{"controls",Json::array()}};for(const auto& r:layoutFrames)j["frames"].push_back({r.x,r.y,r.w,r.h});for(const auto& hit:hits)j["controls"].push_back({{"name",hit.name},{"rect",{hit.r.x,hit.r.y,hit.r.w,hit.r.h}}});writeJson(layoutReport,j);}
   if(uiTest&&frames==2){verifyScreenActions();for(const auto& hit:hits)if(hit.r.x<0||hit.r.y<0||hit.r.x+hit.r.w>W||hit.r.y+hit.r.h>H)throw std::runtime_error("Control outside screen: "+page);std::cout<<"Layout bounds: "<<page<<" passed\n";}
   if(smoke&&frames==2&&!screenshot.empty()){SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);if(!surface)throw std::runtime_error(SDL_GetError());if(SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch)!=0)throw std::runtime_error(SDL_GetError());if(SDL_SaveBMP(surface,screenshot.string().c_str())!=0)throw std::runtime_error(SDL_GetError());SDL_FreeSurface(surface);}
   SDL_RenderPresent(renderer);renderSpaceWindow();if(!loggedReady){std::cerr<<"Ready page="<<page<<std::endl;loggedReady=true;}if(smoke&&++frames>=3)running=false;SDL_Delay(10);
  }return 0;
 }
};
}
int main(int argc,char** argv){
 try{fs::path data,user,screenshot,report;bool smoke=false,uiTest=false,audioTest=false;std::string page="menu";for(int i=1;i<argc;i++){std::string a=argv[i];if(a=="--smoke-test")smoke=true;else if(a=="--ui-test"){uiTest=true;smoke=true;}else if(a=="--audio-test"){audioTest=true;smoke=true;}else if((a=="--data"||a=="--user-dir"||a=="--screenshot"||a=="--page"||a=="--layout-report")&&i+1<argc){std::string v=argv[++i];if(a=="--data")data=fs::u8path(v);else if(a=="--user-dir")user=fs::u8path(v);else if(a=="--page")page=v;else if(a=="--layout-report")report=fs::u8path(v);else screenshot=fs::u8path(v);}else if(a=="--version"){std::cout<<Version<<"\n";return 0;}else throw std::runtime_error("Unknown argument: "+a);}
  if(data.empty()){char* base=SDL_GetBasePath();data=fs::u8path(base?base:".")/"data";SDL_free(base);}
  if(user.empty()){char* pref=SDL_GetPrefPath("UFOAlienAtak","NativeDev");if(!pref)throw std::runtime_error("Cannot locate user directory");user=fs::u8path(pref);SDL_free(pref);}
  fs::create_directories(user);App app(data,user);return app.run(smoke,screenshot,page,uiTest,report,audioTest);
 }catch(const std::exception& e){std::cerr<<"UAA: "<<e.what()<<"\n";if(!SDL_getenv("SDL_VIDEODRIVER"))SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"UFO Alien Atak",e.what(),nullptr);return 1;}
}
