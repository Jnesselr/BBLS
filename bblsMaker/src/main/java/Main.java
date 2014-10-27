import org.jnesselr.bblsMaker.BitData;
import org.jnesselr.bblsMaker.BitFactory;
import org.jnesselr.bblsMaker.BitTree;
import org.jnesselr.bblsMaker.BitType;

import java.io.*;

public class Main {
    private static final BitType uint32 = BitFactory.createBits(32);

    public static void main(String[] args) throws IOException {
        BitcoinHeader header = new BitcoinHeader()
                .setVersion(1)
                .setPrevBlock("81cd02ab7e569e8bcd9317e2fe99f2de44d49ab2b8851ba4a308000000000000")
                .setMerkleRoot("e320b6c2fffc8d750423db8b1eb942ae710e951ed797f7affc8892b0f1fc122b")
                .setTime("c7f5d74d")
                .setDifficulty("f2b9441a");
                //.setNonce("42a14695");

        // First round
        BitData[] result = sha256(header.getData(), 640);

        // Second round
        result = sha256(result, 256);

        BitTree.setOutput(result[7].flipEndian(8));
        BitTree.setOutput(result[6].flipEndian(8));
        BitTree.setOutput(result[5].flipEndian(8));
        BitTree.setOutput(result[4].flipEndian(8));
        BitTree.setOutput(result[3].flipEndian(8));
        BitTree.setOutput(result[2].flipEndian(8));
        BitTree.setOutput(result[1].flipEndian(8));
        BitTree.setOutput(result[0].flipEndian(8));

        BitTree.clean();
        File dataFile = new File("data.txt");
        PrintStream stream = new PrintStream(dataFile);
        BitTree.displayData(stream);
    }

    private static BitData[] sha256(BitData[] data, long length) {
        BitData[] fullData = new BitData[(int) (((length + 576) / 512) * 16)];
        System.arraycopy(data, 0, fullData, 0, data.length);

        for (int i = data.length; i < fullData.length - 2; i++) {
            fullData[i] = uint32.make(0);
        }

        int lengthToShift = (int) (31 - (length % 32));
        int indexToShift = (int) (length / 32);
        BitData mask = uint32.make(1 << lengthToShift);
        fullData[indexToShift] = fullData[indexToShift]
                .rightRotate(lengthToShift + 1)
                .leftShift(lengthToShift + 1)
                .or(mask);

        fullData[fullData.length - 2] = uint32.make(length / 100000000L);
        fullData[fullData.length - 1] = uint32.make(length % 100000000L);

        BitData h0 = uint32.make(0x6a09e667);
        BitData h1 = uint32.make(0xbb67ae85);
        BitData h2 = uint32.make(0x3c6ef372);
        BitData h3 = uint32.make(0xa54ff53a);
        BitData h4 = uint32.make(0x510e527f);
        BitData h5 = uint32.make(0x9b05688c);
        BitData h6 = uint32.make(0x1f83d9ab);
        BitData h7 = uint32.make(0x5be0cd19);

        BitData[] result = new BitData[8];
        BitData[] roundData = new BitData[16];
        for (int i = 0; i < fullData.length; i += 16) {
            System.arraycopy(fullData, i, roundData, 0, 16);
            result = sha256_round(h0, h1, h2, h3, h4, h5, h6, h7, roundData);

            h0 = result[0];
            h1 = result[1];
            h2 = result[2];
            h3 = result[3];
            h4 = result[4];
            h5 = result[5];
            h6 = result[6];
            h7 = result[7];
        }
        return result;
    }

    private static BitData[] sha256_round(BitData h0, BitData h1, BitData h2, BitData h3, BitData h4, BitData h5, BitData h6, BitData h7, BitData[] data) {
        BitData[] w = new BitData[64];
        BitData[] k = getConstants();
        System.arraycopy(data, 0, w, 0, data.length);
        for (int i = 16; i < 64; i++) {
            BitData s0 = (w[i - 15].rightRotate(7).xor(w[i - 15].rightRotate(18)).xor(w[i - 15].rightShift(3)));
            BitData s1 = (w[i - 2].rightRotate(17).xor(w[i - 2].rightRotate(19)).xor(w[i - 2].rightShift(10)));
            w[i] = w[i - 16].add(s0).add(w[i - 7]).add(s1);
        }

        BitData a = h0;
        BitData b = h1;
        BitData c = h2;
        BitData d = h3;
        BitData e = h4;
        BitData f = h5;
        BitData g = h6;
        BitData h = h7;

        for (int i = 0; i < 64; i++) {
            BitData s1 = e.rightRotate(6).xor(e.rightRotate(11)).xor(e.rightRotate(25));
            BitData ch = e.and(f).xor(e.not().and(g));
            BitData temp1 = h.add(s1).add(ch).add(k[i]).add(w[i]);
            BitData s0 = a.rightRotate(2).xor(a.rightRotate(13)).xor(a.rightRotate(22));
            BitData maj = a.and(b).xor(a.and(c)).xor(b.and(c));
            BitData temp2 = s0.add(maj);

            h = g;
            g = f;
            f = e;
            e = d.add(temp1);
            d = c;
            c = b;
            b = a;
            a = temp1.add(temp2);
        }

        h0 = h0.add(a);
        h1 = h1.add(b);
        h2 = h2.add(c);
        h3 = h3.add(d);
        h4 = h4.add(e);
        h5 = h5.add(f);
        h6 = h6.add(g);
        h7 = h7.add(h);

        return new BitData[]{h0, h1, h2, h3, h4, h5, h6, h7};
    }

    private static BitData[] getConstants() {
        return new BitData[]{
                uint32.make(0x428a2f98),
                uint32.make(0x71374491),
                uint32.make(0xb5c0fbcf),
                uint32.make(0xe9b5dba5),
                uint32.make(0x3956c25b),
                uint32.make(0x59f111f1),
                uint32.make(0x923f82a4),
                uint32.make(0xab1c5ed5),
                uint32.make(0xd807aa98),
                uint32.make(0x12835b01),
                uint32.make(0x243185be),
                uint32.make(0x550c7dc3),
                uint32.make(0x72be5d74),
                uint32.make(0x80deb1fe),
                uint32.make(0x9bdc06a7),
                uint32.make(0xc19bf174),
                uint32.make(0xe49b69c1),
                uint32.make(0xefbe4786),
                uint32.make(0x0fc19dc6),
                uint32.make(0x240ca1cc),
                uint32.make(0x2de92c6f),
                uint32.make(0x4a7484aa),
                uint32.make(0x5cb0a9dc),
                uint32.make(0x76f988da),
                uint32.make(0x983e5152),
                uint32.make(0xa831c66d),
                uint32.make(0xb00327c8),
                uint32.make(0xbf597fc7),
                uint32.make(0xc6e00bf3),
                uint32.make(0xd5a79147),
                uint32.make(0x06ca6351),
                uint32.make(0x14292967),
                uint32.make(0x27b70a85),
                uint32.make(0x2e1b2138),
                uint32.make(0x4d2c6dfc),
                uint32.make(0x53380d13),
                uint32.make(0x650a7354),
                uint32.make(0x766a0abb),
                uint32.make(0x81c2c92e),
                uint32.make(0x92722c85),
                uint32.make(0xa2bfe8a1),
                uint32.make(0xa81a664b),
                uint32.make(0xc24b8b70),
                uint32.make(0xc76c51a3),
                uint32.make(0xd192e819),
                uint32.make(0xd6990624),
                uint32.make(0xf40e3585),
                uint32.make(0x106aa070),
                uint32.make(0x19a4c116),
                uint32.make(0x1e376c08),
                uint32.make(0x2748774c),
                uint32.make(0x34b0bcb5),
                uint32.make(0x391c0cb3),
                uint32.make(0x4ed8aa4a),
                uint32.make(0x5b9cca4f),
                uint32.make(0x682e6ff3),
                uint32.make(0x748f82ee),
                uint32.make(0x78a5636f),
                uint32.make(0x84c87814),
                uint32.make(0x8cc70208),
                uint32.make(0x90befffa),
                uint32.make(0xa4506ceb),
                uint32.make(0xbef9a3f7),
                uint32.make(0xc67178f2)
        };
    }
}
