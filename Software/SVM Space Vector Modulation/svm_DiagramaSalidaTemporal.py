import numpy as np
import matplotlib.pyplot as plt
from SVM_Calculos import getSector, get_time_vector, getSecuenciaLabel, getSecuencia


SECTOR1 = 1
SECTOR2 = 2
SECTOR3 = 3
SECTOR4 = 4
SECTOR5 = 5
SECTOR6 = 6

# Estas son la salida
Q1 = 0
Q2 = 1
Q3 = 2

# Este es el tiempo de sampleo
Ts = 1
# Indice de modulacion
M = 1

# Mapeo de vectores activos por sector
# Cada entrada define el orden de vectores [V0, Vx, Vy, V7]
SECTOR_VECTORS = {
    SECTOR1: [2, 1, 0],
    SECTOR2: [1, 2, 0],
    SECTOR3: [1, 0, 2],
    SECTOR4: [0, 1, 2],
    SECTOR5: [0, 2, 1],
    SECTOR6: [2, 0, 1],
}


# Generar la forma de onda de una salida
def generar_vector_salida_temporal(Ts, M, angSwitch, tRise, tf) :
    t = []
    out1 = []
    out2 = []
    out3 = []

    # Esta es la cantidad de switcheos que voy a tener
    cantSwitchs = int(tf / Ts)

    # Angulo switch es el avance de angulo luego de todo un ciclo 
    # de switch 

    # Estos son los tiempos y angulos al principio de mi iteracion
    # En cada paso del ciclo for voy a calcular un nuevo switcheo
    angAct = 0
    tiempoAct = 0

    for i in range(cantSwitchs) :



        # Voy a calcular el tiempo para cada paso
        t0, t1, t2 = get_time_vector(Ts, M, angAct)
        sector = getSector(angAct)
        ordenActivacion = SECTOR_VECTORS[sector]

        estadoOut = [0, 0, 0]

        # Pre paso 1
        tiempoAct += t0/2
        t.append(tiempoAct)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])
        
        # Paso 1
        estadoOut[ordenActivacion[0]] = 1
        t.append(tiempoAct + tRise)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Pre paso 2
        tiempoAct += t1
        t.append(tiempoAct)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Paso 2
        estadoOut[ordenActivacion[1]] = 1
        t.append(tiempoAct + tRise)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Pre paso 3
        tiempoAct += t2
        t.append(tiempoAct)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Paso 3
        estadoOut[ordenActivacion[2]] = 1
        t.append(tiempoAct + tRise)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])
        
        # Pre paso 4
        tiempoAct += t0
        t.append(tiempoAct)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Paso 4
        estadoOut[ordenActivacion[2]] = 0
        t.append(tiempoAct + tRise)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])
        
        # Pre paso 5
        tiempoAct += t2
        t.append(tiempoAct)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Paso 5
        estadoOut[ordenActivacion[1]] = 0
        t.append(tiempoAct + tRise)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Pre paso 6
        tiempoAct += t1
        t.append(tiempoAct)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        # Paso 6
        estadoOut[ordenActivacion[0]] = 0
        t.append(tiempoAct + tRise)
        out1.append(estadoOut[0])
        out2.append(estadoOut[1])
        out3.append(estadoOut[2])

        tiempoAct += t0/2
        angAct += angSwitch

    return t, out1, out2, out3

        
def calcular_fase(t, out1, out2, out3, VBus) :
    faseRS = []
    faseST = []

    for i in range(len(t)):
        faseRS.append((out1[i] - out2[i]) * VBus)
        faseST.append((out2[i] - out3[i]) * VBus)

    return t, faseRS, faseST

def calcular_rms(signal):
    return np.sqrt(np.mean(np.square(signal)))

# 🚀 Ejemplo de uso
Ts = 1e-3  # 1 ms período de muestreo
M = 0.8    # Índice de modulación
angle_deg = 45  # Ángulo espacial

t, out1, out2, out3 = generar_vector_salida_temporal(Ts, M, angSwitch=1.8, tRise=1e-6, tf=0.5)

t2, faseRS, faseST = calcular_fase(t, out1, out2, out3, 1)

rms_RS = calcular_rms(faseRS)
rms_ST = calcular_rms(faseST)

print(f"Valor RMS de fase RS: {rms_RS}")
print(f"Valor RMS de fase ST: {rms_ST}")



# Plot
plt.figure(figsize=(10,5))

# Subplot 1
plt.subplot(2, 1, 1)
plt.plot(t, out1, label="Out 1")
plt.plot(t, out2, label="Out 2")
plt.plot(t, out3, label="Out 3")
plt.title("Forma de onda de SVM")
plt.xlabel("Tiempo [s]")
plt.ylabel("Valor logico de la salida")
plt.grid(True)
plt.legend()

# Subplot 2
plt.subplot(2, 1, 2)
plt.plot(t2, faseRS, label="Dif R-S")
plt.plot(t2, faseST, label="Dif S-T")
plt.title("Valor de fase")
plt.xlabel("Tiempo [s]")
plt.ylabel("Valor lógico")
plt.grid(True)
plt.legend()

# Ajuste final
plt.tight_layout()
plt.show()