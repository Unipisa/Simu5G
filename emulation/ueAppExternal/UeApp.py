import socket
import time
from struct import * 

msgFromClient       = "123"

bytesToSend         = str.encode(msgFromClient)

code = 1
length = 23

serverAddressPort   = ("192.168.3.1", 4500)

bufferSize          = 50


# Create a UDP socket at client side


UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
#UDPClientSocket.bind('192.168.2.1', 8740)
# Send to server using created UDP socket

appName = "MEWarningAlertApp_rest"
bytes = pack('!BB', 0, len(appName))
bytes = bytes + str.encode(appName)
UDPClientSocket.sendto(bytes, serverAddressPort)

msgFromServer = UDPClientSocket.recvfrom(bufferSize) 

respMsg = msgFromServer[0]

length = unpack_from('<b', respMsg, 1)[0]

print(length)

msg = "Message from Server {}".format(msgFromServer[0])
print(msg)

print("sending delete")
appName = "MEWarningAlertApp_rest"
bytes = pack('!BB', 1, len(appName))
bytes = bytes + str.encode(appName)

time.sleep(1)
UDPClientSocket.sendto(bytes, serverAddressPort)

msgFromServer = UDPClientSocket.recvfrom(bufferSize) 
msg = "Message from Server {}".format(msgFromServer[0].decode("utf-8"))
print(msg)



