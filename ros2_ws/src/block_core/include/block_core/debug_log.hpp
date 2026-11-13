#pragma once
#include <json/json.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
namespace block_core {
class DebugLog {
 public:
  static bool enabled_for(const std::string& name,std::string spec) {
    auto trim=[](std::string s){const auto begin=s.find_first_not_of(" \t\r\n");return begin==std::string::npos?std::string():s.substr(begin,s.find_last_not_of(" \t\r\n")-begin+1);};
    std::transform(spec.begin(),spec.end(),spec.begin(),[](unsigned char c){return std::tolower(c);});spec=trim(spec);
    if(spec.empty() || spec=="0" || spec=="false" || spec=="off" || spec=="no")return false;
    if(spec=="1" || spec=="true" || spec=="on" || spec=="yes" || spec=="all")return true;
    std::istringstream in(spec);std::string entry;while(std::getline(in,entry,','))if(trim(entry)==name)return true;return false;
  }
  explicit DebugLog(std::string name):name_(std::move(name)) {
    const auto* spec=std::getenv("BP_DEBUG");enabled_=spec && enabled_for(name_,spec);
    if(!enabled_)return;
    try {
      const auto* directory=std::getenv("BP_DEBUG_DIR");const auto* home=std::getenv("HOME");
      const std::filesystem::path root=directory?directory:std::string(home?home:"/tmp")+"/.ros/block_perception_debug";
      std::filesystem::create_directories(root);path_=root/(name_+".jsonl");
    } catch(...) {enabled_=false;}
  }
  void event(const std::string& name,double ros_time,Json::Value fields=Json::Value(Json::objectValue),double every=0) noexcept {
    if(!enabled_)return;
    try {
      const double monotonic=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      const auto old=last_.find(name);if(every>0 && old!=last_.end() && monotonic-old->second<every)return;last_[name]=monotonic;
      fields["t"]=ros_time;fields["wall"]=std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();fields["node"]=name_;fields["event"]=name;
      Json::StreamWriterBuilder writer;writer["indentation"]="";const auto text=Json::writeString(writer,fields)+'\n';
      if(std::filesystem::exists(path_) && std::filesystem::file_size(path_)+text.size()>=5000000) {
        std::filesystem::remove(path_.string()+".3");
        for(int i=2;i>=1;--i)if(std::filesystem::exists(path_.string()+"."+std::to_string(i)))std::filesystem::rename(path_.string()+"."+std::to_string(i),path_.string()+"."+std::to_string(i+1));
        std::filesystem::rename(path_,path_.string()+".1");
      }
      std::ofstream out(path_,std::ios::app);out<<text;
    } catch(...) { /* Logging must never interrupt control. */ }
  }
 private:
  std::string name_;bool enabled_{};std::filesystem::path path_;std::map<std::string,double> last_;
};
}
