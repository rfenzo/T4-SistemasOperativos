# Tarea 4 : *5-Card Draw Poker*

Integrantes:

	Romano Fenzo Flores- 15202356
	Martin Raddatz Gutierrez - 15634167

Supuestos a considerar:
 - El envío del *pot* (dinero actual del jugador) se realiza mediante dos bytes, incluso cuando es necesario solo uno.
 - Cuando un jugador selecciona una apuesta, ese monto es el valor **TOTAL** de sus apuestas. Para aclarar lo anterior considerese el siguiente ejemplo:
	 - Apuestas iniciales para ambos jugadores (10) 
	 - Jugador 1 apuesta 100 (lo apostado en total será 100, es decir, solo aumenta en 90 su apuesta)
	 - Jugador 2 apuesta 500 (lo apostado en total será 500, es decir, solo aumenta en 490 su apuesta)
	 - Jugador 1 iguala la apuesta de 500 (lo apostado en total será 500, solo aumenta en 400 su apuesta)
 - Se representó el número 10 de carta con la letra D. Y las 1, 11, 12 y 13 como A, J, Q, K. El resto como números decimales.

**Nota:** El codigo para calcular el puntaje de las manos de cada jugador fue adaptado del siguiente repositorio: [FiveCardDraw](https://github.com/dmjio/FiveCardDraw)
