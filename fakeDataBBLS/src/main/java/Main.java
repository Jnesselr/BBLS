import java.io.FileWriter;
import java.io.IOException;
import java.util.Random;
import java.util.Scanner;

public class Main {
    private static final Random random = new Random();

    public static void main(String[] args) throws IOException {
        Scanner scanner = new Scanner(System.in);
        System.out.print("Number of wires: ");
        int numWires = scanner.nextInt();
        System.out.print("Number constant: ");
        int numConstant = scanner.nextInt();
        FileWriter writer = new FileWriter("M:/BBLS/BBLS/test.txt");

        int numCurrent = 0;

        for(; numCurrent < numConstant; numCurrent++) {
            writer.write(String.valueOf(numCurrent+1));
            writer.write("\tC\t");
            writer.write(String.valueOf(random.nextInt(2)));
            writer.write("\n");
        }

        for(; numCurrent < numWires; numCurrent++) {
            writer.write(String.valueOf(numCurrent+1));
            writer.write("\tW\n");
        }

        System.out.print("How many total: ");
        int numTotal = scanner.nextInt();

        for(; numCurrent < numTotal; numCurrent++) {
            writer.write(String.valueOf(numCurrent+1));
            writer.write("\t");
            NodeType randomGate = NodeType.random();
            switch(randomGate) {
                case AndGate:
                    writer.write("A");
                    break;
                case OrGate:
                    writer.write("O");
                    break;
                case XorGate:
                    writer.write("X");
                    break;
                case NotGate:
                    writer.write("N");
                    break;
            }
            int randomIndex1 = random.nextInt(numCurrent)+1;
            writer.write("\t");
            writer.write(String.valueOf(randomIndex1));
            if(randomGate != NodeType.NotGate) {
                int randomIndex2;
                do {
                    randomIndex2 = random.nextInt(numCurrent) + 1;
                } while(randomIndex2 == randomIndex1);
                writer.write("\t");
                writer.write(String.valueOf(randomIndex2));
            }
            if(numCurrent + 1 < numTotal)
                writer.write("\n");
        }
        writer.close();
    }

}

enum NodeType {
    AndGate,
    OrGate,
    XorGate,
    NotGate;

    private static final Random random = new Random();
    public static NodeType random() {
        return values()[random.nextInt(4)];
    }
}
