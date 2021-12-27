import os
import struct
import socket
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(('10.0.0.5', 9025))
from_server = client.recv(4)
print(from_server)

def sendMagic(cmd, argLength):
    MAGIC = 0x98696b77
    headerBytes = MAGIC.to_bytes(4, "little")
    headerBytes += cmd.to_bytes(4, "little")
    headerBytes += argLength.to_bytes(4, "little")
    client.send(headerBytes)

def getStatusCode():
    statusCode = int.from_bytes(client.recv(4, socket.MSG_WAITALL), "little")
    return statusCode

def sendZip(zipPath, targetpath):
    data = None
    with open(zipPath, 'rb') as file:
        data = file.read()
    # Check if param is valid
    sendMagic(0x70200000, len(targetpath))
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        client.close()
        return
    print("Param is valid param")
    
    client.sendall(targetpath.encode())
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        client.close()
        return
    print('Successfully sent file path')

    client.sendall(len(data).to_bytes(4, "little"))

    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        client.close()
        return
    print('Successfully sent file size')

    # Check if able to open file
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print("Was able to get a file descriptor")

    
    client.sendall(data)

    # Last status code
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print('Successfully sent file')

# sendZip("local.zip", "PS4.zip")
