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

typedef struct {
	NodeType type;
	unsigned int output;
	unsigned int inputLeft;
	unsigned int inputRight;
} BBLSNode;

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
	void replaceInputs(unsigned int oldInput, unsigned int newInput);
	bool isUsed(unsigned int input);
	map<unsigned int, BBLSNode*> map;
	set<unsigned int> outputs;
};

