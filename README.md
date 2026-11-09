# TerrenoSkyBill (DirectX 11)

Proyecto de DirectX 11 que usa la librería [Assimp](https://github.com/assimp/assimp) (versión **6.0.5**) para cargar modelos 3D.

Las librerías compiladas de Assimp **no están en el repositorio** (están en el `.gitignore`), sigue esta guia para compilarlas.

## Contenido

1. [Requisitos](#1-requisitos)
2. [Compilar Assimp](#2-compilar-assimp)
3. [Configurar el proyecto en Visual Studio](#3-configurar-el-proyecto-en-visual-studio)
4. [Probar que funciona](#4-probar-que-funciona)
5. [Problemas comunes](#5-problemas-comunes)

---

## 1. Requisitos

- **Visual Studio 2022** con *Desarrollo para el escritorio con C++*.
- El componente **Herramientas de C++ CMake para Windows** (viene incluido en lo anterior; se puede revisar en el Instalador de Visual Studio).
- **PowerShell** (viene con Windows).
- Conexión a internet para descargar el código fuente de Assimp (aprox. 14 MB).

## 2. Compilar Assimp

Abre **PowerShell** en la **raíz del repositorio** (la carpeta donde está la solución `.sln` y donde se creará la carpeta `external`) y ejecuta este bloque completo:

```powershell
# --- Variables ---
$root  = (Get-Location).Path
$ext   = Join-Path $root 'external'
$src   = Join-Path $ext 'assimp'
$cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path $cmake)) { $cmake = 'cmake' }   # si no existe esa ruta, usa el cmake del PATH

# --- Descargar el código fuente de Assimp v6.0.5 (solo si no existe) ---
New-Item -ItemType Directory -Force -Path $ext | Out-Null
if (-not (Test-Path $src)) {
    $zip = Join-Path $ext 'assimp-src.zip'
    Invoke-WebRequest -Uri 'https://github.com/assimp/assimp/archive/refs/tags/v6.0.5.zip' -OutFile $zip -UseBasicParsing
    Expand-Archive -Path $zip -DestinationPath $ext -Force
    Rename-Item (Join-Path $ext 'assimp-6.0.5') 'assimp'
    Remove-Item $zip
}

# --- Opciones de CMake (librería estática, solo importadores OBJ, FBX y glTF) ---
$flags = @(
    '-DASSIMP_BUILD_ASSIMP_TOOLS=OFF',
    '-DASSIMP_BUILD_SAMPLES=OFF',
    '-DASSIMP_BUILD_TESTS=OFF',
    '-DASSIMP_BUILD_ZLIB=ON',
    '-DASSIMP_NO_EXPORT=ON',
    '-DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF',
    '-DASSIMP_BUILD_FBX_IMPORTER=ON',
    '-DASSIMP_BUILD_OBJ_IMPORTER=ON',
    '-DASSIMP_BUILD_GLTF_IMPORTER=ON',
    '-DBUILD_SHARED_LIBS=OFF',
    '-DASSIMP_WARNINGS_AS_ERRORS=OFF',
    '-DASSIMP_INSTALL=OFF'
)

# --- Configurar y compilar: Win32 y x64, en Debug y Release ---
$plataformas = @(
    @{ Carpeta = 'win32'; Arch = 'Win32' },
    @{ Carpeta = 'x64';   Arch = 'x64'   }
)
foreach ($p in $plataformas) {
    $build = Join-Path $ext "assimp-build-$($p.Carpeta)"
    New-Item -ItemType Directory -Force -Path $build | Out-Null
    & $cmake -S $src -B $build -G "Visual Studio 17 2022" -A $p.Arch @flags
    foreach ($cfg in 'Debug', 'Release') {
        & $cmake --build $build --config $cfg --target assimp -- /m /nologo /v:m
    }
}
```

Tarda varios minutos. Al terminar, verifica que se generaron las librerías:

```powershell
Get-ChildItem "$ext" -Recurse -Filter '*.lib' | Select-Object -ExpandProperty FullName
Test-Path "$src\include\assimp\Importer.hpp"   # debe dar True
```

Deben aparecer **8 archivos `.lib`** (4 de Assimp y 4 de zlib):

| Plataforma | Configuración | Assimp | zlib |
|---|---|---|---|
| Win32 | Debug | `assimp-build-win32\lib\Debug\assimp-vc143-mtd.lib` | `assimp-build-win32\contrib\zlib\Debug\zlibstaticd.lib` |
| Win32 | Release | `assimp-build-win32\lib\Release\assimp-vc143-mt.lib` | `assimp-build-win32\contrib\zlib\Release\zlibstatic.lib` |
| x64 | Debug | `assimp-build-x64\lib\Debug\assimp-vc143-mtd.lib` | `assimp-build-x64\contrib\zlib\Debug\zlibstaticd.lib` |
| x64 | Release | `assimp-build-x64\lib\Release\assimp-vc143-mt.lib` | `assimp-build-x64\contrib\zlib\Release\zlibstatic.lib` |

> **Nota:** solo se activaron los importadores **OBJ, FBX y glTF**. Si necesitas otro formato (por ejemplo `.dae` o `.3ds`), agrega la opción correspondiente en `$flags` (por ejemplo `-DASSIMP_BUILD_COLLADA_IMPORTER=ON`), borra la carpeta `assimp-build-*` y vuelve a ejecutar el bloque.

## 3. Configurar el proyecto en Visual Studio

Abre la solución, haz clic derecho en el proyecto **TerrenoSkyBill** → **Propiedades**.

Usamos las macros `$(SolutionDir)`, `$(Platform)` y `$(Configuration)` para que las rutas funcionen en cualquier computadora y en todas las configuraciones. **No uses rutas absolutas** (como `C:\Users\...`), porque no funcionarán en las computadoras de los demás.

> Se asume que el archivo `.sln` está en la raíz del repositorio, junto a la carpeta `external`. Si no es así, reemplaza `$(SolutionDir)` por `$(ProjectDir)..\` (o la ruta relativa que corresponda).

### 3.1 Directorios de inclusión (include)

En la parte superior de la ventana selecciona **Configuración: Todas las configuraciones** y **Plataforma: Todas las plataformas**.

**C/C++ → General → Directorios de inclusión adicionales:**

```
$(SolutionDir)external\assimp\include;$(SolutionDir)external\assimp-build-$(Platform)\include;%(AdditionalIncludeDirectories)
```

Se necesitan **las dos rutas**: la primera tiene los encabezados principales (`Importer.hpp`, `scene.h`, `postprocess.h`) y la segunda tiene el `assimp/config.h` que CMake genera al compilar.

### 3.2 Directorios de bibliotecas (lib)

Con **Todas las configuraciones** y **Todas las plataformas** seleccionadas:

**Vinculador → General → Directorios de bibliotecas adicionales:**

```
$(SolutionDir)external\assimp-build-$(Platform)\lib\$(Configuration);$(SolutionDir)external\assimp-build-$(Platform)\contrib\zlib\$(Configuration);%(AdditionalLibraryDirectories)
```

> Esto funciona porque las configuraciones se llaman exactamente `Debug` y `Release`, y las plataformas `Win32` y `x64` (Windows no distingue mayúsculas de minúsculas en las rutas, así que `Win32` coincide con la carpeta `assimp-build-win32`).

### 3.3 Dependencias del vinculador

Esta configuración **sí debe hacerse por separado** para Debug y Release, porque los nombres de los archivos cambian. Con **Plataforma: Todas las plataformas**:

**Vinculador → Entrada → Dependencias adicionales:**

- Configuración **Debug**:
  ```
  assimp-vc143-mtd.lib;zlibstaticd.lib;%(AdditionalDependencies)
  ```
- Configuración **Release**:
  ```
  assimp-vc143-mt.lib;zlibstatic.lib;%(AdditionalDependencies)
  ```

### 3.4 Biblioteca en tiempo de ejecución (CRT)

Assimp se compiló como librería estática con la CRT dinámica por defecto, así que el proyecto debe usar la misma. Es el valor por defecto de Visual Studio, pero conviene revisarlo:

**C/C++ → Generación de código → Biblioteca en tiempo de ejecución:**

- Debug: **Multithreaded Debug DLL (/MDd)**
- Release: **Multithreaded DLL (/MD)**

## 4. Probar que funciona

Agrega este código a cualquier `.cpp` del proyecto y compila en las cuatro combinaciones (Debug/Release × Win32/x64):

```cpp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void ProbarAssimp()
{
    Assimp::Importer importer;
    const aiScene* escena = importer.ReadFile("prueba.obj", aiProcess_Triangulate);
    // Aunque el archivo no exista, si esto compila y enlaza, la integración funciona.
}
```

## 5. Problemas comunes

| Error | Causa probable | Solución |
|---|---|---|
| `E1696` / `C1083: cannot open source file "assimp/Importer.hpp"` | Faltan los directorios de inclusión | Revisa la sección 3.1 y confirma que `external\assimp\include` existe |
| `cannot open "assimp/config.h"` | Falta `assimp-build-<plataforma>\include` | Revisa la sección 3.1 y que hayas ejecutado el paso 2 |
| `LNK1104: cannot open file 'assimp-vc143-mtd.lib'` | Faltan los directorios de bibliotecas o no se compiló Assimp para esa configuración | Revisa la sección 3.2 y ejecuta de nuevo el paso 2 |
| `LNK2019: unresolved external symbol ...` | Faltan dependencias o zlib | Revisa la sección 3.3 (deben estar `assimp` **y** `zlibstatic`) |
| `LNK1112: module machine type 'x86' conflicts with target machine type 'x64'` | Se está usando la librería de otra plataforma | Revisa que la plataforma activa coincida con la carpeta `assimp-build-win32` / `assimp-build-x64` |
| `LNK2038: mismatch detected for 'RuntimeLibrary'` | La CRT del proyecto no coincide con la de Assimp | Revisa la sección 3.4 |