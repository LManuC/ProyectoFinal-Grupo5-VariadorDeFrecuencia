import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
import time
import random

# Crear una imagen aleatoria como fondo
image = np.random.random((100, 100))

# Crear la figura y el eje
fig, ax = plt.subplots()
im = ax.imshow(image, cmap="gray")

# Dibujar el rectángulo rojo
rect = patches.Rectangle((30, 30), 20, 20, linewidth=2, edgecolor='red', facecolor='none')
ax.add_patch(rect)

# Función para actualizar la posición del rectángulo
def update_rectangle(new_x, new_y):
    rect.set_xy((new_x, new_y))
    fig.canvas.draw()
    fig.canvas.flush_events()

# Activar modo interactivo
plt.ion()
plt.show()

# Mover el rectángulo en un bucle
for i in range(1000):
    time.sleep(0.01)  # Esperar 0.5 segundos entre cada movimiento
    random_x = random.randint(0, 80)  # Generar coordenada X aleatoria (ajustar a tus límites)
    random_y = random.randint(0, 80)  # Generar coordenada Y aleatoria (ajustar a tus límites)
    update_rectangle(random_x, random_y)  # Actualizar posición del rectángulo

plt.ioff()
plt.show()
