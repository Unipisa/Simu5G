import socket
import time
from struct import * 

code = 1
length = 23

devAppAddressPort   = ("192.168.3.1", 4500)
bufferSize  = 50


# Create a UDP socket at client side
UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Send to server using created UDP socket
appName = "MECWarningAlertApp"
bytesToSend = pack('!BB', 0, len(appName))
bytesToSend = bytesToSend + bytes(appName, 'utf-8')
UDPClientSocket.sendto(bytesToSend, devAppAddressPort)

print("Message to device application sent")

msgFromServer = UDPClientSocket.recvfrom(bufferSize) 

respMsg = msgFromServer[0]

code = unpack_from('<b', respMsg, 0)[0]
length = unpack_from('<b', respMsg, 1)[0]
data = unpack_from("%ds" % length, respMsg, 2)[0]

msg = "Message from device application: code {}, data length {}, endpoint {}".format(code, length, data.decode("utf-8"))
print(msg)

# mec app endpoint
ip, port = data.decode("utf-8").split(":")

# red zone coordinates
coords = "210,260,60"
 
start_msg = pack('!BB', 0, len(coords))
start_msg = start_msg + str.encode(coords)
 
UDPClientSocket.sendto(start_msg, (ip, int(port)))
 
print("Message to mec application sent to {} {}".format(ip, int(port)))
 
while(True):
    msgFromServer = UDPClientSocket.recvfrom(bufferSize) 
    respMsg = msgFromServer[0]
    addressPort = msgFromServer[1]
    code = unpack_from('<b', respMsg, 0)[0]
    print("Message arrived from :{}".format(addressPort))
    if(addressPort == (ip, int(port))): # mec app
        if(int(code) == 2): 
            # car enters/leaves the circle
            length = unpack_from('<b', respMsg, 1)[0]
            res = unpack_from('<b', respMsg, 2)[0]
            data = unpack_from("%ds" % length, respMsg, 3)[0]
            msg = "Message from Server: code {}, alert status {}, position {}".format(code, res, data.decode("utf-8"))
            print(msg)
            if(bool(res) == False):
                # car leaves the circle
                print("sending delete to the Device App to remove the MEC App")
                bytes = pack('!BB', 1, len(appName))
                bytes = bytes + str.encode(appName)
                time.sleep(1)
                UDPClientSocket.sendto(bytes, devAppAddressPort)
        else:
            # code unrecognized
            print("Code {} unrecognized".format(code))
    elif(addressPort == devAppAddressPort): # from dev app
        length = unpack_from('<b', respMsg, 1)[0]
        data = unpack_from("%ds" % length, respMsg, 2)[0]
        msg = "Message from the device app {}".format(data.decode("utf-8"))
        print(msg)
        break

