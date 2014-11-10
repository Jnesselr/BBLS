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

struct BBLSNodeHash {
    std::size_t operator () ( const BBLSNode& node ) const
    {
        // (max * (left - 1) + right - 1) * 5 + type-2
        unsigned int minInput = std::min(node.inputLeft, node.inputRight);
        unsigned int maxInput = std::max(node.inputLeft, node.inputRight);
        
        // 1000000 is used since we can't actually know the max value here
        unsigned int hash = (1000000 * minInput + maxInput) * 5 + node.type-1;
        //cout << node.output << ": " << hash << " for " << minInput << " and " << maxInput << " type " << node.type << endl;
        return hash;
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

