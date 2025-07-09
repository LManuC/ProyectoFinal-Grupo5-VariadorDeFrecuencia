# ProyectoFinal-Grupo5-VariadorDeFrecuencia
El presente proyecto propone el desarrollo de un dispositivo electrónico capaz de alimentar un motor trifásico de hasta 1/3 HP, utilizando como fuente una tensión continua de 12V. Este conversor funciona como un variador de frecuencia, generando una salida trifásica con tensión eficaz entre fases de hasta 220 Vrms y frecuencia de salida variable entre 1 Hz y 150 Hz.
Para lograr el control del motor, se utiliza una técnica de modulación espacial (Space Vector Modulation), sin sensores montados en el eje del motor, lo que permite una instalación más sencilla, sin necesidad de encoder. El sistema está comandado por un controlador que gestiona tanto el disparo de los transistores como la interfaz de usuario y las protecciones internas.
El sistema estará contenido en una carcasa plástica especialmente diseñada para garantizar seguridad eléctrica durante la manipulación, proteger los componentes internos y permitir un montaje simple y robusto en el entorno de uso.

# Motivación
En un país como la Argentina, con grandes extensiones de tierras sin suministro eléctrico, la explotación de la energía renovable ha dado lugar a la utilización de grandes máquinas y herramientas eléctricas que tiempo atrás no hubiese sido posible. Si bien es cierto que los generadores eléctricos han dado espacio a estas instalaciones, requerían de mantenimiento periódico para que estuviesen operativas en el momento que se necesitaban, además de necesitar un material combustible que les permitiera funcionar y, en ocasiones podría generar un problema el ruido de los motores.

# Concepto del negocio
El dispositivo diseñado permitirá la conversión, acondicionamiento y regulación de la energía eléctrica para el accionamiento de un motor trifásico de baja potencia (1/3 HP) con velocidad variable generada a partir del motor de combustión interna y el alternador de equipos agrícolas o sistemas de energía solar, eólica con su correspondiente back up de energía eléctrica, posibilitando la operación de motores trifásicos destinados a tareas como sistemas de riego, transporte de materiales mediante cintas motorizadas o procesos de molienda integrados a bordo que puedan emplearse unificando todos los dispositivos que son empleados en instalaciones de energía renovable: Elevadores de tensión, inversores y variadores de frecuencia. La propuesta se enmarca en la necesidad de implementar soluciones energéticas autónomas y portátiles en entornos rurales, donde las condiciones de acceso a la red eléctrica convencional pueden ser limitadas o inexistentes.

## FODA

### Fortalezas
* La alimentación de baja tensión, nos permite meternos en un nuevo nicho. Permite trabajar con sistemas solares reducidos o baterías.
* Producto robusto y económico.
* Mantenimiento preventivo y reparación: Al conocer el diseño del equipo se puede hacer mantenimiento a los equipos y también otorgar una garantía con un taller de revisión y reparación en caso de falla.
* Productor local: Es una característica muy importante ya que el cliente tiene mayor confianza y los plazos de entrega son muchos más rápidos y económicos.
### Oportunidades
* Dada la practicidad puede ocasionar una gran demanda en zonas aisladas, remotas o rurales que se encuentren offgrid.
* Dado que es un único producto de su clase podría aparecer un nuevo rubro o sector diferente en donde encuentre utilidad. 
* El desarrollo nacional nos puede dar ventajas en licitaciones públicas o incentivos tecnológicos del gobierno.
### Debilidades
* Falta de experiencia y desconocimiento del producto puede desalentar o generar desconfianza en el producto.
* La robustez es muy importante en un controlador de un motor, se necesitan años de estudio riguroso del comportamiento y testeo del equipo para asegurar un funcionamiento ejemplar.
* La certificación eléctrica es un aspecto crucial en un equipo de esta clase y estas son muy costosas.
### Amenazas
* Posibilidad de la aparición de competencia con mejores precios y más funcionalidades.
* La escalabilidad no siempre es tan lineal, si aumenta mucho el mercado puede ocurrir que la inversión sea grande.
* El soporte post venta debe funcionar bien ya que en caso de una respuesta lenta al cliente podría perjudicar a la reputación de la marca.

# Alcance
El alcance del proyecto quedará definido en lograr, a partir de una fuente de de 12V de corriente contínua, el accionamiento de un motor trifásico de ⅓ de HP, variar su velocidad de giro, programar sesiones de trabajo diario, informar por señales típicas de 0-10V la velocidad de operación del motor, parada de emergencia, configuración de velocidades a través de un selector.

## Características eléctricas del dispositivo
Vin: 12VDC
Vout: 3 x 220VAC
fout: 0Hz - 150Hz
Pmax: 375W / 1/2HP

# Riesgos
1.  Fallas en la elección de la topología:
    -  Priorización del diseño frente a otras tareas;
    -  Ensayos varios sobre circuitos físicos;
2.  Rendimiento simulado insuficiente
    -  Priorización del diseño frente a otras tareas;
    -  Ensayos varios sobre circuitos físicos;
    -  Pruebas sobre motores utilizando variadores de frecuencia comerciales con entrada de contínua;
3.  No se consiguen los materiales en el mercado local
    -  Diseño temprano de PCB
    -  Compra en exceso de dispositivos críticos
4.  El transformador está mal construído y no entrega la potencia necesaria
    -  Fabricación de transformador con un proveedor especializado
5.  Al PCB le faltaron rutear una o más pistas
    -  Validaciones cruzadas de footprints
    -  Validaciones cruzadas de esquemáticos
    -  Validaciones cruzadas de PCB
6.  Pérdida de componentes
    -  Compra en exceso de dispositivos críticos
    -  Compra en exceso de dispositivos pequeños
    -  Compra en exceso de dispositivos sin marking
7.  Error en el diseño del footprint
    -  Validaciones cruzadas de footprints
    -  Checkbox en componentes para registrar validación
    -  Agregado de hoja de datos al esquemático
    -  Agregado de nombre del componente según el fabricante
8.  Errores de diseño hacen que la tensión no sea lo suficientemente alta
    -  Diseño autodefensivo con tensiones sobredimensionadas
    -  Cálculo de convertido con duty de respaldo
9. Rotura del banco de pruebas
    -  Fabricación de partes críticas de repuesto
    -  Colaboración externa especializada
10. Dificultades para simular el desfasaje
    -  Sobreestimación de los tiempos de trabajo
11. Error en el cálculo de la corriente máxima de algún componente y la corriente quedó sobredimensionada
    -  Justificación y documentación de los componentes elegidos
    -  Compra en exceso de dispositivos críticos
12. Falla el circuito y se quema una de las placas de potencia
    -  Fabricación de placa extra
    -  Compra en exceso de dispositivos críticos
    -  Circuitería de protección ruteada sin montar
13. El transformador está mal calculado y no entrega la potencia necesaria
    -  Ensayos exhaustivos durante el diseño
14. Ruido eléctrico interfiere en el display
    -  Fabricación de pantalla metálica para separar circuitos ruidosos
15. El motor no arranca por falta de potencia en la entrada
    -  Pruebas con batería de auto de respaldo

# Calidad

|Funcionalidad                     |  Importancia  |
|----------------------------------|---------------|
|Tensión de salida                 |  16,3%        |
|Tensión entrada                   |  20,7%        |
|Frecuencia de salida              |   7,1%        |
|Protecciones contra sobrecorriente|   6,9%        |
|Aislamiento entrada salida        |  12,1%        |
|Algoritmo modulación              |   9,6%        |
|Display con menú                  |  10,6%        |
|GPIOs                             |   7,7%        |
|Access point                      |   9,0%        |
