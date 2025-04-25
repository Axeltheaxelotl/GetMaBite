import socket
s = socket.socket()
s.connect(('localhost', 8081))
s.send(b"POST / HTTP/1.1\r\nHost: localhost:8081\r\nContent-Length: 13\r\n\r\n")
s.send(b"test=fragment")
print(s.recv(4096).decode())
s.close()
