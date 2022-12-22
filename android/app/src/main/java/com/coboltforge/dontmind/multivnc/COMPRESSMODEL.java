package com.coboltforge.dontmind.multivnc;


public enum COMPRESSMODEL {
	L0, L1, L2, L3, L4, L5, L6, L7, L8, L9, None;

	public String nameString()
	{
		return super.toString();
	}

	public String toString() {
		switch (this) {
		default:
		case L0:
			return "0";
		case L1:
			return "1";
		case L2:
			return "2";
		case L3:
			return "3";
		case L4:
			return "4";
		case L5:
			return "5";
		case L6:
			return "6";
		case L7:
			return "7";
		case L8:
			return "8";
		case L9:
			return "9";
		case None:
			return "None";
		}
	}

	public int toParameter() {
		switch (this) {
			default:
			case L0:
				return 0;
			case L1:
				return 1;
			case L2:
				return 2;
			case L3:
				return 3;
			case L4:
				return 4;
			case L5:
				return 5;
			case L6:
				return 6;
			case L7:
				return 7;
			case L8:
				return 8;
			case L9:
				return 9;
			case None:
				return 10;
		}
	}

	public static COMPRESSMODEL getTypeByValue(String str) {
		for (COMPRESSMODEL v : values()) {
			if (v.nameString().equals(str)) {
				return v;
			}
		}

		return L0;
	}
}
