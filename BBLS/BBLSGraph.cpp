#include "BBLSGraph.h"

#include <iostream>
#include <set>

BBLSGraph::BBLSGraph()
{
    maxNode = 0;
}


BBLSGraph::~BBLSGraph()
{
	for (map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		delete itr->second;
	}
	gateMap.clear();

	delete[] used;
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
    std::set<unsigned int> notOutputs;

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
			maxNode = std::max(maxNode, node->output);
			if (node->type != ConstantWire && node->type != VariableWire) {
				outputs[node->output] = node->output;

				maxNode = std::max(maxNode, node->inputLeft);
				notOutputs.insert(node->inputLeft);
				outputs[node->inputLeft] = node->inputLeft;
				if (node->type != NotGate) {
					maxNode = std::max(maxNode, node->inputRight);
					notOutputs.insert(node->inputRight);
					outputs[node->inputRight] = node->inputRight;
				}
			}
		}
	}

	// Remove everything from outputs that's in notOutputs
	for (std::set<unsigned int>::iterator itr = notOutputs.begin(); itr != notOutputs.end(); itr++) {
		// Erase the possibilty of it being an output, by value not index
		outputs.erase(*itr);
	}

	// Create the used size now that we know how big it should be
	used = new unsigned int[maxNode]();


	// Set the used counts for all gates
	for (map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		
		if (node->type != ConstantWire && node->type != VariableWire) {
			increaseUsed(node->inputLeft);
			if (node->type != NotGate) {
				increaseUsed(node->inputRight);
			}
		}
	}


	// All outputs are technically used once
	for (std::map<unsigned int, unsigned int>::iterator itr = outputs.begin(); itr != outputs.end(); itr++) {
		increaseUsed(itr->first);
	}

	std::cout << "We have " << outputs.size() << " outputs." << std::endl;
}

void BBLSGraph::write(ostream &fout) {
	fout << "--INPUT--" << std::endl;

	for (map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin(); itr != gateMap.end(); itr++) {
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
	for (map<unsigned int, unsigned int>::iterator itr = outputs.begin(); itr != outputs.end(); itr++) {
		fout << std::endl << itr->first << " -> " << itr->second;
	}
}

bool BBLSGraph::simplify() {
	bool anySimplified = false;
	bool continueSimplifying = false;
	int originalEntries = gateMap.size();

//	std::cout << std::endl << "Simplifying..." << std::endl;
	do {
		continueSimplifying = false;

//		std::cout << " -> Removing duplicates" << std::endl;
		continueSimplifying |= removeDuplicates();
//		std::cout << " -> Simplifying gates" << std::endl;
		continueSimplifying |= simplifyGates();
//		std::cout << " -> Removing unused" << std::endl;
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
	for (map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		BBLSNode* left = NULL;
		BBLSNode* right = NULL;
		if (node->inputLeft != 0 && node->type != ConstantWire) {
			if (!isUsed(node->inputLeft))
				std::cout << node->output << " is using left " << node->inputLeft << " when it doesn't exist" << std::endl;
			left = gateMap[node->inputLeft];
		}
		if (node->inputRight != 0) {
			if (!isUsed(node->inputRight))
				std::cout << node->output << " is using right " << node->inputRight << " when it doesn't exist" << std::endl;
			right = gateMap[node->inputRight];
		}

		if (left != NULL && right == NULL) {
			// ConstantWire and NotGate
			if (node->type == NotGate) {
				if (left->type == ConstantWire) {
					// We can simplify this to the oposite of this constant
					unsigned int newState = (left->inputLeft == 0 ? 1 : 0);
					somethingChanged |= updateNode(node->output, ConstantWire, newState, 0);
				}
				else if (left->type == NotGate) {
					somethingChanged |= replaceInputs(node->output, left->inputLeft);
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
					somethingChanged |= updateNode(node->output, ConstantWire, newState, 0);
				}
				else if (left->type == ConstantWire) {
					if (left->inputLeft == 0) {
						somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
					}
					else {
						somethingChanged |= replaceInputs(node->output, node->inputRight);
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
					}
					else {
						somethingChanged |= replaceInputs(node->output, node->inputLeft);
					}
				}
				else if (node->inputLeft == node->inputRight) {
					somethingChanged |= replaceInputs(node->output, node->inputLeft);
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
					somethingChanged |= updateNode(node->output, NotGate, left->inputLeft, 0);
				}
				else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
					somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
				}
				else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
					somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
				}
			}
			else if (node->type == OrGate) {
				if (left->type == ConstantWire && right->type == ConstantWire) {
					unsigned int newState = 0;
					if (left->inputLeft == 1 || right->inputLeft == 1) {
						newState = 1;
					}
					somethingChanged |= updateNode(node->output, ConstantWire, newState, 0);
				}
				else if (left->type == ConstantWire) {
					if (left->inputLeft == 0) {
						somethingChanged |= replaceInputs(node->output, node->inputRight);
					}
					else {
						somethingChanged |= updateNode(node->output, ConstantWire, 1, 0);
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						somethingChanged |= replaceInputs(node->output, node->inputLeft);
					}
					else {
						somethingChanged |= updateNode(node->output, ConstantWire, 1, 0);
					}
				}
				else if (node->inputLeft == node->inputRight) {
					somethingChanged |= replaceInputs(node->output, node->inputLeft);
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
					somethingChanged |= updateNode(node->output, NotGate, left->inputLeft, 0);
				}
				else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
					somethingChanged |= updateNode(node->output, ConstantWire, 1, 0);
				}
				else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
					somethingChanged |= updateNode(node->output, ConstantWire, 1, 0);
				}
			}
			else if (node->type == XorGate) {
				if (left->type == ConstantWire && right->type == ConstantWire) {
					unsigned int newState = 0;
					if (left->inputLeft != right->inputLeft) {
						newState = 1;
					}
					somethingChanged |= updateNode(node->output, ConstantWire, newState, 0);
				}
				else if (left->type == ConstantWire) {
					if (left->inputLeft == 0) {
						somethingChanged |= replaceInputs(node->output, node->inputRight);
					}
					else {
						somethingChanged |= updateNode(node->output, NotGate, node->inputRight, 0);
					}
				}
				else if (right->type == ConstantWire) {
					if (right->inputLeft == 0) {
						somethingChanged |= replaceInputs(node->output, node->inputLeft);
					}
					else {
						somethingChanged |= updateNode(node->output, NotGate, node->inputLeft, 0);
					}
				}
				else if (node->inputLeft == node->inputRight) {
					somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
					somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
				}
				else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
					somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
				}
				else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
					somethingChanged |= updateNode(node->output, ConstantWire, 0, 0);
				}
				else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->output) {
					somethingChanged |= updateNode(node->output, node->type, left->inputLeft, right->inputLeft);
				}
			}
		}
	}
	return somethingChanged;
}

bool BBLSGraph::updateNode(unsigned int output, NodeType type, unsigned int inputLeft, unsigned int inputRight) {
    map<unsigned int, BBLSNode*>::iterator found = gateMap.find(output);
	BBLSNode* node;
	bool result = false;
	if (found == gateMap.end()) {
		node = createNode(output, type);
		result = true;
	}
	else {
		node = found->second;
		result = node->type != type || node->inputLeft != inputLeft || node->inputRight != inputRight;

		if (node->type != ConstantWire && node->type != VariableWire) {
			reduceUsed(node->inputLeft);
			if (node->type != NotGate) {
				reduceUsed(node->inputRight);
			}
		}
	}

	node->type = type;
	node->inputLeft = inputLeft;
	node->inputRight = inputRight;
	
	if (node->type != ConstantWire && node->type != VariableWire) {
		increaseUsed(node->inputLeft);
		if (node->type != NotGate) {
			increaseUsed(node->inputRight);
		}
	}

	return result;
}

bool BBLSGraph::replaceInputs(unsigned int oldInput, unsigned int newInput) {
	bool result = false;
	for (map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		if (node->type != ConstantWire && node->type != VariableWire) {

			if (node->inputLeft == oldInput) {
				reduceUsed(oldInput);
				node->inputLeft = newInput;
				increaseUsed(newInput);
				result = true;
			}
            if (node->type != NotGate && node->inputRight == oldInput) {
				reduceUsed(oldInput);
				node->inputRight = newInput;
				increaseUsed(newInput);
				result = true;
            }
		}
	}

	for (map<unsigned int, unsigned int>::iterator itr = outputs.begin(); itr != outputs.end(); itr++) {
		if (itr->first == oldInput || itr->second == oldInput) {
			reduceUsed(oldInput);
			outputs[itr->first] = newInput;
			increaseUsed(newInput);
			result = true;
		}
	}

	if (used[oldInput-1] != 0) {
		std::cout << oldInput << " => " << newInput << " is " << used[oldInput-1] << std::endl;
	}
	
	return result;
}

bool BBLSGraph::isUsed(unsigned int input) {
	return used[input-1] > 0;
}

bool BBLSGraph::removeUnused() {
	bool somethingChanged = false;
	map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin();
	while (itr != gateMap.end()) {
		if (!isUsed(itr->second->output)) {
			// Make a copy of the reference before deleting
			map<unsigned int, BBLSNode*>::iterator old = itr;
            itr++;
            BBLSNode* node = old->second;
            if(node->type != VariableWire && node->type != ConstantWire) {
                reduceUsed(node->inputLeft);
                if(node->type != NotGate)
                    reduceUsed(node->inputRight);
            }
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

    std::set<BBLSNode> usedGates;
	for (map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin(); itr != gateMap.end(); itr++) {
		BBLSNode* node = itr->second;
		std::set<BBLSNode>::iterator found = usedGates.find(*node);
		if (node->type != VariableWire) {
			if (found != usedGates.end()) {
				somethingChanged |= replaceInputs(node->output, (*found).output);
			}
			else {
				usedGates.insert(*node);
			}
		}
	}

	return somethingChanged;
}

// Note: There is an unknown bug with this function
bool BBLSGraph::renumber() {
	bool somethingChanged = false;

	unsigned int index = 1;
	map<unsigned int, BBLSNode*>::iterator itr = gateMap.begin();
	while (itr != gateMap.end()) {
		map<unsigned int, BBLSNode*>::iterator found = gateMap.find(index);

		// While this index is taken
		while (found != gateMap.end() && itr != found) {
			index++;
			found = gateMap.find(index);
		}
		
		if (itr != found) {
			// We found an unused index
			somethingChanged |= replaceInputs(itr->second->output, index);
			itr->second->output = index;
			map<unsigned int, BBLSNode*>::iterator old = itr;
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

void BBLSGraph::increaseUsed(unsigned int input) {
    used[input - 1] += 1;
}

void BBLSGraph::reduceUsed(unsigned int input) {
	unsigned int count = used[input-1];
	if (count != 0) {
		count--;
		used[input-1] = count;
	}
}
