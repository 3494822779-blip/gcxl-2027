# Python 行为规格清单

基准提交：`7aaef4c3bf4218f88b93210e703db70cc6c51523`。本清单用于对照迁移，不代表已完成等价验证。

原测试共 100 个函数；参数化场景多于函数数量。当前 C++ 回归覆盖关键路径，但尚未完成本清单的全部对应验收。

## block_perception/test/test_actions.py

- [ ] `test_operations_finish_and_stop`：待完成逐条对应验收。
- [ ] `test_move_emits_reverse_and_cancel_holds`：待完成逐条对应验收。
- [ ] `test_timeout_and_invalid_goals`：待完成逐条对应验收。
- [ ] `test_alignment_does_not_close_and_approach_does`：待完成逐条对应验收。
- [ ] `test_camera_requires_feedback_and_cancel_retains_target`：待完成逐条对应验收。
- [ ] `test_target_task_transport_explicit_class_and_cancel`：待完成逐条对应验收。
- [ ] `test_discard_and_missing_class_are_rejected`：待完成逐条对应验收。
- [ ] `test_autonomous_requires_class`：待完成逐条对应验收。
- [ ] `test_cancel_during_substage_and_stale_requests_do_not_restart`：待完成逐条对应验收。
- [ ] `test_feedback_and_concurrent_admission`：待完成逐条对应验收。
- [ ] `test_timeout_still_stops_when_ros_simulation_clock_is_paused`：待完成逐条对应验收。
- [ ] `test_memory_query_latched_context_and_independent_claw_cancel`：待完成逐条对应验收。
- [ ] `test_claw_action_does_not_change_long_term_closure_settings`：待完成逐条对应验收。
- [ ] `test_sweep_action_interruption_stops_and_records_without_grasp`：待完成逐条对应验收。
- [ ] `test_complete_target_sweep_through_real_node_topics`：待完成逐条对应验收。
- [ ] `test_stop_preempts_move_and_does_not_latch`：待完成逐条对应验收。
- [ ] `test_stop_invalidates_a_goal_accepted_before_execute`：待完成逐条对应验收。
- [ ] `test_emergency_stop_service_requires_explicit_reset_and_new_goal`：待完成逐条对应验收。
- [ ] `test_serial_fault_latches_even_after_link_recovers`：待完成逐条对应验收。
- [ ] `test_align_never_changes_claw_opening`：待完成逐条对应验收。
- [ ] `test_target_loss_before_closure_and_dropout_after_closure`：待完成逐条对应验收。
- [ ] `test_invalid_command_stops_and_latches`：待完成逐条对应验收。
- [ ] `test_local_command_timeout_works_without_ros_clock_progress`：待完成逐条对应验收。
- [ ] `test_pitch_rejects_replay_impossible_angle_and_old_joint_timestamp`：待完成逐条对应验收。
- [ ] `test_stop_does_not_apply_an_unpublished_servo_goal`：待完成逐条对应验收。
- [ ] `test_tracking_motion_budget_counts_movement_and_stops`：待完成逐条对应验收。

## block_perception/test/test_actuator.py

- [ ] `test_wheel_mix_and_limit`：待完成逐条对应验收。
- [ ] `test_claws_keep_first_opening_despite_later_width_change`：待完成逐条对应验收。
- [ ] `test_fixed_camera_sends_constant_angle_and_grasp_closes_claws`：待完成逐条对应验收。
- [ ] `test_forward_preserves_visual_pitch_tracking`：待完成逐条对应验收。
- [ ] `test_servo_track_integrates_pitch_rate_into_angle_and_clamps`：待完成逐条对应验收。
- [ ] `test_servo_camera_starts_from_feedback`：待完成逐条对应验收。
- [ ] `test_servo_grasp_pitches_down_then_closes_then_homes`：待完成逐条对应验收。
- [ ] `test_command_frame_and_feedback_roundtrip`：待完成逐条对应验收。
- [ ] `test_serial_rx_reassembles_lines_and_sets_pitch`：待完成逐条对应验收。
- [ ] `test_closure_angle_limits`：待完成逐条对应验收。
- [ ] `test_closure_parameters_snapshot_for_entire_grasp`：待完成逐条对应验收。
- [ ] `test_config_survives_node_restart_but_task_memory_does_not`：待完成逐条对应验收。

## block_perception/test/test_core.py

- [ ] `test_lock_bottom_edge_then_pitch_order_change_and_loss`：待完成逐条对应验收。
- [ ] `test_lock_follows_block_to_new_tracker_id`：待完成逐条对应验收。
- [ ] `test_lock_rebinding_off_or_without_time`：待完成逐条对应验收。
- [ ] `test_lock_never_rebinds_to_id_seen_before_loss`：待完成逐条对应验收。
- [ ] `test_filter_prediction_noise_and_ordering`：待完成逐条对应验收。
- [ ] `test_deadzone_saturation_and_validation`：待完成逐条对应验收。

## block_perception/test/test_debug_log.py

- [ ] `test_enabled_for`：待完成逐条对应验收。
- [ ] `test_disabled_is_noop`：待完成逐条对应验收。
- [ ] `test_writes_jsonl_and_throttles`：待完成逐条对应验收。

## block_perception/test/test_deepstream.py

- [ ] `test_nvmm_metadata_and_osd`：待完成逐条对应验收。

## block_perception/test/test_discard.py

- [ ] `test_complete_sequence_and_stop`：待完成逐条对应验收。
- [ ] `test_limits_snapshot_and_zero_distance`：待完成逐条对应验收。
- [ ] `test_invalid_settings`：待完成逐条对应验收。

## block_perception/test/test_first_grasp.py

- [ ] `test_explicit_class_and_nearest_real_target`：待完成逐条对应验收。
- [ ] `test_foreground_only_nearest_obstacle_and_center_tie`：待完成逐条对应验收。
- [ ] `test_exponential_decay_side_near_and_exact_threshold`：待完成逐条对应验收。

## block_perception/test/test_gripper.py

- [ ] `test_exact_locked_edges_and_no_fake_angles`：待完成逐条对应验收。
- [ ] `test_prediction_wrong_frame_and_timeout`：待完成逐条对应验收。
- [ ] `test_invalid_edges`：待完成逐条对应验收。

## block_perception/test/test_nodes.py

- [ ] `test_chain_lock_prediction_timeout_reset`：待完成逐条对应验收。
- [ ] `test_grasp_one_shot_and_dynamic_parameters`：待完成逐条对应验收。
- [ ] `test_fixed_camera_drives_forward_until_bottom_edge_in_band`：待完成逐条对应验收。
- [ ] `test_yaw_gain_fades_as_the_block_gets_closer`：待完成逐条对应验收。
- [ ] `test_yaw_gain_scales_with_offset_from_screen_centre`：待完成逐条对应验收。
- [ ] `test_x_deadzone_widens_towards_the_target_line`：待完成逐条对应验收。
- [ ] `test_camera_servo_mode_needs_pitch_and_drives_forward_until_it_is_down`：待完成逐条对应验收。
- [ ] `test_selector_follows_block_after_tracker_id_change`：待完成逐条对应验收。
- [ ] `test_controller_does_not_renew_observation_time`：待完成逐条对应验收。

## block_perception/test/test_target_sweep.py

- [ ] `test_selector_locks_formal_target_never_obstacle_or_new_id`：待完成逐条对应验收。
- [ ] `test_single_sweep_decay_pause_restore_then_grasp_same_target`：待完成逐条对应验收。
- [ ] `test_fixed_camera_opens_once_and_width_does_not_restart_wait`：待完成逐条对应验收。
- [ ] `test_sweep_memory_interruption_not_grasp_success`：待完成逐条对应验收。
- [ ] `test_negative_claw_limits_and_discard_config`：待完成逐条对应验收。
- [ ] `test_ready_is_rechecked_but_saved_width_is_not_recomputed`：待完成逐条对应验收。
- [ ] `test_positive_pitch_tolerance_and_stale_feedback`：待完成逐条对应验收。
- [ ] `test_temporary_frame_matching_does_not_starve_closure`：待完成逐条对应验收。
- [ ] `test_stale_target_command_keeps_identity_and_clears_ready_phase`：待完成逐条对应验收。
- [ ] `test_temporary_matching_never_reuses_unsafe_proposal`：待完成逐条对应验收。
- [ ] `test_open_once_at_measured_twenty_degrees_then_hold_across_pitch_and_width_changes`：待完成逐条对应验收。
- [ ] `test_configurable_open_pitch_feedback_expiry_and_next_task_reset`：待完成逐条对应验收。
- [ ] `test_sweep_restores_saved_opening_without_remeasuring_width`：待完成逐条对应验收。
- [ ] `test_recorded_44_degree_feedback_triggers_and_closes_grasp`：待完成逐条对应验收。
- [ ] `test_overshoot_does_not_bypass_missing_pitch_feedback`：待完成逐条对应验收。
- [ ] `test_lost_target_search_reverses_and_stops_on_reacquisition`：待完成逐条对应验收。
- [ ] `test_no_search_before_lock_or_with_fixed_camera`：待完成逐条对应验收。
- [ ] `test_search_turn_matches_tracking_yaw_sign_and_motor_threshold`：待完成逐条对应验收。
- [ ] `test_search_attempts_are_bounded_per_task`：待完成逐条对应验收。
- [ ] `test_search_finishes_after_one_pass`：待完成逐条对应验收。
- [ ] `test_multiple_obstacles_recover_claw_before_next_sweep`：待完成逐条对应验收。
- [ ] `test_persistent_obstacle_exhausts_budget_without_closing`：待完成逐条对应验收。
- [ ] `test_track_rebind_preserves_saved_opening`：待完成逐条对应验收。
- [ ] `test_grasp_requires_an_observation_after_sweep_completion`：待完成逐条对应验收。

## block_perception/test/test_task_memory.py

- [ ] `test_grasp_dedup_unknown_identity_and_snapshot_isolation`：待完成逐条对应验收。
- [ ] `test_clearance_then_formal_with_distinct_history_and_progress`：待完成逐条对应验收。
- [ ] `test_standalone_discard_links_prior_grasp_and_keeps_completed_grasp_on_cancel`：待完成逐条对应验收。
- [ ] `test_timeout_during_grasp_is_not_success_and_is_idempotent`：待完成逐条对应验收。
- [ ] `test_manual_open_breaks_association_and_restart_forgets_everything`：待完成逐条对应验收。

## serial_minimal/test/test_serial_link.py

- [ ] `test_bridge_failure_aborts_motion_using_real_health_topic`：待完成逐条对应验收。

## serial_minimal/test/test_tx_buffer.py

- [ ] `test_partial_writes_finish_a_line_and_skip_superseded_commands`：待完成逐条对应验收。
- [ ] `test_blocked_write_times_out_and_resynchronizes_before_recovery`：待完成逐条对应验收。
- [ ] `test_old_waiting_command_is_not_sent`：待完成逐条对应验收。
- [ ] `test_invalid_write_count_is_a_link_fault`：待完成逐条对应验收。
