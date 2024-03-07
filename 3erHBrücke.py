import machine
import time
import network
import usocket as socket
import re

pins = [23, 19, 18, 26, 21, 22, 3, 1, 4, 12, 32, 25, 27]

p = machine.Pin(2, machine.Pin.OUT)

pin_objects = {pin: machine.Pin(pin, machine.Pin.OUT) for pin in pins}

print("led noch aus")
p.on()
print("led an")
time.sleep(1)
p.off()

pin_on_0()

def show_options(client_socket):
    client_socket.sendall(b'LED on: "led_an" ...\r\n')
    client_socket.sendall(b'LED off: "led_aus" ...\r\n')
    client_socket.sendall(b'Control all Pins:"motor1u2(wh,slp)" ...\r\n')
    client_socket.sendall(b'Exit: "quit" ...\r\n\r\n')

def control_pin(pin_num, action):
    try:
        pin = pin_objects[pin_num]
        if action == 'on':
            pin.on()
        elif action == 'off':
            pin.off()
    except Exception as e:
        print("Error controlling pin:", e)
 
def evaluate_number(number):
    result = number   # Hier können Sie Ihre eigene Logik einfügen
    return result
 
def parse_command(command):
    match = re.match(r'^\s*motor1u2\s*\((\d+(\.\d+)?)\s*,\s*(\d+(\.\d+)?)\)\s*$', command)
    if match:
        return int(match.group(1)), float(match.group(3))
    else:
        return None

def motor1u2(wh, slp):
    print("Motor1u2 Wiederholungen:", wh)
    print("Motor1u2 Schaltzeit:", slp)
    for i in range(wh):
        pin_on_1()
        pin_off_2()
        pin_on_5()
        pin_off_6()
        pin_on_9()
        pin_off_10()
        time.sleep(slp)
        pin_off_3()
        pin_on_4()
        pin_off_7()
        pin_on_8()
        pin_off_11()
        pin_on_12()
        time.sleep(slp)
        pin_off_1()
        pin_on_2()
        pin_off_5()
        pin_on_6()
        pin_off_9()
        pin_on_10()
        time.sleep(slp)
        pin_on_3()
        pin_off_4()
        pin_on_7()
        pin_off_8()
        pin_on_11()
        pin_off_12()
        time.sleep(slp)


def pin_on_0():
    for pin_num in [27]:
        pin_objects[pin_num].on()
        
def pin_on_1():
    for pin_num in [23]:
        pin_objects[pin_num].on()
        
def pin_on_2():
    for pin_num in [19]:
        pin_objects[pin_num].on()
        
def pin_on_3():
    for pin_num in [18]:
        pin_objects[pin_num].on()
        
def pin_on_4():
    for pin_num in [26]:
        pin_objects[pin_num].on()

def pin_off_0():
    for pin_num in [27]:
        pin_objects[pin_num].off()

def pin_off_1():
    for pin_num in [23]:
        pin_objects[pin_num].off()

def pin_off_2():
    for pin_num in [19]:
        pin_objects[pin_num].off()

def pin_off_3():
    for pin_num in [18]:
        pin_objects[pin_num].off()

def pin_off_4():
    for pin_num in [26]:
        pin_objects[pin_num].off()
        
def pin_on_5():
    for pin_num in [21]:
        pin_objects[pin_num].on()
        
def pin_on_6():
    for pin_num in [22]:
        pin_objects[pin_num].on()
        
def pin_on_7():
    for pin_num in [3]:
        pin_objects[pin_num].on()
        
def pin_on_8():
    for pin_num in [1]:
        pin_objects[pin_num].on()

def pin_off_5():
    for pin_num in [21]:
        pin_objects[pin_num].off()

def pin_off_6():
    for pin_num in [22]:
        pin_objects[pin_num].off()

def pin_off_7():
    for pin_num in [3]:
        pin_objects[pin_num].off()

def pin_off_8():
    for pin_num in [1]:
        pin_objects[pin_num].off()
        
def pin_on_9():
    for pin_num in [4]:
        pin_objects[pin_num].on()
        
def pin_on_10():
    for pin_num in [12]:
        pin_objects[pin_num].on()
        
def pin_on_11():
    for pin_num in [32]:
        pin_objects[pin_num].on()
        
def pin_on_12():
    for pin_num in [25]:
        pin_objects[pin_num].on()

def pin_off_9():
    for pin_num in [4]:
        pin_objects[pin_num].off()

def pin_off_10():
    for pin_num in [12]:
        pin_objects[pin_num].off()

def pin_off_11():
    for pin_num in [32]:
        pin_objects[pin_num].off()

def pin_off_12():
    for pin_num in [25]:
        pin_objects[pin_num].off()
        
def telnet_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('', 23))
    server_socket.listen(1)

    while True:
        print('Waiting for Telnet connection...')
        client_socket, client_address = server_socket.accept()
        print('Telnet connection from:', client_address)
        client_socket.sendall(b'\nWelcome to the Engine!\r\n\n')
        show_options(client_socket)
        print("Showing options")
        
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            command = str(data).strip()
            
            try:
                # Befehl parsen und die Funktion aufrufen
                command = data.decode().strip()
                numbers = parse_command(command)
            
                if numbers is not None:
                    print("Wiederholungen:", numbers[0])
                    print("Schaltzeit:", numbers[1])
                    client_socket.send(("Wiederholungen: "+ str(numbers[0]) + "\n").encode())
                    client_socket.send(("Schaltzeit: "+ str(numbers[1]) + "\n").encode())
                    motor1u2(int(numbers[0]), float(numbers[1]))
                else:
                    client_socket.send("Ungültiger Befehl. Bitte geben Sie einen Befehl der Form 'motor1u2(a,b)' ein.\n".encode())
 
            except ValueError:
                pass
        client_socket.close()

# Start Telnet server
telnet_server()