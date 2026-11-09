 void loadUiOptions(){
  loadTranslations();
  if(!fs::exists(game.userDir/"ui-options.json"))return;
  try{auto o=readJson(game.userDir/"ui-options.json");languageChosen=o.value("languageChosen",false);english=languageChosen?o.value("english",false):false;fullscreenRequested=o.value("fullscreen",false);musicEnabled=o.value("music",true);musicVolume=std::clamp(o.value("musicVolume",.35),0.,1.);soundVolume=std::clamp(o.value("soundVolume",1.),0.,1.);paceIndex=std::clamp(o.value("pace",1),0,2);difficultyIndex=std::clamp(o.value("difficulty",1),0,2);windowResolution=std::clamp(o.value("windowResolution",0),0,3);}catch(const std::exception&){notice="Invalid UI settings; defaults restored";}
 }
 bool saveUiOptions(){try{writeJson(game.userDir/"ui-options.json",{{"english",english},{"languageChosen",languageChosen},{"fullscreen",bool(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)},{"music",musicEnabled},{"musicVolume",musicVolume},{"soundVolume",soundVolume},{"pace",paceIndex},{"difficulty",difficultyIndex},{"windowResolution",windowResolution}});return true;}catch(const std::exception& e){notice=e.what();return false;}}
 void changeOption(int row,int direction){
  if(row==2){const std::array<std::pair<int,int>,4> sizes={{{1024,768},{1280,800},{1366,768},{1920,1080}}};windowResolution=(windowResolution+direction+4)%4;SDL_SetWindowSize(window,sizes[windowResolution].first,sizes[windowResolution].second);}
  else if(row==3){bool full=SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP;SDL_SetWindowFullscreen(window,full?0:SDL_WINDOW_FULLSCREEN_DESKTOP);}
  // In the supplied HTML these two choices are persisted presentation values;
  // they do not modify the simulation or its difficulty formula.
  else if(row==4)paceIndex=(paceIndex+direction+3)%3;else if(row==5)difficultyIndex=(difficultyIndex+direction+3)%3;
 }
 void updateMusic(){
  if(page=="menu"||page=="load"||page=="new-game"||page=="options"||page=="authors"||page=="script"||page=="exit"||page=="map"||page=="editor"){if(musicDevice){SDL_PauseAudioDevice(musicDevice,1);SDL_ClearQueuedAudio(musicDevice);}return;}
  if(!musicActivated||musicFailed)return;
  if(!musicEnabled||musicVolume<=0){if(musicDevice)SDL_PauseAudioDevice(musicDevice,1);return;}
  int wanted=game.state["ufos"].empty()?0:1;
  if(!musicOpened||musicTrack!=wanted){
   if(musicDevice){SDL_CloseAudioDevice(musicDevice);musicDevice=0;}if(musicOpened){drmp3_uninit(&musicDecoder);musicOpened=false;}
   const auto path=game.dataDir/"audio"/(wanted?"alert.mp3":"ambient.mp3");
#ifdef _WIN32
   bool opened=drmp3_init_file_w(&musicDecoder,path.wstring().c_str(),nullptr);
#else
   bool opened=drmp3_init_file(&musicDecoder,path.string().c_str(),nullptr);
#endif
   if(!opened){musicFailed=true;notice="Cannot open original music track";return;}musicOpened=true;musicTrack=wanted;
   if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0){musicFailed=true;notice=SDL_GetError();return;}
   SDL_AudioSpec requested{};requested.freq=int(musicDecoder.sampleRate);requested.format=AUDIO_S16SYS;requested.channels=Uint8(musicDecoder.channels);requested.samples=1024;musicDevice=SDL_OpenAudioDevice(nullptr,0,&requested,nullptr,0);
   if(!musicDevice){musicFailed=true;notice=SDL_GetError();return;}
  }
  SDL_PauseAudioDevice(musicDevice,0);std::array<drmp3_int16,8192> samples{};
  while(SDL_GetQueuedAudioSize(musicDevice)<musicDecoder.sampleRate*musicDecoder.channels){auto frames=drmp3_read_pcm_frames_s16(&musicDecoder,4096,samples.data());if(!frames){if(!drmp3_seek_to_pcm_frame(&musicDecoder,0)){musicFailed=true;notice="Cannot loop music";}break;}
   for(std::size_t i=0;i<frames*musicDecoder.channels;++i)samples[i]=drmp3_int16(samples[i]*musicVolume);
   if(SDL_QueueAudio(musicDevice,samples.data(),Uint32(frames*musicDecoder.channels*sizeof(drmp3_int16)))!=0){musicFailed=true;notice=SDL_GetError();break;}
  }
 }
