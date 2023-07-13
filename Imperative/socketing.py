import struct
import ntcore
import socket
import numpy
from enum import IntEnum
import select
from typing import Any

# in seconds
SEND_INTERVAL = 1/50



class MessageKind(IntEnum):
    UPDATE = 0
    EVENT = 1

class PropType(IntEnum):
    S32 = 0
    F64 = 1
    STR = 2
    BOOL = 3


class Message:
    def __init__(self, kind: MessageKind, name: str, data: int|float|str|bool) -> None:
        self.kind = kind
        self.name = name
        self.data = data


class Server():

    inst = None

    def __init__(self) -> None:

        self.telemTable = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")

        # TODO: host from actual robot
        self.servSock = socket.socket(socket.AddressFamily.AF_INET, socket.SOCK_STREAM)
        self.servSock.bind(("localhost", 7000))
        self.servSock.listen(1) # client backlog
        self.servSock.setblocking(False)
        print("Server loaded")

        self.sendMsgMap: dict[str, bytes] = { }
        self.sendEventList: list[bytes] = [ ]
        self.cliSock = None
        self.lastSendTime = 0


        self.tracked: dict[str, int|float|str|bool] = { }
        self.events: list[str] = [ ]
        self.recvBuf: bytes = b""

        Server.inst = self


    def update(self, curTime: float):

        self.events.clear()

        if self.cliSock == None:

            rlist, wlist, elist = select.select([self.servSock], [], [], 0)
            if(len(rlist) == 0): return

            sock, addr = self.servSock.accept() # start client
            sock.setblocking(False)
            self.cliSock = sock
            print("[SOCKETS] client connected")


        if(curTime - self.lastSendTime > SEND_INTERVAL):

            content = b""
            for p in self.sendMsgMap.items():
                self.lastSendTime = curTime
                content += p[1]
            self.sendMsgMap.clear()

            while len(self.sendEventList) > 0:
                content += self.sendEventList.pop(0)

            while len(content) > 0:

                try:
                    res = self.cliSock.send(content)
                except Exception as e:
                    self.cliSock.close()
                    self.cliSock = None
                    print(f"[SOCKETS] Client ended with exception {repr(e)}")
                    break

                content = content[res:]



        if self.cliSock != None:
            while True:
                rlist, wlist, elist = select.select([self.cliSock], [], [], 0)
                if(len(rlist) != 0):

                    r = self.cliSock.recv(1024, 0)
                    if(len(r) == 0):
                        self.cliSock.close()
                        self.cliSock = None
                        print(f"[SOCKETS] Client disconnected.")
                        break

                    self.recvBuf += r
                else:
                    break

            while True:
                msg, consumed = self.decodeMessage(self.recvBuf)
                if consumed == 0: break
                self.recvBuf = self.recvBuf[consumed:]

                if(msg != None):
                    if(msg.kind == MessageKind.UPDATE):
                        self.tracked.update({ msg.name : msg.data })
                        print(f"UPDATED {msg.name} with {msg.data}")
                    elif(msg.kind == MessageKind.EVENT):
                        self.events.append(msg.name)
                    else:
                        print("[SOCKETS] something has gone terribly wrong")
                        assert(False)








    def putUpdate(self, name: str, value: float|int|str):

        self.telemTable.putValue(name, value)

        self.sendMsgMap.update({ name : self.encodeMessage(MessageKind.UPDATE, name, value) })

    def putEvent(self, name: str):
        self.sendEventList.append(self.encodeMessage(MessageKind.EVENT, name, int(0)))

    def encodeMessage(self, kind: MessageKind, name: str, value: float|int|str|bool) -> bytes:

        content = b""

        valType = 0
        valEncoded = b""

        if(type(value) == int):
            valType = PropType.S32
            # NOTE: ! indicates sending big endian format
            # https://docs.python.org/3/library/struct.html
            valEncoded += struct.pack("!l", value)

        elif(type(value) == float or type(value) == numpy.float64):
            valType = PropType.F64
            valEncoded += struct.pack("!d", value)

        elif(type(value) == str):
            valType = PropType.STR
            valEncoded += value.encode()

        elif(type(value) == bool):
            valType = PropType.BOOL
            valEncoded += struct.pack("!B", 1 if value else 0)


        else:
            print(f"Invalid type in message: {type(value)}")
            assert(False)



        content += struct.pack("!B", kind.value)
        # TODO: message name len capped at 255, bad for events
        content += struct.pack("!B", len(name))
        content += struct.pack("!B", valType.value)
        content += struct.pack("!B", len(valEncoded))

        content += name.encode()
        content += valEncoded

        return content



    # Message format detailed in cppsundial at network.h
    # Consuming 0 bytes indicates waiting on info
    def decodeMessage(self, buffer: bytes) -> tuple[Message|None, int]:

        if(len(buffer) < 4): return (None, 0)
        header = struct.unpack("!BBBB", buffer[:4])
        buffer = buffer[4:]



        if(header[0] != MessageKind.UPDATE.value
           and header[0] != MessageKind.EVENT.value):
            return (None, 1)

        nameLen = int(header[1])

        found = False
        for val in [e.value for e in PropType]:
            if header[2] == val:
                found = True
        if not found: return (None, 3)

        dataSize = int(header[3])





        if(len(buffer) < (nameLen + dataSize)):
            return None, 0


        name = buffer[:nameLen].decode()
        buffer = buffer[nameLen:]
        dataBytes = buffer[:dataSize]



        dataType = PropType(header[2])
        if(dataType == PropType.S32): data = int(struct.unpack("!l", dataBytes)[0])
        elif(dataType == PropType.F64): data = float(struct.unpack("!d", dataBytes)[0])
        elif(dataType == PropType.STR): data = dataBytes.decode()
        elif(dataType == PropType.BOOL): data = True if dataBytes[0] != 0 else False
        else: assert(False)


        m = Message(MessageKind(header[0]), name, data)
        return m, 4 + nameLen + dataSize


    def close(self):
        if self.cliSock != None:
            self.cliSock.close()

        self.servSock.close()


