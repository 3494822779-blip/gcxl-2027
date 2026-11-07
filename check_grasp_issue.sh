#!/bin/bash
# 快速检查为什么没有下发抓取指令

echo "=== 最近的alignment ready检查 ==="
tail -50 ~/.ros/block_perception_debug/alignment.jsonl | grep "ready_check" | tail -10 | python3 -c "
import sys, json
for line in sys.stdin:
    d = json.loads(line)
    print(f\"pitch: {d.get('pitch_current')}/{d.get('pitch_required')} \" \
          f\"pitch_ok={d.get('pitch_ok')} ready={d.get('ready')} \" \
          f\"hold_time={d.get('hold_time')}s\")
"

echo ""
echo "=== 最近的actuator alignment命令 ==="
tail -50 ~/.ros/block_perception_debug/actuator.jsonl | grep "alignment_cmd" | tail -5 | python3 -c "
import sys, json
for line in sys.stdin:
    d = json.loads(line)
    print(f\"grasp_request={d.get('grasp_request')} aligned={d.get('aligned')} \" \
          f\"reason={d.get('reason')}\")
"

echo ""
echo "=== 是否有grasp被拒绝 ==="
tail -100 ~/.ros/block_perception_debug/actuator.jsonl | grep "grasp_rejected" | tail -5 | python3 -c "
import sys, json
for line in sys.stdin:
    d = json.loads(line)
    print(f\"Rejected: {d.get('reason')}\")
"
