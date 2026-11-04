#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "array.h"
#include "singleList.h"
#include "DoubleList.h"
#include "stack.h"
#include "quene.h"
#include "value3.h"

using namespace std;

// Хранит одну структуру данных выбранного типа.
struct Data {
    char type;
    Array* array = nullptr;
    SingleList* single = nullptr;
    DoubleList* doubly = nullptr;
    Stack* stack = nullptr;
    Queue* queue = nullptr;
    AvlTree* tree = nullptr;

    explicit Data(char type) : type(type) {
        if (type == 'M') array = arrayCreate();
        if (type == 'F') single = singleCreate();
        if (type == 'L') doubly = doubleCreate();
        if (type == 'S') stack = stackCreate();
        if (type == 'Q') queue = queueCreate();
        if (type == 'T') tree = avlCreate();
    }

    ~Data() {
        if (array != nullptr) arrayDestroy(array);
        if (single != nullptr) singleDestroy(single);
        if (doubly != nullptr) doubleDestroy(doubly);
        if (stack != nullptr) stackDestroy(stack);
        if (queue != nullptr) queueDestroy(queue);
        if (tree != nullptr) avlDestroy(tree);
    }
};

// Имя структуры связано с ее данными.
using Database = map<string, unique_ptr<Data>>;

Data& getData(Database& db, const string& name, char type, bool create = false) {
    auto it = db.find(name);
    if (it == db.end()) {
        if (!create) throw runtime_error("container does not exist");
        it = db.emplace(name, make_unique<Data>(type)).first;
    }
    if (it->second->type != type) throw runtime_error("wrong container type");
    return *it->second;
}

vector<string> values(const Data& d, bool reverse = false) {
    vector<string> result;
    if (d.type == 'M') result = arrayRead(d.array);
    if (d.type == 'F') result = singleRead(d.single, reverse);
    if (d.type == 'L') result = doubleRead(d.doubly, reverse);
    if (d.type == 'S') result = stackRead(d.stack);
    if (d.type == 'Q') result = queueRead(d.queue);
    if (d.type == 'T') {
        for (int key : avlValues(d.tree)) result.push_back(to_string(key));
    }
    if (reverse && d.type != 'F' && d.type != 'L') std::reverse(result.begin(), result.end());
    return result;
}

string show(const Data& d, bool reverse = false) {
    if (d.type == 'T') return avlReadPreOrder(d.tree);
    vector<string> items = values(d, reverse);
    if (items.empty()) return "(empty)";
    ostringstream output;
    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0) output << ' ';
        output << items[i];
    }
    return output.str();
}

int treeNumber(const string& text) {
    size_t end = 0;
    try {
        int value = stoi(text, &end);
        if (end == text.size()) return value;
    } catch (...) {}
    throw runtime_error("tree key must be an integer");
}

// Восстанавливает структуры из файла перед выполнением команды.
Database loadFile(const string& path) {
    Database db;
    if (!filesystem::exists(path)) return db;
    ifstream file(path);
    int count;
    if (!(file >> count) || count < 0 || count > 1000000) throw runtime_error("bad data file");
    for (int i = 0; i < count; i++) {
        char type;
        string name;
        int size;
        if (!(file >> type >> name >> size) || size < 0 || size > 1000000) throw runtime_error("bad data file");
        if (string("MFLSQT").find(type) == string::npos || db.count(name)) throw runtime_error("bad data file");
        Data& d = getData(db, name, type, true);
        vector<string> items;
        for (int j = 0; j < size; j++) {
            string value;
            if (!(file >> value)) throw runtime_error("bad data file");
            items.push_back(value);
        }
        if (type == 'S') {
            for (int j = size - 1; j >= 0; j--) stackPush(d.stack, items[j]);
        } else {
            for (const string& value : items) {
                if (type == 'M') arrayPush(d.array, value);
                if (type == 'F') singlePush(d.single, value);
                if (type == 'L') doublePush(d.doubly, value);
                if (type == 'Q') queuePush(d.queue, value);
                if (type == 'T') avlInsert(d.tree, treeNumber(value));
            }
        }
    }
    return db;
}

// Сохраняет все структуры после команды, изменившей данные.
void saveFile(const Database& db, const string& path) {
    ofstream file(path);
    if (!file) throw runtime_error("cannot write data file");
    file << db.size() << '\n';
    for (const auto& entry : db) {
        vector<string> items = values(*entry.second);
        file << entry.second->type << ' ' << entry.first << ' ' << items.size() << '\n';
        for (const string& value : items) file << value << '\n';
    }
    if (!file) throw runtime_error("cannot write data file");
}

void need(const vector<string>& words, size_t count) {
    if (words.size() != count) throw runtime_error("wrong number of arguments");
}

int number(const string& text) {
    size_t end = 0;
    try {
        int value = stoi(text, &end);
        if (end == text.size() && value >= 0) return value;
    } catch (...) {}
    throw runtime_error("index must be a non-negative integer");
}

// Выполняет команды массива.
string runArray(Database& db, const vector<string>& words, bool& changed) {
    const string& op = words[0];
    Data& d = getData(db, words[1], 'M', op == "MPUSH" || op == "MINSERT");
    if (op == "MPUSH") {
        need(words, 3);
        arrayPush(d.array, words[2]);
        changed = true;
        return words[2];
    }
    if (op == "MINSERT") {
        need(words, 4);
        arrayInsert(d.array, number(words[2]), words[3]);
        changed = true;
        return words[3];
    }
    if (op == "MGET") { need(words, 3); return arrayGet(d.array, number(words[2])); }
    if (op == "MSET") {
        need(words, 4);
        arraySet(d.array, number(words[2]), words[3]);
        changed = true;
        return words[3];
    }
    if (op == "MDEL") {
        need(words, 3);
        string result = arrayDelete(d.array, number(words[2]));
        changed = true;
        return result;
    }
    if (op == "MLEN") { need(words, 2); return to_string(arraySize(d.array)); }
    if (op == "MREAD") { need(words, 2); return show(d); }
    throw runtime_error("unknown command");
}

// Выполняет команды односвязного и двусвязного списков.
string runList(Database& db, const vector<string>& words, bool& changed) {
    const string& op = words[0];
    string action = op.substr(1);
    bool adding = action == "PUSH" || action == "INSERTHEAD" || action == "INSERTTAIL" ||
                  action == "INSERTBEFORE" || action == "INSERTAFTER";
    Data& d = getData(db, words[1], op[0], adding);
    if (action == "PUSH" || action == "INSERTHEAD" || action == "INSERTTAIL") {
        need(words, 3);
        if (d.type == 'F') {
            if (action == "PUSH") singlePush(d.single, words[2]);
            if (action == "INSERTHEAD") singleInsertHead(d.single, words[2]);
            if (action == "INSERTTAIL") singleInsertTail(d.single, words[2]);
        } else {
            if (action == "PUSH") doublePush(d.doubly, words[2]);
            if (action == "INSERTHEAD") doubleInsertHead(d.doubly, words[2]);
            if (action == "INSERTTAIL") doubleInsertTail(d.doubly, words[2]);
        }
        changed = true;
        return words[2];
    }
    if (action == "INSERTBEFORE" || action == "INSERTAFTER") {
        need(words, 4);
        if (d.type == 'F') {
            if (action == "INSERTBEFORE") singleInsertBefore(d.single, words[2], words[3]);
            else singleInsertAfter(d.single, words[2], words[3]);
        } else {
            if (action == "INSERTBEFORE") doubleInsertBefore(d.doubly, words[2], words[3]);
            else doubleInsertAfter(d.doubly, words[2], words[3]);
        }
        changed = true;
        return words[3];
    }
    if (action == "DEL" || action == "DELBEFORE" || action == "DELAFTER") {
        need(words, 3);
        string result;
        if (d.type == 'F') {
            if (action == "DEL") result = singleDelete(d.single, words[2]);
            if (action == "DELBEFORE") result = singleDeleteBefore(d.single, words[2]);
            if (action == "DELAFTER") result = singleDeleteAfter(d.single, words[2]);
        } else {
            if (action == "DEL") result = doubleDelete(d.doubly, words[2]);
            if (action == "DELBEFORE") result = doubleDeleteBefore(d.doubly, words[2]);
            if (action == "DELAFTER") result = doubleDeleteAfter(d.doubly, words[2]);
        }
        changed = true;
        return result;
    }
    if (action == "DELHEAD" || action == "DELTAIL") {
        need(words, 2);
        string result = d.type == 'F'
            ? (action == "DELHEAD" ? singleDeleteHead(d.single) : singleDeleteTail(d.single))
            : (action == "DELHEAD" ? doubleDeleteHead(d.doubly) : doubleDeleteTail(d.doubly));
        changed = true;
        return result;
    }
    if (action == "REMOVE") {
        need(words, 3);
        string result = d.type == 'F' ? singleRemove(d.single, words[2]) : doubleRemove(d.doubly, words[2]);
        changed = true;
        return result;
    }
    if (action == "FIND") {
        need(words, 3);
        return to_string(d.type == 'F' ? singleFind(d.single, words[2]) : doubleFind(d.doubly, words[2]));
    }
    if (action == "GET") {
        need(words, 3);
        return d.type == 'F' ? singleGet(d.single, words[2]) : doubleGet(d.doubly, words[2]);
    }
    if (action == "READ") {
        if (words.size() != 2 && words.size() != 3) throw runtime_error("wrong number of arguments");
        if (words.size() == 3 && words[2] != "FORWARD" && words[2] != "REVERSE") throw runtime_error("use FORWARD or REVERSE");
        return show(d, words.size() == 3 && words[2] == "REVERSE");
    }
    throw runtime_error("unknown command");
}

// Выполняет команды стека и очереди.
string runStackQueue(Database& db, const vector<string>& words, bool& changed) {
    const string& op = words[0];
    string action = op.substr(1);
    Data& d = getData(db, words[1], op[0], action == "PUSH");
    if (action == "PUSH") {
        need(words, 3);
        if (d.type == 'S') stackPush(d.stack, words[2]);
        else queuePush(d.queue, words[2]);
        changed = true;
        return words[2];
    }
    if (action == "POP") {
        need(words, 2);
        string result = d.type == 'S' ? stackPop(d.stack) : queuePop(d.queue);
        changed = true;
        return result;
    }
    if (action == "GET") {
        need(words, 2);
        return d.type == 'S' ? stackGet(d.stack) : queueGet(d.queue);
    }
    if (action == "READ") { need(words, 2); return show(d); }
    throw runtime_error("unknown command");
}

// Выполняет команды АВЛ-дерева.
string runTree(Database& db, const vector<string>& words, bool& changed) {
    const string& op = words[0];
    Data& d = getData(db, words[1], 'T', op == "TINSERT");
    if (op == "TINSERT") {
        need(words, 3);
        int key = treeNumber(words[2]);
        bool exists = avlContains(d.tree, key);
        avlInsert(d.tree, key);
        changed = !exists;
        return words[2];
    }
    if (op == "TDEL") {
        need(words, 3);
        if (!avlDelete(d.tree, treeNumber(words[2]))) throw runtime_error("value not found");
        changed = true;
        return words[2];
    }
    if (op == "TGET") {
        need(words, 3);
        return avlContains(d.tree, treeNumber(words[2])) ? "TRUE" : "FALSE";
    }
    if (op == "TREAD") { need(words, 2); return show(d); }
    throw runtime_error("unknown command");
}

// Разбирает строку команды и выбирает нужную структуру данных.
string run(Database& db, const string& query, bool& changed) {
    istringstream input(query);
    vector<string> words;
    string word;
    while (input >> word) words.push_back(word);
    if (words.empty()) throw runtime_error("empty command");
    const string& op = words[0];
    if (op == "HELP") {
        need(words, 1);
        return "M: CREATE PUSH INSERT GET SET DEL LEN READ; F/L: CREATE PUSH INSERTHEAD INSERTTAIL INSERTBEFORE INSERTAFTER DEL DELHEAD DELTAIL DELBEFORE DELAFTER REMOVE FIND GET READ (list arguments are values; FIND returns an index); S/Q: CREATE PUSH POP GET READ; T: CREATE INSERT DEL GET READ; PRINT name; ISMEMBER name key";
    }
    if (words.size() < 2) throw runtime_error("container name required");
    if (op == "PRINT") {
        need(words, 2);
        auto it = db.find(words[1]);
        if (it == db.end()) throw runtime_error("container does not exist");
        return show(*it->second);
    }
    if (op == "ISMEMBER") {
        need(words, 3);
        Data& d = getData(db, words[1], 'T');
        return avlContains(d.tree, treeNumber(words[2])) ? "TRUE" : "FALSE";
    }
    if (op.size() < 2 || string("MFLSQT").find(op[0]) == string::npos) throw runtime_error("unknown command");
    char type = op[0];
    if (op == string(1, type) + "CREATE") {
        need(words, 2);
        if (db.count(words[1])) throw runtime_error("container already exists");
        getData(db, words[1], type, true);
        changed = true;
        return "OK";
    }
    if (type == 'M') return runArray(db, words, changed);
    if (type == 'F' || type == 'L') return runList(db, words, changed);
    if (type == 'T') return runTree(db, words, changed);
    return runStackQueue(db, words, changed);
}

// Точка входа: читает аргументы, загружает файл и запускает команду.
int main(int argc, char* argv[]) {
    string file;
    string query;
    for (int i = 1; i + 1 < argc; i += 2) {
        string option = argv[i];
        if (option == "--file") file = argv[i + 1];
        else if (option == "--query") query = argv[i + 1];
        else { cerr << "unknown option\n"; return 1; }
    }
    if (argc != 5 || file.empty() || query.empty()) {
        cerr << "usage: dbms --file data.txt --query \"COMMAND name arguments\"\n";
        return 1;
    }
    try {
        Database db = loadFile(file);
        bool changed = false;
        string result = run(db, query, changed);
        if (changed) saveFile(db, file);
        cout << result << '\n';
    } catch (const exception& error) {
        cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
