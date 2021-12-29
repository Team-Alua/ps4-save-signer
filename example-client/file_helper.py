import socket
import os


RECEIVE_SIZE = 49152
def pipe_read_in(socketHandle, filePath, size):
    with open(filePath, 'wb') as fileHandle:
        offset = 0
        while size > 0:
            buffer_size = min(RECEIVE_SIZE, size)
            buffer = socketHandle.recv(buffer_size)
            if not buffer:
                break
            fileHandle.write(buffer)
            buffer_read = len(buffer)
            offset += buffer_read
            size -= buffer_read
    return size == 0

# Adjust this to match how many bytes
# are sent until the amount of requested to send is greater than what is
# actually sent. Set it to the amount actually sent
SEND_SIZE = 49152

def pipe_write_out(socketHandle, filePath):
    sent_all = False
    with open(filePath, 'rb') as fileHandle:
        offset = 0
        fileHandle.seek(0, os.SEEK_END)
        size = fileHandle.tell()
        fileHandle.seek(0, os.SEEK_SET)
        while offset < size:
            # Ad
            buffer_size = min(SEND_SIZE, size - offset)
            buffer = fileHandle.read(buffer_size)
            sent = socketHandle.send(buffer)
            if not sent:
                break
            offset += sent
            if sent < buffer_size:
                fileHandle.seek(offset, os.SEEK_SET)
        sent_all = size == offset
    return sent_all

def get_size(filePath):
    size = 0
    with open(filePath, 'rb') as f:
        f.seek(0, os.SEEK_END)
        size = f.tell()
    return size