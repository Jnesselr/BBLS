import java.io.FileWriter;
import java.io.IOException;
import java.util.HashSet;
import java.util.Random;
import java.util.Scanner;
import java.util.Set;

public class Main {
    private static final Random random = new Random();
    private static Set<Integer> unused;
    private static int numOutput;
    private static int numOutputWanted;
    private static int numCurrent;

    public static void main(String[] args) throws IOException {
        unused = new HashSet<Integer>();
        Scanner scanner = new Scanner(System.in);
        System.out.print("Number of wires: ");
        int numWires = scanner.nextInt();
        System.out.print("Number constant: ");
        int numConstant = scanner.nextInt();
        FileWriter writer = new FileWriter("M:/BBLS/BBLS/test.txt");

        numCurrent = 0;

        for(; numCurrent < numConstant; numCurrent++) {
            writer.write(String.valueOf(numCurrent +1));
            writer.write("\tC\t");
            writer.write(String.valueOf(random.nextInt(2)));
            writer.write("\n");
            unused.add(numCurrent + 1);
        }

        for(; numCurrent < numWires; numCurrent++) {
            writer.write(String.valueOf(numCurrent +1));
            writer.write("\tW\n");
            unused.add(numCurrent + 1);
        }

        System.out.print("How many total: ");
        int numTotal = scanner.nextInt();
        System.out.print("How many output: ");
        numOutputWanted = scanner.nextInt();
        numOutput = numCurrent;

        for(; numCurrent < numTotal; numCurrent++) {
            writer.write(String.valueOf(numCurrent +1));
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
            int randomIndex1 = getRandom();
            writer.write("\t");
            writer.write(String.valueOf(randomIndex1));
            if(randomGate != NodeType.NotGate) {
                int randomIndex2;
                do {
                    randomIndex2 = getRandom();
                } while(randomIndex2 == randomIndex1 && unused.size() > 1);
                writer.write("\t");
                writer.write(String.valueOf(randomIndex2));
            }
            unused.add(numCurrent + 1);
            numOutput++;
            if(numCurrent + 1 < numTotal)
                writer.write("\n");
        }
        writer.close();
    }

    private static int getRandom() {
        int choice = -1;
        if(numOutput < numOutputWanted || unused.size() == 0) {
            // A used one is fine
            do {
                choice = random.nextInt(numCurrent) + 1;
            } while(unused.contains(choice));
        } else {
            // Get one from the set of unused
            int index = random.nextInt(unused.size());
            int i=0;
            for(int item : unused) {
                if(i == index) {
                    numOutput--;
                    choice = item;
                    break;
                }
                i++;
            }
        }
        unused.remove(choice);
        return choice;
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
