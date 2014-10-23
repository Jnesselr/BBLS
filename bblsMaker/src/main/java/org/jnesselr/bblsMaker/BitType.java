package org.jnesselr.bblsMaker;

import java.math.BigInteger;

public class BitType {
    private int numBits;

    public BitType(int bits) {
        this.numBits = bits;
    }

    public int getBitCount() {
        return numBits;
    }

    public BitData make() {
        return new BitData(numBits);
    }

    public BitData make(long initial) {
        return new BitData(numBits, BigInteger.valueOf(initial));
    }
}
