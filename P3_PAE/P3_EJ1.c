/*
 * P3_EJ1.c
 *
 *  Created on: 21 mar. 2023
 *      Author: javi
 */

/*
Generar una base de temps de l’ordre de 0.1 ms (10 kHz) per controlar la lluminositat del
LED vermell.
a. Crear una base de temps d’una mil·lèsima de segon (una freqüència de 1 kHz).
b. Utilitzarem aquesta base de temps per commutar l’estat del LED vermell cada
cert nombre d’interrupcions d’aquesta base de temps de forma asimètrica. Per
això, implementeu un comptador que arribi fins a 100 i que llavors doni la volta
(i.e. passa a valer 0). A partir d’aquest comptador, el LED s’ha d’encendre
sempre que arribeu al valor màxim de 100 i s’ha d’apagar en un valor que sigui
variable. Aquesta variable controlarà la lluminositat (relació estat on/off del
LED).
c. Implementeu que aquest valor sigui variable. Aquest valor és modificarà de la
següent manera, sempre respectant els límits anterior de màxim 100 i mínim 0,
ambdós inclosos:
i. Joystick amunt: increment d’aquest valor en una quantitat step
ii. Joystick avall: decrement d’aquest valor en una quantitat step
iii. Joystick esquerra: la quantitat step és veu decrementada en 5.
iv. Joystic dreta: la quantitat step és veu incrementada en 5.
d. Un cop heu comprovat el correcte funcionament, baixeu ara les interrupcions a
0.1 ms (freqüència de 10 kHz).

*/



