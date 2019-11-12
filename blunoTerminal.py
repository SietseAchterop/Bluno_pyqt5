#!/usr/bin/env python3

"""
 Simple controller for bluno, working on linux
      Bluetooth low energy
"""
import sys, serial, threading, time, queue

# BLE stuff
from bluepy.btle import DefaultDelegate, Peripheral

# globals
global q, buffer

# callback class
class MyDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
        global q, buffer
        # perhaps check cHandle
        # print(f'next data: {data}')
        buffer += data
        # print(f'buffer: {buffer}')

# BlunoDevice           = "50:65:83:99:4B:54"      # Piano 001
# BlunoDevice           = "50:65:83:99:48:3A"      # Piano 002
BlunoDevice           = "50:65:83:99:4B:5E"      # Piano 003
PrimarService         = '0000dfb0-0000-1000-8000-00805f9b34fb'
ModelNumberStringUUID = '00002a24-0000-1000-8000-00805f9b34fb'
CommandUUID           = '0000dfb2-0000-1000-8000-00805f9b34fb'
SerialPortUUID        = '0000dfb1-0000-1000-8000-00805f9b34fb'


class ControlBluno:
    def __init__(self, per):

        # set callback for notifications
        per.withDelegate(MyDelegate())

        # get the characteristic
        self.c = per.getCharacteristics(uuid=SerialPortUUID)[0]
        # enable notification
        # setup_data = b'\0x01'
        # self.c.write(setup_data, withResponse=True)
        #  staat kennelijk per default op notification
        
    def start(self):
        self.alive = True
        self.transmitter_thread = threading.Thread(target=self.writer, daemon=True)
        self.transmitter_thread.start()

    def writer(self):
        global buffer
        try:
            while self.alive:
                try:
                    line = input('--> ')
                    if len(line) == 0:
                        continue
                    if line[0] == 'q':
                        self.alive = False
                        break
                    else:
                        line += '\r'
                        self.c.write(line.encode())
                    time.sleep(0.1)
                    next_line = buffer
                    if (len(next_line) == 0) or next_line[-1] != ord('\r'):  # beetje knullig
                        time.sleep(0.3)
                        next_line = buffer
                        print("even gewacht")
                    buffer = b''

                    # process result
                    print(f'{next_line.decode()}')

                except Exception as ex:
                    template = "An exception of type {0} occurred in while. Arguments:\n{1!r}"
                    message = template.format(type(ex).__name__, ex.args)
                    print(message)
                    self.alive = False
        except Exception as ex:
            template = "An exception of type {0} occurred. Arguments:\n{1!r}"
            message = template.format(type(ex).__name__, ex.args)
            print (message)
            self.alive = False

#  Start of program


def mainprog(device):
    global q, buffer
    if len(sys.argv) == 2:
        device = str(sys.argv[1])
    elif len(sys.argv) != 1:
        print('Usage: blueTerminal [device]')
        sys.exit(1)

    buffer = b''
    q = queue.SimpleQueue()
    
    try:
        blunocon = ControlBluno(device)
    except serial.SerialException as e:
        sys.stderr.write("could not open device %r: %s\n" % (device, e))
        sys.exit(1)
    
    blunocon.start()

    while True:
        if per.waitForNotifications(0.5):
            continue
        try:
            blunocon.transmitter_thread.join(0.05)
        except KeyboardInterrupt:
            print("keyboard interrupt")
            sys.exit(0)
        if not blunocon.transmitter_thread.isAlive():
            break

if __name__ == '__main__':

    # connect to device
    per = Peripheral(BlunoDevice, "public")
    mainprog(per)
