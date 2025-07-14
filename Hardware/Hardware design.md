# Comunicación entre microcontraldores

## 1. UART (Universal Asynchronous Receiver/Transmitter)
_Tipo:_ Asíncrona

_Velocidad típica:_ Hasta 1 Mbps (aunque la mayoría trabaja entre 9600 y 115200 bps)

_Ventajas:_
* Muy simple de implementar (incluso por software si no hay hardware UART)
* No necesita reloj compartido

_Desventajas:_
* Sin verificación de integridad robusta
* Solo útil para distancias cortas

_Ideal para:_ Comunicación punto a punto a distancias cortas y baja/moderada velocidad

## 2. SPI (Serial Peripheral Interface)
_Tipo:_ Síncrona (requiere línea de reloj)

_Velocidad típica:_ De 1 a 20 Mbps (puede llegar a más dependiendo del microcontrolador)

_Ventajas:_
* Muy rápida
* Full duplex
* Buena para cortas distancias dentro de un mismo PCB

_Desventajas:_
* Requiere más pines (MOSI, MISO, SCLK y a veces CS)
* No hay detección de errores integrada

_Ideal para:_ Comunicación rápida entre micros en un mismo sistema o módulo

## Conclusión

Elegimos SPI por subre UART para poder aprovechar más la velocidad del bus. Creemos que la implemetación en software no debería cambiar entre una u otra, por lo que no habría una ventaja o desventaja de una sobre la otra.

Debemos tener el cuidado suficiente de la con la señal ya que, pese a ser un único producto y un ansamblado fijo, la comunicación se dará entre dos PCBs diferentes a través de un header.

# Pulsadores

Para el traclado matricial utilizaremos los pulsadores con vástago sobresaliente para poder enganchar con la tecla impresa.

# Teclado

Utilizar por lo menos 8 teclas, pero no más (o no mucho más de 10).

El Start y stop cuentan dentro de este teclaod matricial pero el botón golpe de puño para parada de emergencia iría aparte.

Usaríamos un relay para cortar en bus de contínua, comandado por la parada de emergencia y el STM32.

# Dimensiones

El gabinete va a tener un tamaño de 130mm x 325mm (Sujeto a modificaciones) para respetar una relación de aspecto de 2.5.

La idea sería incluir en la parte superior un cooler de 120mm de lado.

# Señales a medir por el ESP

* Nivel de tensión del bus de contínua
* Corriente del bus de contínua

# Conectores

## Señales para el ADC
1. 5V
2. GND
3. Corriente
4. Tensión
5. GND

## Comunicaciones
1. GND
2. NSS
3. SCK
4. MISO
5. MOSI

## Señales de una placa a la otra

1. 0-10V
2. Selección de velocidades
3. Start
4. Stop

## Botones de start y stop

Borneras externas para conectar pulsadores externos en caso de montar el gabinete del producto dentro de otro gabinete.

# Alimentación

1. +12V
2. GND

1. +5V
2. GND