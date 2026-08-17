#!/bin/bash
# batch_insert_bench.sh —— 场景 3：批量写入 vs 单条写入
# 用法: sh batch_insert_bench.sh
# 前提: mysql-master 容器已启动（docker ps 可见），JerryChat 库 + chatmessage 表存在
# 说明: 进入 mysql-master 容器内，用 mysql 客户端直接对比「N 条独立 INSERT」vs「每批 100 条的批量 INSERT」

CONTAINER=mysql-master
MYSQL="docker exec -i $CONTAINER mysql -uroot -p123456 JerryChat"
N=1000
BATCH=100

echo "=== 清理测试数据 ==="
$MYSQL -e "DELETE FROM chatmessage WHERE content LIKE 'bench_%';" 2>/dev/null

# 生成单条插入 SQL（N 条独立 INSERT）
echo "生成单条插入 SQL ($N 条)..."
: > /tmp/bench_single.sql
for i in $(seq 1 $N); do
  echo "INSERT INTO chatmessage(thread_id,sender_id,recv_id,content,message_type) VALUES(1,1,2,'bench_single_$i',0);" >> /tmp/bench_single.sql
done

# 生成批量插入 SQL（每批 BATCH 条，共 N/BATCH 批）
echo "生成批量插入 SQL (每批 $BATCH 条)..."
: > /tmp/bench_batch.sql
for b in $(seq 0 $((N/BATCH-1))); do
  printf "INSERT INTO chatmessage(thread_id,sender_id,recv_id,content,message_type) VALUES" >> /tmp/bench_batch.sql
  for i in $(seq 1 $BATCH); do
    printf "(1,1,2,'bench_batch_${b}_${i}',0)" >> /tmp/bench_batch.sql
    [ $i -lt $BATCH ] && printf "," >> /tmp/bench_batch.sql
  done
  printf ";\n" >> /tmp/bench_batch.sql
done

echo ""
echo "=== 单条插入（$N 条独立 INSERT）==="
time $MYSQL < /tmp/bench_single.sql

echo ""
echo "=== 批量插入（$((N/BATCH)) 批 × $BATCH 条）==="
time $MYSQL < /tmp/bench_batch.sql

echo ""
echo "对比两者 real/user 时间，批量应为单条的 ~5x 以上（预期 ≥5x）"
