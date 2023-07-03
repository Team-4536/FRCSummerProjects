from threading import Thread, Lock
import struct
import socket
from typing import Any
from enum import Enum

MSG_SIZE = 1024


class messageKind(Enum):
    UPDATE = 0
    EVENT = 1

class messageUpdateType(Enum):
    S32 = 0
    F64 = 1
    STR = 2



class message():

    def __init__(self, kind: messageKind, name: str) -> None:
        self.content = b""

        self.content += int(kind.value).to_bytes(1)

        encoded = name.encode()
        l = len(encoded)
        assert(l < 255)
        self.content += int(l).to_bytes(1)
        self.content += encoded



    def addValue(self, value: float|int|str):
        valType = 0
        encoded = b""

        if(type(value) == int):
            valType = messageUpdateType.S32
            encoded += value.to_bytes(4, 'little', signed=True)

        elif(type(value) == float):
            valType = messageUpdateType.F64
            encoded += bytearray(struct.pack("d", value))

        elif(type(value) == str): assert(False) # TODO: this
        else: assert(False)

        self.content += int(valType.value).to_bytes(1)
        self.content += encoded

    def get(self): return self.content




class SyncQueue:

    def __init__(self) -> None:
        self.elemList: list[message] = []
        self.lock = Lock()

    def push(self, elem: message) -> None:
        with self.lock:
            self.elemList.append(elem)

    def consume(self) -> message|None:
        with self.lock:
            if len(self.elemList) == 0: e = None
            else: e = self.elemList.pop(0)
        return e









class server():

    def __init__(self) -> None:
        self.sock = socket.socket(socket.AddressFamily.AF_INET, socket.SOCK_STREAM)
        self.sock.bind(("localhost", 7000))
        self.sock.listen(5) # client backlog
        print("Server loaded")

        self.idSrc = 1
        self.msgQueue = SyncQueue()


    def handleClient(self, id: int, sock: socket.socket):

        while True:
            m = self.msgQueue.consume()
            if m is None: continue
            sock.send(m.get())

        sock.close()

    def sendShit(self):
        while True:
            s = input("Give me a message: ")
            v = input("And a number: ")
            msg = message(messageKind.UPDATE, s)
            msg.addValue(int(v))
            self.msgQueue.push(msg)


    def start(self):

        printThread = Thread(target=self.sendShit, args=[])
        printThread.start()

        while True:
            c, addr = self.sock.accept() # start client

            cliThread = Thread(target=self.handleClient, args=[self.idSrc, c])
            self.idSrc += 1
            cliThread.start()



if __name__ == "__main__":
    s = server()
    s.start()