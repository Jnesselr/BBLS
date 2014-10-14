#include "BBLSGraph.h"
#include <iostream>
using std::endl;

BBLSGraph::BBLSGraph()
{
}


BBLSGraph::~BBLSGraph()
{
	for (auto itr = map.begin(); itr != map.end(); itr++) {
		delete itr->second;
	}
	map.clear();
}

BBLSNode* BBLSGraph::createNode(unsigned int key, NodeType type) {
	BBLSNode* node = new BBLSNode();
	node->output = key;
	node->type = type;
	return node;
}

void BBLSGraph::readGraph(ifstream &fin) {
	unsigned int keyIndex;
	char type;
	unsigned int leftIndex, rightIndex;
	BBLSNode* node = NULL;
	while (!fin.eof()) {
		fin >> keyIndex;
		fin >> type;

		switch (type) {
		case 'C':
			node = createNode(keyIndex, ConstantWire);
			fin >> leftIndex;
			node->inputLeft = leftIndex;
			break;
		case 'W':
			node = createNode(keyIndex, VariableWire);
			break;
		case 'A':
			node = createNode(keyIndex, AndGate);
			fin >> leftIndex;
			fin >> rightIndex;
			node->inputLeft = leftIndex;
			node->inputRight = rightIndex;
			break;
		case 'O':
			node = createNode(keyIndex, OrGate);
			fin >> leftIndex;
			fin >> rightIndex;
			node->inputLeft = leftIndex;
			node->inputRight = rightIndex;
			break;
		case 'X':
			node = createNode(keyIndex, XorGate);
			fin >> leftIndex;
			fin >> rightIndex;
			node->inputLeft = leftIndex;
			node->inputRight = rightIndex;
			break;
		case 'N':
			node = createNode(keyIndex, NotGate);
			fin >> leftIndex;
			node->inputLeft = leftIndex;
			break;
		default:
			node = NULL;
			break;
		}

		if (node != NULL) {
			map[keyIndex] = node;
		}
	}
	fin.close();
}

void BBLSGraph::write(ofstream &fout) {
	for (auto itr = map.begin(); itr != map.end(); itr++) {
		fout << itr->first << "\t";
		BBLSNode* node = itr->second;
		switch (node->type) {
		case ConstantWire:
			fout << "C\t";
			fout << node->inputLeft << endl;
			break;
		case VariableWire:
			fout << "W" << endl;
			break;
		case AndGate:
			fout << "A\t";
			fout << node->inputLeft << "\t";
			fout << node->inputRight << endl;
			break;
		case OrGate:
			fout << "O\t";
			fout << node->inputLeft << "\t";
			fout << node->inputRight << endl;
			break;
		case XorGate:
			fout << "X\t";
			fout << node->inputLeft << "\t";
			fout << node->inputRight << endl;
			break;
		case NotGate:
			fout << "N\t";
			fout << node->inputLeft << endl;
			break;
		}
	}
}

bool BBLSGraph::simplify() {
	bool anySimplified = false;
	bool continueSimplifying = false;
	do {
		continueSimplifying = false;

		continueSimplifying |= simplifyGates();

		if (continueSimplifying)
			anySimplified = true;
	} while (continueSimplifying);
	return anySimplified;
}

bool BBLSGraph::simplifyGates() {
	bool somethingChanged = false;
	for (auto itr = map.begin(); itr != map.end(); itr++) {
		BBLSNode* node = itr->second;
		NodeType originalType = node->type;
		BBLSNode* left = NULL;
		BBLSNode* right = NULL;
		if (node->inputLeft != 0 && node->type != ConstantWire)
			left = map[node->inputLeft];
		if (node->inputRight != 0)
			right = map[node->inputRight];
		
		if (left != NULL && right == NULL) {
			// ConstantWire and NotGate
			if (node->type == NotGate) {
				if (left->type == ConstantWire) {
					// We can simplify this to the oposite of this constant
					unsigned int newState = (left->inputLeft == 0 ? 1 : 0);
					node->type = ConstantWire;
					node->inputLeft = newState;
				}
			}
		}
		else if (left != NULL && right != NULL) {
			// And, Or, and Xor gates
			if (node->type == AndGate) {
				if (left->type == ConstantWire && right->type == ConstantWire) {
					unsigned int newState = 0;
					if (left->inputLeft == 1 && right->inputLeft == 1) {
						newState = 1;
					}
					node->type = ConstantWire;
					node->inputLeft = newState;
					node->inputRight = 0;
				}
				else if (left->type == ConstantWire) {
					if (left->inputLeft == 0) {
						node->type = ConstantWire;
						node->inputLeft = 0;
						node->inputRight = 0;
					}
					else {
						node->type = VariableWire;
						node->inputLeft = left->inputLeft;
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						node->type = ConstantWire;
						node->inputLeft = 0;
						node->inputRight = 0;
					}
					else {
						node->type = VariableWire;
						node->inputLeft = right->inputLeft;
						node->inputRight = 0;
					}
				}
			}
			else if (node->type == OrGate) {
				if (left->type == ConstantWire && right->type == ConstantWire) {
					unsigned int newState = 0;
					if (left->inputLeft == 1 || right->inputLeft == 1) {
						newState = 1;
					}
					node->type = ConstantWire;
					node->inputLeft = newState;
					node->inputRight = 0;
				}
				else if (left->type == ConstantWire) {
					if (left->inputLeft == 0) {
						replaceInputs(node->inputLeft, right->inputLeft);
					}
					else {
						node->type = ConstantWire;
						node->inputLeft = 1;
						node->inputRight = 0;
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						replaceInputs(node->inputLeft, left->inputLeft);
					}
					else {
						node->type = ConstantWire;
						node->inputLeft = 1;
						node->inputRight = 0;
					}
				}
			}
			else if (node->type == XorGate) {
				if (left->type == ConstantWire && right->type == ConstantWire) {
					unsigned int newState = 0;
					if (left->inputLeft != right->inputLeft) {
						newState = 1;
					}
					node->type = ConstantWire;
					node->inputLeft = newState;
					node->inputRight = 0;
				}
				else if (left->type == ConstantWire) {
					if (left->inputLeft == 0) {
						replaceInputs(node->inputLeft, right->inputLeft);
					}
					else {
						node->type = NotGate;
						node->inputLeft = right->inputLeft;
						node->inputRight = 0;
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						replaceInputs(node->inputLeft, left->inputLeft);
					}
					else {
						node->type = NotGate;
						node->inputLeft = left->inputLeft;
						node->inputRight = 0;
					}
				}
			}
		}
		if (originalType != node->type) {
			somethingChanged = true;
		}
	}
	return somethingChanged;
}

void BBLSGraph::replaceInputs(unsigned int oldInput, unsigned int newInput) {
	for (auto itr = map.begin(); itr != map.end(); itr++) {
		BBLSNode* node = itr->second;
		if (node->inputLeft == oldInput && node->type != ConstantWire) {
			node->inputLeft = newInput;
		}
		if (node->inputRight == oldInput && node->type != ConstantWire) {
			node->inputRight = newInput;
		}
	}
}