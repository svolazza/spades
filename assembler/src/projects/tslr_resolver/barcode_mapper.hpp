#pragma once

#include <memory>
#include <utility>
#include <fstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include "io/reads/paired_readers.hpp"
#include <common/assembly_graph/paths/mapping_path.hpp>
#include <cassert>
#include "common/modules/alignment/edge_index.hpp"
#include "common/modules/alignment/kmer_mapper.hpp"
#include "common/modules/alignment/sequence_mapper.hpp"
#include "common/pipeline/config_struct.hpp"

using std::string;
using std::istringstream;

namespace tslr_resolver {
    //constexpr int16_t max_barcodes = 384;

    typedef debruijn_graph::ConjugateDeBruijnGraph Graph;
    typedef debruijn_graph::EdgeIndex<Graph> Index;
    typedef Graph::EdgeId EdgeId;
    typedef Graph::VertexId VertexId;
    typedef omnigraph::IterationHelper <Graph, EdgeId> edge_it_helper;
    typedef debruijn_graph::KmerMapper<Graph> KmerSubs;
    typedef string BarcodeId;

    enum BarcodeLibraryType {
        TSLR,
        TenX,
        Unknown
    };

    inline BarcodeLibraryType GetLibType(const string type) {
        if (type == "tslr")
            return BarcodeLibraryType::TSLR;
        if (type == "tenx")
            return BarcodeLibraryType::TenX;
        return BarcodeLibraryType::Unknown;
    }

    struct tslr_barcode_library {
        string left_;
        string right_;
        string barcode_;
    };


    class BarcodeEncoder {
        std::unordered_map <BarcodeId, int64_t> codes_;
        int64_t barcode_encoder_size;
    public:
        BarcodeEncoder() :
                codes_(), barcode_encoder_size (0)
        { }

        void AddBarcode(const string &barcode) {
            auto it = codes_.find(barcode);
            if (it == codes_.end()) {
                codes_[barcode] = barcode_encoder_size;
                barcode_encoder_size++;
            }
            //VERIFY(barcode_encoder_size < max_barcodes);
        }

        int64_t GetCode (const string& barcode) const {
            VERIFY(codes_.find(barcode) != codes_.end());
            return codes_.at(barcode);
        }

        int64_t GetSize() const {
            return barcode_encoder_size;
        }
    };

    template <class barcode_entry_t>
    class HeadTailMapperBuilder;


    /*This structure contains barcodes extracted from reads aligned to the
     * beginning of every edge in assembly graph.
     */
    class BarcodeMapper {
    public:
    protected:
        const Graph& g_;
        bool is_empty_;
    public:
        BarcodeMapper (const Graph &g) :
                g_(g), is_empty_(true) {}
        virtual ~BarcodeMapper() {}

        //Number of entries in the barcode map. Currently equals to number of edges.
        virtual size_t size() const = 0;


        //Get number of shared barcodes between two edges.
        virtual size_t GetIntersectionSize(const EdgeId &edge1, const EdgeId &edge2) const = 0;
        virtual double GetIntersectionSizeNormalizedByUnion(const EdgeId &edge1, const EdgeId &edge2) const = 0;
        virtual double GetIntersectionSizeNormalizedBySecond(const EdgeId &edge1, const EdgeId &edge2) const = 0;
        virtual double GetIntersectionSizeNormalizedByFirst(const EdgeId &edge1, const EdgeId &edge2) const = 0;
        virtual size_t GetUnionSize(const EdgeId &edge1, const EdgeId &edge2) const = 0;

        //Average barcode coverage of long edges
        virtual double AverageBarcodeCoverage () const = 0;

        //Number of barcodes on the beginning/end of the edge
        virtual size_t GetSizeHeads(const EdgeId& edge) const = 0;
        virtual size_t GetSizeTails(const EdgeId& edge) const = 0;

        //fixme these methods should be moved to DataScanner
        virtual void ReadEntry(ifstream& fin, const EdgeId& edge) = 0;
        virtual void WriteEntry(ofstream& fin, const EdgeId& edge) = 0;

        //Remove low abundant barcodes
        virtual void FilterByAbundance(size_t threshold) = 0;

        //Serialize barcode abundancies. Format:
        //abundancy: number of edges.
        virtual void SerializeOverallDistribution(const string& path) const = 0;
        virtual bool IsEmpty() = 0;

    };

    template <class barcode_entry_t>
    class HeadTailBarcodeMapper : public BarcodeMapper {
    friend class HeadTailMapperBuilder<barcode_entry_t>;
    protected:
        typedef std::unordered_map <EdgeId, barcode_entry_t> barcode_map_t;
        using BarcodeMapper::g_;
        using BarcodeMapper::is_empty_;
        barcode_map_t edge_to_distribution_;

    public:
        HeadTailBarcodeMapper (const Graph &g) :
                BarcodeMapper(g),
                edge_to_distribution_()
        {
            InitialFillMap();
        }

        HeadTailBarcodeMapper (const HeadTailBarcodeMapper& other) = default;

        virtual ~HeadTailBarcodeMapper() {}

        void InitialFillMap() {
            edge_it_helper helper(g_);
            for (auto it = helper.begin(); it != helper.end(); ++it) {
                barcode_entry_t set(*it);
                edge_to_distribution_.insert({*it, set});
            }
        }

        size_t size() const {
            return edge_to_distribution_.size();
        }

        typename barcode_map_t::const_iterator cbegin() const noexcept {
            return edge_to_distribution_.cbegin();
        }

        typename barcode_map_t::const_iterator cend() const noexcept {
            return edge_to_distribution_.cend();
        }


        virtual double GetIntersectionSizeNormalizedByUnion(const EdgeId &edge1, const EdgeId &edge2) const override {
            if (GetUnionSize(edge1, edge2)) {
                return static_cast <double> (GetIntersectionSize(edge1, edge2)) /
                       static_cast <double> (GetUnionSize(edge1, edge2));
            }
            return 0;
        }

        virtual double GetIntersectionSizeNormalizedBySecond(const EdgeId &edge1, const EdgeId &edge2) const override {
            if (GetSizeHeads(edge2) > 0) {
                return static_cast <double> (GetIntersectionSize(edge1, edge2)) /
                       static_cast <double> (GetSizeHeads(edge2));
            }
            return 0;
        }

        virtual double GetIntersectionSizeNormalizedByFirst(const EdgeId &edge1, const EdgeId &edge2) const override {
            if (GetSizeTails(edge1) > 0) {
                return static_cast <double> (GetIntersectionSize(edge1, edge2)) /
                       static_cast <double> (GetSizeTails(edge1));
            }
            return 0;
        }


        size_t GetSizeHeads(const EdgeId& edge) const override {
            return GetEntryHeads(edge).Size();
        }

        size_t GetSizeTails(const EdgeId& edge) const override {
            return GetEntryTails(edge).Size();
        }

        bool IsEmpty() override {
            return is_empty_;
        }

        double AverageBarcodeCoverage() const override {
            edge_it_helper helper(g_);
            int64_t barcodes_overall_heads = 0;
            int64_t barcodes_overall_tails = 0;
            int64_t long_edges = 0;
            size_t len_threshold = cfg::get().ts_res.len_threshold;
            for (auto it = helper.begin(); it != helper.end(); ++it) {
                if (g_.length(*it) > len_threshold) {
                    long_edges++;
                    barcodes_overall_heads += GetSizeHeads(*it);
                    barcodes_overall_tails += GetSizeTails(*it);
                }
            }
            DEBUG("heads: " + std::to_string(barcodes_overall_heads));
            DEBUG("tails: " + std::to_string(barcodes_overall_tails));
            DEBUG("Long edges" + long_edges);
            return static_cast <double> (barcodes_overall_heads) / static_cast <double> (long_edges);
        }

        size_t GetIntersectionSize(const EdgeId &edge1, const EdgeId &edge2) const override {
            return GetEntryTails(edge1).GetIntersectionSize(GetEntryHeads(edge2));
        }

        size_t GetUnionSize(const EdgeId &edge1, const EdgeId &edge2) const override {
            return GetEntryHeads(edge1).GetUnionSize(edge2);
        }


        //Delete low abundant barcodes from every edge
        void FilterByAbundance(size_t trimming_threshold) override {
            for (auto entry = edge_to_distribution_.begin(); entry != edge_to_distribution_.end(); ++entry) {
                entry->second.Filter(trimming_threshold);
            }
        }

        void SerializeOverallDistribution(const string& path) const override {
            ofstream fout;
            fout.open(path);
            std::map <size_t, size_t> overall_distr;
            INFO("Serializing distribution")
            for (auto entry: edge_to_distribution_) {
                auto current_distr = edge_to_distribution_.at(entry.first);
                for (auto it = current_distr.cbegin();
                     it != current_distr.cend() ; ++it) {
                    overall_distr[it->second]++;
                }
            }
            for (auto entry : overall_distr) {
                fout << entry.first << ": " << entry.second << endl;
            }
        }

        void ReadEntry (ifstream& fin, const EdgeId& edge) override {
            barcode_entry_t entry(edge);
            entry.Deserialize(fin);
            edge_to_distribution_[edge] = entry;
            DEBUG(edge.int_id());
            DEBUG(entry.Size());
        }

        void WriteEntry (ofstream& fout, const EdgeId& edge) override {
            fout << g_.int_id(edge) << std::endl;
            GetEntryHeads(edge).Serialize(fout);
        }

    protected:
        barcode_entry_t GetEntryHeads(const EdgeId &edge) const {
            return edge_to_distribution_.at(edge);
        }

        barcode_entry_t GetEntryTails(const EdgeId &edge) const {
            return edge_to_distribution_.at(g_.conjugate(edge));
        }
    };


    //Contains abundancy for each barcode aligned to given edge
    class SimpleBarcodeEntry {
        friend class HeadTailBarcodeMapper<SimpleBarcodeEntry>;
        friend class HeadTailMapperBuilder<SimpleBarcodeEntry>;

        typedef std::unordered_map <int64_t, size_t> barcode_distribution_t;
        EdgeId edge_;
        barcode_distribution_t barcode_distribution_;

    public:
        SimpleBarcodeEntry():
            edge_(), barcode_distribution_() {};
        SimpleBarcodeEntry(const EdgeId& edge) :
                edge_(edge), barcode_distribution_() {}


        barcode_distribution_t GetDistribution() const {
            return barcode_distribution_;
        }

        size_t GetIntersectionSize(const SimpleBarcodeEntry &other) const {
            size_t result = 0;
            for (auto it = barcode_distribution_.begin(); it != barcode_distribution_.end(); ++it) {
                if (other.GetDistribution().find(it-> first) != other.GetDistribution().end()) {
                    result++;
                }
            }
            return result;
        }

        size_t GetUnionSize(const SimpleBarcodeEntry& other) const {
            auto distr_this = barcode_distribution_;
            auto distr_other = other.GetDistribution();
            return Size() + other.Size() - GetIntersectionSize(other);
        }

        void InsertSet (barcode_distribution_t& set) {
            barcode_distribution_ = set;
        }

        void Filter (size_t trimming_threshold) {
            for (auto it = barcode_distribution_.begin(); it != barcode_distribution_.end() ;) {
                if (it->second < trimming_threshold) {
                    DEBUG("Erased " + it->first + ' ' + std::to_string(it->second));
                    barcode_distribution_.erase(it++);
                }
                else {
                    ++it;
                }
            }
        }

        size_t Size() const {
            return barcode_distribution_.size();
        }

        void Serialize(ofstream& fout) {
            fout << Size() << endl;
            for (auto entry : barcode_distribution_) {
                fout << entry.first << ' ' << entry.second << endl;
            }
        }

        void Deserialize(ifstream& fin) {
            size_t distr_size;
            fin >> distr_size;
            for (size_t i = 0; i < distr_size; ++i) {
                int64_t bid;
                size_t abundance;
                fin >> bid >> abundance;
                InsertBarcode(bid, abundance);
            }
        }

        decltype(barcode_distribution_.cbegin()) cbegin() const {
            return barcode_distribution_.cbegin();
        }

        decltype(barcode_distribution_.cend()) cend() const {
            return barcode_distribution_.cend();
        }

    private:
        void InsertBarcode(int64_t code, size_t abundance = 1) {
            if (barcode_distribution_.find(code) == barcode_distribution_.end()) {
                barcode_distribution_.insert({code, abundance});
            }
            else {
                barcode_distribution_.at(code) += abundance;
            }
        }


    };

    template <class barcode_entry_t>
    class HeadTailMapperBuilder {
        const Graph& g_;
        shared_ptr<HeadTailBarcodeMapper<barcode_entry_t>> mapper_;
        size_t tail_threshold_;
        BarcodeEncoder barcode_codes_;

    public:
        HeadTailMapperBuilder(const Graph& g, size_t tail_threshold) :
                g_(g),
                mapper_(make_shared<HeadTailBarcodeMapper<SimpleBarcodeEntry>>(g)),
                tail_threshold_(tail_threshold),
                barcode_codes_() {}
        ~HeadTailMapperBuilder() {}
        shared_ptr<HeadTailBarcodeMapper<barcode_entry_t>> GetMapper() {
            return mapper_;
        }

        void FillMapTSLR (const Index& index, const KmerSubs& kmer_mapper) {
            //fixme move to command line
            std::string tslr_dataset = cfg::get().ts_res.tslr_barcode_dataset;

            auto lib_vec = GetLibrary(tslr_dataset);
            auto mapper = std::make_shared<debruijn_graph::BasicSequenceMapper<Graph, Index> >
                    (g_, index, kmer_mapper);

            //Process every barcode from truspades dataset
            for (size_t i = 0; i < lib_vec.size(); ++i) {
                std::string barcode = lib_vec[i].barcode_;
                std::shared_ptr<io::ReadStream<io::PairedRead>> paired_stream =
                        make_shared<io::SeparatePairedReadStream> (lib_vec[i].left_, lib_vec[i].right_, 1);
                io::PairedRead read;
                while (!paired_stream->eof()) {
                    *paired_stream >> read;
                    auto path_first = mapper -> MapRead(read.first());
                    auto path_second = mapper -> MapRead(read.second());
                    InsertMappingPath(barcode, path_first);
                    InsertMappingPath(barcode, path_second);
                }
                VERBOSE_POWER_T2(i, 100,
                                 "Processed " << i << " barcodes from " << lib_vec.size() << " (" << i * 100 / lib_vec.size()
                                              << "%)");
                if (lib_vec.size() > 10 && i % (lib_vec.size() / 10 + 1) == 0) {
                    INFO("Processed " << i << " barcodes from " << lib_vec.size() << " (" << i * 100 / lib_vec.size() << "%)");
                }
            }
            SetNonEmpty();
        }

        void FillMapTenx(const Index& index, const KmerSubs& kmer_mapper) {
            INFO("Starting 10X barcode construction")
            std::string tslr_dataset = cfg::get().ts_res.tslr_barcode_dataset;

            std::ifstream fin;
            fin.open(tslr_dataset);
            string left_reads_filename, right_reads_filename;
            fin >> left_reads_filename >> right_reads_filename;
            auto mapper = std::make_shared<debruijn_graph::BasicSequenceMapper<Graph, Index> >
                    (g_, index, kmer_mapper);

            //Process every read from 10X dataset
            std::shared_ptr<io::ReadStream<io::PairedRead>> paired_stream =
                    make_shared<io::SeparatePairedReadStream> (left_reads_filename, right_reads_filename, 1);
            io::PairedRead read;
            size_t counter = 0;
            while (!paired_stream->eof()) {
                *paired_stream >> read;
                auto barcode = GetBarcode(read.first());
                if (barcode != "") {
                    barcode_codes_.AddBarcode(barcode);
                    auto path_first = mapper->MapRead(read.first());
                    auto path_second = mapper->MapRead(read.second());
                    //INFO("Inserting")
                    InsertMappingPath(barcode, path_first);
                    InsertMappingPath(barcode, path_second);
                }
                counter++;
                VERBOSE_POWER_T2(counter, 100, "Processed " << counter << " reads.");
            }
            INFO("FillMap finished")
            //INFO("Number of barcodes: " + std::to_string(barcode_codes_.GetSize()))
            SetNonEmpty();
        }

        void FillMap(BarcodeLibraryType lib_type, const Index& index, const KmerSubs& kmer_mapper) {
            switch(lib_type) {
                case TSLR :
                    FillMapTSLR(index, kmer_mapper);
                    break;
                case TenX :
                    FillMapTenx(index, kmer_mapper);
                    break;
                default:
                    INFO("Unknown library type, failed to fill barcode map!");
                    return;
            }
        }

    protected:

        //Read parser here?
        std::string GetBarcode(const io::SingleRead& read) {
            size_t barcode_len = 16;
            size_t start_pos = read.name().find("BX:Z");
            if (start_pos != string::npos) {
                VERIFY(start_pos + 5 + barcode_len <= read.name().length())
                string barcode = read.name().substr(start_pos + 5, barcode_len);
                return barcode;
            }
            return "";
        }


        void InsertBarcode(const BarcodeId& barcode, const EdgeId& edge) {
            int64_t code = barcode_codes_.GetCode(barcode);
            mapper_ -> edge_to_distribution_.at(edge).InsertBarcode(code);
        }

        bool IsAtEdgeTail(const EdgeId &edge, const omnigraph::MappingRange &range) {
            return range.mapped_range.start_pos + tail_threshold_ > g_.length(edge);
        }

        bool IsAtEdgeHead(const omnigraph::MappingRange &range) {
            return range.mapped_range.end_pos < tail_threshold_;
        }

        void InsertMappingPath(const BarcodeId& barcode, const MappingPath<EdgeId>& path) {
            for (size_t j = 0; j < path.size(); j++) {
                if (IsAtEdgeHead(path[j].second))
                    InsertBarcode(barcode, path[j].first);
                if (IsAtEdgeTail(path[j].first, path[j].second))
                    InsertBarcode(barcode, g_.conjugate(path[j].first));
            }
        }

        void SetNonEmpty() {
            mapper_ -> is_empty_ = false;
        }

        std::vector <tslr_barcode_library> GetLibrary(const string& reads_filename) {
            std::vector <tslr_barcode_library> lib_vec;
            std::ifstream fin;
            fin.open(reads_filename);
            string line;
            while (getline(fin, line)) {
                if (!line.empty()) {
                    istringstream tmp_stream(line);
                    tslr_barcode_library lib;
                    tmp_stream >> lib.barcode_;
                    tmp_stream >> lib.left_;
                    tmp_stream >> lib.right_;
                    barcode_codes_.AddBarcode(lib.barcode_);
                    lib_vec.push_back(lib);
                }
            }
            return lib_vec;
        }

    };




} //tslr_resolver

