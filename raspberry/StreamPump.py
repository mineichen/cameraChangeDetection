from threading import Thread
from threading import Condition
from io import BytesIO

from pprint import pprint

class StreamPump(Thread):
    def __init__(self, sink, size):
        Thread.__init__(self)
        self.rPos = 0
        self.wPos = 0
        self.buff = bytearray(size)
        self.size = size
        self.sink = sink
        self.cond = Condition()
        self.isRunning = True
    
    def write(self, data):
        nOfBytes = len(data)
        overflow = self.wPos + nOfBytes - self.size
        if overflow >= 0:
            #print("%d > %d ov" %(overflow,self.rPos))
            if self.wPos < self.rPos < self.size or 0 < self.rPos < overflow:
                print("Error :-(")
                raise  BaseException("Buffer overflow")
            self.buff[self.wPos:self.size] = data[0:nOfBytes-overflow]
            self.buff[0:overflow] = data[nOfBytes-overflow:nOfBytes]
            self.wPos = overflow
        else:
            newWpos = self.wPos+nOfBytes
            #print("%d < %d ov" %(newWpos,self.rPos))
            if self.wPos < self.rPos < newWpos:
                print("Error :-(")
                raise  BaseException("Buffer overflow")
            self.buff[self.wPos:newWpos] = data
            self.wPos = newWpos
        
        self.cond.acquire()
        self.cond.notify()
        self.cond.release()

        return nOfBytes
    def run(self):
        self.isRunning = True
        while True:
            while self.rPos == self.wPos:
                if self.isRunning:
                    self.cond.acquire()
                    self.cond.wait()
                    self.cond.release()
                else:
                    return
            # make sure to always use same wPos
            wPos = self.wPos
            if self.rPos > wPos:
                self.sink.write(self.buff[self.rPos:self.size])
                self.rPos = 0
            
            self.sink.write(self.buff[self.rPos:wPos])
            self.rPos = wPos
    

    def finish(self):
        #print("Finish")
        self.cond.acquire()
        self.isRunning = False
        self.cond.notify()
        self.cond.release()
        self.join()

