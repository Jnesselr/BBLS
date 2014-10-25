package org.jnesselr.bblsMaker;

import java.io.PrintStream;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class BitTree {
    private static Map<Integer, BitTreeNode> graph = new HashMap<Integer, BitTreeNode>();
    private static Set<Integer> outputs = new HashSet<Integer>();
    private static Map<Integer, Integer> used = new HashMap<Integer, Integer>();
    private static int outputCount = 0;

    public static int getNextOutput() {
        outputCount++;
        return outputCount;
    }

    private static void update(int output, BitTreeNode newNode) {
        BitTreeNode oldNode = graph.get(output);
        if(oldNode == null) {
            graph.put(output, newNode);
        } else {
            if(oldNode.type != BitTreeNodeType.VariableWire &&
                    oldNode.type != BitTreeNodeType.ConstantWire) {
                reduceUsed(oldNode.inputLeft);
                if(oldNode.type != BitTreeNodeType.NotGate)
                    reduceUsed(oldNode.inputRight);
            }
        }

        if(newNode.type != BitTreeNodeType.VariableWire &&
                newNode.type != BitTreeNodeType.ConstantWire) {
            increaseUsed(newNode.inputLeft);
            if(newNode.type != BitTreeNodeType.NotGate)
                increaseUsed(newNode.inputRight);
        }
    }

    private static void increaseUsed(int input) {
        Integer count = used.get(input);
        if(count == null) {
            count = 0;
        }
        used.put(input, count + 1);
    }

    private static void reduceUsed(int input) {
        Integer count = used.get(input);
        if(count != null) {
            count--;
            used.put(input, count);
            if(count == 0)
                used.remove(input);
        }
    }

    public static void setVariableWire(int output) {
        BitTreeNode node = new BitTreeNode();
        node.output = output;
        node.type = BitTreeNodeType.VariableWire;
        node.inputLeft = 0;
        node.inputRight = 0;

        update(output, node);
    }

    public static void setConstantWire(int output, WireState state) {
        BitTreeNode node = new BitTreeNode();
        node.output = output;
        node.type = BitTreeNodeType.ConstantWire;
        node.inputLeft = (state == WireState.HIGH ? 1 : 0);
        node.inputRight = 0;

        update(output, node);
    }

    public static void setAndGate(int output, int inputLeft, int inputRight) {
        BitTreeNode node = new BitTreeNode();
        node.output = output;
        node.type = BitTreeNodeType.AndGate;
        node.inputLeft = inputLeft;
        node.inputRight = inputRight;

        update(output, node);
    }

    public static void setOrGate(int output, int inputLeft, int inputRight) {
        BitTreeNode node = new BitTreeNode();
        node.output = output;
        node.type = BitTreeNodeType.OrGate;
        node.inputLeft = inputLeft;
        node.inputRight = inputRight;

        update(output, node);
    }

    public static void setXorGate(int output, int inputLeft, int inputRight) {
        BitTreeNode node = new BitTreeNode();
        node.output = output;
        node.type = BitTreeNodeType.XorGate;
        node.inputLeft = inputLeft;
        node.inputRight = inputRight;

        update(output, node);
    }

    public static void setNotGate(int output, int input) {
        BitTreeNode node = new BitTreeNode();
        node.output = output;
        node.type = BitTreeNodeType.NotGate;
        node.inputLeft = input;
        node.inputRight = 0;

        update(output, node);
    }

    public static void displayData(PrintStream out) {
        int outputIndex = 0;
        int numOutput = 0;
        int graphSize = graph.size();
        while(numOutput < graphSize) {
            outputIndex++;
            BitTreeNode node = graph.get(outputIndex);
            if(node != null) {
                numOutput++;
                out.print(outputIndex);
                out.print("\t");
                out.print(BitTreeNodeType.getChar(node.type));
                if(node.type != BitTreeNodeType.VariableWire) {
                    out.print("\t");
                    out.print(node.inputLeft);
                    if(node.type != BitTreeNodeType.ConstantWire &&
                            node.type != BitTreeNodeType.NotGate) {
                        out.print("\t");
                        out.print(node.inputRight);
                    }
                }
                if(numOutput != graphSize)
                    out.println();
            }
        }
    }

    /*
    Cleaning up the tree involves:
    Removing unused nodes
    todo Renumbering
     */
    public static void clean() {
        System.out.print("Cleaning " + graph.size() + " elements to ");
        Set<Integer> toRemove = new HashSet<Integer>();
        for(Integer key : graph.keySet()) {
            if(!isUsed(key)) {
                toRemove.add(key);
            }
        }
        for(Integer key : toRemove) {
            graph.remove(key);
        }
        System.out.println(graph.size());
    }

    private static boolean isUsed(int output) {
        return outputs.contains(output) || used.containsKey(output);
    }

    public static void setOutput(BitData data) {
        int[] wires = data.getWires();

        for (int wire : wires) {
            outputs.add(wire);
        }
    }
}

class BitTreeNode {
    public int output;
    public BitTreeNodeType type;
    public int inputLeft;
    public int inputRight;
}

enum BitTreeNodeType {
    VariableWire,
    ConstantWire,
    AndGate,
    OrGate,
    XorGate,
    NotGate;

    public static String getChar(BitTreeNodeType type) {
        switch(type) {
            case VariableWire:
                return "W";
            case ConstantWire:
                return "C";
            case AndGate:
                return "A";
            case OrGate:
                return "O";
            case XorGate:
                return "X";
            case NotGate:
                return "N";
        }
        return null;
    }
}
