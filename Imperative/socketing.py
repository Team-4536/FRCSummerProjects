from threading import Thread, Lock
import struct
import socket
from typing import Any
import numpy
from enum import Enum




class MessageKind(Enum):
    UPDATE = 0

class MessageUpdateType(Enum):
    S32 = 0
    F64 = 1
    STR = 2

class Message():

    def __init__(self, kind: MessageKind, name: str, value: float|int|str) -> None:

        self.content = b""
        self.content += int(kind.value).to_bytes(1)

        encoded = name.encode()
        l = len(encoded)
        assert(l < 255)
        self.content += int(l).to_bytes(1)
        self.content += encoded


        valType = 0
        encoded = b""

        if(type(value) == int):
            valType = MessageUpdateType.S32
            encoded += value.to_bytes(4, 'little', signed=True)

        elif(type(value) == float or type(value) == numpy.float64):
            valType = MessageUpdateType.F64
            encoded += bytearray(struct.pack("d", float(value)))

        elif(type(value) == str): assert(False) # TODO: this
        else:
            print(f"Invalid type in message: {type(value)}")
            assert(False)

        self.content += int(valType.value).to_bytes(1)
        self.content += encoded



class SyncQueue:

    def __init__(self) -> None:
        self.elemList: list[Message] = []
        self.lock = Lock()

    def push(self, elem: Message) -> None:
        with self.lock:
            self.elemList.append(elem)

    def consume(self) -> Message|None:
        with self.lock:
            if len(self.elemList) == 0: e = None
            else: e = self.elemList.pop(0)
        return e









class Server():

    def __init__(self) -> None:
        self.sock = socket.socket(socket.AddressFamily.AF_INET, socket.SOCK_STREAM)
        self.sock.bind(("localhost", 7000))
        self.sock.listen(1) # client backlog
        print("Server loaded")

        self.idSrc = 1
        self.msgQueue = SyncQueue()


    def handleClient(self, id: int, sock: socket.socket):

        while True:
            try:
                m = self.msgQueue.consume()
                if m is None: continue
                sock.send(m.content)
            except Exception as e:
                print(f"[SOCKETS] Client ended with exception {repr(e)}")
                break

        sock.close()

    def _startRecieving(self):
        while True:
            c, addr = self.sock.accept() # start client
            print("[SOCKETS] client connected")

            cliThread = Thread(target=self.handleClient, args=[self.idSrc, c])
            self.idSrc += 1
            cliThread.daemon = True
            cliThread.start()

    def start(self):
        thread = Thread(target=self._startRecieving)
        thread.daemon = True
        thread.start()


