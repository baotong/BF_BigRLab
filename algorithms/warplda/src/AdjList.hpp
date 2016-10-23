#pragma once

#include <iostream>
#include <sstream>
#include "Utils.hpp"
#include "Types.hpp"
//#include "Partition.h"
#include "NumaArray.hpp"

template <class T>
void readvec(NumaArray<T> &a, std::istream &f, uint64_t idx_beg, uint64_t count)
{
	a.Assign(count);
	f.seekg(idx_beg * sizeof(T), std::ios::beg);
	f.read((char*)&a[0], count * sizeof(T));
}

template <class TSrc_, class TDst_, class TEdge_, class TDegree_>
class AdjList
{
	//template<class T> using NumaArray=NumaArray1<T>;
	public:
		using TSrc = TSrc_;
		using TDst = TDst_;
		using TEdge = TEdge_;
		using TDegree = TDegree_;
	public:
		AdjList(){}

		bool Load(std::string);
		bool Load(std::istream &idxStream, std::istream &lnkStream)
        {
            n_ = filesize(idxStream) / sizeof(TEdge) - 1;

            beg_ = 0; //p_.Startid(0);
            end_ = n_; //p_.Startid(1);
            n_local_ = end_ - beg_;

            readvec(idx_vec_, idxStream, beg_ , end_ - beg_ + 1);

            idx_ = &idx_vec_[0] - beg_;

            readvec(lnk_vec_, lnkStream, idx_[beg_], idx_[end_] - idx_[beg_]);

            lnk_ = &lnk_vec_[0] - idx_[beg_];

            ne_ = filesize(lnkStream) / sizeof(TDst);

            return true;
        }

		TSrc NumVertices() { return n_; }
		TSrc NumVerticesLocal() { return n_local_; }
		TEdge NumEdges() { return ne_; }
		TDegree Degree(TSrc id) { return idx_[id + 1] - idx_[id]; }
		TEdge Idx(TSrc id) { return idx_[id]; }
		const TDst* Edges(TSrc id) { return &lnk_[idx_[id]]; }
		TSrc Begin() { return beg_; }
		TSrc End() { return end_; }

		template <class Function>
			void Visit(Function f)
			{
				for (TSrc id = beg_; id < end_; id++)
				{
					f(id, Degree(id), lnk_ + idx_[id]);
				}
			}

		TEdge * idx_;
		TDst * lnk_;
	private:
		//Partition p_;
		TSrc n_;
		TSrc n_local_;
		TEdge ne_;
		TSrc beg_;
		TSrc end_;
		NumaArray<TEdge> idx_vec_;
		NumaArray<TDst> lnk_vec_;
};
