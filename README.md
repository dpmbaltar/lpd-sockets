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
sudo apt install git build-essential python3 meson cmake pkg-config libglib2.0-dev libjson-glib-dev libjson-glib-1.0-0 doxygen
```

Para instalar dependencias en Arch Linux o derivados:

```
sudo pacman -S git base-devel python3 meson cmake glib2 json-glib doxygen
```

Para compilar y ejecutar:

1. Abrir una terminal.
2. Clonar el repositorio con `git clone https://github.com/dpmbaltar/lpd-sockets.git`.
3. Cambiar a la **carpeta raíz del proyecto** con `cd lpd-sockets`.
4. Ejecutar `meson setup build` para configurar el proceso de compilación.
  - Este paso se realiza una sola vez, a menos que se borre la carpeta generada `build`.
  - Si se cambia algún archivo `meson.build` entonces ejecutar `meson setup build --reconfigure`.
5. Ejecutar `meson compile -C build` para compilar los programas.
  - Se generan 4 ejecutables:
    1. Servidor central: `build/src/server`
    2. Servidor del clima: `build/src/weather_server`
    3. Servidor del horóscopo: `build/src/horoscope_server`
    4. Cliente: `build/src/client`

Cada programa acepta opciones y se muestran al ejecutarlo con la opción `-h`. Por ejemplo: `build/src/server -h`.
Aunque tienen valores por defecto, el servidor del horóscopo debe ser lanzado desde la **carpeta raíz** con:

```
build/src/horoscope_server -f src/horoscope.txt
```

### Windows

Descargar e instalar [MSYS2](https://www.msys2.org/). Una vez instalado abrir el entorno `MSYS2 UCRT64` desde el inicio.
Para instalar paquetes de desarrollo ejecutar:

```
pacman -S git mingw-w64-ucrt-x86_64-toolchain \
              mingw-w64-ucrt-x86_64-cmake \
              mingw-w64-ucrt-x86_64-meson \
              mingw-w64-ucrt-x86_64-glib2 \
              mingw-w64-ucrt-x86_64-json-glib \
              mingw-w64-ucrt-x86_64-doxygen
```

Para compilar y ejecutar:

1. Abrir el entorno `MSYS2 UCRT64` desde el inicio.
2. Clonar el repositorio con `git clone https://github.com/dpmbaltar/lpd-sockets.git`.
3. Cambiar a la **carpeta raíz del proyecto** con `cd lpd-sockets`.
4. Ejecutar `meson setup build --cross-file mingw-w64-ucrt-x86_64.ini` para configurar el proceso de compilación.
  - Este paso se realiza una sola vez, a menos que se borre la carpeta generada `build`.
  - Si se cambia algún archivo `meson.build` entonces ejecutar `meson setup build --cross-file mingw-w64-ucrt-x86_64.ini --reconfigure`.
5. Ejecutar `meson compile -C build` para compilar los programas.
  - Se generan 4 ejecutables:
    1. Servidor central: `build/src/server.exe`
    2. Servidor del clima: `build/src/weather_server.exe`
    3. Servidor del horóscopo: `build/src/horoscope_server.exe`
    4. Cliente: `build/src/client.exe`

## Documentación

Para generar documentación HTML:

1. Descargar [PlantUML](https://github.com/plantuml/plantuml/releases/download/v1.2023.5/plantuml.jar) y colocar el archivo `plantuml.jar` en la carpeta `doc` del proyecto.
2. Abrir una terminal o entorno MSYS2 UCRT64.
3. Cambiar a la carpeta `doc` del proyecto y ejecutar `doxygen`.
  - Se genera la documentación HTML en `doc/html`.
  - Navegar a `doc/html/index.html` desde un explorador web.

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
