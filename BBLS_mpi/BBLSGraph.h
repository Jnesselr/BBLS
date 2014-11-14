#pragma once

#include<algorithm>
#include<string>
#include<iostream>
#include<map>
#include<mpi.h>
using std::istream;
using std::ostream;
using std::map;
using std::cout;
using std::endl;

enum NodeType {
	VariableWire,
	ConstantWire,
	AndGate,
	OrGate,
	XorGate,
	NotGate
};

#ifndef MCW
#define MCW MPI_COMM_WORLD
#endif
#define MPI_ENUM MPI_INT

enum Command {
	START_PROCESS,
	REQUEST_DATA,
	DATA,
	NO_DATA,
	RESULT,
	END_PROCESS
};

enum DataTag {
	UPDATE_NODE,
	REPLACE_INPUTS,
    NOCHANGE
};

struct BBLSNode{
	NodeType type;
	unsigned int output;
	unsigned int inputLeft;
	unsigned int inputRight;

	bool operator<(const BBLSNode& node) const {
		if (this->inputLeft == node.inputLeft) {
			if (this->inputRight == node.inputRight) {
				return this->type < node.type;
			}
			return this->inputRight < node.inputRight;
		}
		return this->inputLeft < node.inputLeft;
	}

	bool operator==(const BBLSNode& node) const {
        if(this->type == node.type && this->type != VariableWire) {
            if(this->type == ConstantWire || this->type == NotGate)
                return this->inputLeft == node.inputLeft;
            if(this->inputLeft == node.inputLeft &&
               this->inputRight == node.inputRight)
                return true;
            if(this->inputLeft == node.inputRight &&
               this->inputRight == node.inputLeft)
                return true;
        }
        return false;
	}
};

class BBLSGraph
{
public:
	// MPI Stuff
	static void solveThread(int root);
	
	BBLSGraph();
	~BBLSGraph();

	void readGraph(istream &fin);
	void write(ostream &fout);
	bool simplify();

private:
	// MPI Stuff
	static void sendMessage(Command command, int tag=0, int root=0);
	static void createDatatypes();

	static BBLSNode* createNode(unsigned int key, NodeType type);
    static bool simplifyGate(BBLSNode*, BBLSNode*, BBLSNode*);
	bool removeUnused();
	bool removeDuplicates();
	bool renumber();
	bool isUsed(unsigned int input);

	bool updateNode(unsigned int output, NodeType type, unsigned int inputLeft, unsigned int inputRight);
	bool replaceInputs(unsigned int oldInput, unsigned int newInput);
	void increaseUsed(unsigned int input);
	void reduceUsed(unsigned int input);

	map<unsigned int, BBLSNode*> gateMap;
	map<unsigned int, unsigned int> outputs;
	unsigned int *used;
    unsigned int maxNode;
	
	static MPI_Datatype mpi_nodeType;
	static MPI_Datatype mpi_threeNodes;
};

