package org.jnesselr.bblsMaker;

import java.math.BigInteger;

public class BitData {
    private final int bitCount;
    private int[] wires;

    public BitData(int bitCount) {
        this.bitCount = bitCount;
        populateWires();
    }

    public BitData(int bitCount, BigInteger initial) {
        this.bitCount = bitCount;
        populateWires();
        setConstantData(initial);
    }

    private BitData(BitData data) {
        this.bitCount = data.bitCount;
        wires = new int[bitCount];
        System.arraycopy(data.wires, 0, wires, 0, data.wires.length);
    }

    private void populateWires() {
        wires = new int[bitCount];
        for (int i = 0; i < bitCount; i++) {
            wires[i] = BitTree.getNextOutput();
            BitTree.setVariableWire(wires[i]);
        }
    }

    public int[] getWires() {
        return wires;
    }

    private void setConstantData(BigInteger initial) {
        for (int i = bitCount; i > 0; i--) {
            if (initial.testBit(0)) {
                BitTree.setConstantWire(wires[i - 1], WireState.HIGH);
            } else {
                BitTree.setConstantWire(wires[i - 1], WireState.LOW);
            }
            initial = initial.shiftRight(1);
        }
    }

    public BitData add(BitData b) {
        // Todo remove this assert
        assert b.bitCount == bitCount;
        BitData result = new BitData(bitCount);
        if (bitCount == 1) {
            // If there's only 1 bit, just add them.
            BitTree.setXorGate(
                    result.wires[0],
                    wires[0],
                    b.wires[0]
            );
        } else {
            // More than one bit. The start and end have 2 gates.
            // Everything in the middle has 4
            BitTree.setXorGate(
                    result.wires[bitCount - 1],
                    wires[bitCount - 1],
                    b.wires[bitCount - 1]
            );
            int cout = BitTree.getNextOutput();

            BitTree.setAndGate(
                    cout,
                    wires[bitCount - 1],
                    b.wires[bitCount - 1]
            );
            int xorAB, andAB, andABC;
            for (int i = 1; i < bitCount - 1; i++) {
                xorAB = BitTree.getNextOutput();
                andAB = BitTree.getNextOutput();
                andABC = BitTree.getNextOutput();

                int index = bitCount - i - 1;
                BitTree.setXorGate(
                        xorAB,
                        wires[index],
                        b.wires[index]
                );

                BitTree.setXorGate(
                        result.wires[index],
                        xorAB,
                        cout
                );

                BitTree.setAndGate(
                        andAB,
                        wires[index],
                        b.wires[index]
                );

                BitTree.setAndGate(
                        andABC,
                        cout,
                        xorAB
                );

                cout = BitTree.getNextOutput();

                BitTree.setOrGate(
                        cout,
                        andAB,
                        andABC
                );
            }

            xorAB = BitTree.getNextOutput();

            BitTree.setXorGate(
                    xorAB,
                    wires[0],
                    b.wires[0]
            );

            BitTree.setXorGate(
                    result.wires[0],
                    xorAB,
                    cout
            );
        }

        return result;
    }

    public BitData rightRotate(int times) {
        times = times % bitCount;
        BitData result = new BitData(this);
        for (int i = 0; i < bitCount; i++) {
            result.wires[i] = wires[(i + bitCount - times) % bitCount];
        }
        return result;
    }

    public BitData leftRotate(int times) {
        times = times % bitCount;
        BitData result = new BitData(this);
        for (int i = 0; i < bitCount; i++) {
            result.wires[i] = wires[(i + times) % bitCount];
        }
        return result;
    }

    public BitData rightShift(int times) {
        times = times % bitCount;
        BitData result = rightRotate(times);
        for (int i = 0; i < times; i++) {
            result.wires[i] = BitTree.getNextOutput();
            BitTree.setConstantWire(
                    result.wires[i],
                    WireState.LOW
            );
        }
        return result;
    }

    public BitData leftShift(int times) {
        times = times % bitCount;
        BitData result = leftRotate(times);
        for (int i = 0; i < times; i++) {
            result.wires[bitCount - i - 1] = BitTree.getNextOutput();
            BitTree.setConstantWire(
                    result.wires[bitCount - i - 1],
                    WireState.LOW
            );
        }
        return result;
    }

    public BitData and(BitData b) {
        // Todo remove this assert
        assert b.bitCount == bitCount;
        BitData result = new BitData(bitCount);

        for (int i = 0; i < bitCount; i++) {
            BitTree.setAndGate(
                    result.wires[i],
                    wires[i],
                    b.wires[i]
            );
        }

        return result;
    }

    public BitData or(BitData b) {
        // Todo remove this assert
        assert b.bitCount == bitCount;
        BitData result = new BitData(bitCount);

        for (int i = 0; i < bitCount; i++) {
            BitTree.setOrGate(
                    result.wires[i],
                    wires[i],
                    b.wires[i]
            );
        }

        return result;
    }

    public BitData xor(BitData b) {
        // Todo remove this assert
        assert b.bitCount == bitCount;
        BitData result = new BitData(bitCount);

        for (int i = 0; i < bitCount; i++) {
            BitTree.setXorGate(
                    result.wires[i],
                    wires[i],
                    b.wires[i]
            );
        }

        return result;
    }

    public BitData not() {
        BitData result = new BitData(bitCount);

        for (int i = 0; i < bitCount; i++) {
            BitTree.setNotGate(
                    result.wires[i],
                    wires[i]
            );
        }

        return result;
    }
}

