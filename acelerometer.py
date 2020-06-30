# -*- coding: utf-8 -*-
"""
Created on Tue Jun 23 14:55:16 2020

@author: alejandro
"""
import socket
from threading import Thread, Event, Lock
from time import sleep,time 
class acelerometer:
    def __init__(self,ip,port):
        self.serverAddressPort = (ip, port) # server addres and port
        self.bufferSize = 1024 # buffer size
        self.UDPClient = None #socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM) # create the socket
        self._reply_received = Event()
        self._reply_received.set()
        self._communication_lock = Lock()
        self._connection_lock = Lock()
        self._is_reading = False
        self.num_readings = 0 # number of readings, to compare with server message
        self.readings = []
        self._stream_request = False
        self.print_buffer = []
        self._print_thread = None
        self._read_thread = None
        self.stop_printing = False
        self.stop_reading = False
        self.last_command = ''
        self.time_out = 1.0 # timeout in seconds
    def connect(self):
        with self._connection_lock:
            self.UDPClient = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
            self.UDPClient.settimeout(self.time_out)
            self._start_printer()
            self._start_reader()
            
    def disconnect(self):
        with self._connection_lock:
            self.stop_reading = True
            self.stop_printing = True
    def _start_printer(self):
        self._print_thread = Thread(target = self._printer_worker, name = 'Printer')
        self._print_thread.setDaemon(True)
        self._print_thread.start()

    def _start_reader(self):
        self._read_thread =Thread(target = self._reader_worker, name = 'Reader') 
        self._read_thread.setDaemon(True)
        self._read_thread.start()
    def _printer_worker(self):
        while not self.stop_reading:
            if len(self.print_buffer)>0 and self._reply_received.is_set():
                with self._communication_lock:
                    self.last_command = self.print_buffer[0]
                    self.print_buffer.pop(0)
                    self.UDPClient.sendto(str.encode(self.last_command), self.serverAddressPort) 
                    self._reply_received.clear()
            else:
                sleep(0.01)
                
    def _reader_worker(self):
        while not self.stop_reading:
            msg = self.read()
            if msg == self.last_command:
                with self._communication_lock:
                    self._reply_received.set()                    
            if self._stream_request == True:
                if msg[0] == 'R':
                    # self.num_redings = self.num_redings + 1 
                    self.readings.append(msg)
            if msg == 'stop':
                self._stream_request = False
                msg = self.read()
                self.num_readings = int(msg)
            if msg == 'readings':
                self.readings = []
                self._stream_request = True
                
                
    def read(self):
        try:
            msg = self.UDPClient.recvfrom(self.bufferSize)[0]
            print(msg.decode())
            msg = msg.decode()
        except socket.timeout:
            msg = ''    
        return msg     
    def readings(self):
        self.print_buffer.append('readings')
    def stop(self):
        self.print_buffer.append('stop') 


# ejemplo de uso
a = acelerometer(ip = "192.168.1.101",port = 8888)
a.connect()
a.print_buffer.append('readings')
sleep(0.5)
a.print_buffer.append('stop')
sleep(0.5)
a.print_buffer.append('readings')
sleep(0.5)
a.print_buffer.append('stop')
sleep(0.5)
a.disconnect()
