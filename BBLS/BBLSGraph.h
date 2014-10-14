#pragma once

#include<fstream>
#include<map>
using std::ifstream;
using std::ofstream;
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

	void readGraph(ifstream &fin);
	void write(ofstream &fout);

private:
	static BBLSNode* createNode(unsigned int key, NodeType type);
	map<unsigned int, BBLSNode*> map;
};

