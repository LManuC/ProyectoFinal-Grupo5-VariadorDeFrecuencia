import pygame
import sys
import os
import math
import numpy as np
from SVM_Calculos import getSector, get_time_vector, getSecuenciaLabel, getSecuencia

#ejecutar con python prueba.py

# Inicializar pygame
pygame.init()

# Configurar pantalla
screen_width, screen_height = 800, 600
screen = pygame.display.set_mode((screen_width, screen_height))
pygame.display.set_caption("Modulacion de vector espacial")

# Colores
rect_color = (255, 0, 0)
background_color = (128, 128, 128)
text_color = (255, 255, 255)
circle_color = (0, 0, 0)
vector_color = (100, 100, 100)

black_color = (0,0,0)
green_color = (0,255,0)
red_color = (255,0,0)
blue_color = (0,0,255)

# Posicion del rectangulo
rect_width, rect_height = 50, 50
rect_x, rect_y = 100, 100
rect_speed = 5

# Fuente para el texto
font_tiempo = pygame.font.SysFont(None, 36)
font_caracteristicas = pygame.font.SysFont(None, 20)
line_spacing = 30

# Cargamos las imagenes
image_folder = "image"  # Carpeta donde están las imagenes
image_files = [f"E{i}.png" for i in range(8)]  # Lista de nombres de archivos (E0.png, E1.png, ..., E7.png)
# Cargar y escalar las imágenes
imageSwitch = []
for file_name in image_files:
    image_path = os.path.join(image_folder, file_name)  # Crear la ruta completa
    try:
        image = pygame.image.load(image_path)  # Cargar la imagen
        image = pygame.transform.scale(image, (110, 125))  # Escalar la imagen (ajusta el tamaño si es necesario)
        imageSwitch.append(image)  # Agregar la imagen al vector
    except pygame.error as e:
        print(f"Error al cargar la imagen {file_name}: {e}")


# Reloj para controlar FPS y calcular tiempo transcurrido
clock = pygame.time.Clock()
start_time = pygame.time.get_ticks()

# Posición del círculo
circle_center = (screen_width // 2, screen_height // 2)
center_x = screen_width // 2
center_y = screen_height // 2
circle_radius = 250
vector_length = 250
num_vectors = 6






# Variables 
timeScale = 100000000 # Esta nos sirve para relentizar la animacion
rotFreq = 50        # Esta es la frecuencia de rotacion del vector de tension
swFreq = 10000      # Esta es la frecuencia de la conmutacion
forward = 1         # Indica la direccion de rotacion, 1 para sentido horario visto del lado del eje del rotor """


# Esto nos indica que angulo nos va a llevar un periodo de conmutacion
angSw = rotFreq * 360 / swFreq



# Las caracteristicas de nuestro controlador
text_lines = [
    f"Ang conmutacion: {angSw}",
    f"Frecuencia rotor: {rotFreq}",
    f"Frecuencia switch: {swFreq}"
]

# Variable donde guardamos nuestro orden de conmutacion
conmutacion_lineas = []

# Variables globales
proxTiempoTimer = 0
ultTiempoTimer = 0
ang = 0
sector = 0
t0 = 0
t1 = 0
t2 = 0
secuenciaLabel = ""
numImageSwitch = 0

# Bucle principal
clock = pygame.time.Clock()
while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

    # Calcular tiempo transcurrido en segundos
    elapsed_time = (pygame.time.get_ticks() - start_time) 
    microsTime = int(elapsed_time/timeScale * 1000000)
    time_text = f"Tiempo: {microsTime}us"
    text_surface = font_tiempo.render(time_text, True, text_color)

    # Rellenar el fondo con gris
    screen.fill(background_color)

    
    # Dibujar el círculo en el centro de la pantalla
    pygame.draw.circle(screen, circle_color, circle_center, circle_radius, 2)
    # Dibujar los vectores que parten desde el centro
    for i in range(num_vectors):
        angle = (360 / num_vectors) * i  # Ángulo en grados
        angle_rad = math.radians(angle)  # Convertir a radianes
        end_x = circle_center[0] + vector_length * math.cos(angle_rad)
        end_y = circle_center[1] + vector_length * math.sin(angle_rad)
        pygame.draw.line(screen, vector_color, circle_center, (end_x, end_y), 2)




    
    angMag = 250

    duty0 = 80
    duty1 = 60
    duty2 = 40
    

    # Vamos a simular un timer aca que arranca y termina en un periodo de conmutacion
    if(microsTime > proxTiempoTimer): 
        ang = ((elapsed_time/timeScale) * 360 * rotFreq) % 360
        sector = getSector(ang)
        t0, t1, t2 = get_time_vector(1/swFreq, 1, ang)
        secuenciaLabel = getSecuenciaLabel(sector)

        #print(f"SwitchTime: {1/swFreq}, T0: {t0}, T1: {t1}, T2: {t2}")
        #print(f"T1: {t0/2}, T2: {t0/2+t1}, T3: {t0/2+t1+t2}, T4: {t0+t1+t2}, T5: {3*t0/2+t1+t2}, T6: {3*t0/2+t1+t2*2}, T7: {3*t0/2+2*t1+2*t2}, T8: {2*t0+2*t1+2*t2}")

        # Aca tenemos que chequear de como sincronizar nuestro timer con el pwm
        proxTiempoTimer = microsTime + (1/swFreq) * 1000000     # Ambos deben estar en microsegundos
        ultTiempoTimer = microsTime


    # Esto lo deberia hacer el PWM
    tiempoEnCiclo = (microsTime - ultTiempoTimer) / 1000000   # Este es el tiempo en que estoy del ciclo en segs
    #print(f"tiempoEnCiclo: {tiempoEnCiclo}")
    #print(f"T1: {t0/2}, T2: {t0/2+t1}, T3: {t0/2+t1+t2}, T4: {t0+t1+t2}, T5: {3*t0/2+t1+t2}, T6: {3*t0/2+t1+t2*2}, T7: {3*t0/2+2*t1+2*t2}, T8: {2*t0+2*t1+2*t2}")
    if(tiempoEnCiclo < t0/2) :
        numImageSwitch = getSecuencia(sector)[0]
        pygame.draw.circle(screen, green_color, circle_center, 10, 4)
    elif(tiempoEnCiclo < (t0/2+t1)) :
        numImageSwitch = getSecuencia(sector)[1]
        pygame.draw.line(screen, blue_color, circle_center, (angMag * np.cos(np.radians((sector-1) * 60)) + center_x, angMag * np.sin(np.radians((sector-1) * 60)) + center_y), 4)
    elif(tiempoEnCiclo < (t0/2+t1+t2)) :
        numImageSwitch = getSecuencia(sector)[2]
        pygame.draw.line(screen, red_color, circle_center, (angMag * np.cos(np.radians((sector) * 60)) + center_x, angMag * np.sin(np.radians((sector) * 60)) + center_y), 4)
    elif(tiempoEnCiclo < (t0+t1+t2)) :
        numImageSwitch = getSecuencia(sector)[3]
        pygame.draw.circle(screen, green_color, circle_center, 10, 4)
    elif(tiempoEnCiclo < (3*t0/2+t1+t2)) :
        numImageSwitch = getSecuencia(sector)[4]
        pygame.draw.circle(screen, green_color, circle_center, 10, 4)
    elif(tiempoEnCiclo < (3*t0/2+t1+t2*2)) :
        numImageSwitch = getSecuencia(sector)[5]
        pygame.draw.line(screen, red_color, circle_center, (angMag * np.cos(np.radians((sector) * 60)) + center_x, angMag * np.sin(np.radians((sector) * 60)) + center_y), 4)
    elif(tiempoEnCiclo < (3*t0/2+2*t1+2*t2)) :
        numImageSwitch = getSecuencia(sector)[6]
        pygame.draw.line(screen, blue_color, circle_center, (angMag * np.cos(np.radians((sector-1) * 60)) + center_x, angMag * np.sin(np.radians((sector-1) * 60)) + center_y), 4)
    elif(tiempoEnCiclo < (2*t0+2*t1+2*t2)) :
        numImageSwitch = getSecuencia(sector)[7]
        pygame.draw.circle(screen, green_color, circle_center, 10, 4)

    # Dibujar el sector
    rendered_text = font_caracteristicas.render(f"Sector {sector}, Secuencia:", True, text_color)
    screen.blit(rendered_text, (10, 100))
    rendered_text = font_caracteristicas.render(secuenciaLabel, True, text_color)
    screen.blit(rendered_text, (10, 130))

    # Dibujar secuencia
    screen.blit(imageSwitch[numImageSwitch], (20, 450))

    # Dibujar T1 y T2
    rendered_text = font_caracteristicas.render(f"T0: {t0*swFreq* 100:.1f}%", True, text_color)
    screen.blit(rendered_text, (20, 150))
    pygame.draw.rect(screen, black_color, (28, 168, 44, 24), width=2)
    pygame.draw.rect(screen, red_color, (30, 170, t0*swFreq * 40, 20))
    rendered_text = font_caracteristicas.render(f"T1: {t1*swFreq* 100:.1f}%", True, text_color)
    screen.blit(rendered_text, (20, 200))
    pygame.draw.rect(screen, black_color, (28, 218, 44, 24), width=2)
    pygame.draw.rect(screen, red_color, (30, 220, t1*swFreq * 40, 20))
    rendered_text = font_caracteristicas.render(f"T2: {t2*swFreq* 100:.1f}%", True, text_color)
    screen.blit(rendered_text, (20, 250))
    pygame.draw.rect(screen, black_color, (28, 268, 44, 24), width=2)
    pygame.draw.rect(screen, red_color, (30, 270, t2*swFreq * 40, 20))


    # Dibujar el vector de la magnitud de tension
    pygame.draw.line(screen, black_color, circle_center, (angMag * np.cos(np.radians(ang)) + center_x, angMag * np.sin(np.radians(ang)) + center_y), 2)

    

    # Dibujar el texto en la parte superior
    screen.blit(text_surface, (10, 10))
    

    # Dibujamos el periodo de conmutacion
    for i, line in enumerate(text_lines):
        rendered_text = font_caracteristicas.render(line, True, text_color)
        screen.blit(rendered_text, (600, 10 + i * line_spacing))
    
    # Actualizar pantalla
    pygame.display.flip()
    clock.tick(60)  # Limitar a 60 FPS
