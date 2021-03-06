/*******************************************************/
/* oPoSSuM solver: graphviz.cpp                        */
/* Implementation of the graphviz interface    		   */
/* (c) Arnaud Malapert I3S (UNS-CNRS) 2012             */
/*******************************************************/

#include "graphviz.hpp"


#define INST "cplexpb"
#define PSERV "sol-pserv"
#define FLOW "sol-flow-"
#define PATH "sol-path-"
#define DOT ".dot"


void inst2dotty(PSLProblem &problem) {
	ofstream myfile;
	myfile.open (INST DOT);
	problem.toDotty(myfile);
	myfile.close();
}

void gtitle(ostream & out, const char* title, unsigned int stage) {
	if(title) {
		//cout << ">>>>>>>>>> " << title << endl;
		out << "label=\"" << title << " - stage " << stage << "\";" << endl;
		out << "labelloc=\"t\";" << endl;
	}
}

void stylePServers(ostream & out, const int servers) {
	switch (servers) {
	case 0:	out << ",style=dashed"; break;
	case 1:break;
	case 2: out << ",style=filled,fillcolor=lightgoldenrod";break;
	case 3: out << ",style=filled,fillcolor=palegreen";break;
	case 4: out << ",style=filled,fillcolor=lightseagreen";break;
	default:
		out << ",style=filled,fillcolor=salmon";break;
		break;
	}
}

bool pserv2dotty(ostream & out,PSLProblem & problem, abstract_solver & solver, FacilityNode* i) {
	bool display = false;
	for(LinkListIterator l = i->cbegin() ; l!=  i->cend() ; l++) {
		display |= pserv2dotty(out, problem, solver, (*l)->getDestination());
	}
	CUDFcoefficient servers = solver.get_solution(problem.rankX(i));
	if(servers > 0) {
		out << i->getID() << "[shape=record, label=\"{{" << i->getID() << "|" << servers << "}|";
		if(problem.serverTypeCount() > 1) {
			out << "{" << solver.get_solution(problem.rankX(i, 0));
			for (int k = 1; k < problem.serverTypeCount(); ++k) {
				out << "|" << solver.get_solution(problem.rankX(i, k));
			}
			out << "}";
		}
		out << "}\"";
		stylePServers(out, servers);
		out << "];" << endl;
		if (! i->isRoot()) {
			i->toFather()->toDotty(out);
		}
	} else if(display) {
		out << i->getID() << "[shape=box];" << endl;
		if (! i->isRoot()) {
			i->toFather()->toDotty(out);
		}
	}




	return servers > 0;
}


void pserv2dotty(PSLProblem &problem, abstract_solver& solver, char* title) {
	ofstream myfile;
	stringstream ss (stringstream::in | stringstream::out);
	ss << PSERV << DOT;
	//cout << "## " << ss.str() << endl;
	myfile.open(ss.str().c_str());
	myfile << "digraph P" << "{" <<endl;
	gtitle(myfile, title, 0);
	pserv2dotty(myfile, problem, solver, problem.getRoot());
	myfile << endl << "}" << endl;
	myfile.close();

}


void node2dotty(ostream & out, FacilityNode* i,PSLProblem & problem, abstract_solver & solver, unsigned int stage) {
	CUDFcoefficient demand = stage == 0 ?
			solver.get_solution(problem.rankX(i)) : i->getType()->getDemand(stage-1);
	CUDFcoefficient servers = solver.get_solution(problem.rankX(i));
	CUDFcoefficient connections = solver.get_solution(problem.rankY(i, stage));
	out << i->getID();
	out << "[shape=record, label=\"{";
	if(showID) {
		out << "{" << i->getID() << "|" << demand <<"}";
	} else {
		out << demand ;
	}
	if(stage> 0 && servers > 0 ) {
		out << "|{"<< servers << "|" << connections << "}";
	}
	out << "}\"";
	stylePServers(out, servers);
	out << "];" << endl;
}




void colorConnection(ostream & out, const int connections) {
	if(connections >= 50) {
		if(connections >= 1000) {
			out << ",color=firebrick";
		} else if(connections >= 500) {
			out << ",color=salmon";
		} else if(connections >= 250) {

			out << ",color=midnightblue";
			//out << ",color=seagreen";
		}else if(connections >= 100) {
			out << ",color=seagreen";
		} else {
			out << ",color=darkgoldenrod";
		}
	}
}

void flow2dotty(ostream & out, PSLProblem & problem, abstract_solver & solver, unsigned int stage)
{
	for(NodeIterator i = problem.nbegin() ; i!=  problem.nend() ; i++) {
		node2dotty(out, *i, problem, solver, stage);
	}
	for(LinkIterator l = problem.lbegin() ; l!=  problem.lend() ; l++) {
		CUDFcoefficient connections = solver.get_solution(problem.rankY(*l, stage));
		out << l->getOrigin()->getID() << " -> " << l->getDestination()->getID();
		if(connections > 0) {
			out << "[label=\"" << connections << "\"";
			colorConnection(out, connections);
			if (l->isReliable()) {
				out << ", style=bold ";
			}
			out <<"];" << endl;
		} else {
			out << "[style=\"invis\"];\n";
		}
	}
}



void flow2dotty(PSLProblem & problem, abstract_solver & solver, char* title)
{
	for (int i = 0; i < problem.stageCount(); ++i) {
		ofstream myfile;
		stringstream ss (stringstream::in | stringstream::out);
		ss << FLOW << i << DOT;
		//cout << "## " << ss.str() << endl;
		myfile.open(ss.str().c_str());
		myfile << "digraph F" << i << "{" <<endl;
		gtitle(myfile, title, i);
		flow2dotty(myfile, problem, solver, i);
		myfile << endl << "}" << endl;
		myfile.close();
	}
}


void colorUnitBandwidth(ostream & out, const double unitBandwidth) {
	if(unitBandwidth >= 10) {
		if(unitBandwidth >= 1000) {
			out << ",color=firebrick";
		} else if(unitBandwidth >= 250) {
			out << ",color=salmon";
		} else if(unitBandwidth >= 100) {

			out << ",color=midnightblue";
			//out << ",color=seagreen";
		}else if(unitBandwidth >= 50) {
			out << ",color=seagreen";
		} else {
			out << ",color=darkgoldenrod";
		}
	}
}

void path2dotty(ostream& out, PSLProblem & problem, abstract_solver & solver, unsigned int stage)
{

	for(NodeIterator i = problem.nbegin() ; i!=  problem.nend() ; i++) {
		node2dotty(out, *i, problem, solver, stage);
		if( ! i->isLeaf()) {
			NodeIterator j = i->nbegin();
			j++;
			while(j !=  i->nend()) {
				CUDFcoefficient connections = solver.get_solution(problem.rankZ(*i,*j, stage));
				if(connections > 0) {
					double bandwidth = solver.get_real_solution(problem.rankB(*i,*j, stage));
					//	cout << ">>>>>" << bandwidth << endl;
					bandwidth/=connections;
					out.precision(1);
					out << i->getID() << " -> " << j->getID();
					out << "[label=\"" << connections <<
							"\\n" << scientific << bandwidth << "\"";
					colorUnitBandwidth(out, bandwidth);
					if (isReliablePath(*i, *j)) {
						out << ", style=bold ";
					}
					out << "];" << endl;
				}
				j++;
			}
		}
	}
	//Add invisible arcs of the tree if needed
	for(LinkIterator l = problem.lbegin() ; l!=  problem.lend() ; l++) {
		CUDFcoefficient connections = solver.get_solution(problem.rankZ(l->getOrigin(), l->getDestination(), stage));
		if(connections == 0) {
			out << l->getOrigin()->getID() << " -> " << l->getDestination()->getID();
			out << "[style=\"invis\"];" <<endl;
		}
	}

}


void path2dotty(PSLProblem & problem, abstract_solver & solver, char* title)
{
	for (int i = 1; i < problem.stageCount(); ++i) {
		ofstream myfile;
		stringstream ss (stringstream::in | stringstream::out);
		ss << PATH << i << DOT;
		myfile.open (ss.str().c_str());
		myfile << "digraph P" << i << "{" <<endl;
		gtitle(myfile, title, i);
		path2dotty(myfile, problem, solver, i);
		myfile << endl << "}" << endl;
		myfile.close();
	}
}




void solution2dotty(PSLProblem &problem, abstract_solver& solver, char* title) {
	pserv2dotty(problem, solver, title);
	flow2dotty(problem, solver, title);
	path2dotty(problem, solver, title);
}





