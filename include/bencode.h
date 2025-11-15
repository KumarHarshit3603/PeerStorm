#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstddef>

// Forward declare BValue
class BValue;

// --- Bencode Containers ---
using BList = std::vector<BValue>;
using BDict = std::map<std::string, BValue>;

enum class BType {
    INT,
    STRING,
    LIST,
    DICT,
    NONE
};

class BValue {
public:
    BType type = BType::NONE;

    // NOTE: these fields exist in your current code but we will not rely on them
    // for computing the info-hash. The parser will use findInfoValueRange()
    // to get the exact raw slice.
    size_t raw_start = 0;
    size_t raw_end   = 0;

    long long int_val = 0;
    std::string str_val;
    BList list_val;
    BDict dict_val;

    // ----- Constructors -----
    BValue() = default;
    BValue(long long v) : type(BType::INT), int_val(v) {}
    BValue(const std::string& s) : type(BType::STRING), str_val(s) {}
    BValue(const char* s) : type(BType::STRING), str_val(s) {}
    BValue(const BList& lst) : type(BType::LIST), list_val(lst) {}
    BValue(const BDict& d) : type(BType::DICT), dict_val(d) {}

    // ----- Type Checkers -----
    bool isInt() const { return type == BType::INT; }
    bool isString() const { return type == BType::STRING; }
    bool isList() const { return type == BType::LIST; }
    bool isDict() const { return type == BType::DICT; }

    // ----- Accessors -----
    long long asInt() const {
        if (!isInt()) throw std::runtime_error("BValue is not an integer");
        return int_val;
    }

    const std::string& asString() const {
        if (!isString()) throw std::runtime_error("BValue is not a string");
        return str_val;
    }

    const BList& asList() const {
        if (!isList()) throw std::runtime_error("BValue is not a list");
        return list_val;
    }

    const BDict& asDict() const {
        if (!isDict()) throw std::runtime_error("BValue is not a dictionary");
        return dict_val;
    }
};

// ----- Decoder API -----
BValue decodeValue(const std::string& data, size_t& pos);
long long decodeInt(const std::string& data, size_t& pos);
std::string decodeString(const std::string& data, size_t& pos);
BList decodeList(const std::string& data, size_t& pos);
BDict decodeDict(const std::string& data, size_t& pos);
std::string bencode_value(const BValue &v);
void skipBencodeValue(const std::string& data, size_t& pos);

// Find the raw byte-range (start, end) of the bencoded "info" value inside a
// torrent file represented by `data`. Returns pair(start,end) where `start`
// points to the first byte of the info value (usually the 'd' if it's a dict)
// and `end` points just after the final 'e' that terminates the value.
//
// Throws runtime_error on malformed bencode or if not found.
std::pair<size_t, size_t> findInfoValueRange(const std::string& data);
