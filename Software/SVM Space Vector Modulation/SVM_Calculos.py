import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk

SECTOR1 = 1
SECTOR2 = 2
SECTOR3 = 3
SECTOR4 = 4
SECTOR5 = 5
SECTOR6 = 6

# Este es el tiempo de sampleo
Ts = 1
# Indice de modulacion
M = 1

# Lista para almacenar los vectores dibujados
vectors = []
current_angle = 0  # Ángulo inicial

# Funcion que devuelve los tiempos del sector
# M toma un valor entre 0 y 1
def get_time_vector(Ts, M, angle):

    #sector = getSector(angle)
    angle = angle % 60

    # Aca pongo mis constantes de calculo
    # Segun el paper t1 se cuenta a las dos porciones, por ello lo a 
    # dividir por dos para que me quede mejor para mi definicion
    CONST_A = (3 * np.sqrt(3)) / (2 * np.pi) / 2
    CONST_B = 3 / np.pi / 2

    #print(f"CONST_A: {CONST_A}, CONST_B: {CONST_B}")

    varA = CONST_A * Ts
    varB = CONST_B * Ts

    t0 = 0.0
    t1 = 0.0
    t2 = 0.0


    t2 = varB * M * np.sin(np.radians(angle))
    t1 = varA * M * np.cos(np.radians(angle)) - (t2/2)

    # El tiempo de modulacion es igual a t1 + t2
    tMod = t1 + t2

    t0 = Ts / 2 - tMod

    return t0, t1, t2



# Devuelve el sector con un angulo dado desde 0 a 359
def getSector(angle):

    angle = angle % 360

    if(angle >= 0 and angle < 60):
        return SECTOR1
    elif(angle >= 60 and angle < 120):
        return SECTOR2
    elif(angle >= 120 and angle < 180):
        return SECTOR3
    elif(angle >= 180 and angle < 240):
        return SECTOR4
    elif(angle >= 240 and angle < 300):
        return SECTOR5
    elif(angle >= 300 and angle < 360):
        return SECTOR6


    

def getSecuencia(sector):
    if(sector == SECTOR1):
        return [0, 1, 2, 7, 7, 2, 1, 0]
    elif(sector == SECTOR2):
        return [0, 3, 2, 7, 7, 2, 3, 0]
    elif(sector == SECTOR3):
        return [0, 3, 4, 7, 7, 4, 3, 0]
    elif(sector == SECTOR4):
        return [0, 5, 4, 7, 7, 4, 5, 0]
    elif(sector == SECTOR5):
        return [0, 5, 6, 7, 7, 6, 5, 0]
    elif(sector == SECTOR6):
        return [0, 1, 6, 7, 7, 6, 1, 0]

def getSecuenciaLabel(sector):
    if(sector == SECTOR1):
        return "S0-S1-S2-S7-S7-S2-S1-S0"
    elif(sector == SECTOR2):
        return "S0-S3-S2-S7-S7-S2-S3-S0"
    elif(sector == SECTOR3):
        return "S0-S3-S4-S7-S7-S4-S3-S0"
    elif(sector == SECTOR4):
        return "S0-S5-S4-S7-S7-S4-S5-S0"
    elif(sector == SECTOR5):
        return "S0-S5-S6-S7-S7-S6-S5-S0"
    elif(sector == SECTOR6):
        return "S0-S1-S6-S7-S7-S6-S1-S0"