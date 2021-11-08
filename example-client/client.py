import os

import socket
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(('10.0.0.4', 9025))
from_server = client.recv(4)
# client.close()
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
    sendMagic(0x80200000, len(targetpath))
    statusCode = getStatusCode()
    if statusCode != 0x80000001:
        print(hex(statusCode))
        print(int.from_bytes(client.recv(4), "little", signed=True))
        return
    
    client.send(targetpath.encode())
    client.send(len(data).to_bytes(4, "little"))
    client.send(data)
    statusCode = getStatusCode()
    if statusCode != 0x80000001:
        print(hex(statusCode))
        return
    pass


def createSaveGenHeader(dirName: str, titleId: str, copyDirectory: str):
    dataArr = dirName.ljust(0x20, "\x00").encode()
    dataArr += titleId.ljust(0x10, "\x00").encode()
    dataArr += copyDirectory.ljust(0x30, "\x00").encode()
    return dataArr

def receiveSave(out_dir):
    
    os.makedirs(out_dir, exist_ok=True)
    
    sendMagic(0x80200001, 0)

    # check it received it properly
    statusCode = getStatusCode()
    if statusCode != 0x80000001:
        print(hex(statusCode))
        return

    dataArr = createSaveGenHeader("data0000", "CUSA04943", "test0123")
    client.send(dataArr)
    
    # check the headers were okay
    # and any steps for getting save data
    # was okay
    statusCode = getStatusCode()
    if statusCode != 0x80000001:
        print(hex(statusCode))
        return

    statusCode = getStatusCode()
    if statusCode != 0x80000001:
        print(hex(statusCode))
        return

    numOfFiles = int.from_bytes(client.recv(1), "little")
    print("File Number", numOfFiles)
    for _ in range(numOfFiles):
        statusCode = getStatusCode()
        if statusCode != 0x80000001:
            print("Failed status code", hex(statusCode))
            continue
        print("next file is ready");
        filePathSize = int.from_bytes(client.recv(8), "little")
        print("File Name Size", filePathSize)
        fileSize = int.from_bytes(client.recv(8), "little")
        print("File Size", fileSize);
        fileName = client.recv(filePathSize, socket.MSG_WAITALL)
        relativeFilePath = fileName.decode("utf-8")
        print("File Name", relativeFilePath)
        
        fullPath = out_dir + relativeFilePath.replace("sdimg_", "")
        print("File Path", fullPath)
        os.makedirs(os.path.dirname(fullPath), exist_ok=True)
        

        fileData = client.recv(fileSize, socket.MSG_WAITALL)
        
        with open(fullPath, 'wb') as file:
            file.write(fileData)
        pass
    
    statusCode = getStatusCode()
    if statusCode != 0x80000001:
        print(hex(statusCode))
        return
    

sendFile(r"data0000.bin", "test0123/data0000.bin")
receiveSave(r"Z:\PS4\SAVEDATA\5e8f03d08be1dbbc\ ".strip())