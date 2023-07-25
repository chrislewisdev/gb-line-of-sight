# Gameboy line of sight demo

This is some demo code illustrating one way of implementing a line-of-sight system on the gameboy, whereby the map is gradually revealed as you explore it.

The implementation is based primarily on [Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm) as a method for 'casting' lines from the player throughout the map to check for visibility.

