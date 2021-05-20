# coding=utf-8

import socket
import threading
import os

lock = threading.Lock()
EMU_FLAG = False
#套接字列表
socketlist ={}#dict,ID:Socket,可能需要lock
def handleAccept(sk):
    global socketlist
    sk.send("hello")
    buffer = sk.recv(1024)
    if buffer:
        print("Recv Information")
        print(buffer)
        
        #如果是MODEM
        if buffer[0:6] == "MODEM-":
            newModemID = buffer
            print("recv Modem massage")
           
            if (newModemID not in socketlist):#如果键值不存在
                lock.acquire()#加锁
                try:
                    socketlist[newModemID] = sk
                finally:
                    lock.release()#释放锁
            else:#如果键值存在
                a = 0#若Modem 的ID重复，则后面加上数字
                buffernew = []
                while 1:
                    buffernew = newModemID + a.to_str()
                    if buffernew in socketlist:
                        lock.acquire()#加锁
                        try:
                            socketlist[buffernew] = sk
                        finally:
                            lock.release()#释放锁
                    else:
                        a += 1
                        break
        elif buffer == "START_EMULATION":
            print("Recv Page Information:Start Emu")
            #global EMU_FLAG = True
            print(socketlist)
            fdlist=[]
            for value in socketlist.values():#逐个modem套接字发送仿真开始消息,检验套接字的有效性？？
                print(value)
                value.send("START_EMULATION")
                #如果套接字有效
                fdlist.append(value.fileno())
            #调用，仿真...
            print(fdlist)
            cmdStr = './waf --run="scratch/test-python --sFd1=4 --sFd2=5"'
            #for fd in fdlist:
                
            os.system(cmdStr)
    return 0

#创建套接字tcp
tcpServerSocket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)#server的socket

host = "192.168.1.170"
address = (host,8080)
tcpServerSocket.bind(address)
tcpServerSocket.listen(4)#等待队列最大长度，并非最大连接数
print("listen")
while True:
    tcpClientSocket,addr = tcpServerSocket.accept()#client的socket
    print("Accept new request")
    print(tcpClientSocket.fileno())
    t = threading.Thread(target=handleAccept,args=(tcpClientSocket,) )
    t.start()

tcpServerSocket.close()
