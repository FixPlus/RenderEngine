# Render Engine

---

## About
Basic rendering routines for 3D graphics project. This is WIP.
## Prerequisites
* Compiler: gcc 10.0+ / clang 10.0+ / any other compiler supporting core c++20 features
* CMake: 3.20+
* Boost: 1.62+
* Vulkan SDK 1.3.224.1+: https://vulkan.lunarg.com/
* VkWrapper: https://github.com/FixPlus/VkWrapper
## Download & Build
* Linux-like environments:
```
git git@github.com:FixPlus/RenderEngine.git
cd RenderEngine
mkdir build && cd build
cmake .. -DRENDERER_INSTALL_DIR=/path/to/VkWrapper-install
make install -j <num workers>
```
## Usage
### Import
Currently, there are no way to import this library as a package.\
So, to use this library add following directories to your include location list:
```
${ENGINE_INSTALLDIR}/include
```
And add this directory to your library search path list:
```
${ENGINE_INSTALLDIR}/lib
```
Also make sure you add next directory to PATH (or LD_LIBARRY_PATH):
```
${ENGINE_INSTALLDIR}/bin
```
Finnaly, make sure to link with RenderEngine:
```
-lRenderEngine
```
### Use in code
Include needed headers and you are done
```c++
#include <RE/Engine.hpp>

int main(){
    auto engine = re::Engine(...);
}
```