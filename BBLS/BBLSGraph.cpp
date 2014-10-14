#include "BBLSGraph.h"
#include <algorithm>
#include <iostream>

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

void BBLSGraph::readGraph(istream &fin) {
	unsigned int keyIndex;
	char type;
	unsigned int leftIndex, rightIndex;
	BBLSNode* node = NULL;
	set<unsigned int> notOutputs;
	int maxNode = 0;

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
			if (node->type != ConstantWire && node->type != VariableWire) {
				notOutputs.insert(node->inputLeft);
				outputs.insert(node->inputLeft);
				outputs.insert(node->output);
				if (node->type != NotGate) {
					notOutputs.insert(node->inputRight);
					outputs.insert(node->inputLeft);
				}
			}
		}
	}

	// Remove everything from outputs that's in notOutputs
	for (auto itr = notOutputs.begin(); itr != notOutputs.end(); itr++) {
		// Erase the possibilty of it being an output, by value not index
		outputs.erase(*itr);
	}

	std::cout << "We have " << outputs.size() << " outputs remaining" << std::endl;
}

void BBLSGraph::write(ostream &fout) {
	auto itr = map.begin();
	while (itr != map.end()) {
		fout << itr->first << "\t";
		BBLSNode* node = itr->second;
		switch (node->type) {
		case ConstantWire:
			fout << "C\t";
			fout << node->inputLeft;
			break;
		case VariableWire:
			fout << "W";
			break;
		case AndGate:
			fout << "A\t";
			fout << node->inputLeft << "\t";
			fout << node->inputRight;
			break;
		case OrGate:
			fout << "O\t";
			fout << node->inputLeft << "\t";
			fout << node->inputRight;
			break;
		case XorGate:
			fout << "X\t";
			fout << node->inputLeft << "\t";
			fout << node->inputRight;
			break;
		case NotGate:
			fout << "N\t";
			fout << node->inputLeft;
			break;
		}

		itr++;

		if (itr != map.end()) {
			fout << std::endl;
		}
	}
}

bool BBLSGraph::simplify() {
	bool anySimplified = false;
	bool continueSimplifying = false;
	int originalEntries = map.size();

	do {
		continueSimplifying = false;

		continueSimplifying |= simplifyGates();
		continueSimplifying |= removeUnused();

		if (continueSimplifying)
			anySimplified = true;
	} while (continueSimplifying);

	std::cout << "Went from " << originalEntries << " to " << map.size() << " saving ";
	std::cout << (originalEntries - map.size()) << " entries." << std::endl;

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
				else if (left->type == NotGate) {
					replaceInputs(node->output, left->inputLeft);
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
						replaceInputs(node->output, node->inputRight);
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						node->type = ConstantWire;
						node->inputLeft = 0;
						node->inputRight = 0;
					}
					else {
						replaceInputs(node->output, node->inputLeft);
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
						replaceInputs(node->output, node->inputRight);
					}
					else {
						node->type = ConstantWire;
						node->inputLeft = 1;
						node->inputRight = 0;
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						replaceInputs(node->output, node->inputLeft);
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
						replaceInputs(node->output, node->inputRight);
					}
					else {
						node->type = NotGate;
						node->inputLeft = node->inputRight;
						node->inputRight = 0;
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						replaceInputs(node->output, node->inputLeft);
					}
					else {
						node->type = NotGate;
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

	if (outputs.find(oldInput) != outputs.end()) {
		outputs.erase(oldInput);
		outputs.insert(newInput);
	}
}

bool BBLSGraph::isUsed(unsigned int input) {
	for (auto itr = map.begin(); itr != map.end(); itr++) {
		BBLSNode* node = itr->second;
		if ((node->type != ConstantWire && node->type != VariableWire) &&
			node->inputLeft == input || node->inputRight == input)
			return true;
	}
	for (auto itr = outputs.begin(); itr != outputs.end(); itr++) {
		if (input == *itr) {
			return true;
		}
	}
	return false;
}

bool BBLSGraph::removeUnused() {
	bool somethingChanged = false;
	auto itr = map.begin();
	while (itr != map.end()) {
		if (!isUsed(itr->second->output)) {
			// Make a copy of the reference before deleting
			auto old = itr;
			itr++;
			map.erase(old);
			somethingChanged = true;
		}
		else {
			itr++;
		}
	}
	return somethingChanged;
}