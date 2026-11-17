# TraffiSync

TraffiSync is a small C and Raylib traffic-intersection control prototype. It
simulates four vehicle approaches and displays queue lengths alongside adaptive
or manually selected signal phases. It is a visual simulation only and does
not connect to real traffic lights or sensors.

## Controls

- **A** or the panel button: switch between adaptive and manual control.
- **Space**: pause or resume the simulation.
- **R** or the panel button: reset traffic, session time, and cleared-vehicle count.
- In manual mode, press **1** for north/south green or **2** for east/west green.
- Use the **Less** and **More** buttons to adjust simulated traffic density.
- The panel reports live queues, vehicles on the road, session time, and vehicles cleared.

## Build

Install Raylib and a C compiler, then build from this folder.

Windows with MinGW:

```sh
gcc main.c -o traffisync.exe -O2 -std=c99 -lraylib -lopengl32 -lgdi32 -lwinmm
```

Linux:

```sh
cc main.c -o traffisync -O2 -std=c99 -lraylib -lm -lpthread -ldl -lrt -lX11
```

Run the resulting `traffisync` executable.
