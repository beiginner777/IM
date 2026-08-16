#!/bin/bash
# sentinel_failover.sh —— 场景 7：Redis 哨兵故障切换
# 用法: sh sentinel_failover.sh
# 前提: 哨兵集群已启动（sentinel-1/2/3）

# 切到 docker-compose 所在目录，便于用服务名操作
cd "$(dirname "$0")/../linux-server/docker" 2>/dev/null || cd ../linux-server/docker

get_master_ip() {
  docker compose exec -T sentinel-1 redis-cli -p 26379 SENTINEL get-master-addr-by-name mymaster 2>/dev/null | head -1
}

echo "=== 切换前 master IP ==="
OLD=$(get_master_ip)
echo "  $OLD"

echo ""
echo "=== 触发故障（pause redis-master）==="
docker compose pause redis-master

echo ""
echo "=== 监控切换（每 1s 查一次，最多 60s）==="
for i in $(seq 1 60); do
  NEW=$(get_master_ip)
  echo "[$i s] master IP = $NEW"
  if [ -n "$NEW" ] && [ "$NEW" != "$OLD" ]; then
    echo "=== 切换完成，耗时约 ${i}s（预期 ≤5s）==="
    break
  fi
  sleep 1
done

echo ""
echo "=== 恢复（unpause redis-master，它会自动降级为 slave）==="
docker compose unpause redis-master
