/*
 * P3_EJ1.c
 *
 *  Created on: 21 mar. 2023
 *      Author: javi
 */

/*
Generar una base de temps de l�ordre de 0.1 ms (10 kHz) per controlar la lluminositat del
LED vermell.
a. Crear una base de temps d�una mil�l�sima de segon (una freq��ncia de 1 kHz).
b. Utilitzarem aquesta base de temps per commutar l�estat del LED vermell cada
cert nombre d�interrupcions d�aquesta base de temps de forma asim�trica. Per
aix�, implementeu un comptador que arribi fins a 100 i que llavors doni la volta
(i.e. passa a valer 0). A partir d�aquest comptador, el LED s�ha d�encendre
sempre que arribeu al valor m�xim de 100 i s�ha d�apagar en un valor que sigui
variable. Aquesta variable controlar� la lluminositat (relaci� estat on/off del
LED).
c. Implementeu que aquest valor sigui variable. Aquest valor �s modificar� de la
seg�ent manera, sempre respectant els l�mits anterior de m�xim 100 i m�nim 0,
ambd�s inclosos:
i. Joystick amunt: increment d�aquest valor en una quantitat step
ii. Joystick avall: decrement d�aquest valor en una quantitat step
iii. Joystick esquerra: la quantitat step �s veu decrementada en 5.
iv. Joystic dreta: la quantitat step �s veu incrementada en 5.
d. Un cop heu comprovat el correcte funcionament, baixeu ara les interrupcions a
0.1 ms (freq��ncia de 10 kHz).

*/



