import machine
import time
import network
import usocket as socket
import re
import math

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
    client_socket.sendall(b'Control Pins 25, 12, 22: "pin_on_a+" or "pin_off_a-" ...\r\n')
    client_socket.sendall(b'Control Pins 32, 4, 2: "pins_on_b+" or "pins_off_b-" ...\r\n')
    client_socket.sendall(b'Control all Pins:"pin_on_ab(wh,slp)" ...\r\n')
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
     
        return int(match.group(1)), float(match.group(3)), float(match.group(2))
    else:
        return None
#"on_1" Definition mit Nummer a1+b1+c1 ist 3Eck 1 on
def pin_on_1():
    for pin_num in [25, 12, 22]:
        pin_objects[pin_num].on()
        
def pin_on_a1():
    for pin_num in [25]:
        pin_objects[pin_num].on()
        
def pin_on_b1():
    for pin_num in [12]:
        pin_objects[pin_num].on()
        
def pin_on_c1():
    for pin_num in [22]:
        pin_objects[pin_num].on()
#"off_1" Definition mit Nummer a1+b1+c1 ist 3Eck 1 off
def pin_off_1():
    for pin_num in [25, 12, 22]:
        pin_objects[pin_num].off()
        
def pin_off_a1():
    for pin_num in [25]:
        pin_objects[pin_num].off()

def pin_off_b1():
    for pin_num in [12]:
        pin_objects[pin_num].off()
        
def pin_off_c1():
    for pin_num in [22]:
        pin_objects[pin_num].off()
#"on_2" Definition mit Nummer a2+b2+c2 ist 3Eck 2 on
def pin_on_2():
    for pin_num in [32, 4, 2]:
        pin_objects[pin_num].on()

def pin_on_a2():
    for pin_num in [32]:
        pin_objects[pin_num].on()

def pin_on_b2():
    for pin_num in [4]:
        pin_objects[pin_num].on()

def pin_on_c2():
    for pin_num in [2]:
        pin_objects[pin_num].on()
#"off_2" Definition mit Nummer a2+b2+c2ist 3Eck 2 off
def pin_off_2():
    for pin_num in [32, 4, 2]:
        pin_objects[pin_num].off()

def pin_off_a2():
    for pin_num in [32]:
        pin_objects[pin_num].off()
        
def pin_off_b2():
    for pin_num in [4]:
        pin_objects[pin_num].off()
        
def pin_off_c2():
    for pin_num in [2]:
        pin_objects[pin_num].off()
#pin_onoff_1 alle pin 3eck 1 an und aus pin_onoff_1(wh,slp)
def pin_onoff_1(wh):
    pin_on_1()
    time.sleep(slp)
    pin_off_1()
    time.sleep(slp)
#pin_onoff_2 alle pin 3eck 2 an und aus pin_onoff_2(wh,slp)
def pin_onoff_2(wh):
    pin_on_2()
    time.sleep(slp)
    pin_off_2()
    time.sleep(slp)


def pin_on_ab(wh, slp):
    print("Pin_on_1&2 Wiederholungen: "+ str(wh))
    print("Pin_on_1&2 Schaltzeit: " + str(slp))
    for i in range(wh):
        pin_on_a1()
        time.sleep(slp)
        pin_off_a1()
        time.sleep(slp)
        pin_on_a2()
        time.sleep(slp)
        pin_off_a2()
        time.sleep(slp)
        pin_on_b1()
        time.sleep(slp)
        pin_off_b1()
        time.sleep(slp)
        pin_on_b2()
        time.sleep(slp)
        pin_off_b2()
        time.sleep(slp)
        pin_on_c1()
        time.sleep(slp)
        pin_off_c1()
        time.sleep(slp)
        pin_on_c2()
        time.sleep(slp)
        pin_off_c2()
        time.sleep(slp)
        
def sine_wave(angle, period):
    return 0.5 + 0.5 * math.sin(2 * math.pi * angle / period)

def run_sine_wave(total_time, slp, speed):
    period_a = 2 * math.pi / speed
    period_b = 2 * math.pi / speed

    for angle in range(int(total_time * period_a)):
        value_a1 = math.sin(2 * math.pi * angle / period_a)
        value_a2 = math.sin(2 * math.pi * angle / period_b)

        # Steuerung für a1, b1, c1
        if value_a1 > 0.5:
            pin_on_a1()
        else:
            pin_off_a1()

        if value_a1 > 0.5:
            pin_on_b1()
        else:
            pin_off_b1()

        if value_a1 > 0.5:
            pin_on_c1()
        else:
            pin_off_c1()

        # Steuerung für a2, b2, c2
        if value_a2 > 0.5:
            pin_on_a2()
        else:
            pin_off_a2()

        if value_a2 > 0.5:
            pin_on_b2()
        else:
            pin_off_b2()

        if value_a2 > 0.5:
            pin_on_c2()
        else:
            pin_off_c2()
            
            #Steuerung a1,a2,b1,b2,c1,c2 mit abhängigkeiten
       if value_a2 > 0.5:
            pin_on_a2()
            pin_off_a1()
        else:
            pin_off_a2()
            pin_on_a1()

        if value_a2 > 0.5:
            pin_on_b2()
            pin_off_b1()
        else:
            pin_off_b2()
            pin_on_b1()

        if value_a2 > 0.5:
            pin_on_c2()
            pin_off_c1()
        else:
            pin_off_c2()
            pin_on_c1()

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
                command = data.decode().strip()
                numbers = parse_command(command)

                if numbers is not None:
                    print("Wiederholungen:", numbers[0])
                    print("Schaltzeit:", numbers[1])
                    print("Geschwindigkeit:", numbers[2])
                    client_socket.send(("Wiederholungen: "+ str(numbers[0]) + "\n").encode())
                    client_socket.send(("Schaltzeit: "+ str(numbers[1]) + "\n").encode())
                    client_socket.send(("Geschwindigkeit: "+ str(numbers[2]) + "\n").encode())
                    run_sine_wave(float(numbers[0]), float(numbers[1]), float(numbers[2]))
                else:
                    client_socket.send("Ungültiger Befehl. Bitte geben Sie einen Befehl der Form 'sequenz(a,b,c)' ein.\n".encode())

            except ValueError:
                pass

# Start Telnet server
telnet_server()
client_socket.close()