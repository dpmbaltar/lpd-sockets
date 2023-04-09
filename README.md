# lpd-sockets

Laboratorio de Programación Distribuída - Sockets

## Instrucciones

Instalar paquetes de desarrollo para C y [Meson](https://mesonbuild.com/). Ver instrucciones según el sistema operativo.
Preferentemente utilizar [VSCode](https://code.visualstudio.com/) con las extensiones:

- [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
- [Meson](https://marketplace.visualstudio.com/items?itemName=mesonbuild.mesonbuild)

### Linux

Para instalar paquetes de desarrollo en Ubuntu o derivados:

```
sudo apt update
sudo apt install git build-essential python3 meson make cmake libglib2.0-dev libjson-glib-dev libjson-glib-1.0-0 doxygen
```

Para instalar paquetes de desarrollo en Arch Linux o derivados:

```
sudo pacman -S git base-devel python3 meson make cmake glib2 json-glib doxygen
```

Para compilar y ejecutar:

1. Abrir una terminal en la **carpeta raíz del proyecto** y ejecutar el comando `meson setup build` para configurar el
   proceso de compilación. Este paso se realiza una sola vez, a menos que se cambien parámetros de algún archivo
   `meson.build`, deberá ejecutarse `meson setup build --reconfigure`. Esto genera una carpeta `build`.
2. Ejecutar `meson compile -C build` (**¡siempre desde el directorio raíz!**) para compilar los programas.
3. Se generan los 4 ejecutables:
    1. Servidor central: `build/src/server`
    2. Servidor del clima: `build/src/weather_server`
    3. Servidor del horóscopo: `build/src/horoscope_server`
    4. Cliente: `build/src/client`
4. Cada programa cuenta con opciones que pueden verse al ejecutar el programa con la opción `-h`.
   Por ejemplo: `build/src/server -h`
5. Si bien cada programa tiene valores por defecto para cada opción, el servidor del horóscopo tiene una excepción:
   debe ser lanzado con el parámetro `-f`, que indica el archivo de los datos del horóscopo.
   Por ejemplo, desde la raíz del proyecto ejecutar: `build/src/horoscope_server -f src/horoscope.txt`

### Windows

Descargar e instalar [MSYS2](https://www.msys2.org/). Una vez instalado abrir el entorno UCRT64 desde el inicio.

Para instalar paquetes de desarrollo en el entorno UCRT64 ejecutar:

```
pacman -S git mingw-w64-ucrt-x86_64-toolchain \
              mingw-w64-ucrt-x86_64-cmake \
              mingw-w64-ucrt-x86_64-meson \
              mingw-w64-ucrt-x86_64-glib2 \
              mingw-w64-ucrt-x86_64-json-glib \
              mingw-w64-ucrt-x86_64-doxygen
```

Para compilar y ejecutar desde el entorno UCRT64:

1. Ubicarse en la **carpeta raíz del proyecto** (i.e. `cd lpd-sockets`) y ejecutar el comando `meson setup build --cross-file mingw-w64-ucrt-x86_64.ini` para configurar el
   proceso de compilación. Este paso se realiza una sola vez, a menos que se cambien parámetros de algún archivo
   `meson.build`, deberá ejecutarse `meson setup build  --cross-file mingw-w64-ucrt-x86_64.ini --reconfigure`. Esto genera una carpeta `build`.
2. El resto de los pasos es igual a Linux. Aunque los programas se generan con la extensión .exe, pueden omitirse desde éste entorno.

## Extensiones en VSCode

1. En la pestaña de la extensión Meson se muestran "Targets" para compilar los ejecutables por separado, por ejemplo:
    - lpd-sockets 0.1.0
        - Targets
            - client
            - server
            - weather_server
            - horoscope_server
2. En la pestaña de ejecución y depuración, están definidas las opciones:
- (gdb) Iniciar servidor del clima
- (gdb) Iniciar servidor del horóscopo
- (gdb) Iniciar servidor
- (gdb) Iniciar cliente

En la carpeta `.vscode` del proyecto se encuentran los archivos de configuración para las extensiones:

- `c_cpp_properties.json` - para definir "include paths", etc.
- `launch.json` - para definir como ejecutar/depurar los programas desde VSCode.
