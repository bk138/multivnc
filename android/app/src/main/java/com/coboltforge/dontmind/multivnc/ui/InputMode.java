package com.coboltforge.dontmind.multivnc.ui;

public enum InputMode {
    DEFAULT("default"),
    JUMP("jump");

    private final String value;

    InputMode(String value) {
        this.value = value;
    }

    public String value() {
        return value;
    }

    public static InputMode fromValue(String value) {
        if (value == null)
            return DEFAULT;

        for (InputMode mode : values()) {
            if (mode.value.equals(value))
                return mode;
        }

        return DEFAULT;
    }
}
