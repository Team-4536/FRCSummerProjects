import struct
import socket
import numpy
from enum import Enum
import select

# in seconds
SEND_INTERVAL = 1/60



class MessageKind(Enum):
    UPDATE = 0
    EVENT = 1

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

        self.msgMap: dict[str, Message] = { }
        self.eventList: list[Message] = [ ]
        self.cliSock = None
        self.lastSendTime = 0


    def update(self, curTime: float):

        if self.cliSock == None:

            rlist, wlist, elist = select.select([self.sock], [], [], 0)
            if(len(rlist) == 0): return

            sock, addr = self.sock.accept() # start client
            sock.setblocking(False)
            self.cliSock = sock
            print("[SOCKETS] client connected")


        if(curTime - self.lastSendTime > SEND_INTERVAL):

            content = b""
            for p in self.msgMap.items():
                self.lastSendTime = curTime

                content += p[1].content

            while len(self.eventList) > 0:
                content += self.eventList.pop(0).content

            try:
                self.cliSock.send(content)
            except Exception as e:
                self.cliSock.close()
                self.cliSock = None
                print(f"[SOCKETS] Client ended with exception {repr(e)}")

    def putUpdate(self, name: str, value: float|int):
        m = Message(MessageKind.UPDATE, name, value)
        self.msgMap.update({ name : m })

    def putEvent(self, name: str):
        self.eventList.append(Message(MessageKind.EVENT, name, int(0)))



    def close(self):
        if self.cliSock != None:
            self.cliSock.close()

        self.sock.close()


