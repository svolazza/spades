/*
 * debruijn.hpp
 *
 *  Created on: 25.02.2011
 *      Author: vyahhi
 */

#ifndef DEBRUIJN_HPP_
#define DEBRUIJN_HPP_

#include "seq.hpp"
#include <google/sparse_hash_map> // ./configure, make and sudo make install from libs/sparsehash-1.10
#include <iostream> // for debug
#include <map>
#include <tr1/unordered_map>

template <int size_>
class DeBruijn {
private:
	class data {
	public:
		data() {
			std::fill(out_edges, out_edges + 4, 0);
			std::fill(in_edges, in_edges + 4, 0);
		}
	public: // make private
		int out_edges[4];
		int in_edges[4];
	};
	typedef Seq<size_> key;
	typedef data value;
	//typedef google::sparse_hash_map<key, value,	typename key::hash, typename key::equal_to> hash_map;
	typedef std::map<key, value, typename key::less> hash_map;
	//typedef std::tr1::unordered_map<key, value,	typename key::hash, typename key::equal_to> hash_map;
	hash_map _nodes;
public:
	data& addNode(const Seq<size_> &seq) {
		std::pair<const key, value> p = make_pair(seq, data());
		std::pair<typename hash_map::iterator, bool> node = _nodes.insert(p);
		return node.first->second; // return node's data
	}
	void addEdge(const Seq<size_> &from, const Seq<size_> &to) {
		data &d_from = addNode(from);
		data &d_to = addNode(to);
		d_from.out_edges[(size_t)to[size_-1]]++;
		d_to.in_edges[(size_t)from[0]]++;
	}
	size_t size() const {
		return _nodes.size();
	}
};

#endif /* DEBRUIJN_HPP_ */
