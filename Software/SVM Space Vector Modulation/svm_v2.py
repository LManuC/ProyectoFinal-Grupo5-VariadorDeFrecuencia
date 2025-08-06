import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk

# Función para calcular el índice de modulación
def calculate_modulation_index(angle):
    angle_rad = np.radians(angle)
    return np.sin(angle_rad) * np.sqrt(3)

# Función para actualizar el gráfico polar
def update_plot1(angle_slider):
    angle = float(angle_slider.get())
    mod_index = calculate_modulation_index(angle)
    
    # Actualizar gráfico polar
    polar_ax.clear()
    polar_ax.set_theta_zero_location("N")
    polar_ax.set_theta_direction(-1)
    polar_ax.set_rticks([0.5, 1, 1.5, 2])  # Marcas radiales
    polar_ax.grid(True)
    
    # Agregar vectores básicos (representan V1, V2, V3, etc.)
    #vectors_angles = [0, 120, 240]
    #for v_angle in vectors_angles:
    #    polar_ax.quiver(np.radians(v_angle), 0, 0, 1, angles='xy', scale_units='xy', scale=1, color='gray', alpha=0.5)
    
    # Agregar vector resultante
    polar_ax.quiver(np.radians(angle), 0, 0, mod_index, angles='xy', scale_units='xy', scale=1, color='red')
    polar_ax.set_title("Gráfico Polar: Modulación SVM", va='bottom')
    
    canvas.draw()

def update_plot2(angle_slider):
    angle = float(angle_slider.get())
    mod_index = calculate_modulation_index(angle)
    
    # Eliminar solo los vectores para redibujarlos
    for artist in polar_ax.lines + polar_ax.collections:
        artist.remove()
    
    # Agregar vectores básicos (representan V1, V2, V3, etc.)
    vectors_angles = [0, 120, 240]
    for v_angle in vectors_angles:
        polar_ax.quiver(np.radians(v_angle), 0, 0, 1, angles='xy', scale_units='xy', scale=1, color='gray', alpha=0.5)
    
    # Agregar vector resultante
    polar_ax.quiver(np.radians(angle), 0, 0, mod_index, angles='xy', scale_units='xy', scale=1, color='red')
    
    canvas.draw()

def update_plot(angle_slider):
    angle = float(angle_slider.get())
    global current_angle
    current_angle = angle
    mod_index = calculate_modulation_index(angle)

    # Eliminar solo los vectores existentes (sin borrar el resto del gráfico)
    for line in vectors:
        line.remove()
    vectors.clear()

    # Dibujar los nuevos vectores
    vector1 = polar_ax.quiver(0, 0, 1, 0, angles='xy', scale_units='xy', scale=1, color='blue', alpha=0.7)
    vector2 = polar_ax.quiver(2 * np.pi / 3, 0, 1, 0, angles='xy', scale_units='xy', scale=1, color='green', alpha=0.7)
    result_vector = polar_ax.quiver(np.radians(angle), 0, mod_index, 0, angles='xy', scale_units='xy', scale=1, color='red')

    # Actualizar lista de vectores
    vectors.extend([vector1, vector2, result_vector])

    polar_ax.set_theta_direction(-1)
    polar_ax.set_rticks([0.5, 1, 1.5, 2])
    polar_ax.grid(True)

    canvas.draw()


# Función para incrementar el ángulo automáticamente
def animate():
    global current_angle
    current_angle = (current_angle + 1) % 360  # Incrementar el ángulo, reiniciando a 0 después de 359
    update_plot(current_angle)  # Redibujar el gráfico
    root.after(100, animate)  # Llamar a esta función nuevamente después de 100 ms





# Crear ventana principal
root = tk.Tk()
root.title("Modulación SVM con Gráfico Polar")

# Crear figura de Matplotlib para el gráfico polar
fig, polar_ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(6, 6))
polar_ax.set_theta_zero_location("N")
polar_ax.set_theta_direction(-1)
polar_ax.set_rticks([0.5, 1, 1.5, 2])
polar_ax.grid(True)
polar_ax.set_title("Gráfico Polar: Modulación SVM", va='bottom')


# Lista para almacenar los vectores dibujados
vectors = []
current_angle = 0  # Ángulo inicial

canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas_widget.pack()

# Crear un slider para cambiar el ángulo
angle_slider = tk.Scale(root, from_=0, to=360, orient="horizontal", label="Ángulo", command=lambda val: update_plot(angle_slider))
angle_slider.pack()

# Iniciar la animación
animate()

# Iniciar la interfaz gráfica
root.mainloop()
