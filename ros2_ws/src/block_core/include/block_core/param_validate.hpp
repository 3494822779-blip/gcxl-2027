#pragma once
#include <json/json.h>
#include <cmath>
#include <stdexcept>
namespace block_core {
inline void validate_parameters(const Json::Value& p) {
  auto require=[](bool ok,const std::string& reason){if(!ok) throw std::invalid_argument(reason);};
  auto value=[&](const char* k){return p[k].asDouble();};
  for(const auto& k:p.getMemberNames()) if(p[k].isNumeric()) require(std::isfinite(p[k].asDouble()),k+" must be finite");
  for(const auto* k:{"process_noise","measurement_noise","initial_variance","prediction_timeout","input_timeout","pitch_timeout","max_yaw_rate","max_pitch_rate","hold_seconds","track_width","max_wheel_speed","pitch_home_tol","grasp_pitch_tol","claw_settle_sec","mm_per_px","link_length","discard_speed_mps","discard_turn_speed_mps","wheel_speed_scale","serial_health_timeout","feedback_reconnect_sec","approach_slow_pitch_deg","max_tracking_motion_sec","search_pitch_speed_deg","search_dwell_sec","search_turn_sec","search_turn_speed_mps","sweep_tau_sec","sweep_duration_sec","sweep_max_yaw_rate","sweep_near_multiplier"})
    if(p.isMember(k)) require(value(k)>0,std::string(k)+" must be positive");
  for(const auto* k:{"rebind_timeout","discard_distance_m","forward_speed","forward_max_error_x","grasp_error_x","claw_margin_mm","min_turn_speed","claw_init_deg","reverse_speed","pitch_feedback_margin_deg"})
    if(p.isMember(k)) require(value(k)>=0,std::string(k)+" must be nonnegative");
  for(const auto* k:{"first_grasp_overlap","deadzone_x","deadzone_y"}) if(p.isMember(k)) require(value(k)>=0 && value(k)<1,std::string(k)+" must be in [0,1)");
  for(const auto* k:{"rebind_iou","yaw_near_weight","yaw_center_weight","sweep_weight","sweep_overlap"}) if(p.isMember(k)) require(value(k)>=0 && value(k)<=1,std::string(k)+" must be in [0,1]");
  for(const auto* k:{"yaw_full_error_x","bottom_target","too_close_y"}) if(p.isMember(k)) require(value(k)>0 && value(k)<=1,std::string(k)+" must be in (0,1]");
  for(const auto* k:{"search_max_attempts","max_sweep_attempts","max_sweeps"}) if(p.isMember(k)) require(p[k].isIntegral() && !p[k].isBool() && value(k)>=1,std::string(k)+" must be a positive integer");
  if(p.isMember("grasp_pitch")) require(std::abs(value("grasp_pitch"))<1.5707963267948966,"grasp_pitch outside (-pi/2,pi/2)");
  if(p.isMember("search_rebind_iou") && value("rebind_iou")!=0) require(value("search_rebind_iou")>0 && value("search_rebind_iou")<=value("rebind_iou"),"search_rebind_iou outside (0,rebind_iou]");
  if(p.isMember("cam_min_deg")) require(value("cam_min_deg")<value("cam_max_deg"),"camera limits inverted");
  for(const auto* k:{"grasp_pitch_deg","claw_open_pitch_deg"}) if(p.isMember(k)) require(value(k)>=value("cam_min_deg") && value(k)<=value("cam_max_deg"),std::string(k)+" outside camera limits");
  if(p.isMember("claw_max_deg")) require(value("claw_max_deg")>=value("claw_init_deg"),"claw limits inverted");
  for(const auto* k:{"left_claw_close_deg","right_claw_close_deg"}) if(p.isMember(k)) require(value(k)>=value("claw_init_deg") && value(k)<=value("claw_max_deg"),std::string(k)+" outside claw limits");
  if(p.isMember("claw_min_deg")) require(value("claw_min_deg")<=value("sweep_claw_deg") && value("sweep_claw_deg")<0 && value("claw_init_deg")>=0,"invalid sweep claw limits");
  if(p.get("discard_enabled",false).asBool()) throw std::invalid_argument("discard is disabled");
  if(p.isMember("search_turn_speed_mps")) require(value("search_turn_speed_mps")<=value("max_wheel_speed"),"search turn exceeds wheel speed");
  if(p.isMember("approach_min_speed")) require(value("approach_min_speed")>=0 && value("approach_min_speed")<=value("forward_speed"),"approach_min_speed outside [0,forward_speed]");
  if(p.isMember("autonomous_target_class")) require(value("autonomous_target_class")>=-1,"autonomous_target_class below -1");
}
}
