#include "graphio.hpp"
#include "graphSimplification.hpp"
#include "graphVisualizer.hpp"
#include <stdio.h>
#include "common.hpp"
#include "pairedGraph.hpp"
#include "iostream"
#include "fstream"

using namespace paired_assembler;

inline int codeNucleotide(char a) {
	if (a == 'A')
		return 0;
	else if (a == 'C')
		return 1;
	else if (a == 'G')
		return 2;
	else if (a == 'T')
		return 3;
	else {
		std::cerr << "oops!";
		return -1;
	}
}

void codeRead(char *read, char *code) {
	for (int i = 0; i < readLength; i++) {
		code[i] = codeNucleotide(read[i]);
	}
}

ll extractMer(char *read, int shift, int length) {
	ll res = 0;
	for (int i = 0; i < length; i++) {
		res = res << 2;
		res += read[shift + i];
	}
	return res;
}

string decompress(ll a, int l) {

	string res = "";
	res.reserve(l);
	forn(i,l)
		res += " ";
	forn(i, l) {
		res[l - i - 1] = nucl((a & 3));
		a >>= 2;
	}
	return res;
}

void outputLongEdges(longEdgesMap &longEdges, ostream &os) {
	gvis::GraphPrinter<int> g("Paired_ext", os);
	char Buffer[100];
	for (longEdgesMap::iterator it = longEdges.begin(); it != longEdges.end(); ++it) {
		if (it->second->EdgeId == it->first) {
			sprintf(Buffer, "%i (%i) cov %i", it->first, it->second->length, it->second->coverage);
			//		else sprintf(Buffer,"%i (%i) FAKE now it is %d",it->first, it->second->length,it->second->EdgeId);

			g.addEdge(it->second->FromVertex, it->second->ToVertex, Buffer);
			cerr << it->first << " (" << it->second->length << ") cov "<< it->second->coverage<<":" << endl;
			if (it->second->length < 500) {
				cerr << it->second->upper->str() << endl;
				cerr << it->second->lower->str() << endl;
			}
		}
	}
	g.output();
}

void outputLongEdges(longEdgesMap &longEdges) {
	outputLongEdges(longEdges, cout);
}

void outputLongEdges(longEdgesMap &longEdges, string fileName) {
	ofstream s;
	s.open(fileName.c_str());
	cerr << "Open file " << fileName << endl;
	outputLongEdges(longEdges, s);
	s.close();
}

void outputLongEdges(longEdgesMap &longEdges, PairedGraph &graph, ostream &os) {
	gvis::GraphPrinter<int> g("Paired_ext", os);
	char Buffer[100];
	bool UsedV[20000];
	forn(i,20000)
		UsedV[i] = false;
	pair<int, int> vDist;
	for (longEdgesMap::iterator it = longEdges.begin(); it != longEdges.end(); ++it) {
		if (it->second->EdgeId == it->first) {
			if (!UsedV[it->second->FromVertex]) {
				vDist = vertexDist(longEdges, graph, it->second->FromVertex);
				sprintf(Buffer, "Vertex_%i (%i, %i)", it->second->FromVertex,
						vDist.first, vDist.second);
				g.addVertex(it->second->FromVertex, Buffer);
				UsedV[it->second->FromVertex] = true;
			}
			if (!UsedV[it->second->ToVertex]) {
				vDist = vertexDist(longEdges, graph, it->second->ToVertex);
				sprintf(Buffer, "Vertex_%i (%i, %i)", it->second->ToVertex,
						vDist.first, vDist.second);
				g.addVertex(it->second->ToVertex, Buffer);
				UsedV[it->second->ToVertex] = true;
			}

			sprintf(Buffer, "%i (%i) cov %i", it->first, it->second->length, it->second->coverage);
			//		else sprintf(Buffer,"%i (%i) FAKE now it is %d",it->first, it->second->length,it->second->EdgeId);

			g.addEdge(it->second->FromVertex, it->second->ToVertex, Buffer);
			cerr << it->first << " (" << it->second->length << ") cov "<< it->second->coverage<<":" << endl;
			if (it->second->length < 500) {
				cerr << it->second->upper->str() << endl;
				cerr << it->second->lower->str() << endl;
			}
		}
	}
	g.output();
}

void outputLongEdges(longEdgesMap &longEdges, PairedGraph &graph) {
	outputLongEdges(longEdges, graph, cout);
}

void outputLongEdges(longEdgesMap &longEdges, PairedGraph &graph,
		string fileName) {
	ofstream s;
	s.open(fileName.c_str());
	outputLongEdges(longEdges, graph, s);
	s.close();
}

Sequence readGenome(istream &is) {
	SequenceBuilder sb;
	string buffer;
	while(!is.eof()){
		is >> buffer;
		sb.append(Sequence(buffer));
	}
	return sb.BuildSequence();
}

Sequence readGenomeFromFile(const string &fileName) {
	ifstream is;
	is.open(fileName.c_str());
	Sequence result(readGenome(is));
	is.close();
	return result;
}

int findStartVertex(PairedGraph &graph) {
	int result = -1;
	for (int i = 0; i < graph.VertexCount; i++) {
		if (graph.degrees[i][0] == 0 && graph.degrees[i][1] == 1) {
			if (result >= 0) {
				cerr << "Ambigious start point for threading!" << endl;
				return result;
			}
			result = i;
		}
	}
	return result;
}

bool checkEdge(Edge *nextEdge, int genPos, Sequence &genome) {
	for (size_t i = 0; i < nextEdge->upper->size(); i++)
		if (nextEdge->upper->operator [](i) != genome[genPos + i]
				|| nextEdge->lower->operator [](i) != genome[genPos + i
						+ readLength + insertLength])
			return false;
	return true;
}

string createEdgeLabel(int edgeNum, int edgeId, int length) {
	stringstream ss;
	ss << edgeNum << ":" << edgeId << " (" << length << ")";
	return ss.str();
}

int moveThroughEdge(gvis::GraphPrinter<int> &g, PairedGraph &graph,
		Edge *nextEdge, int edgeNum, int genPos) {
	cerr << "Edge found" << endl;
	edgeNum++;
	string label(createEdgeLabel(edgeNum, nextEdge->EdgeId, nextEdge->length));
	g.addEdge(nextEdge->FromVertex, nextEdge->ToVertex, label);
	cerr << nextEdge->EdgeId << " (" << nextEdge->length << "):" << endl;
	if (graph.longEdges[nextEdge->EdgeId]->length < 500) {
		cerr << nextEdge->upper->str() << endl;
		cerr << nextEdge->lower->str() << endl;
	}
	genPos += nextEdge->length;
	return nextEdge->ToVertex;
}

Edge *chooseNextEdge(int currentVertex, int genPos, Sequence &genome,
		PairedGraph &graph) {
	for (int v = 0; v < graph.degrees[currentVertex][1]; v++) {
		int notRealId = graph.edgeIds[currentVertex][v][OUT_EDGE];
		int edgeId = edgeRealId(notRealId, graph.longEdges);
		Edge *nextEdge = graph.longEdges[edgeId];
		cerr << "possible edge" << edgeId << endl;
		if (checkEdge(nextEdge, genPos, genome)) {
			return nextEdge;
		}
	}
	return NULL;
}

void outputLongEdgesThroughGenome(PairedGraph &graph, ostream &os) {
	assert(k==l);
	cerr << "Graph output through genome" << endl;
	gvis::GraphPrinter<int> g("Paired_ext", os);
	Sequence genome(readGenomeFromFile("data/input/MG1655-K12_cut.fasta"));
	cerr << "Try to process" << endl;
	int edgeNum = 0;
	int genPos = 0;
	int currentVertex = findStartVertex(graph);
	cerr << "Start vertex " << currentVertex << endl;
	while (graph.degrees[currentVertex][1] != 0) {
		cerr << "Try to found next edge" << endl;
		Edge *nextEdge = chooseNextEdge(currentVertex, genPos, genome, graph);
		if (nextEdge != NULL) {
			currentVertex
					= moveThroughEdge(g, graph, nextEdge, edgeNum, genPos);
			edgeNum++;
			genPos += nextEdge->length;
		} else {
			cerr << "BAD GRAPH. I can not cover all genome" << endl;
			break;
		}
	}
	g.output();
}

void outputLongEdgesThroughGenome(PairedGraph &graph) {
	outputLongEdgesThroughGenome(graph, cout);
}

void outputLongEdgesThroughGenome(PairedGraph &graph, string fileName) {
	ofstream s;
	s.open(fileName.c_str());
	outputLongEdgesThroughGenome(graph, s);
	s.close();
}

DataReader::DataReader(char *fileName) {
	f_ = fopen(fileName, "r");
	assert(f_ != NULL);
}

DataPrinter::DataPrinter(char *fileName) {
	f_ = fopen(fileName, "w");
	assert(f_ != NULL);
}

void DataReader::close() {
	fclose(f_);
}

void DataPrinter::close() {
	fclose(f_);
}

void DataReader::read(int &a) {
	assert(fscanf(f_, "%d\n", &a)==1);
}

void DataPrinter::output(int a) {
	fprintf(f_, "%d\n", a);
}

void DataReader::read(long long &a) {
	fscanf(f_, "%lld\n", &a);
}

void DataPrinter::output(long long a) {
	fprintf(f_, "%lld\n", a);
}

void DataReader::read(Sequence * &sequence) {
	int length;
	read(length);
	if (length == 0) {
		fscanf(f_, "\n");
		sequence = new Sequence("");
	} else {
		char *s = new char[length + 1];
		fscanf(f_, "%s\n", s);
		sequence = new Sequence(s);
	}
}

void DataPrinter::output(Sequence *sequence) {
	output((int) sequence->size());
	fprintf(f_, "%s\n", sequence->str().c_str());
}

void DataReader::read(VertexPrototype * &v) {
	int id;
	Sequence *lower;
	bool b;
	read(id);
	read(lower);
	int tmpInt;
	read(tmpInt);
	b = tmpInt;
	v = new VertexPrototype(lower, id);
	v->used = b;
}

void DataPrinter::output(VertexPrototype *v) {
	output(v->VertexId);
	output(v->lower);
	output(v->used);
}

void DataReader::read(Edge * &edge) {
	int from, to, len, id, cov;
	Sequence *up, *low;
	read(id);
	read(from);
	read(to);
	read(len);
	read(cov);
	read(up);
	read(low);
	edge = new Edge(up, low, from, to, len, id, cov);
}

void DataPrinter::output(Edge *edge) {
	output(edge->EdgeId);
	output(edge->FromVertex);
	output(edge->ToVertex);
	output(edge->length);
	output(edge->coverage);
	output(edge->upper);
	output(edge->lower);
}

void DataPrinter::outputLongEdgesMap(longEdgesMap &edges) {
	output((int) edges.size());
	for (longEdgesMap::iterator it = edges.begin(); it != edges.end(); ++it) {
		if (it->first == it->second->EdgeId) {
			output(it->first);
			output(it->second);
		}
	}
	Sequence *emptySequence = new Sequence("");
	Edge *emptyEdge = new Edge(emptySequence, emptySequence, 0, 0, 0, 0);
	for (longEdgesMap::iterator it = edges.begin(); it != edges.end(); ++it) {
		if (it->first != it->second->EdgeId) {
			output(it->first);
			emptyEdge->EdgeId = it->second->EdgeId;
			output(emptyEdge);
		}
	}
	delete emptyEdge;
}

void DataReader::readLongEdgesMap(longEdgesMap &edges) {
	int size;
	read(size);
	for (int i = 0; i < size; i++) {
		int id;
		read(id);
		Edge *edge;
		read(edge);
		if (id == edge->EdgeId) {
			edges.insert(make_pair(id, edge));
		} else {
			edges.insert(make_pair(id, edges[edge->EdgeId]));
			delete edge;
		}
	}
}

void DataReader::readIntArray(int *array, int length) {
	for (int i = 0; i < length; i++) {
		fscanf(f_, "%d ", array + i);
	}
	fscanf(f_, "\n");
}

void DataPrinter::outputIntArray(int *array, int length) {
	for (int i = 0; i < length; i++) {
		fprintf(f_, "%d ", array[i]);
	}
	fprintf(f_, "\n");
}

void DataReader::readIntArray(int *array, int length, int width) {
	int cur = 0;
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < width; j++) {
			fscanf(f_, "%d ", array + cur);
			cur++;
		}
		fscanf(f_, "\n");
	}
	fscanf(f_, "\n");
}

void DataPrinter::outputIntArray(int *array, int length, int width) {
	int cur = 0;
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < width; j++) {
			fprintf(f_, "%d ", array[cur]);
			cur++;
		}
		fprintf(f_, "\n");
	}
	fprintf(f_, "\n");
}

void save(char *fileName, PairedGraph &g, longEdgesMap &longEdges,
		int &VertexCount, int EdgeId) {
	DataPrinter dp(fileName);
	dp.output(VertexCount);
	dp.output(EdgeId);
	dp.outputLongEdgesMap(longEdges);
	//TODO: FIX!!!
	//	dp.outputIntArray(g.inD, MAX_VERT_NUMBER);
	//	dp.outputIntArray(g.outD, MAX_VERT_NUMBER);
	//	dp.outputIntArray((int*) g.outputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	//	dp.outputIntArray((int*) g.inputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	dp.close();
}
void load(char *fileName, PairedGraph &g, longEdgesMap &longEdges,
		int &VertexCount, int &EdgeId) {
	DataReader dr(fileName);
	dr.read(VertexCount);
	dr.read(EdgeId);
	dr.readLongEdgesMap(longEdges);
	//TODO: fix;
	//	dr.readIntArray(g.inD, MAX_VERT_NUMBER);
	//	dr.readIntArray(g.outD, MAX_VERT_NUMBER);
	//	dr.readIntArray((int*) g.outputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	//	dr.readIntArray((int*) g.inputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	dr.close();
}

void save(char *fileName, PairedGraph &g) {
	DataPrinter dp(fileName);
	dp.output(g.VertexCount);
	dp.output(g.EdgeId);
	dp.outputLongEdgesMap(g.longEdges);
	//TODO: FIX!!!
	//	dp.outputIntArray(g.inD, MAX_VERT_NUMBER);
	//	dp.outputIntArray(g.outD, MAX_VERT_NUMBER);
	//	dp.outputIntArray((int*) g.outputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	//	dp.outputIntArray((int*) g.inputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	dp.close();
}
void load(char *fileName, PairedGraph &g) {
	DataReader dr(fileName);
	dr.read(g.VertexCount);
	dr.read(g.EdgeId);
	dr.readLongEdgesMap(g.longEdges);
	//TODO: fix;
	//	dr.readIntArray(g.inD, MAX_VERT_NUMBER);
	//	dr.readIntArray(g.outD, MAX_VERT_NUMBER);
	//	dr.readIntArray((int*) g.outputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	//	dr.readIntArray((int*) g.inputEdges, MAX_VERT_NUMBER, MAX_DEGREE);
	dr.close();
}
