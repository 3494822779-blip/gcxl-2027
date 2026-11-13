#pragma once
#include <rclcpp/rclcpp.hpp>
#include <block_core/param_validate.hpp>
inline Json::Value parameter_value(const rclcpp::Parameter& p) {
  switch(p.get_type()) {
    case rclcpp::ParameterType::PARAMETER_DOUBLE:return p.as_double();
    case rclcpp::ParameterType::PARAMETER_INTEGER:return Json::Int64(p.as_int());
    case rclcpp::ParameterType::PARAMETER_BOOL:return p.as_bool();
    case rclcpp::ParameterType::PARAMETER_STRING:return p.as_string();
    default:return Json::Value();
  }
}
inline rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_guard(rclcpp::Node& node) {
  auto validate=[&node](const std::vector<rclcpp::Parameter>& changes) {
    rcl_interfaces::msg::SetParametersResult result; result.successful=true;
    try {
      Json::Value proposed;
      for(const auto& key:node.list_parameters({},10).names) proposed[key]=parameter_value(node.get_parameter(key));
      for(const auto& value:changes) proposed[value.get_name()]=parameter_value(value);
      block_core::validate_parameters(proposed);
    } catch(const std::exception& e) { result.successful=false; result.reason=e.what(); }
    return result;
  };
  const auto initial=validate({}); if(!initial.successful) throw std::invalid_argument(initial.reason);
  return node.add_on_set_parameters_callback(validate);
}
