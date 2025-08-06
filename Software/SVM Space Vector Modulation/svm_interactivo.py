import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk

# Función para calcular el índice de modulación
def calculate_modulation_index(angle):
    angle_rad = np.radians(angle)
    return np.sin(angle_rad) * np.sqrt(3)

# Función para actualizar el gráfico
def update_plot(angle_slider):
    angle = float(angle_slider.get())
    mod_index = calculate_modulation_index(angle)

    # Actualizar datos del gráfico
    ax.clear()
    ax.plot(angles, modulation_indices, label="Modulación")
    ax.scatter([angle], [mod_index], color='red', label=f"Índice = {mod_index:.2f}")
    ax.set_xlabel("Ángulo (grados)")
    ax.set_ylabel("Índice de Modulación")
    ax.set_title("Modulación SVM vs Ángulo")
    ax.legend()
    canvas.draw()

# Crear ventana principal
root = tk.Tk()
root.title("Modulación SVM")

# Crear una figura de Matplotlib
fig, ax = plt.subplots(figsize=(6, 4))
angles = np.linspace(0, 360, 100)
modulation_indices = [calculate_modulation_index(a) for a in angles]
ax.plot(angles, modulation_indices, label="Modulación")
ax.set_xlabel("Ángulo (grados)")
ax.set_ylabel("Índice de Modulación")
ax.set_title("Modulación SVM vs Ángulo")
ax.legend()

canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas_widget.pack()

# Crear un slider para cambiar el ángulo
angle_slider = tk.Scale(root, from_=0, to=360, orient="horizontal", label="Ángulo", command=lambda val: update_plot(angle_slider))
angle_slider.pack()

# Iniciar la interfaz gráfica
root.mainloop()