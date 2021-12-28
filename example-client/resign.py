
import os
import struct
import socket
from file_helper import pipe_read_in, pipe_write_out, get_size
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(('10.0.0.4', 9025))
from_server = client.recv(4)
print(from_server)

def createSaveResignHeader(entries: dict):
    dataArr = b""
    dataArr += struct.pack("<Q", entries["psnAccountId"])
    dataArr += struct.pack("<Q", entries["targetPsnAccountId"])
    dataArr += entries["dirName"].ljust(0x20, "\x00").encode()
    dataArr += entries["titleId"].ljust(0x10, "\x00").encode()
    dataArr += entries["zipName"].ljust(0x30, "\x00").encode()
    dataArr += struct.pack("<Q", entries["saveBlocks"])
    return dataArr


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

    client.sendall(get_size(zipPath).to_bytes(4, "little"))

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


    pipe_write_out(client, zipPath)

    # Last status code
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print('Successfully sent file')


def receiveResignedSave(header: dict):
    sendMagic(0x70200004, 0)

    # check it received it properly
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return

    print("Received save resign command")
    saveResignHeader = createSaveResignHeader(header)
    print(len(saveResignHeader))
    client.sendall(saveResignHeader)
    
    # check if received save resigned packet okay
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print("Received packet")
    # Check if temp mounted successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print("Failed to mount temp save", hex(statusCode))
        return
    print("Mounted temp save")

    # Unmounted temp unmounted successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print("Failed to unmount temp save", hex(statusCode))
    else:
        print("Unmounted temp save")
    # Unmounted mounted successfully

    # Check if extraction was successful
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print("Failed to extract zip", hex(statusCode))
    else:
        print("Extracted zip")

        # Check if mount was successful
        statusCode = getStatusCode()
        if statusCode != 0x70000001:
            print("Failed to unmount real save", hex(statusCode))
        else:
            print("Unmounted real save")


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
    "psnAccountId": 0x0000000000000001,
    "targetPsnAccountId": 0x0000000000000005,
    "dirName": "data0000",
    "titleId": "CUSA04943",
    "zipName": "PS4_test.zip",
    "saveBlocks": 114,
}
sendZip("PS4.zip", "PS4_test.zip")

receiveResignedSave(header)