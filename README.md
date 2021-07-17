# Tetris on ATmega328p (with shift registers!)
Below is a brief descripction of the _Tetris_ project. The code was written by me and a few colleagues to familiarize ourselves with the digital electronics (the original version used MAX7219 displays so it was much easier to make). After building my version, I adapted the code to the shift registers.  
The design is amateurish, the code is not 100% optimal, the soldering is also not professional, but it was done to kill the spare time.

# Components used:
1. Microcontroller ATmega328p
2. Shift registers 74HC595
3. Led matrixes (columns - cathodes, rows - anodes)
4. Resistors, capacitors
5. Simple buttons

# Schematic
### Simplifications:
To make the schematic clearer, I simplified the pinout of the led matrices. You should assume that e.g. **row1** and **row1+** are internally connected.
<p align="center">
<img src="https://github.com/JZimnol/Tetris_ATmega328p/blob/main/Images/Schematic_1.png" width="750">
</p>
Shift register schematics have been also simplified.

### Full schematic:
(right-click + open in new window for better resolution)
<p align="center">
<img src="https://github.com/JZimnol/Tetris_ATmega328p/blob/main/Images/Schematic_full.png" width="900">
</p>

# Final product:
<p align="center">
<img src="https://github.com/JZimnol/Tetris_ATmega328p/blob/main/Images/Front_side.jpg" width="700">
</p>
<p align="center">
<img src="https://github.com/JZimnol/Tetris_ATmega328p/blob/main/Images/Back_side.jpg" width="700">
</p>
<p align="center">
<img src="https://github.com/JZimnol/Tetris_ATmega328p/blob/main/Images/Play.jpg" width="700">
</p>
<p align="center">
<img src="https://github.com/JZimnol/Tetris_ATmega328p/blob/main/Images/Gameplay_1.jpg" width="700">
</p>
<p align="center">
<img src="https://github.com/JZimnol/Tetris_ATmega328p/blob/main/Images/Gameplay_2.jpg" width="700">
</p>
