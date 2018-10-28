package com.coboltforge.dontmind.multivnc;


public enum COLORMODEL {
	C24bit, C256, C64, C8, C4, C2;

	public int bpp() {
		switch (this) {
		case C24bit:
			return 4;
		default:
			return 1;
		}
	}

	public int[] palette() {
		switch (this) {
		case C24bit:
			return null;
		case C256:
			return ColorModel256.colors;
		case C64:
			return ColorModel64.colors;
		case C8:
			return ColorModel8.colors;
		case C4:
			return ColorModel64.colors;
		case C2:
			return ColorModel8.colors;
		default:
			return ColorModel256.colors;
		}
	}
	
	public String nameString()
	{
		return super.toString();
	}


	public String toString() {
		switch (this) {
		case C24bit:
			return "24-bit color (4 bpp)";
		case C256:
			return "256 colors (1 bpp)";
		case C64:
			return "64 colors (1 bpp)";
		case C8:
			return "8 colors (1 bpp)";
		case C4:
			return "Greyscale (1 bpp)";
		case C2:
			return "Black & White (1 bpp)";
		default:
			return "256 colors (1 bpp)";
		}
	}
}
