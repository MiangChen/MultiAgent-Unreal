#!/usr/bin/env python3
"""
TCP Stream Viewer - 接收 MACameraSensor 的视频流
用法: python tcp_stream_viewer.py [port]
默认端口: 9000
"""

import socket
import struct
import sys
import numpy as np

try:
    import cv2
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False
    print("Warning: OpenCV not installed. Install with: pip install opencv-python")

def receive_frame(sock):
    """接收一帧图像"""
    # 读取 4 字节帧大小
    size_data = b''
    while len(size_data) < 4:
        chunk = sock.recv(4 - len(size_data))
        if not chunk:
            return None
        size_data += chunk
    
    frame_size = struct.unpack('<I', size_data)[0]
    
    # 读取 JPEG 数据
    jpeg_data = b''
    while len(jpeg_data) < frame_size:
        chunk = sock.recv(min(frame_size - len(jpeg_data), 65536))
        if not chunk:
            return None
        jpeg_data += chunk
    
    return jpeg_data

def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9000
    host = sys.argv[2] if len(sys.argv) > 2 else '127.0.0.1'
    
    print(f"Connecting to {host}:{port}...")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    print("Connected! Receiving frames...")
    
    frame_count = 0
    
    try:
        while True:
            jpeg_data = receive_frame(sock)
            if jpeg_data is None:
                print("Connection closed")
                break
            
            frame_count += 1
            
            if HAS_CV2:
                # 解码并显示
                frame = cv2.imdecode(np.frombuffer(jpeg_data, np.uint8), cv2.IMREAD_COLOR)
                if frame is not None:
                    cv2.imshow(f'Camera Stream (Port {port})', frame)
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        break
            else:
                # 无 OpenCV，只打印信息
                if frame_count % 30 == 0:
                    print(f"Received frame {frame_count}, size: {len(jpeg_data)} bytes")
    
    except KeyboardInterrupt:
        print("\nStopped by user")
    finally:
        sock.close()
        if HAS_CV2:
            cv2.destroyAllWindows()
        print(f"Total frames received: {frame_count}")

if __name__ == '__main__':
    main()
