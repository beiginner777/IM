#!/bin/bash
# stability_monitor.sh —— 场景 9：长时稳定性监控
# 用法: sh stability_monitor.sh [采样次数] [间隔秒]
# 默认: 6 次，间隔 600s（10 分钟，配合 1h 压测）
# 配合: 终端另开一个跑 tcp_bench 200 连接 3600s

N=${1:-6}
INTERVAL=${2:-600}

for i in $(seq 1 $N); do
  echo "===== $(date '+%Y-%m-%d %H:%M:%S') 采样 $i/$N ====="
  docker stats --no-stream --format "table {{.Name}}\t{{.MemUsage}}\t{{.CPUPerc}}\t{{.NetIO}}"
  echo "8090 端口 TCP 连接数: $(ss -tn state established '( sport = :8090 )' 2>/dev/null | wc -l)"
  echo ""
  [ $i -lt $N ] && sleep $INTERVAL
done

echo "=== 采样结束 ==="
echo "重点看: 业务服务(chatserver1) RSS 是否持续上涨(泄漏信号)、MySQL/Redis 连接数是否稳定"
