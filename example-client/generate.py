import os
import struct
import socket
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(('10.0.0.4', 9025))
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

def sendFile(filepath, targetpath):
    data = None
    with open(filepath, 'rb') as file:
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



def createSaveGenHeader(entries: dict):
    dataArr = b""
    dataArr += struct.pack("<Q", entries["psnAccountId"])
    dataArr += entries["dirName"].ljust(0x20, "\x00").encode()
    dataArr += entries["titleId"].ljust(0x10, "\x00").encode()
    dataArr += struct.pack("<Q", entries["saveBlocks"])
    dataArr += entries["zipName"].ljust(0x80, "\x00").encode()
    dataArr += entries["title"].ljust(0x80, "\x00").encode()
    dataArr += entries["subtitle"].ljust(0x80, "\x00").encode()
    dataArr += entries["details"].ljust(0x400, "\x00").encode()
    dataArr += struct.pack("<L", entries.get("userParam", 0))
    dataArr += struct.pack("<L", 0) # unknown1
    dataArr += struct.pack("<q", 0) # mtime
    dataArr += b"\x00" * 0x20 # unknown2
    return dataArr

def receiveSave(header: dict):
    sendMagic(0x70200001, 0)

    # check it received it properly
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
        
    print("Received save gen command")
    saveGenHeader = createSaveGenHeader(header)
    print(len(saveGenHeader))
    client.sendall(saveGenHeader)
    
    # check if received save gen packet okay
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return

    # Check if mounted successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print("Mounted")

    # Unmounted mounted successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print("Failed to unmount", hex(statusCode))
    else:
        print("Unmounted")
    # Unmounted mounted successfully
    
    statusCode = getStatusCode()
    if statusCode == 0x70000001:
        statusCode = getStatusCode()
        if statusCode != 0x70000001:
            print("Failed to zip", hex(statusCode))
        else:
            print("Zipped")
            numOfFiles = int.from_bytes(client.recv(1), "little")
            print("File Number", numOfFiles)
            for _ in range(numOfFiles):
                statusCode = getStatusCode()
                if statusCode != 0x70000001:
                    print("Failed to open file", hex(statusCode))
                    continue
                
                statusCode = getStatusCode()
                if statusCode != 0x70000001:
                    print("Failed to seek to the end", hex(statusCode))
                    continue
                
                statusCode = getStatusCode()
                if statusCode != 0x70000001:
                    print("Failed to seek to the beginning", hex(statusCode))
                    continue
                
                print("next file is ready");
                filePathSize = int.from_bytes(client.recv(8), "little")
                print("File Name Size", filePathSize)
                fileSize = int.from_bytes(client.recv(8), "little")
                print("File Size", fileSize);
                fileName = client.recv(filePathSize, socket.MSG_WAITALL)
                relativeFilePath = fileName.decode("utf-8")
                print("File Name", relativeFilePath)
                
                fullPath = relativeFilePath.replace("sdimg_", "")
                print("File Path", fullPath)

                fileData = client.recv(fileSize, socket.MSG_WAITALL)
                
                with open(fullPath, 'wb') as file:
                    file.write(fileData)
                pass
    else:
        print("Did not success in zipping up contents", hex(statusCode))

    # Check if deleted mount successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print("Mounted deleted")

header = {
    "psnAccountId": 0x0000000000000000,
    "dirName": "SAVE",
    "titleId": "CUSA01130",
    "zipName": "",
    "saveBlocks": 320,
    "title": "{} Save".format("ac2pic"),
    "subtitle": "Kat Gravity Rush",
    "details": "When the imposter is sus"
}

receiveSave(header)
