#include "BBLSGraph.h"

#include <iostream>
#include <set>

// Declaring memory for data types needed
MPI_Datatype BBLSGraph::mpi_nodeType;
MPI_Datatype BBLSGraph::mpi_threeNodes;

BBLSGraph::BBLSGraph()
{
	createDatatypes();
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
		// Erase the possibility of it being an output, by value not index
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
	int numProcesses;
	MPI_Comm_size(MCW, &numProcesses);
	
	MPI_Status status;
	Command command;
	int numWorking = 0;
	bool continueSimplifying = true;
	bool anySimplified = false;
    bool shutdownProcesses = false;
    BBLSNode node;
	map<unsigned int, BBLSNode*>::iterator itr;

    double startTime = MPI_Wtime();
    int originalEntries = gateMap.size();
	do {
        itr = gateMap.begin();
        continueSimplifying = removeDuplicates();
		do {
			MPI_Recv(&command, 1, MPI_ENUM, MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &status);
			if(status.MPI_ERROR == 0) {
				int source = status.MPI_SOURCE;
				int tag = status.MPI_TAG;
				if(command == REQUEST_DATA) {
                    if(shutdownProcesses) {
                        itr = gateMap.end();
                        sendMessage(END_PROCESS, 0, source);
                        numProcesses--;
					} else if(itr == gateMap.end()) {
						sendMessage(NO_DATA, 0, source);
					} else {
                        node.output = itr->second->output;
                        node.type = itr->second->type;
                        node.inputLeft = itr->second->inputLeft;
                        node.inputRight = itr->second->inputRight;
                        sendMessage(DATA, 1, source);
						MPI_Send(&node, 1, mpi_nodeType, source, 1, MCW);
                        if(node.type != ConstantWire && node.type != VariableWire) {
                           MPI_Send(gateMap[node.inputLeft],
                           1, mpi_nodeType, source, 1, MCW);
                            if(node.type != NotGate) {
                                MPI_Send(gateMap[node.inputRight],
                                1, mpi_nodeType, source, 1, MCW);
                            }
                        }
						numWorking++;
						itr++;
					}
				} else if(command == RESULT) {
					if(tag == UPDATE_NODE) {
						MPI_Recv(&node, 1, mpi_nodeType, source, MPI_ANY_TAG, MCW, &status);
						if(status.MPI_ERROR == 0) {
							continueSimplifying |= updateNode(node.output,
                            node.type, node.inputLeft, node.inputRight);
						}
						numWorking--;
					} else if(tag == REPLACE_INPUTS) {
						unsigned int oldInput, newInput;
						MPI_Recv(&oldInput, 1, MPI_UNSIGNED, source, MPI_ANY_TAG, MCW, &status);
						MPI_Recv(&newInput, 1, MPI_UNSIGNED, source, MPI_ANY_TAG, MCW, &status);
						continueSimplifying |= replaceInputs(oldInput, newInput);
						numWorking--;
					} else if (tag == NOCHANGE) {
                        numWorking--;
                    }
				}
			}
		} while(itr != gateMap.end() || numWorking > 0);
        continueSimplifying |= removeUnused();
		anySimplified |= continueSimplifying;
        if(!continueSimplifying) {
            shutdownProcesses = true;
        }
	} while(numProcesses > 1);
    double endTime = MPI_Wtime();
    std::cout << "We went from " << originalEntries << " to " << gateMap.size()
    << " in " << endTime - startTime << std::endl;
	return anySimplified;
}

bool BBLSGraph::simplifyGate(BBLSNode* node, BBLSNode* left, BBLSNode* right) {
	if (left != NULL && right == NULL) {
		// ConstantWire and NotGate
		if (node->type == NotGate) {
			if (left->type == ConstantWire) {
				// We can simplify this to the opposite of this constant
				unsigned int newState = (left->inputLeft == 0 ? 1 : 0);
				node->type = ConstantWire;
				node->inputLeft = newState;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == NotGate) {
                node->output = left->inputLeft;
                return true;
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
				return true;
			}
			else if (left->type == ConstantWire) {
				if (left->inputLeft == 0) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
				return true;
				}
				else {
                    node->output = node->inputRight;
                    return true;
				}
			}
			else if (right->type == ConstantWire) {
				if (right->inputLeft == 0) {
					node->type = ConstantWire;
					node->inputLeft = 0;
					node->inputRight = 0;
				return true;
				}
				else {
                    node->output = node->inputLeft;
                    return true;
				}
			}
			else if (node->inputLeft == node->inputRight) {
                node->output = node->inputLeft;
                return true;
			}
			else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
				node->type = NotGate;
				node->inputLeft = left->inputLeft;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
				node->type = ConstantWire;
				node->inputLeft = 0;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
				node->type = ConstantWire;
				node->inputLeft = 0;
				node->inputRight = 0;
				return true;
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
				return true;
			}
			else if (left->type == ConstantWire) {
				if (left->inputLeft == 0) {
                    node->output = node->inputRight;
                    return true;
				}
				else {
					node->type = ConstantWire;
				    node->inputLeft = 1;
				    node->inputRight = 0;
				    return true;
				}
			}
			else if (right->type == ConstantWire) {
				if (right->inputLeft == 0) {
                    node->output = node->inputLeft;
                    return true;
				}
				else {
					node->type = ConstantWire;
					node->inputLeft = 1;
					node->inputRight = 0;
				    return true;
				}
			}
			else if (node->inputLeft == node->inputRight) {
				node->output = node->inputLeft;
				return true;
			}
			else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
				node->type = NotGate;
				node->inputLeft = left->inputLeft;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
				node->type = ConstantWire;
				node->inputLeft = 1;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
				node->type = ConstantWire;
				node->inputLeft = 1;
				node->inputRight = 0;
				return true;
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
				return true;
			}
			else if (left->type == ConstantWire) {
				if (left->inputLeft == 0) {
					node->output = node->inputRight;
					return true;
				}
				else {
					node->type = NotGate;
				    node->inputLeft = node->inputRight;
				    node->inputRight = 0;
				    return true;
				}
			}
			else if (right->type == ConstantWire) {
				if (right->inputLeft == 0) {
					node->output = node->inputLeft;
					return true;
				}
				else {
					node->type = NotGate;
				    node->inputLeft = node->inputLeft;
				    node->inputRight = 0;
				    return true;
				}
			}
			else if (node->inputLeft == node->inputRight) {
				node->type = ConstantWire;
				node->inputLeft = 0;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->inputLeft) {
				node->type = ConstantWire;
				node->inputLeft = 0;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == NotGate && right->type == VariableWire && left->inputLeft == right->output) {
				node->type = ConstantWire;
				node->inputLeft = 0;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == VariableWire && right->type == NotGate && left->output == right->inputLeft) {
				node->type = ConstantWire;
				node->inputLeft = 0;
				node->inputRight = 0;
				return true;
			}
			else if (left->type == NotGate && right->type == NotGate && left->inputLeft == right->output) {
				node->type = node->type;
				node->inputLeft = left->inputLeft;
				node->inputRight = right->inputLeft;
				return true;
			}
		}
	}
	return false;
}

bool BBLSGraph::updateNode(unsigned int output, NodeType type, unsigned int inputLeft, unsigned int inputRight) {
    map<unsigned int, BBLSNode*>::iterator found = gateMap.find(output);
    BBLSNode* node;
    bool result = false;
    if (found == gateMap.end()) {
        node = createNode(output, type);
        result = true;
    } else {
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
            if(node->output == oldInput) {
                reduceUsed(node->inputLeft);
                if(node->type != NotGate) {
                    reduceUsed(node->inputRight);
                }
            }

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

void BBLSGraph::solveThread(int root) {
	int pid;
	MPI_Comm_rank(MCW, &pid);
	createDatatypes();
	
	Command command;
	MPI_Status status;
    BBLSNode* node = new BBLSNode;
    BBLSNode* left = NULL;
    BBLSNode* right = NULL;
	do {
		sendMessage(REQUEST_DATA, root, START_PROCESS);
		MPI_Recv(&command, 1, MPI_ENUM, root, MPI_ANY_TAG, MCW, &status);
		if(status.MPI_ERROR == 0) {
			int source = status.MPI_SOURCE;
			int tag = status.MPI_TAG;
			if(command == DATA) { 
				MPI_Recv(node, 1, mpi_nodeType, root, MPI_ANY_TAG, MCW, &status);
				
                if(node->type != ConstantWire && node->type != VariableWire) {
                    left = new BBLSNode;
                    MPI_Recv(left, 1, mpi_nodeType, root, MPI_ANY_TAG, MCW, &status);
                    if(node->type != NotGate) {
                        right = new BBLSNode;
                        MPI_Recv(right, 1, mpi_nodeType, root, MPI_ANY_TAG, MCW, &status);
                    }
                }
                unsigned int oldInput = node->output;
				
				if(status.MPI_ERROR == 0) {
                    bool result = simplifyGate(node, left, right);
                    if(result) {
                        if(node->output == oldInput) {
                            sendMessage(RESULT, UPDATE_NODE);
                            MPI_Send(node, 1, mpi_nodeType, root, 1, MCW);
                        } else {
                            sendMessage(RESULT, REPLACE_INPUTS);
                            MPI_Send(&oldInput, 1, MPI_UNSIGNED, root, 1, MCW);
                            MPI_Send(&(node->output), 1, MPI_UNSIGNED, root, 1, MCW);
                        }
                    } else {
                        sendMessage(RESULT, NOCHANGE);
                    }
				}
			}
		}
        if(left != NULL) {
            delete left;
            left = NULL;
        }
        if(right != NULL) {
            delete right;
            right = NULL;
        }
	} while(command != END_PROCESS);
    delete node;
}

void BBLSGraph::createDatatypes() {
	// BBLSNode struct
	int block_lengths[5];
	block_lengths[0] = 1;
	block_lengths[1] = 1;
	block_lengths[2] = 1;
	block_lengths[3] = 1;
	block_lengths[4] = 1;
	
	MPI_Aint displacements[5];
	displacements[0] = offsetof(BBLSNode, type);
	displacements[1] = offsetof(BBLSNode, output);
	displacements[2] = offsetof(BBLSNode, inputLeft);
	displacements[3] = offsetof(BBLSNode, inputRight);
	displacements[4] = sizeof(BBLSNode);
	
	MPI_Datatype types[5];
	types[0] = MPI_INT;
	types[1] = MPI_UNSIGNED;
	types[2] = MPI_UNSIGNED;
	types[3] = MPI_UNSIGNED;
	types[4] = MPI_UB;
	
	MPI_Type_struct(5, block_lengths, displacements, types, &mpi_nodeType);
	MPI_Type_commit(&mpi_nodeType);
	
	// 3 BBLSNodes
	MPI_Type_contiguous(3, mpi_nodeType, &mpi_threeNodes);
	MPI_Type_commit(&mpi_threeNodes);
}

void BBLSGraph::sendMessage(Command command, int tag, int root) {
	MPI_Send(&command,
		1,
		MPI_ENUM,
		root,
		tag,
		MCW);
}
