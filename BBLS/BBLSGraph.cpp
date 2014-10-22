#include "BBLSGraph.h"
#include <algorithm>
#include <iostream>

BBLSGraph::BBLSGraph()
{
}


BBLSGraph::~BBLSGraph()
{
	for (auto itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		delete itr->second;
	}
	gateMap.clear();
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
			gateMap[keyIndex] = node;
			if (node->type != ConstantWire && node->type != VariableWire) {
				notOutputs.insert(node->inputLeft);
				outputs[node->inputLeft] = node->inputLeft;
				outputs[node->output] = node->output;
				if (node->type != NotGate) {
					notOutputs.insert(node->inputRight);
					outputs[node->inputRight] = node->inputRight;
				}
			}
		}
	}

	// Remove everything from outputs that's in notOutputs
	for (auto itr = notOutputs.begin(); itr != notOutputs.end(); itr++) {
		// Erase the possibilty of it being an output, by value not index
		outputs.erase(*itr);
	}

	std::cout << "We have " << outputs.size() << " outputs." << std::endl << std::endl;
}

void BBLSGraph::write(ostream &fout) {
	fout << "--INPUT--" << std::endl;

	for (auto itr = gateMap.begin(); itr != gateMap.end(); itr++) {
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

		fout << std::endl;
	}

	fout << std::endl << "--OUTPUT--";
	for (auto itr = outputs.begin(); itr != outputs.end(); itr++) {
		fout << std::endl << itr->first << " -> " << itr->second;
	}
}

bool BBLSGraph::simplify() {
	bool anySimplified = false;
	bool continueSimplifying = false;
	int originalEntries = gateMap.size();

	std::cout << std::endl << "Simplifying..." << std::endl;
	do {
		continueSimplifying = false;

		continueSimplifying |= removeDuplicates();
		continueSimplifying |= simplifyGates();
		continueSimplifying |= removeUnused();

		if (continueSimplifying) {
			anySimplified = true;
			std::cout << "Went from " << originalEntries << " to " << gateMap.size() << " saving ";
			std::cout << (originalEntries - gateMap.size()) << " entries." << std::endl;
		}
	} while (continueSimplifying);

	return anySimplified;
}

bool BBLSGraph::simplifyGates() {
	bool somethingChanged = false;
	for (auto itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		NodeType originalType = node->type;
		BBLSNode* left = NULL;
		BBLSNode* right = NULL;
		if (node->inputLeft != 0 && node->type != ConstantWire)
			left = gateMap[node->inputLeft];
		if (node->inputRight != 0)
			right = gateMap[node->inputRight];

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
					somethingChanged = replaceInputs(node->output, left->inputLeft);
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
						somethingChanged = replaceInputs(node->output, node->inputRight);
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						node->type = ConstantWire;
						node->inputLeft = 0;
						node->inputRight = 0;
					}
					else {
						somethingChanged = replaceInputs(node->output, node->inputLeft);
					}
				}
				else if (node->inputLeft == node->inputRight) {
					somethingChanged = replaceInputs(node->output, node->inputLeft);
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
					node->type = NotGate;
					node->inputLeft = left->inputLeft;
					node->inputRight = 0;
				}
				else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
				}
				else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
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
						somethingChanged = replaceInputs(node->output, node->inputRight);
					}
					else {
						node->type = ConstantWire;
						node->inputLeft = 1;
						node->inputRight = 0;
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						somethingChanged = replaceInputs(node->output, node->inputLeft);
					}
					else {
						node->type = ConstantWire;
						node->inputLeft = 1;
						node->inputRight = 0;
					}
				}
				else if (node->inputLeft == node->inputRight) {
					somethingChanged = replaceInputs(node->output, node->inputLeft);
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
					node->type = NotGate;
					node->inputLeft = left->inputLeft;
					node->inputRight = 0;
				}
				else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
					node->type = ConstantWire;
					node->inputLeft = 1;
					node->inputRight = 0;
				}
				else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
					node->type = ConstantWire;
					node->inputLeft = 1;
					node->inputRight = 0;
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
						somethingChanged = replaceInputs(node->output, node->inputRight);
					}
					else {
						node->type = NotGate;
						node->inputLeft = node->inputRight;
						node->inputRight = 0;
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						somethingChanged = replaceInputs(node->output, node->inputLeft);
					}
					else {
						// The left input doesn't change
						node->type = NotGate;
						node->inputRight = 0;
					}
				}
				else if (node->inputLeft == node->inputRight) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
				}
				else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
				}
				else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->output) {
					node->inputLeft = left->inputLeft;
					node->inputRight = right->inputLeft;
					somethingChanged = true;
				}
			}
		}
		if (originalType != node->type) {
			somethingChanged = true;
		}
	}
	return somethingChanged;
}

bool BBLSGraph::replaceInputs(unsigned int oldInput, unsigned int newInput) {
	bool result = false;
	for (auto itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		if (node->inputLeft == oldInput) {
			node->inputLeft = newInput;
			result = true;
		}
		if (node->inputRight == oldInput) {
			node->inputRight = newInput;
			result = true;
		}
	}

	for (auto itr = outputs.begin(); itr != outputs.end(); itr++) {
		if (itr->first == oldInput || itr->second == oldInput) {
			outputs[itr->first] = newInput;
			result = true;
		}
	}
	return result;
}

bool BBLSGraph::isUsed(unsigned int input) {
	for (auto itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		if (node->type != ConstantWire && node->type != VariableWire) {
			if (node->inputLeft == input)
				return true;
			if (node->type != NotGate && node->inputRight == input)
				return true;
		}
	}
	for (auto itr = outputs.begin(); itr != outputs.end(); itr++) {
		if (input == itr->second) {
			return true;
		}
	}
	return false;
}

bool BBLSGraph::removeUnused() {
	bool somethingChanged = false;
	auto itr = gateMap.begin();
	while (itr != gateMap.end()) {
		if (!isUsed(itr->second->output)) {
			// Make a copy of the reference before deleting
			auto old = itr;
			itr++;
			gateMap.erase(old);
			somethingChanged = true;
		}
		else {
			itr++;
		}
	}
	return somethingChanged;
}

bool BBLSGraph::removeDuplicates() {
	bool somethingChanged = false;

	set<BBLSNode> used;
	for (auto itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		auto found = used.find(*node);
		if (node->type != VariableWire) {
			if (found != used.end()) {
				somethingChanged |= replaceInputs(node->output, found->output);
			}
			else {
				used.insert(*node);
			}
		}
	}

	return somethingChanged;
}

// Note: There is an unknown bug with this function
bool BBLSGraph::renumber() {
	bool somethingChanged = false;

	unsigned int index = 1;
	auto itr = gateMap.begin();
	while (itr != gateMap.end()) {
		auto found = gateMap.find(index);

		// While this index is taken
		while (found != gateMap.end() && itr != found) {
			index++;
			found = gateMap.find(index);
		}
		
		if (itr != found) {
			// We found an unused index
			somethingChanged |= replaceInputs(itr->second->output, index);
			itr->second->output = index;
			auto old = itr;
			itr++;
			gateMap[index] = old->second;
			gateMap.erase(old);
		}
		else {
			itr++;
		}

	}

	return somethingChanged;
}