# cwpx_cgi

_Biblioteca para desarrollo web en C siguiendo conceptos similares a PHP, JSP, ASP.NET y Django._

## Comenzando 🚀

_Estas instrucciones te permitirán poner en funcionamiento el proyecto en tu máquina local para propósitos de desarrollo y pruebas._

### Pre-requisitos 📋

_Para utilizar el software se necesita un servidor web con soporte para CGI. En este proyecto se utiliza Apache._

```
* Instalar Apache en Windows
  [Apache](https://sourceforge.net/projects/apache2portable/) - Apache Portable para Windows
  
  Luego de la "instalación" se debe configurar el archivo ApachePortable/conf/httpd.conf para configurar la ejecución de CGI en el directorio web del servidor. La extensión para los archivos CGI será ".cwpx":
  
  <Directory "C:/ApachePortable/htdocs">
    # Indexes Includes FollowSymLinks SymLinksifOwnerMatch ExecCGI MultiViews
    Options Indexes FollowSymLinks ExecCGI
    AllowOverride All
    Order allow,deny
    Allow from all

    AddHandler cgi-script .cwpx
  </Directory>
  
  En este caso, el directorio web del servidor será "C:/ApachePortable/htdocs"
  
  
  
* Instalar Apache en Linux (Ubuntu)
  sudo apt-get install apache2
  
  Luego de la instalación se debe configurar el archivo /etc/apache2/sites-available/default para configurar la ejecución de CGI en el directorio web del servidor:
  
  <VirtualHost *:80>
    ...
    <Directory /var/www/>
      Options ... ExecCGI
      ...
      AddHandler cgi-script .cwpx
      
      
  Para confirmar los cambios se debe reiniciar el servidor web:
    sudo service apache2 restart
    
    
  En este caso, el directorio web del servidor será "/var/www"



* Instalar Apache en Mac (Mountain Lion):
  Apache viene pre-instalado en Mountain Lion, solamente se debe iniciar el servicio:
  sudo apachectl start
  
  Luego, se debe configurar el archivo /etc/apache2/httpd.conf para configurar la ejecución de CGI en el directorio web del servidor, como se observa en la sección de Ubuntu.
  
  En este caso, el directorio web del servidor será "/Library/WebServer/Documents"
  
  
Nota: Las rutas y nombres de los archivos del servidor Apache podrían variar de un sistema a otro.
```

### Instalación 🔧

_Para tener un entorno de desarrollo ejecutándose, además de la instalación un servidor web y su respectiva configuración para ejecutar CGI, se debe configurar el sistema para que reconozca la ruta donde se encuentra la biblioteca del marco de desarrollo._

_Para compilar la biblioteca utilice los siguientes comandos:_

```
cd cwpx_cgi

make

```

_O bien, :_

```
make -f "Makefile - Windows"
```

_También puede descargar una imagen de Docker con un entorno Linux+Apache+GCC con la configuración instalada:_

```
docker pull enrmng6/cwpx_image:latest
```

_Los comandos anteriores crean archivos importantes en la carpeta **lib**: cwpx_core.dll (o libcwpx_core.so) y cwpx_interface.o. Esos dos archivos deberán ser referenciados a la hora de compilar los scripts CGI. Además, la ruta de la carpeta **lib**, en especial la biblioteca cwpx_core.dll (o libcwpx_core.so) debe ser configurada para que sea reconocida por el servidor web, como se indicó anteriormente._


_Los archivos CGI compilados deben ser colocados en el directorio web del servidor, ya sea en una carpeta exclusiva para archivos CGI, o en cualquier ruta del directorio web. Los CGI compilados utilizan la biblioteca del marco de desarrollo, la cual es una biblioteca dinámica y por lo tanto, al ser ejecutados se les debe especificar la ruta donde se encuentra la biblioteca. En algunos sistemas opertativos, por el momento, las pruebas se están realizando colocando la biblioteca (el .dll o el .so) en la misma carpeta donde se ejecuta el CGI. También, se puede establecer la ruta de la biblioteca en una variable de entorno como "PATH"._

_En Ubuntu, colocar la biblioteca en la misma carpeta que el CGI no funciona, ni tampoco establecer la ruta de la biblioteca en la variable de entorno "PATH", ni establecer la variable de entorno "LD_LIBRARY_PATH". Como solución, se puede crear un archivo en la ruta /etc/ld.so.conf.d, por ejemplo, /etc/ld.so.conf.d/cwpx.conf. El contenido del archivo será la ruta donde se encuentra la biblioteca:_

```
coloque_aquí_la_ruta_absoluta_sin_comillas
```

_Luego de guardar los cambios en el archivo /etc/ld.so.conf.d/cwpx.conf, se ejecuta el siguiente comando para cargar la biblioteca en el sistema:_

```
ldconfig
```

_En la carpeta **include** se encuentran los archivos de encabezado (header files) que deberán ser incluídos mediante la directiva **#include** en un archivo C. En **lib** se encuentran los archivos binarios que deberán ser especificados en la compilación de los archivos C. En la carpeta **test** se encuentran ejemplos de scripts CGI utilizando el marco de desarrollo._

## Desarrollando los scripts ⚙️

_En esta sección se explica cómo crear los scripts en C que utilizan el marco de desarrollo_

### Crear y compilar los scripts 🔩

_En la carpeta **test** ya hay ejemplos de scripts. Para crear un script nuevo, cree un archivo C nuevo, por ejemplo, **prueba.c**. Coloque el siguiente código:_

```
#include <cwpx.h>

void do_http(Request request, Response response){
	
	response.write("Hola Mundo ...");
	
	return;
}

```

_Para compilar el script utilice el siguiente comando (cambie las rutas si es necesario):_

```
gcc -I"../include" prueba.c -o prueba.cwpx -L"../lib" -lcwpx
```

_Si no ocurrió ningún error, ahora se cuenta con un archivo llamado **prueba.cwpx**_

### Probar los scripts ⌨️

_El archivo generado con la compilación es el archivo que debe ser probado. Al tratarse de CGI, se puede probar el archivo ejecutándolo directamente desde una terminal o línea de comandos. Sin embargo, un servidor web establece variables de entorno y algunas otras configuraciones necesarias para ejecutar los CGI que no pueden ser emuladas fácilmente en una línea de comandos, por lo que lo mejor será probar el script directamente desde un servidor web._

_Habiendo configurado la ruta de la carpeta **lib** para que sea reconocida por el servidor web, el archivo compilado **prueba.cwpx** se coloca en el directorio web del servidor (o en la ruta específica para scripts CGI). Luego, mediante un cliente web como por ejemplo un navegador web, se realiza una solicitud al archivo:_

```
http://localhost/mis_scripts/prueba.cwpx
```

_Si todo marchó bien. se mostrará el resultado del script, de otro modo se mostrará un error "500 Internal Server Error" o "404 Not Found"._

## Despliegue 📦

_El despliegue se realiza de manera "natural", es decir, los archivos compilados son colocados directamente en el directorio web del servidor y estarán inmediatamente disponibles para cuando el recurso sea solicitado por un cliente web._

## Construido con 🛠️

_Para el desarrollo de este proyecto se utilizaron las siguientes herramientas:_

* [DevCpp, NotePad++, GEdit, Xcode, Visual Studio Code](https://notepad-plus-plus.org/, https://wiki.gnome.org/Apps/Gedit, https://developer.apple.com/xcode/) - Para escribir el código C
* [MinGW, GCC, CLang](https://www.mingw-w64.org/, https://gcc.gnu.org/, https://clang.llvm.org/) - Para compilar el código C
* [Apache](https://www.apache.org/) - Para ejecutar el código C en la Web

## Contribuyendo 🖇️

Por favor lee el [CONTRIBUTING.md](https://gist.github.com/alenmumo/xxxxxx) para detalles de nuestro código de conducta, y el proceso para enviarnos pull requests.

## Wiki 📖

Puede encontrar la documentación correspondiente en la carpeta **doc/html**. Dicha carpeta se debe colocar en el directorio web de un servidor web para poder ser visualizada. En el caso de que haya descargado la imagen de Docker, podrá acceder a la documentación bajo la ruta "http://ipcontainer**/cwpx/html/index.html**"

Puedes encontrar mucho más de cómo utilizar este proyecto en nuestra [Wiki](https://github.com/alenmumo/cwpx_cgi/wiki).

## Versionado 📌

Usamos [GitHub](https://github.com) para el versionado.

## Autores ✒️

_._

* **Alejandro Enrique Muñoz Monge** - *Trabajo Inicial* - [alenmumo](https://github.com/alenmumo)

También puedes mirar la lista de todos los [contribuyentes](https://github.com/alenmumo/cwpx_cgi/contributors) quienes han participado en este proyecto. 

## Licencia 📄

Este proyecto está bajo la Licencia (Tu Licencia) - mira el archivo [LICENSE.md](LICENSE.md) para detalles

## Expresiones de Gratitud 🎁

* Comenta a otros sobre este proyecto 📢
* Invita una cerveza 🍺 o un café ☕ a alguien del equipo. 
* Da las gracias públicamente 🤓.
* etc.

---
⌨️ con ❤️ por [cwpx](https://github.com/alenmumo) 😊
