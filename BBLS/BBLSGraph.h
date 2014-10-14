#pragma once

#include<fstream>
#include<map>
using std::ifstream;
using std::map;

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

	void BBLSGraph::readGraph(ifstream &fin);

private:
	static BBLSNode* createNode(unsigned int key, NodeType type);
	map<unsigned int, BBLSNode*> map;
};

