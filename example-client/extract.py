import zipfile
import io
import re

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
    print("Was able to create a file descriptor")

    # Pipe file 
    client.sendall(data)

    # Last status code
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print('Successfully sent file')

def createSaveExtractHeader(entries: dict):
    dataArr = b""
    dataArr += entries["dirName"].ljust(0x20, "\x00").encode()
    dataArr += entries["titleId"].ljust(0x10, "\x00").encode()
    dataArr += entries["zipname"].ljust(0x30, "\x00").encode()
    dataArr += struct.pack("<Q", entries["saveBlocks"])
    return dataArr


def receiveSaveDump(extractHeader: dict, outDir: str = ""):
    sendMagic(0x70200002, 0)


    # it received the command successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return


    dataArr = createSaveExtractHeader(extractHeader)
    client.sendall(dataArr)
    
    # check the gen headers were okay
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return

    # Check that the temp save mounted successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return

    # Check that the temp unmounted successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return

    # Check that zip extraction was successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return

    # Check that the true mount was successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return


    numOfFiles = int.from_bytes(client.recv(1), "little")
    print("File Number", numOfFiles)
    for _ in range(numOfFiles):
        statusCode = getStatusCode()
        if statusCode != 0x70000001:
            print("Failed to open file", hex(statusCode))
            continue

        statusCode = getStatusCode()
        if statusCode != 0x70000001:
            print("Failed to seek to end of file", hex(statusCode))
            continue

        statusCode = getStatusCode()
        if statusCode != 0x70000001:
            print("Failed to seek to beginning of file", hex(statusCode))
            continue
        
        print("next file is ready");
        filePathSize = int.from_bytes(client.recv(8), "little")
        print("File Name Size", filePathSize)
        fileSize = int.from_bytes(client.recv(8), "little")
        print("File Size", fileSize)
        fileName = client.recv(filePathSize, socket.MSG_WAITALL)
        relativeFilePath = fileName.decode("utf-8")
        print("File Name", relativeFilePath)
        
        fullPath = outDir + relativeFilePath
        print("File Path", fullPath)
        os.makedirs(os.path.dirname(fullPath) or '.', exist_ok=True)
        # Pipe this
        fileData = client.recv(fileSize, socket.MSG_WAITALL)
        
        with open(fullPath, 'wb') as file:
            file.write(fileData)

    # Check that the true unmount was successful
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print("Save was unmounted");

    # Check that the save was deleted successfully
    statusCode = getStatusCode()
    if statusCode != 0x70000001:
        print(hex(statusCode))
        return
    print("Save was deleted.");
    


def openZip(filePath):
    data = io.BytesIO(filePath)
    zf = None
    try:
        zf = zipfile.ZipFile(data, "r")
    except zipfile.BadZipFile as e:
        return e.args[0], None
    except zipfile.LargeZipFile as e:
        return e.args[0], None
    except Exception as e:
        return e, None
    return None, zf

def getSaveFiles(zf, files):
    unique_files = {}
    for entry in zf.infolist():
        if entry.is_dir():
            continue
    
        # Zip allows multiple entries to have the same name
        if unique_files.get(entry.filename, False) == True:
            continue

        unique_files[entry.filename] = True


        matched = re.search(r"PS4\/SAVEDATA\/[\da-fA-F]{16}\/[\w\d]+\/(.*)$", entry.filename)
        if not matched:
            continue
        
        matchOffset = matched.start()
        if matchOffset > 0:
            rootDir = entry.filename[:matchOffset]
            badPath = entry.filename
            goodPath = entry.filename[matchOffset:]
            return f'Move the PS4 folder out of "{rootDir}" so that "{badPath}" becomes "{goodPath}". '
        files.append(entry.filename)

    if len(files) != len(unique_files):
        return "Not all files in zip are necessary", None
    return None, ""


import shutil

def prepareArguments(zipPath: str):
    zipdata = open(zipPath, 'rb')
    err, zf = openZip(zipdata.read())
    zipdata.close()
    if err:
        return err, None
    files = []
    err, _ = getSaveFiles(zf, files)
    if err:
        return err, None
    

    if len(files) != 2:
        return 'Must only have 2 files in the zip!', None
    if files[0].replace('.bin', '') != files[1].replace('.bin', ''):
        return "Must have correct pair of save files!", None
    
    genericForm = files[0]
    if genericForm.endswith('.bin'):
        genericForm = genericForm[0:len(genericForm) - 4]
    matched = re.search(r"PS4\/SAVEDATA\/([0-9a-fA-F]{16})\/([A-Z]{4}[0-9]{5})\/(.*)$", genericForm)
    psnId, titleId, dirName = matched.groups()
    trueFileName = os.path.basename(genericForm)
    os.makedirs('ex', exist_ok=True)

    # Determine this before sending anything
    extractHeader = {
        "dirName": dirName,
        "titleId": titleId,
        "zipname": "",
        "saveBlocks": int(zf.getinfo(genericForm).file_size/32768)
    }
    zfFixed = zipfile.ZipFile("PS4_fixed.zip", "w")
    with zf.open(genericForm) as entry, zfFixed.open('sdimg_' + trueFileName, 'w') as f:
        shutil.copyfileobj(entry, f)
    with zf.open(genericForm + '.bin') as entry, zfFixed.open(trueFileName + '.bin', 'w') as f:
        shutil.copyfileobj(entry, f)

    zfFixed.close()
    return extractHeader

extractHeader = prepareArguments("PS41.zip")
extractHeader["zipname"] = "PS4.zip"
print(extractHeader)
sendZip("PS4_fixed.zip", "PS4.zip")
receiveSaveDump(extractHeader)
