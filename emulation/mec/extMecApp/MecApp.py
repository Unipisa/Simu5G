import socket
import time
from struct import * 


import socket

localPort   = 4022
bufferSize = 64

# Create a datagram socket

UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

 

# Bind to address and ip

UDPServerSocket.bind(('', localPort))

print("Mec Applciation up and listening on port {}".format(localPort))

# Listen for incoming datagrams

while(True):

    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)

    message = bytesAddressPair[0]

    address = bytesAddressPair[1]

    code = unpack_from('<b', message, 0)[0]
    length = unpack_from('<b', message, 1)[0]
    data = unpack_from("%ds" % length, message, 2)[0]

    msg = "Message from Server: code {}, data length {}, coords {} from {}".format(code, length, data.decode("utf-8"), address)
    print(msg)


    coords = "56,85"    
    alert_msg = pack('!BBB', 2, True, len(coords))
    alert_msg = alert_msg + str.encode(coords)

    UDPServerSocket.sendto(alert_msg, address)
    print("Message to ueApp sent")

    #send alert to ue
    




































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
print("Message to device application sent")

msgFromServer = UDPClientSocket.recvfrom(bufferSize) 

respMsg = msgFromServer[0]

code = unpack_from('<b', respMsg, 0)[0]
length = unpack_from('<b', respMsg, 1)[0]
data = unpack_from("%ds" % length, respMsg, 2)[0]

msg = "Message from Server: code {}, data length {}, endpoint {}".format(code, length, data.decode("utf-8"))
print(msg)

ip, port = data.decode("utf-8").split(":")

coords = "210,260,60"

start_msg = pack('!BB', 0, len(coords))
start_msg = start_msg + str.encode(coords)

UDPClientSocket.sendto(start_msg, (ip, int(port)))

print("Message to mec application sent")

msgFromServer = UDPClientSocket.recvfrom(bufferSize) 
respMsg = msgFromServer[0]

# car enters the circle

code = unpack_from('<b', respMsg, 0)[0]
length = unpack_from('<b', respMsg, 1)[0]
res = unpack_from('<b', respMsg, 2)[0]

data = unpack_from("%ds" % length, respMsg, 3)[0]

msg = "Message from Server: code {}, alert status {}, position {}".format(code, res, data.decode("utf-8"))
print(msg)

# car exits the circle
msgFromServer = UDPClientSocket.recvfrom(bufferSize) 
respMsg = msgFromServer[0]

code = unpack_from('<b', respMsg, 0)[0]
length = unpack_from('<b', respMsg, 1)[0]
res = unpack_from('<b', respMsg, 2)[0]

data = unpack_from("%ds" % length, respMsg, 3)[0]

msg = "Message from Server: code {}, alert status {}, position {}".format(code, res, data.decode("utf-8"))
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



