#include "array.h"
#include "DoubleList.h"
#include "stack.h"
#include "value3.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif
using namespace std;

static string join(const vector<string>& a, size_t from = 0) {
    string s;
    for (size_t i = from; i < a.size(); ++i) { if (i > from) s += ' '; s += a[i]; }
    return s;
}
static long long integer(const string& s) {
    size_t n = 0;
    long long x;
    try { x = stoll(s, &n); } catch (...) { throw runtime_error("invalid integer: " + s); }
    if (n != s.size()) throw runtime_error("invalid integer: " + s);
    return x;
}
static vector<string> split(const string& s) {
    istringstream in(s); vector<string> v; string x;
    while (in >> x) v.push_back(x);
    return v;
}

// 1. RPN: the linked Stack from Lab 1 stores intermediate results.
static long long rpn(const string& expression) {
    vector<string> tokens = split(expression);
    bool compact = tokens.size() == 1;
    if (compact) { try { integer(tokens[0]); compact = false; } catch (...) {} }
    if (compact && tokens[0].find_first_of("+-*") != string::npos) {
        tokens.clear();
        for (char c : expression) if (!isspace(static_cast<unsigned char>(c))) tokens.push_back(string(1, c));
    }
    unique_ptr<Stack, decltype(&stackDestroy)> st(stackCreate(), stackDestroy);
    for (const string& t : tokens) {
        if (t == "+" || t == "-" || t == "*") {
            if (!st->top || !st->top->next) throw runtime_error("invalid RPN expression");
            long long b = integer(stackPop(st.get())), a = integer(stackPop(st.get()));
            const auto lo = numeric_limits<long long>::min(), hi = numeric_limits<long long>::max();
            bool overflow = false;
            if (t == "+") overflow = (b > 0 && a > hi-b) || (b < 0 && a < lo-b);
            else if (t == "-") overflow = (b < 0 && a > hi+b) || (b > 0 && a < lo+b);
            else if (a && b) {
                if (a > 0) overflow = b > 0 ? a > hi/b : b < lo/a;
                else overflow = b > 0 ? a < lo/b : a < hi/b;
            }
            if (overflow) throw runtime_error("integer overflow");
            long long value = t == "+" ? a+b : t == "-" ? a-b : a*b;
            stackPush(st.get(), to_string(value));
        } else stackPush(st.get(), to_string(integer(t)));
    }
    if (!st->top || st->top->next) throw runtime_error("invalid RPN expression");
    return integer(stackGet(st.get()));
}

struct Entry { string key, value; };
class HashTable {
    bool cuckoo;
    vector<vector<Entry>> chains;
    vector<optional<Entry>> first, second;
    size_t count = 0;
    uint64_t seed = 0;
    static uint64_t hash(const string& s, uint64_t salt) {
        uint64_t h = 1469598103934665603ULL ^ salt;
        for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
        h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33;
        return h;
    }
    size_t index(const string& key, int table) const {
        size_t cap = cuckoo ? first.size() : chains.size();
        return hash(key, seed + (table ? 0x9e3779b97f4a7c15ULL : 0)) % cap;
    }
    optional<Entry>* slot(const string& key, int table) {
        return &(table ? second : first)[index(key, table)];
    }
    const optional<Entry>* slot(const string& key, int table) const {
        return &(table ? second : first)[index(key, table)];
    }
    void growChains() {
        auto old = std::move(chains); chains.resize(old.size() * 2 + 1); count = 0;
        for (auto& bucket : old) for (auto& e : bucket) put(e.key, e.value);
    }
    optional<Entry> insertRaw(Entry e) {
        int table = 0;
        for (size_t i = 0; i < 4 * first.size(); ++i) {
            auto* p = slot(e.key, table);
            if (!*p) { *p = std::move(e); ++count; return nullopt; }
            swap(e, **p); table ^= 1;
        }
        return e;
    }
    void rebuild(vector<Entry> entries, size_t capacity) {
        for (;;) {
            first.assign(capacity, nullopt); second.assign(capacity, nullopt);
            count = 0; ++seed;
            bool failed = false;
            for (size_t i = 0; i < entries.size(); ++i) {
                auto displaced = insertRaw(entries[i]);
                if (displaced) {
                    vector<Entry> retry(entries.begin() + i + 1, entries.end());
                    retry.push_back(*displaced);
                    for (auto& p : first) if (p) retry.push_back(*p);
                    for (auto& p : second) if (p) retry.push_back(*p);
                    entries = std::move(retry);
                    failed = true; break;
                }
            }
            if (!failed) return;
            capacity = capacity * 2 + 1;
        }
    }
    vector<Entry> entries() const {
        vector<Entry> out;
        if (cuckoo) {
            for (auto& p : first) if (p) out.push_back(*p);
            for (auto& p : second) if (p) out.push_back(*p);
        } else for (auto& bucket : chains) for (auto& e : bucket) out.push_back(e);
        return out;
    }
public:
    explicit HashTable(bool useCuckoo = false) : cuckoo(useCuckoo) {
        if (cuckoo) { first.resize(11); second.resize(11); }
        else chains.resize(11);
    }
    size_t size() const { return count; }
    size_t capacity() const { return cuckoo ? first.size() * 2 : chains.size(); }
    string method() const { return cuckoo ? "cuckoo" : "chain"; }
    bool get(const string& key, string& value) const {
        if (cuckoo) {
            for (int i = 0; i < 2; ++i) { auto p = slot(key, i); if (*p && (*p)->key == key) { value = (*p)->value; return true; } }
        } else {
            for (auto& e : chains[index(key, 0)]) if (e.key == key) { value = e.value; return true; }
        }
        return false;
    }
    bool put(const string& key, const string& value) {
        if (cuckoo) {
            for (int i = 0; i < 2; ++i) { auto p = slot(key, i); if (*p && (*p)->key == key) { (*p)->value = value; return false; } }
            if ((count + 1) * 2 > capacity()) { auto all = entries(); all.push_back({key, value}); rebuild(std::move(all), first.size() * 2 + 1); return true; }
            auto displaced = insertRaw({key, value});
            if (displaced) { auto all = entries(); all.push_back(*displaced); rebuild(std::move(all), first.size() * 2 + 1); }
            return true;
        }
        auto& bucket = chains[index(key, 0)];
        for (auto& e : bucket) if (e.key == key) { e.value = value; return false; }
        if ((count + 1) * 10 > chains.size() * 9) growChains();
        chains[index(key, 0)].push_back({key, value}); ++count; return true;
    }
    bool erase(const string& key) {
        if (cuckoo) {
            for (int i = 0; i < 2; ++i) { auto p = slot(key, i); if (*p && (*p)->key == key) { p->reset(); --count; return true; } }
        } else {
            auto& b = chains[index(key, 0)];
            for (size_t i = 0; i < b.size(); ++i) if (b[i].key == key) { b.erase(b.begin() + i); --count; return true; }
        }
        return false;
    }
    vector<string> keys() const { vector<string> v; for (auto& e : entries()) v.push_back(e.key); sort(v.begin(), v.end()); return v; }
};

// 2. Named persistent sets; in-memory operations use the chained hash table.
static map<string, HashTable> loadSets(const string& path) {
    map<string, HashTable> db; ifstream in(path); if (!in) return db;
    string magic; size_t n; if (!(in >> magic >> n) || magic != "LAB2SETS" || n > 1000000) throw runtime_error("bad set file");
    for (size_t i = 0; i < n; ++i) { string name, value; if (!(in >> name >> value)) throw runtime_error("bad set file"); db.try_emplace(name); db.at(name).put(value, "1"); }
    return db;
}
static void saveSets(const map<string, HashTable>& db, const string& path) {
    ofstream out(path, ios::trunc); if (!out) throw runtime_error("cannot write set file");
    size_t n = 0; for (auto& [_, set] : db) n += set.size();
    out << "LAB2SETS " << n << '\n';
    for (auto& [name, set] : db) for (auto& key : set.keys()) out << name << ' ' << key << '\n';
    if (!out) throw runtime_error("cannot write set file");
}
static string setQuery(const string& path, const string& query) {
    auto w = split(query); if (w.size() != 3) throw runtime_error("query must be SETADD/SETDEL/SET_AT name value");
    auto db = loadSets(path); string result, v;
    if (w[0] == "SETADD") { db.try_emplace(w[1]); result = db.at(w[1]).put(w[2], "1") ? "TRUE" : "FALSE"; saveSets(db, path); }
    else if (w[0] == "SETDEL") { auto it = db.find(w[1]); result = it != db.end() && it->second.erase(w[2]) ? "TRUE" : "FALSE"; if (result == "TRUE") saveSets(db, path); }
    else if (w[0] == "SET_AT") { auto it = db.find(w[1]); result = it != db.end() && it->second.get(w[2], v) ? "TRUE" : "FALSE"; }
    else throw runtime_error("unknown set operation");
    return result;
}

// 3. Equal-sum partition: use the set from task 2 to remove duplicate input values.
static vector<vector<long long>> partition(long long target, const vector<string>& args) {
    if (target <= 0) throw runtime_error("sum must be positive");
    HashTable unique; vector<long long> numbers; long long total = 0;
    for (auto& s : args) { long long v = integer(s); if (v <= 0) throw runtime_error("elements must be natural numbers"); if (unique.put(to_string(v), "1")) { if (v > numeric_limits<long long>::max() - total) throw runtime_error("sum overflow"); numbers.push_back(v); total += v; } }
    if (numbers.empty() || total % target) throw runtime_error("partition impossible");
    size_t groups = total / target; if (groups > numbers.size()) throw runtime_error("partition impossible");
    sort(numbers.rbegin(), numbers.rend()); if (numbers[0] > target) throw runtime_error("partition impossible");
    vector<long long> sums(groups); vector<vector<long long>> result(groups);
    function<bool(size_t)> search = [&](size_t i) {
        if (i == numbers.size()) return all_of(sums.begin(), sums.end(), [&](long long x){ return x == target; });
        set<long long> tried;
        for (size_t j = 0; j < groups; ++j) {
            if (sums[j] + numbers[i] > target || !tried.insert(sums[j]).second) continue;
            bool empty = sums[j] == 0; sums[j] += numbers[i]; result[j].push_back(numbers[i]);
            if (search(i + 1)) return true;
            result[j].pop_back(); sums[j] -= numbers[i];
            if (empty) break;
        }
        return false;
    };
    if (!search(0)) throw runtime_error("partition impossible");
    return result;
}

// 4. The assignment's example enumerates subsets, including the empty one.
static vector<string> subsets(const vector<string>& args) {
    unique_ptr<Array, decltype(&arrayDestroy)> a(arrayCreate(), arrayDestroy);
    auto sorted = args; sort(sorted.begin(), sorted.end());
    for (auto& x : sorted) arrayPush(a.get(), x);
    vector<string> out;
    function<void(int, vector<string>&)> generate = [&](int at, vector<string>& current) {
        out.push_back("{" + join(current) + "}");
        for (int i = at; i < arraySize(a.get()); ++i) {
            if (i > at && arrayGet(a.get(), i) == arrayGet(a.get(), i-1)) continue;
            current.push_back(arrayGet(a.get(), i)); generate(i + 1, current); current.pop_back();
        }
    };
    vector<string> current; generate(0, current); return out;
}

// 5. Reuse Lab 1 tree nodes, but insert without AVL rotations to test the actual BST.
static void bstInsert(AvlTree& tree, int key) {
    AvlNode** p = &tree.root;
    while (*p) { if (key == (*p)->key) return; p = key < (*p)->key ? &(*p)->left : &(*p)->right; }
    *p = new AvlNode(key);
}
static bool bstBalanced(const AvlTree& tree) {
    vector<pair<AvlNode*, bool>> todo; unordered_map<AvlNode*, int> height;
    if (tree.root) todo.push_back({tree.root, false});
    bool okay = true;
    while (!todo.empty()) {
        auto [p, done] = todo.back(); todo.pop_back();
        if (!done) { todo.push_back({p, true}); if (p->right) todo.push_back({p->right, false}); if (p->left) todo.push_back({p->left, false}); }
        else { int l = p->left ? height[p->left] : 0, r = p->right ? height[p->right] : 0; if (abs(l-r) > 1) okay = false; height[p] = 1 + max(l,r); }
    }
    return okay;
}

// 5*. Huffman tree with deterministic ties; the header lets decoding work alone.
using HuffNode = AvlNode;
struct HuffOrder { bool operator()(const HuffNode* a, const HuffNode* b) const { return a->weight != b->weight ? a->weight > b->weight : a->minimum > b->minimum; } };
static HuffNode* huffTree(const vector<uint64_t>& counts) {
    priority_queue<HuffNode*, vector<HuffNode*>, HuffOrder> q;
    for (int i = 0; i < 256; ++i) if (counts[i]) q.push(new HuffNode{counts[i], i, i, nullptr, nullptr});
    while (q.size() > 1) { auto a=q.top();q.pop(); auto b=q.top();q.pop(); q.push(new HuffNode{a->weight+b->weight, -1, min(a->minimum,b->minimum), a,b}); }
    return q.empty() ? nullptr : q.top();
}
static void huffFree(HuffNode* p) { if (!p) return; huffFree(p->left); huffFree(p->right); delete p; }
static void huffCodes(HuffNode* p, const string& prefix, vector<string>& codes) {
    if (p->symbol >= 0) { codes[p->symbol] = prefix.empty() ? "0" : prefix; return; }
    huffCodes(p->left, prefix+"0", codes); huffCodes(p->right, prefix+"1", codes);
}
static string huffEncode(const string& input) {
    vector<uint64_t> counts(256); for (unsigned char c : input) ++counts[c];
    unique_ptr<HuffNode, decltype(&huffFree)> root(huffTree(counts), huffFree);
    vector<string> codes(256); if (root) huffCodes(root.get(), "", codes);
    ostringstream out; bool first = true;
    for (int i = 0; i < 256; ++i) if (counts[i]) { if (!first) out << ','; first = false; out << hex << uppercase << setw(2) << setfill('0') << i << dec << ':' << counts[i]; }
    out << '|'; for (unsigned char c : input) out << codes[c]; return out.str();
}
static string huffDecode(const string& archive) {
    size_t bar = archive.find('|'); if (bar == string::npos) throw runtime_error("invalid Huffman archive");
    vector<uint64_t> counts(256); istringstream header(archive.substr(0,bar)); string part; uint64_t total=0;
    while (getline(header, part, ',')) { size_t colon=part.find(':'); if (colon != 2) throw runtime_error("invalid Huffman header"); int byte; istringstream(part.substr(0,2)) >> hex >> byte; if (byte < 0 || byte > 255 || counts[byte]) throw runtime_error("invalid Huffman header"); uint64_t n=stoull(part.substr(3)); if (!n) throw runtime_error("invalid Huffman header"); counts[byte]=n; total+=n; }
    unique_ptr<HuffNode, decltype(&huffFree)> root(huffTree(counts), huffFree); string bits=archive.substr(bar+1), out;
    if (!root) { if (!bits.empty()) throw runtime_error("invalid Huffman bits"); return out; }
    HuffNode* p=root.get();
    for (char bit : bits) {
        if (bit!='0' && bit!='1') throw runtime_error("invalid Huffman bits");
        if (root->symbol < 0) p = bit=='0' ? p->left : p->right;
        else if (bit!='0') throw runtime_error("invalid Huffman bits");
        if (!p) throw runtime_error("invalid Huffman bits");
        if (p->symbol >= 0) { out += char(p->symbol); p=root.get(); }
    }
    if (p!=root.get() || out.size()!=total) throw runtime_error("incomplete Huffman bits");
    vector<uint64_t> actual(256); for (unsigned char c:out) ++actual[c]; if (actual!=counts) throw runtime_error("Huffman frequency mismatch");
    return out;
}

static vector<string> utf8(const string& s) {
    vector<string> out;
    for (size_t i=0;i<s.size();) { unsigned char c=s[i]; size_t n=c<128?1:c>=0xF0?4:c>=0xE0?3:c>=0xC0?2:0; if (!n || i+n>s.size()) throw runtime_error("invalid UTF-8"); for(size_t j=1;j<n;++j) if ((static_cast<unsigned char>(s[i+j])&0xC0)!=0x80) throw runtime_error("invalid UTF-8"); out.push_back(s.substr(i,n)); i+=n; }
    return out;
}
// 6.2. Bidirectional mapping through our two hash-table implementations.
static bool isomorphic(const string& a, const string& b) {
    auto x=utf8(a), y=utf8(b); if(x.size()!=y.size()) return false;
    HashTable ab(false), ba(true); string v;
    for(size_t i=0;i<x.size();++i) {
        if(ab.get(x[i],v)) { if(v!=y[i]) return false; } else ab.put(x[i],y[i]);
        if(ba.get(y[i],v)) { if(v!=x[i]) return false; } else ba.put(y[i],x[i]);
    }
    return true;
}

// 7. Hash lookup plus Lab 1 doubly linked list. Equal frequencies evict the oldest access.
class LFU {
    struct Item { string key, value; uint64_t frequency, time; bool alive; };
    size_t limit; uint64_t tick=0; HashTable index; vector<Item> items;
    unique_ptr<DoubleList, decltype(&doubleDestroy)> order;
public:
    explicit LFU(size_t n):limit(n),order(doubleCreate(),doubleDestroy) {}
    string get(const string& key) {
        string id; if(!index.get(key,id)) return "-1";
        auto& e=items[stoull(id)]; ++e.frequency; e.time=++tick;
        doubleRemove(order.get(),key); doublePush(order.get(),key); return e.value;
    }
    void set(const string& key,const string& value) {
        if(!limit) return;
        string id;
        if(index.get(key,id)) { auto& e=items[stoull(id)]; e.value=value; ++e.frequency; e.time=++tick; doubleRemove(order.get(),key); doublePush(order.get(),key); return; }
        if(index.size()==limit) {
            string victim; uint64_t freq=numeric_limits<uint64_t>::max();
            for(DoubleNode* p=order->head;p;p=p->next) { string pos; index.get(p->value,pos); auto& e=items[stoull(pos)]; if(e.frequency<freq) { freq=e.frequency; victim=p->value; } }
            index.get(victim,id); size_t reuse=stoull(id); index.erase(victim); doubleRemove(order.get(),victim);
            index.put(key,id); items[reuse]={key,value,1,++tick,true}; doublePush(order.get(),key); return;
        }
        index.put(key,to_string(items.size())); items.push_back({key,value,1,++tick,true}); doublePush(order.get(),key);
    }
};

static void hashSession(bool cuckoo) {
    HashTable table(cuckoo); string line;
    while(getline(cin,line)) {
        auto w=split(line); if(w.empty()) continue;
        if(w[0]=="SET" && w.size()==3) { table.put(w[1],w[2]); cout<<"OK\n"; }
        else if(w[0]=="GET" && w.size()==2) { string v; cout<<(table.get(w[1],v)?v:"-1")<<'\n'; }
        else if(w[0]=="DEL" && w.size()==2) cout<<(table.erase(w[1])?"TRUE":"FALSE")<<'\n';
        else if(w[0]=="STATS" && w.size()==1) cout<<table.method()<<" size="<<table.size()<<" capacity="<<table.capacity()<<'\n';
        else throw runtime_error("hash commands: SET key value, GET key, DEL key, STATS");
    }
}
static void benchmark(size_t n) {
    for(bool cuckoo:{false,true}) {
        HashTable t(cuckoo); auto start=chrono::steady_clock::now();
        for(size_t i=0;i<n;++i) t.put("key"+to_string(i),to_string(i));
        auto inserted=chrono::steady_clock::now(); string value;
        for(size_t i=0;i<n;++i) if(!t.get("key"+to_string(i),value) || value!=to_string(i)) throw runtime_error("benchmark verification failed");
        auto found=chrono::steady_clock::now();
        cout<<t.method()<<" n="<<n<<" insert_us="<<chrono::duration_cast<chrono::microseconds>(inserted-start).count()<<" lookup_us="<<chrono::duration_cast<chrono::microseconds>(found-inserted).count()<<" capacity="<<t.capacity()<<'\n';
    }
}
static void usage() {
    cerr << "lab2 rpn EXPR | lab2 --file FILE --query 'SETADD name value' | lab2 partition SUM NUMBERS... | lab2 subsets VALUES... | lab2 balanced NUMBERS... 0 | lab2 huffman encode TEXT | lab2 huffman decode ARCHIVE | lab2 hash chain|cuckoo | lab2 benchmark N | lab2 isomorphic A B | lab2 lfu CAPACITY (commands on stdin)\n";
}
int main(int argc,char** argv) {
    try {
#ifdef _WIN32
        // Windows narrow argv uses the system code page; normalize arguments to UTF-8.
        int wideCount = 0;
        wchar_t** wide = CommandLineToArgvW(GetCommandLineW(), &wideCount);
        if (!wide) throw runtime_error("cannot read command line");
        vector<string> storage;
        vector<char*> pointers;
        for (int i = 0; i < wideCount; ++i) {
            int n = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
            string value(n, '\0');
            WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, value.data(), n, nullptr, nullptr);
            value.pop_back(); storage.push_back(value);
        }
        LocalFree(wide);
        for (auto& value : storage) pointers.push_back(value.data());
        argc = wideCount; argv = pointers.data();
#endif
        vector<string> a(argv+1,argv+argc); if(a.empty()) {usage();return 1;}
        if(a.size()==4 && a[0]=="--file" && a[2]=="--query") cout<<setQuery(a[1],a[3])<<'\n';
        else if(a[0]=="rpn" && a.size()>1) cout<<rpn(join(a,1))<<'\n';
        else if(a[0]=="partition" && a.size()>2) { auto groups=partition(integer(a[1]),vector<string>(a.begin()+2,a.end())); for(auto& g:groups) { cout<<'{' ; for(size_t i=0;i<g.size();++i) cout<<(i?" ":"")<<g[i]; cout<<"}\n"; } }
        else if(a[0]=="subsets") for(auto& s:subsets(vector<string>(a.begin()+1,a.end()))) cout<<s<<'\n';
        else if(a[0]=="balanced") { unique_ptr<AvlTree,decltype(&avlDestroy)> tree(avlCreate(),avlDestroy); for(size_t i=1;i<a.size();++i) { long long x=integer(a[i]); if(!x) break; if(x<numeric_limits<int>::min()||x>numeric_limits<int>::max()) throw runtime_error("tree key out of range"); bstInsert(*tree,(int)x); } cout<<(bstBalanced(*tree)?"YES":"NO")<<'\n'; }
        else if(a.size()==3 && a[0]=="huffman" && a[1]=="encode") cout<<huffEncode(a[2])<<'\n';
        else if(a.size()==3 && a[0]=="huffman" && a[1]=="decode") cout<<huffDecode(a[2])<<'\n';
        else if(a.size()==2 && a[0]=="hash" && (a[1]=="chain"||a[1]=="cuckoo")) hashSession(a[1]=="cuckoo");
        else if(a.size()==2 && a[0]=="benchmark") { auto n=integer(a[1]); if(n<0) throw runtime_error("N must be non-negative"); benchmark((size_t)n); }
        else if(a.size()==3 && a[0]=="isomorphic") cout<<(isomorphic(a[1],a[2])?"YES":"NO")<<'\n';
        else if(a.size()==2 && a[0]=="lfu") { auto n=integer(a[1]); if(n<0) throw runtime_error("capacity must be non-negative"); LFU cache((size_t)n); string line; while(getline(cin,line)) { auto w=split(line); if(w.empty())continue; if(w[0]=="SET"&&w.size()==3) cache.set(w[1],w[2]); else if(w[0]=="GET"&&w.size()==2) cout<<cache.get(w[1])<<'\n'; else throw runtime_error("LFU commands: SET key value, GET key"); } }
        else { usage(); return 1; }
    } catch(const exception& e) { cerr<<"error: "<<e.what()<<'\n'; return 1; }
}
