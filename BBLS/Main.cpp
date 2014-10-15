#include<fstream>
#include<iostream>
#include<string>

#include"BBLSGraph.h"

using namespace std;

int main(int argc, char* argv[]) {
	string fileName;
	ifstream fin;
	do {
		cout << "File name: ";
		cin >> fileName;
		fin.open(fileName);
		if (fin.fail()) {
			cout << "I'm sorry, that wasn't a valid name" << endl << endl;
		}
	} while (fin.fail());

	BBLSGraph graph;
	graph.readGraph(fin);
	fin.close();

	ofstream fout;
	do {
		cout << "Output file name: ";
		cin >> fileName;
		fout.open(fileName);
	} while (fout.fail());

	graph.simplify();

	cout << "Writing file..." << endl;
	graph.write(fout);
	fout.close();

	return 0;
}