#include "map_editor.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
extern char** environ;
#endif
namespace uaa {
Result launchMapEditor(const fs::path& installation){
#ifdef _WIN32
 const auto exe=fs::absolute(installation)/"mapeditor"/"uaa-mapeditor.exe";
#else
 const auto exe=fs::absolute(installation)/"mapeditor"/"uaa-mapeditor";
#endif
 const auto data=fs::absolute(installation)/"mapeditor"/"uaa-data.pak";
 const auto maps=fs::absolute(installation)/"maps";
 if(!fs::is_regular_file(exe)||!fs::is_regular_file(data))return {false,"Mapeditor is optional and is not installed. Add it using the installer."};
 fs::create_directories(maps/"pozemni");fs::create_directories(maps/"podvodni");
#ifdef _WIN32
 // CreateProcess receives an explicit application and a quoted argument list; no shell.
 const auto binary=exe.wstring();auto args=L"\""+binary+L"\" --data \""+data.wstring()+L"\"";
 STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
 if(!CreateProcessW(binary.c_str(),args.data(),nullptr,nullptr,FALSE,0,nullptr,maps.wstring().c_str(),&startup,&process))return {false,"Unable to launch UAA Mapeditor: Win32 error "+std::to_string(GetLastError())};
 CloseHandle(process.hThread);CloseHandle(process.hProcess);
#else
 const auto binary=exe.string(),pack=data.string();char flag[]="--data";
 char* argv[]={const_cast<char*>(binary.c_str()),flag,const_cast<char*>(pack.c_str()),nullptr};pid_t child=0;
 posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);const int cd=posix_spawn_file_actions_addchdir_np(&actions,maps.string().c_str());if(cd){posix_spawn_file_actions_destroy(&actions);return {false,"Unable to open maps directory"};}
 const int rc=posix_spawn(&child,binary.c_str(),&actions,nullptr,argv,environ);posix_spawn_file_actions_destroy(&actions);
 if(rc)return {false,"Unable to launch UAA Mapeditor: error "+std::to_string(rc)};
 std::thread([child]{int status;while(waitpid(child,&status,0)<0&&errno==EINTR){}}).detach();
#endif
 return {true,"UAA Mapeditor launched"};
}
}
