#pragma once

#include <boost/test/unit_test.hpp>

#include "graph_construction.hpp"
#include "io/rc_reader_wrapper.hpp"
#include "test_utils.hpp"
#include "simple_tools.hpp"
#include "seq_map.hpp"
#include <tr1/unordered_set>

namespace debruijn_graph {

BOOST_AUTO_TEST_SUITE(basic_debruijn_graph_tests)

using io::SingleRead;
using io::PairedRead;

BOOST_AUTO_TEST_CASE( EmptyGraphTest ) {
	Graph g(11);
	BOOST_CHECK_EQUAL(11, g.k());
	BOOST_CHECK_EQUAL(0u, g.size());
}

BOOST_AUTO_TEST_CASE( OneVertexGraphTest ) {
	Graph g(11);
	g.AddVertex();
	BOOST_CHECK_EQUAL(2u, g.size());
	VertexId v = *(g.begin());
	VertexId rcv = g.conjugate(v);
	BOOST_CHECK(v != rcv);
	BOOST_CHECK_EQUAL(v, g.conjugate(rcv));
}

pair<vector<VertexId> , vector<EdgeId> > createGraph(Graph &graph,
		int edgeNumber) {
	vector<VertexId> v;
	vector<EdgeId> e;
	v.push_back(graph.AddVertex());
	for (int i = 0; i < edgeNumber; i++) {
		v.push_back(graph.AddVertex());
		e.push_back(
				graph.AddEdge(v[v.size() - 2], v[v.size() - 1],
						Sequence("AAAAAAAAAAAAAAAAA")));
	}
	return make_pair(v, e);
}

BOOST_AUTO_TEST_CASE( OneEdgeGraphTest ) {
	Graph g(11);
	pair<vector<VertexId> , vector<EdgeId> > data = createGraph(g, 1);
	BOOST_CHECK_EQUAL(1u, g.OutgoingEdgeCount(data.first[0]));
	BOOST_CHECK_EQUAL(0u, g.OutgoingEdgeCount(data.first[1]));
	BOOST_CHECK_EQUAL(data.second[0], g.GetUniqueOutgoingEdge(data.first[0]));
	BOOST_CHECK_EQUAL(g.conjugate(data.second[0]),
			g.GetUniqueOutgoingEdge(g.conjugate(data.first[1])));
	BOOST_CHECK_EQUAL(data.second[0],
			g.conjugate(g.conjugate(data.second[0])));
	BOOST_CHECK_EQUAL(!(g.EdgeNucls(data.second[0])),
			g.EdgeNucls(g.conjugate(data.second[0])));
}

/*void EdgeMethodsSimpleTest() {
	Graph g(11);
	pair<vector<VertexId> , vector<EdgeId> > data = createGraph(g, 2);
//	ASSERT_EQUAL(data.second[0], &g.GetData(data.second[0]));
	ASSERT_EQUAL(
			true,
			g.AreLinkable(data.first[0], data.first[1],
					Sequence("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")));
	ASSERT_EQUAL(
			false,
			g.AreLinkable(data.first[0], data.first[1],
					Sequence("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC")));
}*/

BOOST_AUTO_TEST_CASE( VertexMethodsSimpleTest ) {
	Graph g(11);
	pair<vector<VertexId> , vector<EdgeId> > data = createGraph(g, 2);
	BOOST_CHECK_EQUAL(data.second[0], g.GetUniqueIncomingEdge(data.first[1]));
	BOOST_CHECK_EQUAL(data.second[0], g.GetUniqueOutgoingEdge(data.first[0]));
	BOOST_CHECK_EQUAL(false, g.CanCompressVertex(data.first[0]));
	BOOST_CHECK_EQUAL(true, g.CanCompressVertex(data.first[1]));
	BOOST_CHECK_EQUAL(false, g.CheckUniqueIncomingEdge(data.first[0]));
	BOOST_CHECK_EQUAL(true, g.CheckUniqueIncomingEdge(data.first[1]));
	BOOST_CHECK_EQUAL(false, g.CheckUniqueOutgoingEdge(data.first[2]));
	BOOST_CHECK_EQUAL(true, g.CheckUniqueOutgoingEdge(data.first[1]));
	BOOST_CHECK_EQUAL(true, g.IsDeadEnd(data.first[2]));
	BOOST_CHECK_EQUAL(false, g.IsDeadEnd(data.first[1]));
	BOOST_CHECK_EQUAL(true, g.IsDeadStart(data.first[0]));
	BOOST_CHECK_EQUAL(false, g.IsDeadStart(data.first[1]));
}

//void GraphMethodsSimpleTest() {
//	EdgeGraph g(11);
//	pair<vector<VertexId> , vector<EdgeId> > data = createGraph(g, 2);
//	ASSERT_EQUAL(vector<ActionHandler*> (), g.GetHandlers());
//	ActionHandler* handler = new ActionHandler();
//	g.AddActionHandler(handler);
//	vector<ActionHandler*> handlers = g.GetHandlers();
//	ASSERT_EQUAL(1u, handlers.size());
//	ASSERT_EQUAL(handler, handlers[0]);
//	g.RemoveActionHandler(handler);
//	ASSERT_EQUAL(vector<ActionHandler*> (), g.GetHandlers());
//}

BOOST_AUTO_TEST_CASE( SmartIteratorTest ) {
	Graph g(11);
	pair<vector<VertexId> , vector<EdgeId> > data = createGraph(g, 4);
	size_t num = 0;
	set<VertexId> visited;
	std::less<VertexId> comp;
	auto it = g.SmartVertexBegin(comp);
	for (auto it = g.SmartVertexBegin(comp); !it.IsEnd(); ++it) {
		num++;
		DEBUG( "with seq in vert" << g.VertexNucls(*it).str());
		visited.insert(*it);
	}
	BOOST_CHECK_EQUAL(num, data.first.size() * 2);
	for (size_t i = 0; i < data.first.size(); i++) {
		BOOST_CHECK(visited.find(data.first[i]) != visited.end());
		BOOST_CHECK(visited.find(g.conjugate(data.first[i])) != visited.end());
	}
}

typedef string MyRead;
typedef pair<MyRead, MyRead> MyPairedRead;
typedef string MyEdge;
typedef pair<MyEdge, MyEdge> MyEdgePair;
typedef multimap<MyEdgePair, pair<int, double>> EdgePairInfo;
typedef map<MyEdge, double> CoverageInfo;
typedef tr1::unordered_set<MyEdge> Edges;

string print(const Edges& es) {
	string s = "Edge set : {";
	for (auto i = es.begin(); i != es.end(); ++i) {
		s += "'" + *i + "'; ";
	}
	return s + "}";
}

const Edges AddComplement(const Edges& edges) {
	Edges ans;
	for (auto it = edges.begin(); it != edges.end(); ++it) {
		ans.insert(*it);
		ans.insert(ReverseComplement(*it));
	}
	return ans;
}

const CoverageInfo AddComplement(const CoverageInfo& coverage_info) {
	CoverageInfo ans;
	for (auto it = coverage_info.begin(); it != coverage_info.end(); ++it) {
		ans.insert(*it);
		ans.insert(make_pair(ReverseComplement((*it).first), (*it).second));
	}
	return ans;
}

const EdgePairInfo AddBackward(const EdgePairInfo& pair_info) {
	EdgePairInfo ans;
	for (auto it = pair_info.begin(); it != pair_info.end(); ++it) {
		ans.insert(*it);
		ans.insert(make_pair(make_pair((*it).first.second, (*it).first.first), make_pair(-(*it).second.first, (*it).second.second)));
	}
	return ans;
}

const EdgePairInfo AddComplement(const EdgePairInfo& pair_info) {
	EdgePairInfo ans;
	for (auto it = pair_info.begin(); it != pair_info.end(); ++it) {
		ans.insert(*it);
		ans.insert(make_pair(make_pair(ReverseComplement((*it).first.second), ReverseComplement((*it).first.first)), (*it).second));
	}
	return ans;
}

void EdgesEqual(const Edges& s1, const Edges& s2) {
	BOOST_CHECK_EQUAL(s2.size(), s1.size());
	for (auto it = s1.begin(); it != s1.end(); ++it) {
		BOOST_CHECK(s2.count(*it) > 0);
	}
}

const io::SingleRead MakeRead(const MyRead& read) {
	//todo fill with good quality
	std::string qual;
	qual.resize(read.size());
	return io::SingleRead("", read, qual);
}

const vector<io::SingleRead> MakeReads(const vector<MyRead>& reads) {
	vector<io::SingleRead> ans;
	for (size_t i = 0; i < reads.size(); ++i) {
		ans.push_back(MakeRead(reads[i]));
	}
	return ans;
}

const vector<PairedRead> MakePairedReads(const vector<MyPairedRead>& paired_reads, size_t insert_size) {
	vector<PairedRead> ans;
	for (size_t i = 0; i < paired_reads.size(); ++i) {
		ans.push_back(PairedRead(MakeRead(paired_reads[i].first), MakeRead(paired_reads[i].second), insert_size));
	}
	return ans;
}

void AssertEdges(Graph& g, const Edges& etalon_edges) {
	Edges edges;
	for (auto it = g.SmartEdgeBegin(); !it.IsEnd(); ++it) {
		edges.insert(g.EdgeNucls(*it).str());
	}
	EdgesEqual(edges, etalon_edges);
}

template<size_t kmer_size_>
void AssertGraph(const vector<string>& reads, const vector<string>& etalon_edges) {
	typedef VectorStream<SingleRead> RawStream;
	typedef io::RCReaderWrapper<SingleRead> Stream;
	RawStream raw_stream(MakeReads(reads));
	Stream read_stream(raw_stream);
	Graph g(kmer_size_);
	EdgeIndex<kmer_size_ + 1, Graph> index(g);

	ConstructGraph<kmer_size_, Stream>(g, index, read_stream);

	AssertEdges(g, AddComplement(Edges(etalon_edges.begin(), etalon_edges.end())));
}

bool EqualDouble(double d1, double d2) {
	return std::abs(d1 - d2) < 1e-5;
}

void AssertCoverage(Graph& g, const CoverageInfo& etalon_coverage) {
	for (auto it = g.SmartEdgeBegin(); !it.IsEnd(); ++it) {
		auto etalon_cov_it = etalon_coverage.find(g.EdgeNucls(*it).str());
		BOOST_CHECK_MESSAGE(etalon_cov_it != etalon_coverage.end(), "Etalon didn't contain edge '" << g.EdgeNucls(*it) << "'");
		BOOST_CHECK_MESSAGE(EqualDouble(g.coverage(*it), (*etalon_cov_it).second),
				"Coverage for edge '" << g.EdgeNucls(*it) << "' was " << g.coverage(*it) << " but etalon is " << (*etalon_cov_it).second);
	}
}

typedef PairedInfoIndex<Graph> PairedIndex;
typedef PairedIndex::PairInfos PairInfos;
//typedef PairedIndex::InnerPairInfo PairInfo;

void AssertPairInfo(const Graph& g, /*todo const */PairedIndex& paired_index, const EdgePairInfo& etalon_pair_info) {
	for (auto it = paired_index.begin(); it != paired_index.end(); ++it) {
		PairInfos infos = *it;
		for (auto info_it = infos.begin(); info_it != infos.end(); ++info_it) {
			PairInfo<EdgeId> pair_info = *info_it;
			if (pair_info.first == pair_info.second && rounded_d(pair_info) == 0) {
				continue;
			}
			pair<MyEdge, MyEdge> my_edge_pair(g.EdgeNucls(pair_info.first).str(), g.EdgeNucls(pair_info.second).str());
			auto equal_range = etalon_pair_info.equal_range(my_edge_pair);

			string my_edge_pair_str = "[" + my_edge_pair.first + ", " + my_edge_pair.second + "]";
			BOOST_CHECK_MESSAGE(equal_range.first != equal_range.second,
					"Pair of edges " << my_edge_pair_str << " wasn't found in etalon");

			double etalon_weight = -1.0;

			for (auto range_it = equal_range.first; range_it != equal_range.second; ++range_it) {
				if ((*range_it).second.first == rounded_d(pair_info)) {
					etalon_weight = (*range_it).second.second;
				}
			}
			BOOST_CHECK_MESSAGE(etalon_weight > 0,
					"Etalon didn't contain distance=" << rounded_d(pair_info) << " for edge pair " << my_edge_pair_str);
			BOOST_CHECK_MESSAGE(EqualDouble(etalon_weight, pair_info.weight),
					"Actual weight for edge pair " << my_edge_pair_str << " on distance " << rounded_d(pair_info) << " was " << pair_info.weight << " but etalon is " <<  etalon_weight);
		}
	}
}

template<size_t k>
void AssertGraph(const vector<MyPairedRead>& paired_reads, size_t insert_size, const vector<MyEdge>& etalon_edges
		, const CoverageInfo& etalon_coverage, const EdgePairInfo& etalon_pair_info) {
	typedef VectorStream<PairedRead> RawStream;
	typedef io::RCReaderWrapper<PairedRead> Stream;

	RawStream raw_stream(MakePairedReads(paired_reads, insert_size));
	Stream paired_read_stream(raw_stream);

	graph_pack<Graph, k> gp((Sequence()));
	PairedInfoIndex<Graph> paired_index(gp.g);

	ConstructGraphWithPairedInfo<k>(gp, paired_index, paired_read_stream);

	AssertEdges(gp.g, AddComplement(Edges(etalon_edges.begin(), etalon_edges.end())));

	AssertCoverage(gp.g, AddComplement(etalon_coverage));

	AssertPairInfo(gp.g, paired_index, AddComplement(AddBackward(etalon_pair_info)));
}

//todo rename tests

BOOST_AUTO_TEST_CASE( TestSimpleThread ) {
	vector<string> reads = { "ACAAACCACCA" };
//	vector<string> edges = { "ACAAACCACCA" };
	AssertGraph<5> (reads, reads);
}

BOOST_AUTO_TEST_CASE( TestSimpleThread2 ) {
	vector<string> reads = { "ACAAACCACCC", "AAACCACCCAC" };
	vector<string> edges = { "ACAAACCACCCAC" };
	AssertGraph<5> (reads, edges);
}

BOOST_AUTO_TEST_CASE( TestSplitThread ) {
	vector<string> reads = { "ACAAACCACCA", "ACAAACAACCC" };
	vector<string> edges = { "ACAAAC", "CAAACCACCA", "CAAACAACCC" };
	AssertGraph<5> (reads, edges);
}

BOOST_AUTO_TEST_CASE( TestSplitThread2 ) {
	vector<string> reads = { "ACAAACCACCA", "ACAAACAACCA" };
	vector<string> edges = { "AACCACCA", "ACAAAC", "CAAACCA", "CAAACAACCA" };
	AssertGraph<5> (reads, edges);
}

BOOST_AUTO_TEST_CASE( TestBuldge ) {
	vector<string> reads = { "ACAAAACACCA", "ACAAACCACCA" };
//	vector<string> edges = { "ACAAAACACCA", "ACAAACCACCA" };
	AssertGraph<5> (reads, reads);
}

BOOST_AUTO_TEST_CASE( TestCondenseSimple ) {
	vector<string> reads = { "CGAAACCAC", "CGAAAACAC", "AACCACACC", "AAACACACC" };
	vector<string> edges = { "CGAAAACACAC", "CACACC", "CGAAACCACAC" };
	AssertGraph<5> (reads, edges);
}

//BOOST_AUTO_TEST_CASE( TestStrange ) {
//	vector<string> reads = {"TTCTGCATGGTTATGCATAACCATGCAGAA", "ACACACACTGGGGGTCCCTTTTGGGGGGGGTTTTTTTTG"};
//	typedef VectorStream<SingleRead> RawStream;
//	typedef io::RCReaderWrapper<SingleRead> Stream;
//	RawStream raw_stream(MakeReads(reads));
//	Stream read_stream(raw_stream);
//	Graph g(27);
//	EdgeIndex<28, Graph> index(g);
//
//	ConstructGraph<27, Stream>(g, index, read_stream);
//	EdgeId e = index.get(Seq<28>("TTCTGCATGGTTATGCATAACCATGCAG")).first;
//	VertexId start = g.EdgeEnd(e);
//	vector<EdgeId> edgeIds[2];
//	edgeIds[0] = g.OutgoingEdges(start);
//	edgeIds[1] = g.IncomingEdges(start);
//	for(int ii = 0; ii < 2; ii++)
//		for(auto e_iter = edgeIds[ii].begin(), end_iter = edgeIds[ii].end(); e_iter != end_iter; e_iter++) {
//			g.DeleteEdge(*e_iter);
//		}
//	g.DeleteVertex(start);
//
////		g.DeleteEdge(e);
////
////
////
////	g.DeleteEdge(r_e);
////	g.DeleteVertex(start);
//
//	INFO("FINISH");
//
////	AssertEdges(g, AddComplement(Edges(etalon_edges.begin(), etalon_edges.end())));
//
//}

BOOST_AUTO_TEST_CASE( SimpleTestEarlyPairedInfo ) {
	vector<MyPairedRead> paired_reads = {{"CCCAC", "CCACG"}, {"ACCAC", "CCACA"}};
	vector<MyEdge> edges = {"CCCA", "ACCA", "CCAC", "CACG", "CACA"};
	CoverageInfo coverage_info = {{"CCCA", 1}, {"ACCA", 1}, {"CCAC", 4}, {"CACG", 1}, {"CACA", 1}};
	EdgePairInfo edge_pair_info = {{{"CCCA", "CACG"}, {2, 1.0}}, {{"ACCA", "CACA"}, {2, 1.0}}
		, {{"CCCA", "CCAC"}, {1, 1.0}}, {{"ACCA", "CCAC"}, {1, 1.0}}
		, {{"CCAC", "CACG"}, {1, 1.0}}, {{"CCAC", "CACA"}, {1, 1.0}}};

	AssertGraph<3> (paired_reads, 6, edges, coverage_info, edge_pair_info);
}

BOOST_AUTO_TEST_CASE( TestSelfRCEdgeMerge ) {
	Graph g(5);
	VertexId v1 = g.AddVertex();
	VertexId v2 = g.AddVertex();
	EdgeId edge1 = g.AddEdge(v1, v2, Sequence("AACGCTATT"));
	EdgeId edge2 = g.AddEdge(v2, g.conjugate(v2), Sequence("CTATTCACGTGAATAG"));
	vector<EdgeId> path = {edge1, edge2};
	g.MergePath(path);
	BOOST_CHECK_EQUAL(2u, g.size());
	BOOST_CHECK_EQUAL(1u, g.OutgoingEdgeCount(v1));
	BOOST_CHECK_EQUAL(Sequence("AACGCTATTCACGTGAATAGCGTT"), g.EdgeNucls(g.GetUniqueOutgoingEdge(v1)));
}

BOOST_AUTO_TEST_SUITE_END()

}
