import machine
import time
import network
import usocket as socket
import re

pins = [27, 25, 32, 12, 4, 22, 2]
led_pin = machine.Pin(2, machine.Pin.OUT)

pin_objects = {pin: machine.Pin(pin, machine.Pin.OUT) for pin in pins}

def led_an():
    led_pin.value(1)

def led_aus():
    led_pin.value(0)

def show_options(client_socket):
    client_socket.sendall(b'LED on: "led_an" ...\r\n')
    client_socket.sendall(b'LED off: "led_aus" ...\r\n')
    client_socket.sendall(b'Control Pins 25, 12, 22: "pin_on_a" or "pin_off_a" ...\r\n')
    client_socket.sendall(b'Control Pins 32, 4, 2: "pin_on_b" or "pin_off_b" ...\r\n')
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

# Differentialgleichung
def apply_differential_equation(pin_num, y, dy_dt, damping_factor, natural_frequency, time_step):
    dy_dt += time_step * (-2 * damping_factor * natural_frequency * dy_dt - natural_frequency ** 2 * y)
    y += time_step * dy_dt

    if y > 0.5:
        control_pin(pin_num, 'on')
    else:
        control_pin(pin_num, 'off')

    return y, dy_dt

# Befehl: sequenz(total_wiederholungen, dauer, abstand, (wiederholungen1, dur1) (wiederholungen2, dur2) ...)
def parse_command(command):
    match = re.match(r'^\s*sequenz\s*\((\d+)\s*,\s*(\d+(?:\.\d+)?)\)\s*$', command)
    if match:
        repetitions = int(match.group(1))
        duration = float(match.group(2))
        return repetitions, duration
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

# Funktion für den zeitlichen Verlauf der Pins
def simulate_pin_sequence(pin_sequence, total_time, damping_factor, natural_frequency, time_step):
    for t in range(int(total_time / time_step)):
        for pin_num in pin_sequence:
            y, dy_dt = apply_differential_equation(pin_num, 0.0, 0.0, damping_factor, natural_frequency, time_step)
            time.sleep(time_step)

def pin_on_ab(repetitions, duration):
    print("Pin_on_ab Wiederholungen:", repetitions)
    print("Pin_on_ab Schaltzeit:", duration)
    simulate_pin_sequence([25, 12, 22, 32, 4, 2], repetitions * duration, 0.1, 2 * 3.14 / duration, 0.01)

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
            command = str(data, 'utf-8').strip()

            try:
                repetitions, duration = parse_command(command)
                if repetitions is not None:
                    print("Wiederholungen:", repetitions)
                    print("Schaltzeit:", duration)
                    client_socket.send(("Wiederholungen: " + str(repetitions) + "\n").encode())
                    client_socket.send(("Schaltzeit: " + str(duration) + "\n").encode())
                    pin_on_ab(repetitions, duration)
                else:
                    client_socket.send("Ungültiger Befehl. Bitte geben Sie einen Befehl der Form 'sequenz(repetitions, duration)' ein.\n".encode())
            except ValueError:
                pass

# Start Telnet server
telnet_server()