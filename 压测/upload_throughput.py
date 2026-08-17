#!/usr/bin/env python3
# upload_throughput.py —— 场景 8：文件传输吞吐（滑动窗口 8 片）
# 协议: ResourceServer [2B msg_id 大端][4B len 大端] + JSON body
# 用法: python3 upload_throughput.py <host> <port> <uid> <file_size_mb> <jwt_secret>
# 例:   python3 upload_throughput.py 127.0.0.1 9090 1000 10 im-jwt-secret-2026

import socket, struct, json, time, base64, hashlib, hmac, sys, threading

# 注意：ResourceServer 文件上传实际注册的 handler 是 ID_IMAGE_CHAT_MSG_REQ(1034)/RSP(1035)，
# ID_UPLOAD_FILE_REQ(1028) 是未注册的废弃枚举值，服务端会报"无法找到 msg_id=1028 的回调函数"
ID_IMAGE_CHAT_MSG_REQ, ID_IMAGE_CHAT_MSG_RSP = 1034, 1035
CHUNK, WINDOW = 2048, 8


def gen_jwt(uid: int, secret: str) -> str:
    def e(s): return base64.urlsafe_b64encode(s).rstrip(b"=").decode()
    now = int(time.time())
    h = e(b'{"alg":"HS256","typ":"JWT"}')
    p = e(f'{{"uid":{uid},"username":"u{uid}","client_type":"desktop","iat":{now},"exp":{now+86400}}}'.encode())
    sig = hmac.new(secret.encode(), (h + "." + p).encode(), hashlib.sha256).hexdigest()
    return f"{h}.{p}.{sig}"


def frame(mid: int, body: bytes) -> bytes:
    return struct.pack(">h", mid) + struct.pack(">i", len(body)) + body


def recv_frame(sock) -> tuple:
    hdr = b""
    while len(hdr) < 6:
        b = sock.recv(6 - len(hdr))
        if not b:
            raise ConnectionError("closed")
        hdr += b
    mid, ln = struct.unpack(">hi", hdr)
    body = b""
    while len(body) < ln:
        body += sock.recv(ln - len(body))
    return mid, body


def run(host: str, port: int, uid: int, total_bytes: int, secret: str) -> float:
    s = socket.create_connection((host, port))
    total_chunks = total_bytes // CHUNK
    fn = f"tp_{uid}_{int(time.time())}.bin"
    md5 = hashlib.md5(f"{uid}{total_bytes}".encode()).hexdigest()
    token = gen_jwt(uid, secret)
    acked = set()
    base, nxt = 1, 1
    t0 = time.time()

    def reader():
        while True:
            try:
                mid, body = recv_frame(s)
            except Exception:
                return
            if mid == ID_IMAGE_CHAT_MSG_RSP:
                r = json.loads(body)
                seq = r.get("seq") or r.get("last_ack_seq")
                if seq:
                    acked.add(int(seq))

    threading.Thread(target=reader, daemon=True).start()
    chunk = base64.b64encode(b"x" * CHUNK).decode()
    while base <= total_chunks:
        while nxt < base + WINDOW and nxt <= total_chunks:
            body = json.dumps({
                "filename": fn, "seq": nxt, "lastseq": nxt - 1,
                "transferredsize": (nxt - 1) * CHUNK, "totolsize": total_bytes,
                "data": chunk, "md5": md5, "type": 0, "uid": uid, "token": token,
            }).encode()
            s.sendall(frame(ID_IMAGE_CHAT_MSG_REQ, body))
            nxt += 1
        while base in acked:
            base += 1
    dt = time.time() - t0
    s.close()
    return total_bytes / dt / 1024 / 1024  # MB/s


if __name__ == "__main__":
    if len(sys.argv) < 6:
        print("usage: python3 upload_throughput.py <host> <port> <uid> <file_size_mb> <jwt_secret>")
        sys.exit(1)
    host, port, uid = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    mb, secret = int(sys.argv[4]), sys.argv[5]
    for _ in range(3):
        print(f"{run(host, port, uid, mb * 1024 * 1024, secret):.1f} MB/s")
