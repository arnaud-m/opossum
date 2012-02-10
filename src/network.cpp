/**
 *  Copyright (c) 2011, Arnaud Malapert and Mohamed Rezgui
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of Arnaud Malapert nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "./network.hpp"
#include <queue>

FacilityNode::~FacilityNode() {
	delete type;
	delete father;
	for_each(children.begin(), children.end(), Delete());
}


unsigned int FacilityNode::getMinIncomingConnections(ServerTypeList* servers) {
	//FIXME Invalid computation of min incoming connections
	const unsigned int capa = type->getConnexionCapacity(servers);
	unsigned int res = type->getTotalDemand() >  capa ? type->getTotalDemand() -  capa : 0 ;
	for ( size_t i = 0; i < children.size(); ++i ) {
		res+= children[i]->getDestination()->getMinIncomingConnections(servers);
	}
	return res;
}


ostream & FacilityNode::toDotty(ostream & out)
{
	out << getID();
	out << "[shape=record, label=\"{{" <<  getID() << "}|{" << getType()->getTotalDemand() << "|" << getType()->getTotalCapacity() << "}}\"];" << endl;
	if(! isRoot()) {
		toFather()->toDotty(out);
	}
	for ( size_t i = 0; i < children.size(); ++i ) {
		children[i]->getDestination()->toDotty(out);
	}
	return out;
}

ostream & FacilityNode::toGEXF(ostream & out)
{
	return out;
}



FacilityNode *FacilityNode::getFather() const
{
	return father->getOrigin();
}

inline FacilityNode *FacilityNode::getChild(unsigned int i) const
{
	return children[i]->getDestination();
}

bool FacilityNode::isReliableFromRoot()
{
	return isRoot() ? true : toFather()->isReliable() && getFather()->isReliableFromRoot();
}

void FacilityNode::levelCounts(IntList& levels)
{
	levels[type->getLevel()] = levels[type->getLevel()] + 1;
	for ( size_t i = 0; i < children.size(); ++i ) {
		children[i]->getDestination()->levelCounts(levels);
	}
}

void FacilityNode::print(ostream& out) {
	string shift = string(2 * type->getLevel(),' ');
	string sep = string(15,'-');
	if(children.size() > 0) {
		out << shift << children.size() << " " << sep << endl;
		for ( size_t i = 0; i < children.size(); ++i ) {
			out << shift <<  *children[i] << endl;
		}
		for ( size_t i = 0; i < children.size(); ++i ) {
			children[i]->getDestination()->print(out);
		}
	}
}

bool FacilityType::genRandomReliability()
{
	return randd() < reliabilityProbability;
}


unsigned int FacilityType::genRandomBandwidthIndex()
{
	double cum = 0;
	const double p = randd();
	for (unsigned int i = 0; i < bandwidthProbabilities.size(); ++i) {
		cum+= bandwidthProbabilities[i];
		if(p <= cum) {return i;}
	}
	assert(cum == 1);
	exit (1);
}

unsigned int FacilityType::genRandomFacilities()
{
	//cout << "B(" << binornd->distribution().t() <<  ", " << binornd->distribution().p() << ")" <<endl;
	return (*binornd)();
}


unsigned int FacilityType::genRandomBandwidthIndex(unsigned int maxIndex) {
	double tot = 0;
	//compute total probability
	for (unsigned int i = 0; i <= maxIndex; ++i) {
		tot += bandwidthProbabilities[i];
	}
	//compute random values using normalized probabilities
	double cum = 0;
	const double p = randd();
	for (unsigned int i = 0; i <= maxIndex; ++i) {
		cum+= bandwidthProbabilities[i]/tot;
		if(p <= cum) { return i;}
	}
	assert(cum == 1);
	exit (1);
}

#ifdef NDEBUG
mt19937 FacilityType::random_generator(static_cast<unsigned int>(std::time(0)));
#else
mt19937 FacilityType::random_generator;
#endif

uniform_01< mt19937&, double > FacilityType::randd(FacilityType::random_generator);
variate_generator<mt19937&, binomial_distribution<> > FacilityType::fake_binornd(random_generator, binomial_distribution<>(1,1));

istream & FacilityType::read(istream & in, const PSLProblem& problem)
{
	int tmp;
	in >> level;
	unsigned int n = problem.getNbGroups();
	while(demands.size() < n) {
		in >> tmp;
		demands.push_back(tmp);
	}
	n = problem.getNbServers();
	while(serverCapacities.size() < n) {
		in >> tmp;
		serverCapacities.push_back(tmp);
	}
	int t;
	in >> t;
	double p;
	in >> p;
	delete binornd;
	binornd = new variate_generator<mt19937&, binomial_distribution<> >(random_generator, binomial_distribution<>(t,p));
	n = problem.getNbBandwidths();
	double d;
	while(bandwidthProbabilities.size() < n) {
		in >> d;
		bandwidthProbabilities.push_back(d);
	}
	in >> reliabilityProbability;
	return in;
}

unsigned int FacilityType::getConnexionCapacity(const ServerTypeList* servers) const
{
	unsigned int tot = 0;
	for (unsigned int i = 0; i < servers->size(); ++i) {
		tot+= serverCapacities[i] *  (*servers)[i]->getMaxConnections();
	}
	return tot;
}

ostream& operator<<(ostream& out, const FacilityType& f) {

	out << "Level:" << f.getLevel() << "\tDemand:{ ";
	copy(f.demands.begin(), f.demands.end(), ostream_iterator<int>(out, " "));
	out << "}\tCapacities:{ ";
	copy(f.serverCapacities.begin(), f.serverCapacities.end(), ostream_iterator<int>(out, " "));
	out << "}" << endl << "Probabilities -> Facilities:B(" << f.binornd->distribution().t() << "," << f.binornd->distribution().p() <<")\tBandwidths:{ ";
	copy(f.bandwidthProbabilities.begin(), f.bandwidthProbabilities.end(), ostream_iterator<double>(out, " "));
	out << "}\tReliability:" << f.reliabilityProbability;
	return out;

}

ostream& operator<<(ostream& out, const ServerType& f) {
	return out << "Capacity:" << f.getMaxConnections() << "\tCost:" << f.getCost();
}


istream & operator >>(istream & in, ServerType & s)
{
	in >> s.capacity >> s.cost;
	return in;
}


ostream & operator <<(ostream & out, const PSLProblem & f)
{
	cout << "Bandwidths: {";
	copy(f.bandwidths.begin(), f.bandwidths.end(), ostream_iterator<int>(out, " "));
	cout << "}" << endl ;
	transform(f.servers.begin(), f.servers.end(), ostream_iterator<ServerType>(out, "\n"), dereference<ServerType>);
	transform(f.facilities.begin(), f.facilities.end(), ostream_iterator<FacilityType>(out, "\n"), dereference<FacilityType>);
	if(f.root) f.root->print(out);
	return out;
}


FacilityNode *PSLProblem::generateNetwork() {
	return generateNetwork(true);
}

FacilityNode *PSLProblem::generateNetwork(bool hierarchic)
{
	levelNodeCounts.clear();
	levelNodeCounts.push_back(0);
	nodeCount = 0;
	queue<FacilityNode*> queue;
	root = new FacilityNode(nodeCount++, facilities[0]);
	queue.push(root);
	unsigned int ftype=1, clevel=0, idx=0;
	FacilityNode* current = queue.front();
	do {
		queue.pop();
		idx=ftype;
		while(idx  < facilities.size() && facilities[idx]->getLevel() == clevel + 1) {
			//number of children
			const unsigned int nbc = facilities[idx]->genRandomFacilities();
			//generate children
			for (unsigned int i = 0; i < nbc; ++i) {
				FacilityNode* child = new FacilityNode(nodeCount, facilities[idx]);
				new NetworkLink(nodeCount - 1, current, child, *this, hierarchic);
				queue.push(child);
				nodeCount++;
			}
			idx++;
		}
		if(queue.empty()) break;
		current = queue.front();
		if(current->getType()->getLevel() == clevel + 1) {
			levelNodeCounts.push_back(queue.size()); //Add number of facilities at level clevel
			clevel=current->getType()->getLevel();
			ftype=idx;
		} else assert(current->getType()->getLevel() == clevel);
	} while(ftype < facilities.size());
	return root;
}

ostream & PSLProblem::toDotty(ostream & out)
{
	if(root) {
		out << "digraph G {\n";
		root->toDotty(out);
		out << "}\n";
	}
	return out;
}


unsigned int PSLProblem::generateBreadthFirstNumberedTree(bool hierarchic)
{
	IntList levelNodeCounts;
	levelNodeCounts.push_back(0);
	nodeCount = 0;
	queue<FacilityNode*> queue;
	root = new FacilityNode(nodeCount++, facilities[0]);
	queue.push(root);
	unsigned int ftype=1, clevel=0, idx=0;
	FacilityNode* current = queue.front();
	do {
		queue.pop();
		idx=ftype;
		while(idx  < facilities.size() && facilities[idx]->getLevel() == clevel + 1) {
			//number of children
			const unsigned int nbc = facilities[idx]->genRandomFacilities();
			//generate children
			for (unsigned int i = 0; i < nbc; ++i) {
				FacilityNode* child = new FacilityNode(nodeCount, facilities[idx]);
				new NetworkLink(nodeCount - 1, current, child, *this, hierarchic);
				queue.push(child);
				nodeCount++;
			}
			idx++;
		}
		if(queue.empty()) break;
		current = queue.front();
		if(current->getType()->getLevel() == clevel + 1) {
			levelNodeCounts.push_back(queue.size()); //Add number of facilities at level clevel
			clevel=current->getType()->getLevel();
			ftype=idx;
		} else assert(current->getType()->getLevel() == clevel);
	} while(ftype < facilities.size());
	//copy(levelCounts.begin(), levelCounts.end(), ostream_iterator<unsigned int>(cout, " "));
	//cout << endl;

	return 0;
}


void PSLProblem::generateSubtree(FacilityNode *current, bool hierarchic)
{
	unsigned int idx = 0;
	const unsigned int level = current->getType()->getLevel();
	const unsigned int n = facilities.size();
	while(idx  < n && facilities[idx]->getLevel() <= level) {idx++;}
	while(idx  < n && facilities[idx]->getLevel() == level + 1) {
		const unsigned int nbc = facilities[idx]->genRandomFacilities();
		for (unsigned int i = 0; i < nbc; ++i) {
			FacilityNode* child = new FacilityNode(nodeCount, facilities[idx]);
			new NetworkLink(nodeCount - 1, current, child, *this, hierarchic);
			nodeCount++;
			generateSubtree(child, hierarchic);
		}
		idx++;
	}
}

istream & operator >>(istream & in, PSLProblem & problem)
{
	int n;
	in >> n;
	for (int i = 0; i < n; ++i) {
		int bandwidth;
		in >> bandwidth;
		problem.bandwidths.push_back( bandwidth);
	}
	in >> n;
	for (int i = 0; i < n; ++i) {
		ServerType* stype = new ServerType();
		in >> *stype;
		problem.servers.push_back(stype);
	}
	in >> problem.numberOfGroups;
	in >> n;
	for (int i = 0; i < n; ++i) {
		FacilityType* ftype = new FacilityType();
		ftype->read(in, problem);
		problem.facilities.push_back(ftype);
	}
	return in;
}

ostream & operator <<(ostream& out, const NetworkLink& l)
{
	return out << *l.getOrigin() << " -> " << *l.getDestination() <<" (" << l.getBandwidth() << ", " << l.isReliable() <<")";

}

ostream & operator <<(ostream& out, const FacilityNode& n)
{
	return out << n.getID(); // << " " << n.getChildrenCount();
}


NetworkLink::NetworkLink(unsigned int id, FacilityNode* father, FacilityNode* child, PSLProblem& problem, bool hierarchic) : id(id), origin(father), destination(child), bandwidth(0),reliable(0)
{
	father->children.push_back(this);
	child->father=this;
	if( father->isRoot() || !hierarchic) {
		bandwidth = problem.getBandwidth(child->getType()->genRandomBandwidthIndex());
		reliable = child->getType()->genRandomReliability();
	} else {
		unsigned int fbandw = father->toFather()->getBandwidth();
		int maxIndex = problem.getNbBandwidths() - 1;
		while( maxIndex >= 0 && problem.getBandwidth(maxIndex) > fbandw) {maxIndex--;}
		bandwidth = problem.getBandwidth(child->getType()->genRandomBandwidthIndex(maxIndex));
		reliable = father->toFather()->isReliable() && child->getType()->genRandomReliability();
	}
}

void NetworkLink::forEachPath() const
{
	FacilityNode* ancestor = getDestination();
	FacilityList successors;
	do {
		ancestor= ancestor->getFather();
		successors.push_back(getDestination());
		while( ! successors.empty() ) {
			FacilityNode* successor= successors.back();
			successors.pop_back();
			cout << *ancestor << " -> " << *successor<< endl;
			if(! successor->isLeaf()) {
				for (unsigned int i = 0; i < successor->getChildrenCount(); ++i) {
					successors.push_back(successor->getChild(i));
				}
			}
		}
	} while(! ancestor->isRoot());

}

ostream & NetworkLink::toDotty(ostream & out)
{
	out << origin->getID() << " -> " << destination->getID();
	out << "[label=\"" << getBandwidth() << "\"";
	if(isReliable()) {
		out << ", style=bold ";
	}
	out << "];" <<endl;
	return out;
}




void NetworkLink::forEachPath(void(*ptr)(FacilityNode *n1, FacilityNode *n2)) const
{
	FacilityNode* ancestor = getDestination();
	FacilityList successors;
	do {
		ancestor= ancestor->getFather();
		successors.push_back(getDestination());
		while( ! successors.empty() ) {
			FacilityNode* successor= successors.back();
			successors.pop_back();
			(*ptr)(ancestor, successor);
			if(! successor->isLeaf()) {
				for (unsigned int i = 0; i < successor->getChildrenCount(); ++i) {
					successors.push_back(successor->getChild(i));
				}
			}
		}
	} while(! ancestor->isRoot());
}

ostream & NetworkLink::toGEXF(ostream & out)
{
	return out;
}

int RankMapper::rankX(FacilityNode *node)
{
	return node->getID();
}


inline int RankMapper::offsetXk()
{
	return nodeCount;
}

inline int RankMapper::rankX(FacilityNode *node, unsigned int stype)
{
	return offsetXk() + node->getID() * serverCount + stype;
}


inline int RankMapper::offsetYi()
{
	return offsetXk() + nodeCount * serverCount;
}

int RankMapper::rankY(FacilityNode *node, unsigned int stage)
{
	return offsetYi() + node->getID() * stageCount + stage;
}

inline int RankMapper::offsetYij()
{
	return offsetYi() + nodeCount * stageCount;
}

inline int RankMapper::rankY(NetworkLink *link, unsigned int stage)
{
	return offsetYij() + link->getID() * stageCount + stage;

}

inline int RankMapper::rank(FacilityNode* source, FacilityNode* destination) {
	//TODO not yet implemented
	return 0;

}

inline int RankMapper::rank(FacilityNode* source, FacilityNode* destination, unsigned int stage) {

	return rank(source, destination) * stageCount + stage;

}

inline int RankMapper::offsetZ()
{
	return offsetYij() + linkCount() * stageCount;
}

inline int RankMapper::rankZ(FacilityNode *source, FacilityNode *destination, unsigned int stage)
{
	return offsetZ() + rank(source, destination, stage);
}


inline int RankMapper::offsetB()
{
	return offsetZ() + pathCount() * stageCount;
}


inline int RankMapper::rankB(FacilityNode *source, FacilityNode *destination, unsigned int stage)
{
	return offsetB() + rank(source, destination, stage);
}




























