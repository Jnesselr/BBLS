import org.jnesselr.bblsMaker.BitData;
import org.jnesselr.bblsMaker.BitFactory;
import org.jnesselr.bblsMaker.BitType;

public class BitcoinHeader {
    private static final BitType uint32 = BitFactory.createBits(32);
    private BitData version;
    private BitData[] prevBlock;
    private BitData[] merkleRoot;
    private BitData timestamp;
    private BitData difficulty;
    private BitData nonce;

    public BitcoinHeader() {
        version = uint32.make();
        prevBlock = new BitData[8];
        merkleRoot = new BitData[8];
        for (int i = 0; i < 8; i++) {
            prevBlock[i] = uint32.make();
            merkleRoot[i] = uint32.make();
        }
        timestamp = uint32.make();
        difficulty = uint32.make();
        nonce = uint32.make();
    }

    public BitcoinHeader setVersion(long version) {
        this.version = uint32.make(version);
        return this;
    }

    public BitcoinHeader setPrevBlock(String prevHash) {
        // Todo remove this assert
        assert prevHash.length() == 256;

        for (int i = 0; i < 8; i++) {
            prevBlock[i] = uint32.make(Long.valueOf(prevHash.substring(8 * i, 8 * i + 7), 16));
        }

        return this;
    }

    public BitcoinHeader setMerkleRoot(String merkle) {
        // Todo remove this assert
        assert merkle.length() == 256;

        for (int i = 0; i < 8; i++) {
            merkleRoot[i] = uint32.make(Long.valueOf(merkle.substring(8 * i, 8 * i + 7), 16));
        }

        return this;
    }

    public BitcoinHeader setTime(String time) {
        timestamp = uint32.make(Long.valueOf(time, 16));
        return this;
    }

    public BitcoinHeader setDifficulty(String difficultyBits) {
        difficulty = uint32.make(Long.valueOf(difficultyBits, 16));
        return this;
    }

    public BitcoinHeader setNonce(String nonceString) {
        nonce = uint32.make(Long.valueOf(nonceString, 16));
        return this;
    }

    public BitData[] getData() {
        return new BitData[] {
                version,
                prevBlock[0],
                prevBlock[1],
                prevBlock[2],
                prevBlock[3],
                prevBlock[4],
                prevBlock[5],
                prevBlock[6],
                prevBlock[7],
                merkleRoot[0],
                merkleRoot[1],
                merkleRoot[2],
                merkleRoot[3],
                merkleRoot[4],
                merkleRoot[5],
                merkleRoot[6],
                merkleRoot[7],
                timestamp,
                difficulty,
                nonce
        };
    }
}
