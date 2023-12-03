import machine
import time
import network
import usocket as socket
import re

pins = [27, 25, 32, 12, 4, 22, 2]
led_pin = machine.Pin(2, machine.Pin.OUT)

pin_objects = {pin: machine.Pin(pin, machine.Pin.OUT) for pin in pins
               }


def led_an():
    led_pin.value(1)

def led_aus():
    led_pin.value(0)
#hier eine 3ecksfunktion zwischen den pins 25+12+22 und den pins 32+4+2, in abhängigkeit zu einander durch zeit differenz
def show_options(client_socket):
    client_socket.sendall(b'LED on: "led_an" ...\r\n')
    client_socket.sendall(b'LED off: "led_aus" ...\r\n')
    client_socket.sendall(b'Control Pins 25, 12, 22: "pin_on_a" or "pin_off_a" ...\r\n')
    client_socket.sendall(b'Control Pins 32, 4, 2: "pins_on_b" or "pins_off_b" ...\r\n')
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
 
 #befehl: sequenz+(wiederholungen,schaltzeit in Sekunden) durch komma getrennt
def parse_command(command):
    match = re.match(r'^\s*sequenz\s*\((\d+(\.\d+)?)\s*,\s*(\d+(\.\d+)?)\)\s*$', command)
    if match:
     
        return int(match.group(1)), float(match.group(3))
    else:
        return None

def pin_on_a():
    for pin_num in [25, 12, 22]:
        pin_objects[pin_num].on()
        
def pin_off_a():
    for pin_num in [25, 12, 22]:
        pin_objects[pin_num].off()

def pin_on_b():
    for pin_num in [32, 4, 2]:
        pin_objects[pin_num].on()

def pin_off_b():
    for pin_num in [32, 4, 2]:
        pin_objects[pin_num].off()

def pin_onoff_a():
    pin_on_a()
    time.sleep(slp)
    pin_off_a()

def pin_onoff_b():
    pin_on_b()
    time.sleep(slp)
    pin_off_b()
    
def pin_on_ab(wh, slp):
    print("Pin_on_ab Wiederholungen: "+ str(wh))
    print("Pin_on_ab Schaltzeit: " + str(slp))
    for i in range(wh):
        pin_on_a()
        time.sleep(slp)
        pin_off_a()
        time.sleep(slp)
        pin_on_b()
        time.sleep(slp)
        pin_off_b()
        time.sleep(slp)
        
def telnet_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('', 23))
    server_socket.listen(1)

    while True:
        print('Waiting for Telnet connection...')
        client_socket, client_address = server_socket.accept()
        print('Telnet connection from:', client_address)
        client_socket.sendall(b'\nWelcome to the Engine Telnet Server!\r\n\n')
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
                    pin_on_ab(int(numbers[0]), float(numbers[1]))
                else:
                    client_socket.send("Ungültiger Befehl. Bitte geben Sie einen Befehl der Form 'pinonab(a,b)' ein.\n".encode())
 
            except ValueError:
                pass
                

# Start Telnet server
telnet_server()
client_socket.close()


