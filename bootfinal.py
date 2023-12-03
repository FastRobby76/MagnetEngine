# This file is executed on every boot (including wake-boot from deepsleep)
#import esp
#esp.osdebug(None)
import machine
import time
import network

def connect_to_wifi(ssid, password):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    
    # Check if already connected
    if not wlan.isconnected():
        print("Connecting to WiFi...")
        wlan.connect(ssid, password)

        # Wait for the WiFi connection
        while not wlan.isconnected():
            pass
    print("WiFi connected.")
    print("IP Address:", wlan.ifconfig()[0])

# Definiere den Pin, an dem die LED angeschlossen ist
led_pin = machine.Pin(2, machine.Pin.OUT)

# Schalte die LED ein
led_pin.value(1)

# Warte 3 Sekunden
time.sleep(3)

# Schalte die LED aus
led_pin.value(0)

# Set your WiFi credentials
ssid = "hierNetzwerknamer"
password = "hierPasswort"
# Connect to WiFi
connect_to_wifi(ssid, password)

import main

