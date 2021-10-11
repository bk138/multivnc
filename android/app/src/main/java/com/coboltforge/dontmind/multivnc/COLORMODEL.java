package com.coboltforge.dontmind.multivnc;


public enum COLORMODEL {
	C24bit, C16bit;

	public int bpp() {
		switch (this) {
		case C24bit:
			return 4;
		case C16bit:
			return 2;
		default:
			return 1;
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
		case C16bit:
			return "16-bit color (2 bpp)";
		default:
			return "";
		}
	}

	public static COLORMODEL getTypeByValue(String str) {
		for (COLORMODEL v : values()) {
			if (v.nameString().equals(str)) {
				return v;
			}
		}

		return C24bit;
	}
}
