# IM 系统 Docker 镜像（Ubuntu 24.04，gRPC 1.51）
# 构建上下文: linux-server/ 目录
# docker compose build --no-cache
# docker compose up -d

FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake g++ pkg-config \
    libboost-all-dev \
    libssl-dev \
    libjsoncpp-dev \
    libprotobuf-dev protobuf-compiler \
    libgrpc++-dev protobuf-compiler-grpc \
    libmysqlcppconn-dev \
    libhiredis-dev \
    libspdlog-dev \
    libfmt-dev \
    libxcrypt-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

# 生成 protobuf 文件
RUN for dir in AuthServer StatusServer ChatServer1 ChatServer2 ResourceServer SeckillServer; do \
        protoc --cpp_out=$dir --grpc_out=$dir \
            --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
            -I$dir $dir/message.proto; \
    done

# 构建所有服务
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# ==================== 运行时 ====================
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-system1.83.0 \
    libboost-filesystem1.83.0 \
    libssl3 \
    libjsoncpp25 \
    libprotobuf32t64 \
    libgrpc++1.51t64 \
    libmysqlcppconn7t64 \
    libhiredis1.1.0 \
    libfmt9 \
    libspdlog1.12 \
    libcrypt1 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 文件上传存储目录（挂载 named volume，容器重启数据不丢）
RUN mkdir -p /app/upload

# 复制所有二进制（CMakeLists 设了 CMAKE_RUNTIME_OUTPUT_DIRECTORY=bin）
COPY --from=builder /build/bin/AuthServer       /app/bin/
COPY --from=builder /build/bin/StatusServer     /app/bin/
COPY --from=builder /build/bin/ChatServer1      /app/bin/
COPY --from=builder /build/bin/ChatServer2      /app/bin/
COPY --from=builder /build/bin/ResourceServer   /app/bin/
COPY --from=builder /build/bin/SeckillServer    /app/bin/

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
