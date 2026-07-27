# IM 系统 Docker 镜像
# 构建上下文: server/ 目录
# docker compose build --no-cache
# docker compose up -d

FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake g++ \
    libboost-all-dev \
    libssl-dev \
    libjsoncpp-dev \
    libprotobuf-dev protobuf-compiler \
    libgrpc++-dev protobuf-compiler-grpc \
    libmysqlcppconn-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

# 构建所有服务
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# ==================== 运行时 ====================
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-system1.74.0 \
    libssl3 \
    libjsoncpp25 \
    libprotobuf23 \
    libgrpc++1.51 \
    libmysqlcppconn7v5 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 复制所有二进制
COPY --from=builder /build/build/AuthServer/AuthServer       /app/bin/
COPY --from=builder /build/build/StatusServer/StatusServer    /app/bin/
COPY --from=builder /build/build/ChatServer1/ChatServer1      /app/bin/
COPY --from=builder /build/build/ChatServer2/ChatServer2      /app/bin/
COPY --from=builder /build/build/ResourceServer/ResourceServer /app/bin/
COPY --from=builder /build/build/SeckillServer/SeckillServer  /app/bin/

# 复制配置文件
COPY AuthServer/config.ini     /app/config/AuthServer.ini
COPY StatusServer/config.ini   /app/config/StatusServer.ini
COPY ChatServer1/config.ini    /app/config/ChatServer1.ini
COPY ChatServer2/config.ini    /app/config/ChatServer2.ini
COPY ResourceServer/config.ini /app/config/ResourceServer.ini
COPY SeckillServer/config.ini  /app/config/SeckillServer.ini

# 通用入口脚本: 第一个参数是服务名，复制对应 config 后启动
RUN echo '#!/bin/bash\n\
SERVICE=$1\n\
CONFIG=/app/config/${SERVICE}.ini\n\
if [ ! -f "$CONFIG" ]; then\n\
    echo "Usage: docker run <image> <ServiceName>"\n\
    echo "  e.g. docker run <image> AuthServer"\n\
    exit 1\n\
fi\n\
cp $CONFIG /app/config.ini\n\
exec /app/bin/$SERVICE\n' > /app/entrypoint.sh && chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]
