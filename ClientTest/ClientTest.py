import socket
import time

s = socket.socket()
s.connect(('localhost', 27062))
s.send(b"hello from python script ")
time.sleep(1)
s.send(b"bye")
r = s.recv(1024)
print(r)
input()
s.close()
