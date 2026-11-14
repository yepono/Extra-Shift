# TerrenoSkyBill (DirectX 11)

Proyecto de DirectX 11 que usa la librería [Assimp](https://github.com/assimp/assimp) (versión **6.0.5**) para cargar modelos 3D.

Las librerías compiladas de Assimp **no están en el repositorio** (están en el `.gitignore`), sigue esta guia para compilarlas.

## Contenido

1. [Requisitos](#1-requisitos)
2. [Compilar Assimp](#2-compilar-assimp)
3. [Configurar el proyecto en Visual Studio](#3-configurar-el-proyecto-en-visual-studio)
4. [Probar que funciona](#4-probar-que-funciona)
5. [Problemas comunes](#5-problemas-comunes)
6. [Documentación de desarrollo](#6-documentación-de-desarrollo)

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

---

## 6. Documentación de desarrollo

Esta sección es la referencia interna del proyecto: qué hace cada archivo, cómo se conecta el flujo de render, qué convenciones respetar y qué trampas ya conocemos. **Mantenerla actualizada cuando cambie la arquitectura.** El código y los comentarios existentes están en español; esta guía también.

### 6.1 Resumen del proyecto

Demo de render 3D en tiempo real con **DirectX 11**, escrito en C++ y organizado alrededor de la clase `DXRR`:

- **Terreno** generado desde un mapa de alturas (`hm_desierto.png`) con mezcla de 3 texturas según la elevación (`Shader.fx`).
- **SkyDome** procedural (esfera de `slices × stacks`) con textura (`SkyDome.fx`).
- **Modelos OBJ estáticos** (personas, objetos, edificios) con textura difusa y especular (`ModeloRR` + `Modelo.fx`).
- **Modelo animado FBX** con *skinning* por huesos, cargado con Assimp (`FBXImporter` + `AnimatedModel`).
- **Cámara en primera y tercera persona** (`Camara`), con teclado, ratón (DirectInput) y mando Xbox (XInput).
- **Audio XACT3** preparado pero todavía no conectado al juego.

La ventana es un `WS_POPUP` de **1920 × 1080**. La llamada a pantalla completa está comentada en `TerrenoSkyBill.cpp`, así que corre en ventana.

### 6.2 Compilar y ejecutar

Requisitos y compilación de Assimp: secciones [2](#2-compilar-assimp) y [3](#3-configurar-el-proyecto-en-visual-studio). Las rutas de Assimp **ya están puestas** en `v1dx11rr.vcxproj`; la sección 3 sirve si se recrea el proyecto o si Visual Studio pierde la configuración.

Desde la raíz del repositorio:

```powershell
# Restaurar paquetes NuGet (D3DX y DirectXMath)
nuget restore TerrenoSkyBill.sln

# Compilar (ejemplo x64 Debug)
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
    TerrenoSkyBill.sln /p:Configuration=Debug /p:Platform=x64 /m
```

Reglas de ejecución:

- **El directorio de trabajo debe ser `v1dx11rr\`.** Todas las rutas de assets son relativas y están escritas a mano (`L"Shader.fx"`, `"Assets/Modelos/..."`, `L"Skydome.jpg"`). Al depurar desde Visual Studio el directorio de trabajo por defecto es `$(ProjectDir)`, por eso funciona. Si abres el `.exe` desde otra carpeta, el terreno mostrará el mensaje *"Terrain initialization failed"* y los modelos no aparecerán.
- Solo se activaron los importadores **OBJ, FBX y glTF** de Assimp. Un formato nuevo requiere recompilar Assimp (sección 2).
- La configuración **Debug|x64** del `.vcxproj` todavía apunta a librerías D3DX de **x86** con rutas absolutas de una computadora concreta (ver 6.13). Si falla el enlace, usar **Win32** o corregir esas rutas.

### 6.3 Mapa de archivos

#### Núcleo y bucle principal

| Archivo | Rol | Estado |
|---|---|---|
| `v1dx11rr/TerrenoSkyBill.cpp` | `WinMain`, ventana, DirectInput (teclado/ratón), XInput, bucle de mensajes y `ProcessInput`. **Punto de entrada real.** | Activo |
| `v1dx11rr/DXRR.h` | Orquestador: crea el device/swap chain, carga escena, actualiza y dibuja todo. | Activo |
| `v1dx11rr/Camara.h` | Cámara en primera persona (`UpdateCamera`) y tercera persona (`UpdateThirdPerson`). | Activo |
| `v1dx11rr/GamePadRR.h` | Envoltorio de XInput: estado, conexión y vibración. | Activo |
| `v1dx11rr/main.cpp` | `WinMain` viejo, comentado y con includes inexistentes (`Terrain.h`, `Timer.h`). **No está en el `.vcxproj` ni se compila.** | Muerto |
| `v1dx11rr/TextureLoader.h/.cpp` | Vacíos (el `.cpp` pesa 0 bytes). | Muerto |

#### Terreno

| Archivo | Rol |
|---|---|
| `v1dx11rr/TerrenoRR.h` | Malla del terreno desde heightmap, normales/tangentes/binormales, índices, shaders, colisión (`Superficie`) y dibujo. |
| `v1dx11rr/Shader.fx` | VS/PS del terreno: mezcla 3 texturas por altura + luz direccional. |
| `coast_sand_02_diff_2k.jpg`, `mud_cracked_dry_03_diff_2k.jpg`, `baseball_playground_diff_2k.jpg` | Texturas de altura baja / media / alta. Deben estar en `v1dx11rr\`. |
| `hm_desierto.png` | Mapa de alturas del terreno. |

#### Modelos estáticos (OBJ)

| Archivo | Rol |
|---|---|
| `v1dx11rr/loadModel.h/.cpp` | `CObjParser`: parser de OBJ (v/vn/vt/f) que **des-indexa** la malla y la deja lista para `Draw`. |
| `v1dx11rr/ModeloRR.h` | Carga un OBJ + texturas, crea buffers/shaders y lo dibuja con transformaciones. |
| `v1dx11rr/Modelo.fx` | Shader con textura difusa, especular y luz direccional. |
| `v1dx11rr/Assets/Modelos/...` | `.obj`, `.jpeg`, `.png` de personajes y objetos. |

#### Cielo y billboards

| Archivo | Rol |
|---|---|
| `v1dx11rr/SkyDome.h` | Esfera procedural con textura; sigue a la cámara y se dibuja sin depth. |
| `v1dx11rr/SkyDome.fx` | Shader del domo. |
| `v1dx11rr/Billboard.h` / `Billboard.hlsl` | Billboard con normal map y frames. **Desactivado** (comentado en `DXRR`). |

#### Animación FBX

| Archivo | Rol |
|---|---|
| `v1dx11rr/FBXImporter.h/.cpp` | Assimp → `SubMesh`, esqueleto, pesos de huesos, materiales y clips de animación. |
| `v1dx11rr/AnimatedModel.h/.cpp` | Buffers GPU, shaders de skinning (HLSL embebido), muestreo de clips y dibujo. |
| `v1dx11rr/Skeleton.h` | `SkeletonNode`, `BoneInfo` y matrices finales por hueso. |
| `v1dx11rr/AnimationClip.h` | Keys de posición/rotación/escala y búsqueda de canales por nombre. |
| `v1dx11rr/AnimatedVertex.h` | Formato de vértice: posición, normal, UV, 4 pesos y 4 índices de hueso. |
| `v1dx11rr/SkinnedModel.fx` | Alternativa en archivo al HLSL embebido. **No se usa** (excluido de la compilación). |

#### Audio

| Archivo | Rol |
|---|---|
| `v1dx11rr/XACT3Util.h/.cpp` | Envoltorio mínimo de XACT3: motor, wave bank, sound bank. |
| `v1dx11rr/Win/*.xgs/.xsb/.xwb`, `Assets/Sonido/Desierto.wav` | Proyecto de audio y sonido de ambiente. |

### 6.4 Flujo de arranque y por frame

```
WinMain
  ├─ RegisterClassEx / CreateWindowEx (1920×1080, WS_POPUP)
  ├─ new DXRR(hWnd, 1920, 1080)
  │    ├─ IniciaD3D()               → device, swap chain, RTV, depth
  │    ├─ new Camara(0,80,6 → 0,80,0)
  │    ├─ new TerrenoRR(500,500)    → heightmap, malla, shaders, texturas
  │    ├─ new SkyDome(32,32,100)
  │    ├─ new ModeloRR(...) × N     → OBJ estáticos
  │    └─ new AnimatedModel(-40,51) → alien.fbx + clips Idle/Walk/Pickup
  ├─ new GamePadRR(1) y DirectInput (ratón + teclado)
  └─ bucle:
       PeekMessage → ProcessInput(dt) → dxrr->Update(dt) → dxrr->Render()
```

Cada frame:

1. `ProcessInput` limpia los deltas (`moveBackForward`, `moveLeftRight`, `camYaw`, `camPitch`) y los vuelve a acumular desde teclado, ratón y mando, multiplicando por `deltaTime`.
2. `DXRR::Update` aplica esos deltas a la cámara/jugador, ajusta la altura del jugador al terreno y elige la animación (`Walk` si hay movimiento, `Idle` si no).
3. `DXRR::Render` limpia buffers, actualiza la vista, dibuja **cielo → terreno → modelos → modelo animado** y hace `Present(1,0)` (vsync).

El `deltaTime` se calcula con `QueryPerformanceCounter` y se limita a `0.1 s` para evitar saltos tras un *breakpoint*.

### 6.5 Convenciones que hay que respetar

#### Sistema de coordenadas

- **Mano izquierda** (`D3DXMatrixLookAtLH`, `PerspectiveFovLH`), **+Y arriba**, el suelo es el plano **XZ**.
- Unidades aproximadas a metros. Velocidad base de movimiento: **30 u/s**.
- Terreno: **500 × 500** unidades, centrado en el origen → `x, z ∈ [-250, 250]`.
- Altura del terreno: `byte / 9.0` → rango `0 .. 28.33`.
- Cámara: FOV `π/3`, near `0.01`, far `1000`.
- SkyDome: radio `100`, centrado en la cámara (se traslada con ella, `y - 50`).

#### Regla de oro de las matrices (muy importante)

El proyecto mezcla dos convenciones y **no son intercambiables**:

| Shader | Declaración | Cómo subir la matriz |
|---|---|---|
| `Shader.fx`, `Modelo.fx`, `SkyDome.fx` | `matrix` (column_major por defecto) + `mul(vector, matrix)` | **Transponer en CPU** (`D3DXMatrixTranspose`). `DXRR::Render` genera `gpuView` y `gpuProjection` ya transpuestas. |
| HLSL embebido en `AnimatedModel.cpp` | `row_major float4x4` | **Sin transponer**. Se le pasa `&camara->View._11` directamente. |

Si se transpone dos veces (o ninguna) el síntoma es geometría invisible, deformada o “detrás” de la cámara. Las normales se transforman con la matriz de mundo sin inversa-traspuesta; funciona porque todas las escalas son uniformes.

#### Rutas y assets

- Rutas **relativas al directorio de trabajo**, con `/` y escritas a mano en el código.
- Extensiones de textura: `.jpg`, `.jpeg`, `.png`; los OBJ no llevan `.mtl` (las texturas se pasan al constructor).
- Los nombres distinguen mayúsculas/minúsculas en algunos sistemas: respetar `Assets/Modelos/...` tal cual.

#### Estilo de código

- Comentarios y nombres de dominio en **español** (`CargaParametros`, `Superficie`, `terreno`), API en inglés.
- Mezcla de `PascalCase` (clases viejas) y `camelCase` (código nuevo). Mantener el estilo del archivo que se toque.
- Recursos COM se liberan a mano con `Release()`; no hay `ComPtr`.
- Varios archivos antiguos están en **Windows-1252** (por eso a veces aparecen acentos corruptos). No convertirlos a UTF-8 sin revisar, o los comentarios se rompen.

### 6.6 `DXRR`: el orquestador

Miembros públicos relevantes (los usa `ProcessInput`):

| Miembro | Uso |
|---|---|
| `moveBackForward`, `moveLeftRight` | Delta de traslación acumulado del frame. |
| `camYaw`, `camPitch` | Delta de rotación acumulado del frame. |
| `thirdPerson` | `true` = tercera persona. Arranca en `true`. |
| `playerPosition` | Posición XZ (y altura) del jugador en tercera persona. |
| `terreno`, `camara`, `skydome` | Subsistemas principales. |
| `alien_anim`, `alienAnimLoaded` | Modelo FBX y bandera de carga correcta. |

Métodos:

- `IniciaD3D(HWND)`: prueba `HARDWARE → WARP → SOFTWARE`, feature levels `11_0 / 10_1 / 10_0`, swap chain ventana 1 buffer, formato `R8G8B8A8_UNORM`, depth `D24_UNORM_S8_UINT`.
- `Render()`: orden de dibujo y transposición de matrices (ver 6.5). En primera persona recoloca la cámara a `Superficie(x,z) + 3.7` y reconstruye la vista.
- `Update(dt)`: aplica input, sincroniza altura del jugador, mueve el alien a `playerPosition` y cambia entre `Walk`/`Idle`.
- `TurnOn/OffAlphaBlending`, `TurnOn/OffDepth`: crean el estado **cada llamada** (deuda técnica, ver 6.13).
- `UpdateCameraFromInput()`: delega en `Camara` según `thirdPerson` y limpia los deltas.
- `isPointInsideSphere`, `billCargaFuego`: helpers heredados; `billCargaFuego` prepara UVs de 32 frames para el billboard y hoy no se llama.

### 6.7 Cámara y controles

#### Controles

| Entrada | Acción |
|---|---|
| `W` / `S` | Avanzar / retroceder (30 u/s). |
| `A` / `D` | Desplazarse a la izquierda / derecha. |
| Flechas `←` `→` | Rotar cámara en horizontal (1.5 rad/s). |
| Flechas `↑` `↓` | Rotar cámara en vertical (1.5 rad/s). |
| Ratón | Mirar (sensibilidad `0.0025` rad/píxel). El cursor se oculta y se recentra. |
| `C` | Alternar primera/tercera persona. |
| `Esc` | Salir. |
| Stick izquierdo | Mover (30 u/s). |
| Stick derecho | Rotar cámara (2.1 rad/s). |
| `LB` / `RB` | Reducir (`×0.33`) / aumentar (`×3`) la velocidad. |
| `B` | Alternar primera/tercera persona. |

Zona muerta del mando: `0.19`. Al cambiar a tercera persona, el jugador se coloca en la posición XZ de la cámara y se ajusta a la altura del terreno.

#### `Camara`

- `UpdateCamera(move, strafe, yaw, pitch)`: primera persona. Limita `pitch` a ±89°, calcula `Forward`/`Right` solo con *yaw* y reconstruye `View` con `LookAtLH`.
- `UpdateThirdPerson(playerPosition, move, strafe, yaw, pitch, distance, height)`: mueve al jugador, limita `pitch` a `[-30°, +75°]` y coloca la cámara detrás a `distance = 8`, mirando a `altura = 1.5`.
- No hay colisión de cámara: puede atravesar el terreno o los modelos.

### 6.8 Terreno (`TerrenoRR`)

Construcción: `new TerrenoRR(500, 500, device, context)`. Internamente llama a `CargaParametros(L"coast_sand_02_diff_2k.jpg", L"hm_desierto.png", 100.0f)` (hoy las tres texturas de color están **hardcodeadas** dentro).

Pipeline:

1. `LoadHeightData(heightTex)`: obtiene `Width/Height` de la imagen, crea una textura `STAGING`, la mapea y copia el **canal R** a `alturaData[x][y]`.
2. Genera `anchoTexTerr × altoTexTerr` vértices. `x = deltax*y - 250`, `z = deltay*x - 250`, `y = altura/9`.
3. `generaNormales` acumula normales y tangentes por triángulo y calcula binormales.
4. `estableceIndices` genera 2 triángulos por celda (`6` índices) con `R32_UINT`.
5. Carga las 3 texturas de color y crea sampler *wrap* + 3 constant buffers (`world`, `view`, `proj`).
6. `Draw(gpuView, gpuProjection)` fija pipeline, actualiza CBs y hace `DrawIndexed`.

Colisión/altura (`Superficie(x, z)`):

- Calcula el índice de celda, elige uno de los dos triángulos según la posición fraccional y devuelve la `y` de la intersección de una línea vertical con ese plano.
- **No valida límites**: con `x` o `z` fuera de `[-250, 250]` lee fuera del arreglo y puede crashear. Es la causa de que en `DXRR::Render` algunos modelos (hotel, oxxo) usen coordenadas distintas a su posición real.

Shader (`Shader.fx`):

- `height = saturate(terrainHeight / 28)`; pesos `low = 1 - 3h`, `mid = 1 - |h - 0.5|·4`, `high = (h - 0.65)·3`, normalizados.
- Luz: ambiente `0.1` + direccional `(0.5, -1, 0)`.
- `tile` controla el mosaico de UV; hoy `100`.

Para cambiar el aspecto: reemplazar los archivos de textura manteniendo los nombres, o editar las rutas en `CargaParametros` y `Draw`.

### 6.9 Modelos estáticos OBJ (`ModeloRR` + `CObjParser`)

`CObjParser::LoadFile`:

- Lee `v`, `vn`, `vt` y `f`; soporta `f a/b/c` y `f a//c`; **solo triángulos** (si el OBJ trae quads, solo usa los 3 primeros índices del token).
- De-indexa la malla a un arreglo de `VertexObj` (`pos`, `tex0`, `norm`, `campos`).
- Invierte la V (`1 - vt.y`).

`ModeloRR`:

```cpp
ModeloRR(device, context,
         "Assets/Modelos/objetos/cartel/cartel.obj",   // ruta OBJ (char*)
         L"Assets/.../Material_45_baseColor.png",       // difusa (WCHAR*)
         L"Assets/.../Material_45_normal.png",          // especular (WCHAR*)
         x, z);                                          // posición en el mundo
```

- Dibuja con `Draw(vista, proyeccion, ypos, posCam, specForce, rot, axis, scale)`.
  - `axis`: `'X'`, `'Y'`, `'Z'` para rotar en ese eje; cualquier otra letra (`'A'`) = sin rotación.
  - `rot` en radianes; `scale` uniforme; `specForce` es el exponente especular.
- La traslación usa **la `posX/posZ` del constructor**, no las coordenadas que recibe `Draw`. Solo `ypos` viene de fuera.
- Usa `Draw` sin índices: cada vértice se procesa individualmente.
- No hay *frustum culling*: todos los modelos se dibujan siempre.

#### Agregar un modelo estático

1. Copiar el `.obj` y sus texturas a `v1dx11rr/Assets/...`.
2. Declarar `ModeloRR* miModelo;` en `DXRR`.
3. Instanciarlo en el constructor de `DXRR` con su posición XZ.
4. Llamar `miModelo->Draw(gpuView, gpuProjection, terreno->Superficie(x, z), camara->Position, 10.0f, 0.0f, 'A', 1.0f);` en `Render`.
5. Liberarlo en `~DXRR` (hoy los modelos no se liberan).

### 6.10 SkyDome

- Genera `slices × stacks` vértices (32 × 32) con índices de 16 bits y textura `Skydome.jpg`.
- `Update(view, projection)` guarda las matrices; `Render(camPos)` traslada el domo a `(camPos.x, camPos.y - 50, camPos.z)` para que parezca infinito.
- Se dibuja **antes del terreno y con depth desactivado** (`DXRR::Render`), por eso siempre queda detrás de todo.
- Sampler anisotrópico ×8.
- La textura se puede cambiar desde el constructor de `SkyDome` en `DXRR` (`L"Skydome.jpg"`).

### 6.11 Animación FBX (Assimp)

#### Pipeline

```
alien.fbx ──► FBXImporter::Load
               ├─ ParseNodeHierarchy  → SkeletonNode (árbol, transform local transpuesta)
               ├─ vértices/normales/UV/índices por submesh
               ├─ ExtractBoneWeights  → 4 pesos máx., normalizados + offsetMatrix
               └─ materiales          → rutas de texturas difusa/normal/especular

alien_idle.fbx ──► FBXImporter::LoadAnimationFile("Idle")
                     └─ canales (posición/rotación/escala) que coincidan por NOMBRE con nodos

AnimatedModel
  ├─ crea VS/PS (HLSL embebido, vs_5_0/ps_5_0) y layout BLENDWEIGHT/BLENDINDICES
  ├─ buffers inmutables por submesh + textura difusa (blanca 1×1 si falta)
  ├─ Update(dt): avanza tiempo, muestrea canales, calcula matrices de hueso
  └─ Draw(...): sube constant buffers y dibuja cada submesh
```

#### Detalles importantes

- **Límite de 128 huesos.** Si el modelo tiene más, `Load` falla y escribe en la salida de depuración. El shader además hace `min(boneIndex, 127)`.
- Las animaciones se cargan **por archivo separado** (no vienen dentro del FBX del personaje). El nombre del clip es el que tú pasas: `"Idle"`, `"Walk"`, `"Pickup"`.
- Para que un clip funcione, los nombres de sus canales deben coincidir con los nodos del esqueleto. Si no coincide ninguno, se rechaza y se escribe un mensaje.
- `PlayAnimation(name)` solo reinicia el tiempo si el clip cambió; llamarlo cada frame con el mismo nombre es seguro.
- `Update(dt)` usa `dt * ticksPerSecond` y hace `fmod` con la duración → loop automático. No hay *blending* entre clips.
- Matrices: `final = offsetMatrix * globalMatrix * globalInverseRoot`; el shader usa `row_major` y `mul(vector, matrix)`.
- `AnimatedModel::Load` no permite cargar dos veces el mismo objeto (guarda contra recarga).
- El dibujo usa `Draw(view, projection, terrainY, yaw, xCorr, yCorr, zCorr, scale, footOffset)`. En `DXRR` se llama con correcciones `0, -90°, -90°`, `scale = 0.3` y `footOffset = -15.5` (ajustes de orientación y de altura del modelo).
- `m_importer->directory` se usa para resolver texturas relativas del FBX; si la textura no se encuentra, se usa una textura blanca.
- `SkinnedModel.fx` **no se usa**; el shader real está embebido como string en `AnimatedModel.cpp`.

#### Agregar un personaje animado

1. Copiar el FBX del modelo (con esqueleto) y sus animaciones a `Assets/Modelos/...`.
2. Declarar `AnimatedModel* miPersonaje;` y una bandera `bool miPersonajeLoaded;` en `DXRR`.
3. `miPersonaje = new AnimatedModel(device, context, x, z);`
4. `miPersonajeLoaded = miPersonaje->Load("Assets/.../modelo.fbx");`
5. Por cada clip: `miPersonaje->LoadAnimationFile("Assets/.../clip.fbx", "NombreClip");`
6. `miPersonaje->PlayAnimation("Idle");`
7. En `Update`: `miPersonaje->SetPosition(x, z); miPersonaje->PlayAnimation(...); miPersonaje->Update(dt);`
8. En `Render`: `miPersonaje->Draw(&camara->View._11, &camara->Projection._11, terreno->Superficie(x, z), camara->Yaw, 0.0f, D3DXToRadian(-90.0f), D3DXToRadian(-90.0f), 0.3f, -15.5f);`
9. Liberarlo en `~DXRR`.

### 6.12 Audio (XACT3)

Estado actual: `DXRR` tiene un `CXACT3Util m_XACT3`, pero **nunca se llama a `Initialize` ni se cargan bancos**. El destructor sí llama a `Terminate()` (ver 6.13).

Para activarlo:

```cpp
m_XACT3.Initialize();
m_XACT3.LoadWaveBank(L"Win/Wave Bank.xwb");
m_XACT3.LoadSoundBank(L"Win/Sound Bank.xsb");
// Reproducir un cue (el índice sale del proyecto de audio XACT):
// m_XACT3.m_pSoundBank->Play(cueIndex, 0, 0, nullptr);
```

Y llamar `m_XACT3.DoWork()` una vez por frame. El `XACTINDEX cueIndex;` de `DXRR` ya está declarado para ese uso. Los assets están en `v1dx11rr/Win/` y `Assets/Sonido/Desierto.wav`.

### 6.13 Deuda técnica y trampas conocidas

| # | Problema | Dónde | Impacto | Arreglo sugerido |
|---|---|---|---|---|
| 1 | `main.cpp` no se compila y referencia headers inexistentes | `main.cpp` | Confusión | Borrarlo |
| 2 | `TextureLoader.*` vacíos | `TextureLoader.*` | Ninguno | Borrarlos del proyecto |
| 3 | Rutas absolutas `C:\Users\...` y libs **x86** en `Debug|x64` | `v1dx11rr.vcxproj` (PropertyGroups Debug) | El enlace x64 falla en otra PC | Usar `$(SolutionDir)packages\...\release\lib\$(Platform)` (Win32→x86) |
| 4 | `Superficie()` no valida límites del terreno | `TerrenoRR.h` | Crash si la cámara sale de `[-250,250]` | *Clamp* del índice o devolver `0` |
| 5 | Heightmap asume RGBA de 4 bytes e ignora `RowPitch` | `TerrenoRR.h::LoadHeightData` | Alturas incorrectas con otras imágenes | Usar `subResrc.RowPitch` y bytes por píxel del formato |
| 6 | `delete` en vez de `delete[]` para arreglos | `SkyDome.h` (`vertices`, `indices`), `Billboard.h` | Comportamiento indefinido | `delete[]` |
| 7 | Blend/depth states creados **cada frame** y sin liberar | `DXRR.h::TurnOn/Off*` | Fuga de recursos y CPU | Crearlos una vez en `IniciaD3D` |
| 8 | Vertex buffer recreado cada frame | `Billboard.h::Draw` | Fuga fuerte | Crear una vez y actualizar |
| 9 | Proyección con `ancho`/`alto` sin inicializar | `ModeloRR.h` | Basura (no se usa) | Eliminar código muerto |
| 10 | `viewMatrix *= rotationMat` acumulativo | `ModeloRR.h::Draw` | Confuso (no se sube a GPU) | Eliminar |
| 11 | La `y` del terreno se consulta con coordenadas distintas a la posición del modelo | `DXRR.h::Render` (refri, repisas, bar, hotel, oxxo, mostrador) | Objetos flotando o hundidos | Usar `getPosX()/getPosZ()` o mover el modelo a esas coords |
| 12 | `~DXRR` no libera `terreno`, `skydome` ni los `ModeloRR` | `DXRR.h` | Fugas al salir | `delete` de cada uno |
| 13 | `Terminate()` sin `Initialize()` | `DXRR.h` | `CoUninitialize` desbalanceado | Inicializar audio o quitar la llamada |
| 14 | Billboard desactivado e incompleto | `Billboard.h`, `DXRR.h` | No usable | Terminarlo antes de activarlo |
| 15 | `SkinnedModel.fx` no se usa | `SkinnedModel.fx` | Confusión | Borrarlo o migrar el HLSL embebido ahí |
| 16 | Sin *culling* ni instancing; modelos se dibujan siempre | `DXRR::Render` | Rendimiento | *Frustum culling*, buffers compartidos |
| 17 | `Map()` sin comprobar `HRESULT` y sin `Unmap` si falla | `TerrenoRR.h::LoadHeightData` | Crash difícil de diagnosticar | Validar y liberar en error |
| 18 | Shaders fallan solo a `OutputDebugString` | `ModeloRR`, `TerrenoRR`, `SkyDome` | Pantalla negra sin pista | `MessageBox` con el error |
| 19 | `ProcessInput` asume teclado/ratón creados | `TerrenoSkyBill.cpp` | Crash si falla DirectInput | Comprobar punteros |
| 20 | Ratón exclusivo + `SetCursorPos` cada frame | `TerrenoSkyBill.cpp` | Cursor atrapado, problemas al cambiar de ventana | Manejar foco y `ClipCursor` |
| 21 | `DoWork` usa `state && XACT_CUESTATE_PLAYING` en vez de `&` | `XACT3Util.cpp` | El motor no se actualiza bien | Cambiar a `&` |
| 22 | `CargaParametros(diffuseTex, ...)` ignora `diffuseTex` | `TerrenoRR.h` | Parámetro engañoso | Usarlo o quitarlo |
| 23 | `SkyDome::UnloadContent` no libera `matrices` | `SkyDome.h` | Fuga pequeña | `delete matrices` |
| 24 | `Load` de `AnimatedModel` no libera buffers si falla a medias | `AnimatedModel.cpp` | Fuga en error | Limpiar antes de salir |

### 6.14 Recetas rápidas

**Cambiar la textura del cielo:** reemplazar `v1dx11rr/SkyDome.jpg` (mismo nombre) o cambiar la ruta en `new SkyDome(..., L"Skydome.jpg")` dentro de `DXRR`.

**Cambiar el relieve:** reemplazar `hm_desierto.png` (mismo tamaño idealmente) o cambiar la ruta en `TerrenoRR::CargaParametros`. La escala de altura es `byte / 9`; si el terreno queda muy plano/muy alto, ajustar ese divisor (y el `/28` del shader).

**Cambiar texturas del terreno:** editar las rutas hardcodeadas en `TerrenoRR::CargaParametros` y los `register(t0..t2)` de `Shader.fx` si se agregan más capas.

**Agregar un clip nuevo a un personaje existente:** `alien_anim->LoadAnimationFile("ruta.fbx", "NuevoClip");` y luego `PlayAnimation("NuevoClip")`. Recordar que el nombre del clip es el que se pasa, no el interno del FBX.

**Agregar una tecla:** añadir el `if (keyboardData[DIK_X] & 0x80)` en `ProcessInput` y consumir el estado desde `DXRR` (como `thirdPerson`).

**Agregar un botón del mando:** usar `pad.wButtons & XINPUT_GAMEPAD_X` y el patrón `previousX` para detectar flanco (como hace `B`).

### 6.15 Lista de verificación antes de compilar

- [ ] Las 8 librerías de Assimp existen (`external/assimp-build-*/lib/...` y `.../contrib/zlib/...`).
- [ ] `external/assimp/include/assimp/Importer.hpp` existe.
- [ ] NuGet restauró `Microsoft.DXSDK.D3DX` y `directxtk_desktop_win10`.
- [ ] La plataforma activa coincide con la carpeta de Assimp (`Win32` ↔ `assimp-build-win32`).
- [ ] El directorio de trabajo de depuración es `v1dx11rr\`.
- [ ] Los shaders (`Shader.fx`, `Modelo.fx`, `SkyDome.fx`, `Billboard.hlsl`) y los assets referenciados están en `v1dx11rr\`.
- [ ] Si se agregó un `.cpp`, está incluido en `v1dx11rr.vcxproj` (Visual Studio no lo agrega solo).
- [ ] El nuevo modelo usa `/` en las rutas y la extensión correcta de textura.

### 6.16 Glosario

| Término | Significado en este proyecto |
|---|---|
| **Heightmap** | Imagen cuyos valores de brillo definen la altura del terreno. |
| **Skydome** | Esfera grande con textura que simula el cielo. |
| **Billboard** | Quad que siempre mira a la cámara (hoy desactivado). |
| **Skinning** | Deformación de la malla por pesos de huesos. |
| **Clip / canal** | Animación y conjunto de keys de un nodo del esqueleto. |
| **Cue** | Sonido individual definido en el proyecto XACT. |
| **Superficie** | Altura `y` del terreno en un `(x, z)` dado. |
| **Const buffer** | Buffer de constantes que recibe el shader (matrices, cámara, etc.). |
