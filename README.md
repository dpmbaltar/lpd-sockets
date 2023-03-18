# lpd-sockets

Laboratorio de Programación Distribuída - Sockets

## Instrucciones

Pre-requisitos: tener instalado paquetes de desarrollo para C y [Meson](https://mesonbuild.com/).
Preferentemente utilizar [VSCode](https://code.visualstudio.com/) con las extensiones:

- [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
- [Meson](https://marketplace.visualstudio.com/items?itemName=mesonbuild.mesonbuild)

### Linux

Para instalar paquetes de desarrollo en Ubuntu/Mint/Debian:

```
sudo apt update
sudo apt install git build-essential python3 python3-pip python3-setuptools python3-wheel ninja-build meson
```

Arch Linux/Manjaro:

```
sudo pacman -S git base-devel python meson
```

Para compilar y ejecutar:

1. Abrir una terminal en la carpeta raíz del proyecto y ejecutar el comando `meson setup builddir` para configurar el
   proceso de compilación. Este paso se realiza una sola vez, a menos que se cambien parámetros de algún archivo
   `meson.build`, deberá ejecutarse `meson setup builddir --reconfigure`.
2. Cambiar a la carpeta generada `builddir` ejecutando `cd builddir`.
3. Ejecutar `meson compile` (dentro de `builddir`) para compilar los programas.
4. Iniciar el servidor y cliente generados en:
    1. Servidor: `builddir/server`
    2. Cliente: `builddir/client`

## Instrucciones para usar extensiones en VSCode

1. En la pestaña de la extensión Meson se muestran "Targets" para compilar los ejecutables por separado, por ejemplo:
    - lpd-sockets 0.1.0
        - Targets
            - server
            - client
   Se compilan al hacer clic en cada "target".
2. En la pestaña de ejecución y depuración, están definidas dos opciones:
- (gdb) Iniciar servidor
- (gdb) Iniciar cliente
   Se ejecutan seleccionando y haciendo clic en la flecha o usando <kbd>F5</kbd>.

En la carpeta `.vscode` del proyecto se encuentran los archivos de configuración para las extensiones:

- `c_cpp_properties.json` - para definir "include paths", etc.
- `launch.json` - para definir como ejecutar/depurar los programas desde VSCode.
