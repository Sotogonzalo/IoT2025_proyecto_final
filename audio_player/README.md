En este proyecto, se ha implementado un sistema que controla el audio de la placa de extesión ESP-LyraT-8311A v1.2.
Para compilar el sistema por primera vez se debe ejecutar el comando: Build Flash and Monitor. El sistema iniciará mostrando en consola como se conecta a la red WiFi, 
inicializará el servidor web, el SPIFF para almacenar las canciones en la flash junto con su partitions.csv, el led embebido para el parpadeo según el estado del audio, 
el servicio de NTP (tiempo_embebido) para la sincronización de la hora y algunas especificaciones más que se explicaran en el informe final.

La página web se puede acceder desde cualquier navegador ingresando la dirección IP que se muestra en la consola por parte del WIFI.
En la página web se puede controlar el audio, ver canciones disponibles, agregar nuevas canciones y eliminar canciones, y ver el log de acciones.
Tambien se podrá configurar el nombre de la red WiFi y la contraseña, así como el broker MQTT y su puerto.
Una vez inicializado el broker MQTT, publicará y subscribirá al topic "iot/demo/ESP32S2_A/rx" para recibir comandos y manipular el audio de igual manera que la página web.