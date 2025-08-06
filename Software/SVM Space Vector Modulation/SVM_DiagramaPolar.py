import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk

# Lista para almacenar los vectores dibujados
vectors = []
current_angle = 0  # Ángulo inicial

# Función para actualizar el gráfico polar
def update_plot(angle):
    global vectors

    #current_angle = angle
    #v1, v2, v3 = calculate_modulation_index(angle)

    # Eliminar solo los vectores existentes (sin borrar el resto del gráfico)
    for line in vectors:
        line.remove()
    vectors.clear()

    magBase = 1.3

    ang1 = 90
    ang2 = 210
    ang3 = 330

    mag1 = magBase * np.cos(np.radians(ang1 - angle)) 
    mag2 = magBase * np.cos(np.radians(ang2 - angle)) 
    mag3 = magBase * np.cos(np.radians(ang3 - angle)) 


    magResultX = mag1 * np.cos(np.radians(ang1)) + mag2 * np.cos(np.radians(ang2)) + mag3 * np.cos(np.radians(ang3))
    magResultY = mag1 * np.sin(np.radians(ang1)) + mag2 * np.sin(np.radians(ang2)) + mag3 * np.sin(np.radians(ang3))

    magResult = np.sqrt(magResultX**2 + magResultY**2)

    if mag1 < 0:
        ang1 += 180
        mag1 = -mag1

    if mag2 < 0:
        ang2 += 180
        mag2 = -mag2

    if mag3 < 0:
        ang3 += 180
        mag3 = -mag3

    # Dibujar los nuevos vectores
    vector1 = polar_ax1.quiver(np.radians(ang1), 0, 0, mag1, angles='xy', scale_units='xy', scale=1, color='blue', alpha=0.7)
    vector2 = polar_ax1.quiver(np.radians(ang2), 0, 0, mag2, angles='xy', scale_units='xy', scale=1, color='green', alpha=0.7)
    vector3 = polar_ax1.quiver(np.radians(ang3), 0, 0, mag3, angles='xy', scale_units='xy', scale=1, color='red', alpha=0.7)
    result_vector = polar_ax1.quiver(np.radians(angle), 0, 0, magResult, angles='xy', scale_units='xy', scale=1, color='black')

    
    
    polar_ax1.set_theta_zero_location("N")
    polar_ax1.set_theta_direction(-1)
    polar_ax1.set_rticks([0.5, 1, 1.5, 2])
    polar_ax1.grid(True)



    if(angle < 30) :
        mag1 = 0
    elif(angle >= 150 and angle < 210) :
        mag1 = 0
    elif(angle >= 330) :
        mag1 = 0
    
    if(angle >= 90 and angle < 150) :
        mag2 = 0
    elif(angle >= 270 and angle < 330) :
        mag2 = 0
    
    if(angle >= 30 and angle < 90) :
        mag3 = 0
    elif(angle >= 210 and angle < 270) :
        mag3 = 0

 

    if(mag1 != 0) :  # si el vector 1 no es cero entonces el otro tiene modulo mag2 + mag3
        
        catetoX = mag1 * np.cos(np.radians(60)) + (mag2 + mag3)
        catetoY = mag1 * np.sin(np.radians(60))
        magResult = np.sqrt(catetoX**2 + catetoY**2)

    elif(mag2 != 0) : # si el vector 2 no es cero entonces el otro tiene modulo mag1 + mag3
        catetoX = mag2 * np.cos(np.radians(60)) + (mag1 + mag3)
        catetoY = mag2 * np.sin(np.radians(60))
        magResult = np.sqrt(catetoX**2 + catetoY**2)


    vector4 = polar_ax2.quiver(np.radians(ang1), 0, 0, mag1, angles='xy', scale_units='xy', scale=1, color='blue', alpha=0.7)
    vector5 = polar_ax2.quiver(np.radians(ang2), 0, 0, mag2, angles='xy', scale_units='xy', scale=1, color='green', alpha=0.7)
    vector6 = polar_ax2.quiver(np.radians(ang3), 0, 0, mag3, angles='xy', scale_units='xy', scale=1, color='red', alpha=0.7)
    result_vector2 = polar_ax2.quiver(np.radians(angle), 0, 0, magResult, angles='xy', scale_units='xy', scale=1, color='black')

    polar_ax2.set_theta_zero_location("N")
    polar_ax2.set_theta_direction(-1)
    polar_ax2.set_rticks([0.5, 1, 1.5, 2])
    polar_ax2.grid(True)

    # Actualizar lista de vectores
    vectors.extend([vector1, vector2, vector3, result_vector, vector4, vector5, vector6, result_vector2])
    canvas.draw()

# Función para sincronizar el slider y actualizar el gráfico
def slider_update(val):
    angle = int(val)
    update_plot(angle)

# Función para incrementar el ángulo automáticamente
def animate():
    global current_angle
    current_angle = (current_angle + 2) % 360  # Incrementar el ángulo, reiniciando a 0 después de 359
    #angle_slider.set(current_angle)  # Sincronizar el slider con la animación
    update_plot(current_angle)  # Redibujar el gráfico
    root.after(1, animate)  # Llamar a esta función nuevamente después de 100 ms

# Crear ventana principal
root = tk.Tk()
root.title("Modulación SVM con Gráfico Polar")

# Crear una figura con dos graficos polares
fig, (polar_ax1, polar_ax2) = plt.subplots(1, 2, subplot_kw={'projection': 'polar'}, figsize=(10, 5))

# Configurar el primer grafico polar
polar_ax1.set_title("Grafico Polar 1", va='bottom')
polar_ax1.set_theta_zero_location("N")
polar_ax1.set_theta_direction(-1)
polar_ax1.set_rticks([0.5, 1, 1.5, 2])
polar_ax1.grid(True)
polar_ax1.legend()

# Configurar el segundo grafico polar
polar_ax2.set_title("Grafico Polar 2", va='bottom')
polar_ax2.set_theta_zero_location("N")
polar_ax2.set_theta_direction(-1)
polar_ax2.set_rticks([0.5, 1, 1.5, 2])
polar_ax2.grid(True)



canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas_widget.pack()

# Crear un slider para cambiar el ángulo
#angle_slider = tk.Scale(root, from_=0, to=360, orient="horizontal", label="Ángulo", command=slider_update)
#angle_slider.pack()

# Iniciar la animación
animate()

# Iniciar la interfaz gráfica
root.mainloop()
