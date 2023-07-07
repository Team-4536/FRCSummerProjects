import struct
import socket
import numpy
from enum import Enum
import select




class MessageKind(Enum):
    UPDATE = 0

class MessageUpdateType(Enum):
    S32 = 0
    F64 = 1
    STR = 2

class Message():

    def __init__(self, kind: MessageKind, name: str, value: float|int|str) -> None:

        self.content = b""


        valType = 0
        valEncoded = b""

        if(type(value) == int):
            valType = MessageUpdateType.S32
            valEncoded += value.to_bytes(4, 'little', signed=True)

        elif(type(value) == float or type(value) == numpy.float64):
            valType = MessageUpdateType.F64
            valEncoded += bytearray(struct.pack("d", float(value)))

        elif(type(value) == str): assert(False) # TODO: this
        else:
            print(f"Invalid type in message: {type(value)}")
            assert(False)



        self.content += int(kind.value).to_bytes(1)
        self.content += len(name).to_bytes(1)
        self.content += int(valType.value).to_bytes(1)
        self.content += len(valEncoded).to_bytes(1)

        self.content += name.encode()
        self.content += valEncoded



class Server():

    def __init__(self) -> None:

        self.sock = socket.socket(socket.AddressFamily.AF_INET, socket.SOCK_STREAM)
        self.sock.bind(("localhost", 7000))
        self.sock.listen(1) # client backlog
        self.sock.setblocking(False)
        print("Server loaded")

        self.msgList = []
        self.cliSock = None


    def update(self):

        if self.cliSock == None:

            rlist, wlist, elist = select.select([self.sock], [], [], 0)
            if(len(rlist) == 0): return

            sock, addr = self.sock.accept() # start client
            sock.setblocking(False)
            self.cliSock = sock
            print("[SOCKETS] client connected")



        if len(self.msgList) != 0:
            m = self.msgList.pop(0)

            try:
                status = self.cliSock.send(m.content)
            except Exception as e:
                self.cliSock.close()
                self.cliSock = None
                print(f"[SOCKETS] Client ended with exception {repr(e)}")



    def close(self):
        if self.cliSock != None:
            self.cliSock.close()

        self.sock.close()


