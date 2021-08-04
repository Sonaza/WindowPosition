# WindowPosition
It positions and resizes window. Because ~~Spotify~~ some programs are too f\*ken stupid to do it by themselves.

## Usage
WindowPosition.exe `<monitor index> <left offset> <right offset> <top offset> <bottom offet> <process name> [window title] [window class]`

* **Monitor index** declares which monitor the window will be placed on. The value should be 1 or higher and not exceed the number of monitors connected.

* Offsets are described in absolute percentages between [0-100]. For example setting left to 50 and right to 100 aligns a window on the right side half portion of the screen. Floating point values are also allowed for more fine tuned control.
	
* **Process name** is the executable file name running on the computer.

* Optionally a **window title** and a **window class** can be specified to focus the search.
