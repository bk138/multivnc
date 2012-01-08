package com.coboltforge.dontmind.multivnc;

import java.io.IOException;

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

	public void setPixelFormat(RfbProto rfb, boolean reverserPixelOrder) throws IOException {
		switch (this) {
		case C24bit:
			// 24-bit color
			if(reverserPixelOrder)
				rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16, false);
			else
				rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 16, 8, 0, false);
			break;
		case C256:
			if(reverserPixelOrder)
				rfb.writeSetPixelFormat(8, 8, false, true, 3, 7, 7, 6, 3, 0, false); // not ideal, but wtf...
			else
				rfb.writeSetPixelFormat(8, 8, false, true, 7, 7, 3, 0, 3, 6, false);
			break;
		case C64:
			if(reverserPixelOrder)
				rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 0, 2, 4, false);
			else
				rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 4, 2, 0, false);
			break;
		case C8:
			if(reverserPixelOrder)
				rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 0, 1, 2, false);
			else
				rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 2, 1, 0, false);
			break;
		case C4:
			// Greyscale
			if(reverserPixelOrder)
				rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 0, 2, 4, true);
			else
				rfb.writeSetPixelFormat(8, 6, false, true, 3, 3, 3, 4, 2, 0, true);
			break;
		case C2:
			// B&W
			if(reverserPixelOrder)
				rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 0, 1, 2, true);
			else
				rfb.writeSetPixelFormat(8, 3, false, true, 1, 1, 1, 2, 1, 0, true);
			break;
		default:
			// Default is 24bit colors
			if(reverserPixelOrder)
				rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16, false);
			else
				rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 16, 8, 0, false);
			break;
		}
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
