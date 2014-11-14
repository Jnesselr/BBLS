#include<fstream>
#include<iostream>
#include<string>
#include<mpi.h>

#include"BBLSGraph.h"

using namespace std;

#ifndef MCW
#define MCW MPI_COMM_WORLD
#endif

#ifndef ROOT
#define ROOT 0
#endif

int main(int argc, char* argv[]) {
    int pid;
    int numProcesses;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MCW, &pid);
    MPI_Comm_size(MCW, &numProcesses); 

    if(pid == ROOT) {
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

    	graph.simplify();

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
    } else {
		BBLSGraph::solveThread(ROOT);
	}

    MPI_Finalize();

	return 0;
}
