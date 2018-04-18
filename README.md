# Word Box

## Description

The word box allows you to **record words and to play it again and again when moving the box from downside to upside** (like a ["Moo box"](https://en.wikipedia.org/wiki/Moo_box)).

## Components

The project is composed by multiple components :

* **1 Arduino card** ESP8266 hosting a WiFi network with a web server
* **1 accelerometer** MPU6050 to detect movements
* **1 amplifier** PAM8403 and **1 speaker** Dual Cone to broadcast the sound

## Dependencies

The project uses the following libraries :

* **spiffs** for the file system
* **mpu6050_tockn** to manage the accelerometer
* **wire** to manage the connections between all the components
* **esp8266** for the Arduino card

## How does it work ?

At the start, the Arduino card **create a WiFi network** :

* SSID : TwoMenOnePled 
* Password : Azerty1234

You will have to **connect to this WiFi network** and to **go to the following address** : 192.169.4.1

On the web server, **you can record a sound**. This sound will **be played at each box movement**.
