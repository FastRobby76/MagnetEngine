import math
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

def evaluate_number(number):
    result = number   # Hier können Sie Ihre eigene Logik einfügen
    return result

# Befehl: sequenz(total_wiederholungen, dauer, abstand, (wiederholungen1, dur1) (wiederholungen2, dur2) ...)
def parse_command(command):
    match = re.match(r'^\s*sequenz\s*\((\d+)\s*,\s*(\d+(?:\.\d+)?)\s*,\s*(\d+(?:\.\d+)?)\s*,\s*((?:\(\d+\s*,\s*\d+(?:\.\d+)?\)\s*)+)\)\s*$', command)
    if match:
        repetitions_total = int(match.group(1))
        dauer = float(match.group(2))
        abstand = float(match.group(3))
        repetitions_string = match.group(4)
        repetitions = []
        while repetitions_string:
            repetitions_string = repetitions_string.strip()
            submatch = re.match(r'\((\d+)\s*,\s*(\d+(?:\.\d+)?)\)', repetitions_string)
            if not submatch:
                break
            repetitions_string = repetitions_string[len(submatch.group(0)):]
            repetitions.append((int(submatch.group(1)), float(submatch.group(2))))
        return repetitions_total, dauer, abstand, repetitions
    else:
        return None

def pin_on_a():
    for pin_num in [25, 12, 22]:
        pin_objects[pin_num].on()
        pin_objects[27].on()  # Unterstützende Aktion für Pin 27

def pin_off_a():
    for pin_num in [25, 12, 22]:
        pin_objects[pin_num].off()
        pin_objects[27].off()  # Unterstützende Aktion für Pin 27

def pin_on_b():
    for pin_num in [32, 4, 2]:
        pin_objects[pin_num].on()
        pin_objects[27].on()  # Unterstützende Aktion für Pin 27

def pin_off_b():
    for pin_num in [32, 4, 2]:
        pin_objects[pin_num].off()
        pin_objects[27].off()  # Unterstützende Aktion für Pin 27

def simulate_triangle_motion(repetitions_total, dauer, abstand, repetitions):
    damping_factor = 0.1
    natural_frequency = 2 * math.pi / dauer
    time_step = 0.01  # Zeitschritt für das Euler-Verfahren

    for _ in range(repetitions_total):
        for rep, multiplier in repetitions:
            duration_a = dauer * multiplier
            duration_b = dauer * abstand

            # Initialbedingungen
            y_a = 0.0
            dy_dt_a = 0.0
            y_b = 0.0
            dy_dt_b = 0.0

            for _ in range(rep):
                pin_on_a()

                # Euler-Verfahren für Bewegung von Pin A
                dy_dt_a += time_step * (-2 * damping_factor * natural_frequency * dy_dt_a - natural_frequency ** 2 * y_a)
                y_a += time_step * dy_dt_a

                pin_off_a()
                time.sleep(duration_a)

                pin_on_b()

                # Euler-Verfahren für Bewegung von Pin B
                dy_dt_b += time_step * (-2 * damping_factor * natural_frequency * dy_dt_b - natural_frequency ** 2 * y_b)
                y_b += time_step * dy_dt_b

                pin_off_b()
                time.sleep(duration_b)

#