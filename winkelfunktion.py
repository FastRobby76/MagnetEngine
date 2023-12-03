import math

def check_triangle_properties(alpha, beta, gamma, r):
    # Überprüfe, ob die gegenüberliegenden Winkel gleich sind
    condition1 = alpha == beta and beta == gamma

    # Überprüfe die gegebene Gleichung für Seitenlängen und Winkel im Dreieck
    condition2 = r / (2 * math.sin(math.radians(alpha))) == beta + gamma

    # Gib True zurück, wenn beide Bedingungen erfüllt sind, ansonsten False
    return condition1 and condition2
    
   #oder:
       
       
       import math

def check_triangle_properties(alpha, beta, gamma, r):
    return (
        alpha == beta == gamma
        and r / (2 * math.sin(math.radians(alpha))) == beta + gamma
    )
    
    
    #oder wenn die winkel gegenüber liegen:
        
        
        
        import math

def check_opposite_angles(alpha, beta, gamma, r):
    return (
        alpha == beta
        and beta == gamma
        and r / (2 * math.sin(math.radians(alpha))) == beta + gamma
    )
    
    
    
    
    #daraus herraus:
        
        
        
        
        
        
        import math
from gpio_module import Pin  # Annahme: Ein Modul zum Steuern von GPIO-Pins

def calculate_lengths_ratio(a, b, c):
    total_length = a + b + c

    ratio_a = a / total_length
    ratio_b = b / total_length
    ratio_c = c / total_length

    return ratio_a, ratio_b, ratio_c

def check_opposite_angles(alpha, beta, gamma, r):
    return (
        alpha == beta
        and beta == gamma
        and r / (2 * math.sin(math.radians(alpha))) == beta + gamma
    )

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

# Beispielaufrufe
a_length = 0.3  # Beispielhafte Längen für die Seiten
b_length = 0.4
c_length = 0.3

lengths_ratio = calculate_lengths_ratio(a_length, b_length, c_length)

if check_opposite_angles(60, 60, 60, 1.0):
    pin_on_a()
else:
    pin_on_b()