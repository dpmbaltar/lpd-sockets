# lpd-sockets

Laboratorio de Programación Distribuída - Sockets

## Instrucciones

Instalar paquetes de desarrollo para C y [Meson](https://mesonbuild.com/) (ver instrucciones según el sistema operativo).
Preferentemente utilizar [VSCode](https://code.visualstudio.com/) con las extensiones:

- [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
- [Meson](https://marketplace.visualstudio.com/items?itemName=mesonbuild.mesonbuild)

### Linux

Para instalar dependencias en Ubuntu o derivados:

```
sudo apt update
sudo apt install git build-essential python3 meson cmake libglib2.0-dev libjson-glib-dev libjson-glib-1.0-0 doxygen
```

Para instalar dependencias en Arch Linux o derivados:

```
sudo pacman -S git base-devel python3 meson cmake glib2 json-glib doxygen
```

Para compilar y ejecutar:

1. Abrir una terminal en la **carpeta raíz del proyecto**.
2. Ejecutar `meson setup build` para configurar el proceso de compilación.
    - Este paso se realiza una sola vez, a menos que se borre la carpeta generada `build`.
    - Si se cambia algún archivo `meson.build` ejecutar `meson setup build --reconfigure`.
3. Ejecutar `meson compile -C build` para compilar los programas.
4. Se generan 4 ejecutables:
    1. Servidor central: `build/src/server`
    2. Servidor del clima: `build/src/weather_server`
    3. Servidor del horóscopo: `build/src/horoscope_server`
    4. Cliente: `build/src/client`

Cada programa acepta opciones que pueden verse al ejecutar el programa con la opción `-h`.
Por ejemplo: `build/src/server -h`.

Cada programa tiene valores por defecto en cada opción, **excepto** una en el servidor del horóscopo:

- Debe ser lanzado con la opción `-f ARCHIVO.TXT`, que indica el archivo de los datos del horóscopo.
  Por ejemplo, desde la raíz del proyecto ejecutar:

```
build/src/horoscope_server -f src/horoscope.txt
```

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

1. Los pasos son los mismos que en Linux, **excepto el primero**, que debe agregarse la opción `--cross-file ARCHIVO`.
   En este caso es: `meson setup build --cross-file mingw-w64-ucrt-x86_64.ini`
2. Seguir el resto de los pasos al igual que se explica en Linux.

## Documentación

Para generar documentación HTML:

1. Descargar [PlantUML](https://github.com/plantuml/plantuml/releases/download/v1.2023.5/plantuml.jar) y colocar el archivo `plantuml.jar` en la carpeta `doc` del proyecto.
2. Abrir una terminal en la carpeta `doc` del proyecto y ejecutar `doxygen`.
3. Se genera la documentación HTML en `doc/html`.
4. Navegar a `doc/html/index.html` desde un explorador web.

## Extensiones en VSCode (Opcional)

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
