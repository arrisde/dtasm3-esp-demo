# Dtasm ESP32 demo

This is a demo for running a [dtasm module](https://2021.international.conference.modelica.org/proceedings/papers/Modelica2021session6A_paper3.pdf) on an ESP32-WROVER MCU. It demonstrates the platform-independent nature of dtasm, and its suitability for relatively modest hardware platforms. The demo is based on [dtasm3](https://github.com/siemens/dtasm/tree/main/runtime/dtasm3), a dtasm runtime around the lightweight WebAssembly interpreter [wasm3](https://github.com/wasm3/wasm3). 

## Requirements
- Any ESP32 WROVER development board, e.g. [Lolin D32 Pro](https://www.wemos.cc/en/latest/d32/d32_pro.html) or TTGO T8. 
- [PlatformIO](https://platformio.org/) (e.g. Visual Studio Code with the [PlatformIO extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)) for building the ESP32 application. 
- [WASI SDK](https://github.com/WebAssembly/wasi-sdk) to build the double pendulum dtasm module used by the demo.
- A working C/C++ toolchain (e.g. gcc/g++) for your PC, as well as GNU `make` in your `$PATH`. 

## Steps
Be sure to clone the repo including submodules (`git clone --recurse-submodules ...`). Connect the ESP32 board to your PC and run these steps in the repo's root directory: 
- `pio run`
  This builds the demo application for the ESP32 target. 
- `pio run -t buildfs` 
  Builds [dpend_cpp.wasm](external/dtasm.git/module/dpend_cpp) dtasm module and copies to `data` folder, then creates an ESP32 filesystem image that contains `dpend_cpp.wasm` on a SPIFFS partition. 
- `pio run -t uploadfs` 
  uploads the filesystem image to your ESP32,
- `pio run -t upload -t monitor`
  uploads the application to your ESP32 and executes it. 

If all goes well you should see the following output: 
```
Reading file dpend_cpp.wasm... read 60921 bytes
Model description verifies ok
ID: {8ad29d6a-2525-4576-95be-e1facde6860e}
Name: Double Pendulum
Description: Double pendulum simulation by solving equations of motion using a simple Runge-Kutta scheme (http://www.physics.usyd.edu.au/~wheat/dpend_html/solve_dpend.c)
Generating Tool: 
 can_handle_variable_step_size: 1
 can_interpolate_inputs: 0
 can_reset_step: 0
Init returned status: OK
t;theta1;joint1.velocity;theta2;joint2.velocity
0;3.14159;0;0.0174533;0
0.1;3.14074;-0.0170964;0.0157574;-0.0336386
0.2;3.13817;-0.0342788;0.0108326;-0.064093
0.3;3.13387;-0.0519586;0.00313258;-0.088812
0.4;3.12773;-0.0712036;-0.00668981;-0.106453
0.5;3.11951;-0.0940834;-0.0179282;-0.117355
0.6;3.10868;-0.124047;-0.0300091;-0.123883
0.7;3.09429;-0.166369;-0.0427059;-0.130633
...
```

Congrats, you just ran the `dpend_cpp` dtasm module on your ESP32! Results will be exactly the same as if you executed the module on any other dtasm runtime on any platform. If you like you can visualize the simulated double pendulum motion using the provided [Jupyter notebook](extern/dtasm.git/tools/dp_plot/double_pendulum.ipynb). 

### Trivia
- _Why the need for a WROVER module, can the demo not run on the simpler and cheaper ESP32-WROOM series?_
  The limition here is the RAM available to the ESP32. The provided dtasm modules make use of WASI extensions, which wasm3 only supports on ESP32 boards with PSRAM extension. In principle, if the dtasm module was compiled without WASI imports, it could work on a regular ESP32-WROOM as well. 
- _Can the Rust-based dtasm modules be run on the ESP32?_ 
  Rust unfortunately is not very good at compiling small binaries. There are some tricks ([1](https://rustwasm.github.io/docs/book/reference/code-size.html), [2](https://nickb.dev/blog/avoiding-allocations-in-rust-to-shrink-wasm-modules/)), but the dependency on flatbuffers makes it difficult to build dtasm modules that would fit in the memory of an ESP32. 