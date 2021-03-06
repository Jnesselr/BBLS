#pragma once

#include<algorithm>
#include<iostream>
#include<map>
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
	BBLSGraph();
	~BBLSGraph();

	void readGraph(istream &fin);
	void write(ostream &fout);
	bool simplify();

private:
	static BBLSNode* createNode(unsigned int key, NodeType type);
	bool simplifyGates();
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
};

