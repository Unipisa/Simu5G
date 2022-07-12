# Author: Alessandro Noferi
#
# This script has been tested with Python 3.6.9

import socket
import time
from struct import * 
import requests
import json

# try to import C parser then fallback in pure python parser.
try:
    from http_parser.parser import HttpParser
except ImportError:
    from http_parser.pyparser import HttpParser

localPort  = 4022
bufferSize = 64

# Create a datagram socket

UeSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
ServiceSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)

# Bind to address and ip

UeSocket.bind(('', localPort))

print("Mec Application up and listening on port {}".format(localPort))

# Listen for incoming datagrams
bytesAddressPair = UeSocket.recvfrom(bufferSize)

message = bytesAddressPair[0]
address = bytesAddressPair[1]

code = unpack_from('<b', message, 0)[0]
length = unpack_from('<b', message, 1)[0]
data = unpack_from("%ds" % length, message, 2)[0]

msg = "Message from UE app: code {}, data length {}, coords {} from {}".format(code, length, data.decode("utf-8"), address)
print("raw message: "+message.decode('utf-8'))
print(msg)
x, y, r = data.decode("utf-8").split(",")

#
# NOTE the MEC app is able to to know the real address of the UE app only if it is present in the START message.
# To test this simple scenario a static solution has been used, i.e. I know the address of the UE (10.0.0.1)
ueAddress = "10.0.0.1"
body =  "{  \"circleNotificationSubscription\": {"\
        "\"callbackReference\" : {"\
        "\"callbackData\":\"1234\","\
        "\"notifyURL\":\"example.com/notification/1234\"},"\
        "\"checkImmediate\": \"false\","\
        "\"address\": \""+ ueAddress +"\","\
        "\"clientCorrelator\": \"notUsed\","\
        "\"enteringLeavingCriteria\": \"Entering\","\
        "\"frequency\": 5,"\
        "\"radius\":" + r +","\
        "\"trackingAccuracy\": 10,"\
        "\"latitude\":" + x +","\
        "\"longitude\": " + y +""\
        "}"\
        "}\r\n"


# retrieve Location service address
#URL = "http://10.0.5.2:10021/example/mec_service_mgmt/v1/services"
#ser_name = "LocationService"
#PARAMS = {'ser_name':ser_name}
#r = requests.get(url = URL, params = PARAMS)
#res = r.json()

#locationAddress = str(res[0]["transportInfo"]["endPoint"]["addresses"]["host"])
#locationPort = int(res[0]["transportInfo"]["endPoint"]["addresses"]["port"])

# Use static address of the MEC service, the above method does not works (Response type error on successive instructions
# It is possible, however, to manually connect to the service registry and retrieve the endpoint
locationAddress = "192.168.2.1"
locationPort = 10020

pload = "POST /example/location/v2/subscriptions/area/circle HTTP/1.1\r\n"\
        "Host: "+locationAddress+":"+str(locationPort)+"\r\n"\
        "Content-Type: application/json\r\n"\
        "Content-Length: "+str(len(body))+"\r\n"\
        "\r\n"+body

print("Connecting to the Locatione Service")
ServiceSocket.connect((locationAddress, locationPort))
print("Connected to the Location Service")
ServiceSocket.send(str.encode(pload))
print("Subscription request sent!")

# send ACK start to the UE app
ack_msg = pack('!BB', 3, 0)

UeSocket.sendto(ack_msg, address)

p = HttpParser()
resBody = []

# Wait for 204 response from the service
while True:
    data = ServiceSocket.recv(1024)
    if not data:
        print("No data")
        break

    recved = len(data)
    nparsed = p.execute(data, recved)
    assert nparsed == recved

    if p.is_partial_body():
        resBody.append(p.recv_body().decode('utf-8'))

    if p.is_message_complete():
        break

# print(p.get_headers())
print("Subscription created! Waiting for notification...\n" + "".join(resBody) + "\n")

p = HttpParser()
resBody = []

# Wait for an event notification from the service
while True:
    data = ServiceSocket.recv(1024)
    if not data:
        print("No data")
        break

    recved = len(data)
    nparsed = p.execute(data, recved)
    assert nparsed == recved

    if p.is_partial_body():
        resBody.append(p.recv_body().decode('utf-8'))

    if p.is_message_complete():
        break

res = json.loads("".join(resBody))

print("Event Notification arrived!!\n" + json.dumps(res,indent=4)+ "\n")

coordinates = res['subscriptionNotification']['terminalLocationList']['currentLocation']
coords = str(coordinates['x'])+","+str(coordinates['y'])
print("coordinates are: {}".format(coords))
alert_msg = pack('!BBB', 2, len(coords), True)
alert_msg = alert_msg + bytes(coords, 'utf-8')

#send alert to ue
UeSocket.sendto(alert_msg, address)
print("Message to ueApp sent")

# modify subscription
pl = {
  "circleNotificationSubscription": {
  "callbackReference":  {
    "callbackData": "1234",
    "notifyURL": "example.com/notification/1234"
   },
  "checkImmediate" : "false",
  "address" : ueAddress,
  "clientCorrelator" : "null",
  "enteringLeavingCriteria": "Leaving",
  "frequency" : 5,
  "radius" : int(r),
  "trackingAccuracy" : 10,
  "latitude" : int(x),
  "longitude" : int(y)
  }
}

plBody = json.dumps(pl)

pload = "PUT /example/location/v2/subscriptions/area/circle/0 HTTP/1.1\r\n"\
        "Host: "+locationAddress+":"+str(locationPort)+"\r\n"\
        "Content-Type: application/json\r\n"\
        "Content-Length: "+str(len(plBody))+"\r\n"\
        "\r\n"+plBody


ServiceSocket.send(str.encode(pload))
print("Subscription PUT request sent!")

p = HttpParser()
resBody = []

# Wait for 204 response from the service
while True:
    data = ServiceSocket.recv(1024)
    if not data:
        print("No data")
        break

    recved = len(data)
    nparsed = p.execute(data, recved)
    assert nparsed == recved

    if p.is_partial_body():
        resBody.append(p.recv_body().decode('utf-8'))

    if p.is_message_complete():
        break

print(p.get_headers())
print("Subscription created! Waiting for notification...\n" + "".join(resBody) + "\n")

p = HttpParser()
resBody = []

# Wait for an event notification from the service
while True:
    data = ServiceSocket.recv(1024)
    if not data:
        print("No data")
        break

    recved = len(data)
    nparsed = p.execute(data, recved)
    assert nparsed == recved

    if p.is_partial_body():
        resBody.append(p.recv_body().decode('utf-8'))

    if p.is_message_complete():
        break

res = json.loads("".join(resBody))
print("Event Notification arrived!!\n" + json.dumps(res,indent=4)+ "\n")

coordinates = res['subscriptionNotification']['terminalLocationList']['currentLocation']

coords = str(coordinates['x'])+","+str(coordinates['y'])
alert_msg = pack('!BBB', 2, len(coords), False)
alert_msg = alert_msg + str.encode(coords)

UeSocket.sendto(alert_msg, address)
print("Message to ueApp sent")
ServiceSocket.close()
