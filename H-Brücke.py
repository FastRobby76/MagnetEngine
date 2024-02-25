import machine
import time
import network
import usocket as socket
import re

# Pins für die H-Brückenverbindungen definieren
motor_pins = {
    1: [23, 19],  # Motor 1
    2: [18, 26],  # Motor 2
    3: [21, 22],  # Motor 3
    4: [3, 1],    # Motor 4
    5: [4, 12],   # Motor 5
    6: [32, 25]   # Motor 6
}

# Initialisieren aller Motor-Pins als Output
pin_objects = {pin: machine.Pin(pin, machine.Pin.OUT) for motor in motor_pins.values() for pin in motor}

def motor_control(motor, action, delay):
    pins = motor_pins[motor]
    if action == 'on':
        for pin in pins:
            pin_objects[pin].on()
    elif action == 'off':
        for pin in pins:
            pin_objects[pin].off()
    time.sleep(delay)  # Geschwindigkeit durch Verzögerung kontrollieren

def show_options(client_socket):
    client_socket.sendall(b'Um einen Motor anzusteuern: "motor_on(motor_num, delay)" ...\r\n')
    client_socket.sendall(b'Um einen Motor zu stoppen: "motor_off(motor_num, delay)" ...\r\n')
    client_socket.sendall(b'Exit: "quit" ...\r\n\r\n')

def parse_command(command):
    match = re.match(r'^\s*motor_(on|off)\s*\(\s*(\d+)\s*,\s*(\d+\.?\d*)\s*\)\s*$', command)
    if match:
        action = match.group(1)
        motor_num = int(match.group(2))
        delay = float(match.group(3))
        return action, motor_num, delay
    else:
        return None, None, None

def telnet_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('', 23))
    server_socket.listen(1)

    while True:
        print('Waiting for Telnet connection...')
        client_socket, client_address = server_socket.accept()
        print('Telnet connection from:', client_address)
        client_socket.sendall(b'\nWelcome to the Motor Control Server!\r\n\n')
        show_options(client_socket)

        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            command = data.decode().strip()
            
            try:
                action, motor_num, delay = parse_command(command)
                if action and motor_num:
                    if 1 <= motor_num <= 6:
                        motor_control(motor_num, action, delay)
                    else:
                        client_socket.send("Ungültiger Motornummer. Bitte geben Sie eine Nummer zwischen 1 und 6 ein.\n".encode())
                else:
                    client_socket.send("Ungültiger Befehl. Bitte geben Sie einen gültigen Befehl ein.\n".encode())
            except ValueError:
                pass
        client_socket.close()

# Start Telnet server
telnet_server()