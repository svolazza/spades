#pragma once
#include "read.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace corrector {
typedef std::vector<WeightedPositionalRead> WeightedReadStorage;

class InterestingPositionProcessor {
    std::string contig_;
    std::vector<int> is_interesting_;
    std::vector<std::vector<size_t> > read_ids_;
    WeightedReadStorage wr_storage_;
    std::unordered_map<size_t, position_description> interesting_weights;
    std::unordered_map<size_t, position_description> changed_weights_;

//I wonder if anywhere else in spades google style guide convention on consts names is kept
    const int kAnchorGap = 100;
    const int kAnchorNum = 6;
    static const size_t kMaxErrorCount = 6;
    const int error_weight[kMaxErrorCount] = { 100, 10, 8, 5, 2, 1 };

public:
    InterestingPositionProcessor() {
    }
    void set_contig(string &ctg);
    inline int get_error_weight(size_t i) const {
        if (i >= kMaxErrorCount)
            return 0;
        else
            return error_weight[i];
    }
    inline bool IsInteresting(size_t position) const {
        if (position >= contig_.length())
            return 0;
        else
            return is_interesting_[position];
    }

    unordered_map<size_t, position_description> get_weights() const {
        return changed_weights_;
    }
    void UpdateInterestingRead(const PositionDescriptionMap &ps);
    void UpdateInterestingPositions();

    size_t FillInterestingPositions(vector<position_description> &charts);

};
}
;