#include<fstream>
#include<iostream>
#include<string>
#include<mpi.h>

#include"BBLSGraph.h"

using namespace std;

int main(int argc, char* argv[]) {
	string fileName;
	ifstream fin;
	do {
		cout << "File name: ";
		cin >> fileName;
		fin.open(fileName.c_str());
		if (fin.fail()) {
			cout << "I'm sorry, that wasn't a valid name" << endl << endl;
		}
	} while (fin.fail());

	BBLSGraph graph;
	graph.readGraph(fin);
	fin.close();

	ofstream fout;
	cout << "Output file name: ";
	cin >> fileName;

    double startTime = MPI_Wtime();
	graph.simplify();
    double time = MPI_Wtime() - startTime;

	fout.open(fileName.c_str());
	while (fout.fail()) {
		cout << "I'm sorry, but I couldn't open that file for writing" << endl;
		cout << "File name: ";
		cin >> fileName;
		fout.open(fileName.c_str());
	}
	cout << endl << "Writing file..." << endl;
	graph.write(fout);
	fout.close();

    cout << "Time: " << time << std::endl;

	return 0;
}
