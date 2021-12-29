
import os
import struct
import socket
from file_helper import pipe_read_in, pipe_write_out, get_size
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(('10.0.0.5', 9025))
from_server = client.recv(4)
print(from_server)

def createSaveResignHeader(entries: dict):
    dataArr = b""
    dataArr += struct.pack("<Q", entries["psnAccountId"])
    dataArr += struct.pack("<Q", entries["targetPsnAccountId"])
    dataArr += entries["dirName"].ljust(0x20, "\x00").encode()
    dataArr += entries["titleId"].ljust(0x10, "\x00").encode()
    dataArr += entries["zipName"].ljust(0x80, "\x00").encode()
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


def receiveResignedSave(header: dict, outDir = ''):
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
                    
                    print("next file is ready")
                    filePathSize = int.from_bytes(client.recv(8), "little")
                    print("File Name Size", filePathSize)
                    fileSize = int.from_bytes(client.recv(8), "little")
                    print("File Size", fileSize)
                    fileName = client.recv(filePathSize, socket.MSG_WAITALL)
                    relativeFilePath = fileName.decode("utf-8")
                    print("File Name", relativeFilePath)
                    
                    fullPath = relativeFilePath
                    print("File Path", fullPath)
                    fullPath = outDir + header["zipName"]
                    print("File Path", fullPath)
                    os.makedirs(os.path.dirname(fullPath) or '.', exist_ok=True)
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

import re
import zipfile 
import io
def openZip(filePath):
    zf = None
    try:
        zf = zipfile.ZipFile(filePath, "r")
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



def prepareArguments(zipPath: str):
    err, zf = openZip(zipPath)
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

    # Determine this before sending anything
    extractHeader = {
        "psnAccountId": int("0x" + psnId, 16),
        "dirName": dirName,
        "titleId": titleId,
        "zipName": "",
        "saveBlocks": int(zf.getinfo(genericForm).file_size/32768)
    }
    zf.close()
    return extractHeader



def do(local_zip_path, ps4_zip_path, targetPsnId, outDir = ""):
    header = prepareArguments(local_zip_path)
    header["targetPsnAccountId"] = targetPsnId
    header["zipName"] = ps4_zip_path

    sendZip(local_zip_path, ps4_zip_path)

    receiveResignedSave(header, outDir)