#!/usr/bin/env python3
# reliable_upload.py —— 场景 2：文件分片可靠送达（验证 20% 丢包下的滑动窗口重传）
# 协议: ResourceServer [2B msg_id 大端][4B len 大端] + JSON body
# 用法: python3 reliable_upload.py

import socket, struct, json, time, base64, threading, hashlib, hmac

HOST, PORT = "127.0.0.1", 9090
ID_UPLOAD_FILE_REQ, ID_UPLOAD_FILE_RSP = 1028, 1029
CHUNK = 2048
WINDOW = 8
TIMEOUT = 0.5
SECRET = "im-jwt-secret-2026"


def gen_jwt(uid: int, secret: str = SECRET) -> str:
    def e(s): return base64.urlsafe_b64encode(s).rstrip(b"=").decode()
    now = int(time.time())
    header = e(b'{"alg":"HS256","typ":"JWT"}')
    payload = e(f'{{"uid":{uid},"username":"u{uid}","client_type":"desktop","iat":{now},"exp":{now+86400}}}'.encode())
    sig = hmac.new(secret.encode(), (header + "." + payload).encode(), hashlib.sha256).hexdigest()
    return f"{header}.{payload}.{sig}"


def frame(msg_id: int, body: bytes) -> bytes:
    return struct.pack(">h", msg_id) + struct.pack(">i", len(body)) + body


def recv_frame(sock) -> tuple:
    hdr = b""
    while len(hdr) < 6:
        b = sock.recv(6 - len(hdr))
        if not b:
            raise ConnectionError("closed")
        hdr += b
    msg_id, ln = struct.unpack(">hi", hdr)
    body = b""
    while len(body) < ln:
        body += sock.recv(ln - len(body))
    return msg_id, body


def upload(uid: int, token: str, total: int):
    s = socket.create_connection((HOST, PORT))
    filename = f"bench_{uid}_{int(time.time())}.bin"
    md5 = hashlib.md5(f"{uid}{total}".encode()).hexdigest()
    pending = {}   # seq -> 发送时间
    acked = set()
    base, nxt = 1, 1

    def send_one(seq):
        body = json.dumps({
            "filename": filename, "seq": seq, "lastseq": seq - 1,
            "transferredsize": (seq - 1) * CHUNK, "totolsize": total * CHUNK,
            "data": base64.b64encode(b"x" * CHUNK).decode(),
            "md5": md5, "type": 0, "uid": uid, "token": token,
        }).encode()
        s.sendall(frame(ID_UPLOAD_FILE_REQ, body))
        pending[seq] = time.time()

    def reader():
        while True:
            try:
                mid, body = recv_frame(s)
            except Exception:
                return
            if mid == ID_UPLOAD_FILE_RSP:
                r = json.loads(body)
                seq = r.get("seq") or r.get("last_ack_seq")
                if seq:
                    acked.add(int(seq))

    threading.Thread(target=reader, daemon=True).start()
    end = time.time() + 30
    while base <= total and time.time() < end:
        # 发送窗口内未发送的分片
        while nxt < base + WINDOW and nxt <= total:
            send_one(nxt)
            nxt += 1
        # 超时重传未 ACK 的分片
        now = time.time()
        for seq, t in list(pending.items()):
            if seq not in acked and now - t > TIMEOUT:
                send_one(seq)
        # 推进窗口：连续 ACK 的分片
        while base in acked:
            base += 1
        time.sleep(0.01)
    s.close()
    return len(acked), total


if __name__ == "__main__":
    uid, total = 1000, 1000
    token = gen_jwt(uid)
    got, alln = upload(uid, token, total)
    print(f"ACK {got}/{alln}, 到达率 {got/alln*100:.1f}%")
