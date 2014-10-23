#pragma once

#include<iostream>
#include<map>
#include<vector>
#include <set>
using std::set;
using std::istream;
using std::ostream;
using std::map;
using std::vector;

enum NodeType {
	ConstantWire,
	VariableWire,
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
		return this->inputLeft == node.inputLeft &&
			this->inputRight == node.inputRight &&
			this->type == node.type;
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
	bool updateNode(unsigned int, BBLSNode*);
	bool replaceInputs(unsigned int oldInput, unsigned int newInput);
	bool isUsed(unsigned int input);

	void increaseUsed(unsigned int input);
	void reduceUsed(unsigned int input);

	map<unsigned int, BBLSNode*> gateMap;
	map<unsigned int, unsigned int> outputs;
	map<unsigned int, unsigned int> used;
};

