#include <stdexcept>
#include "DoubleList.h"

using namespace std;

static DoubleNode* findNode(const DoubleList* list, const string& value) {
    for (DoubleNode* node = list->head; node != nullptr; node = node->next)
        if (node->value == value) return node;
    return nullptr;
}

static string removeNode(DoubleList* list, DoubleNode* node) {
    if (node == nullptr) throw runtime_error("no element to delete");
    if (node->prev == nullptr) list->head = node->next;
    else node->prev->next = node->next;
    if (node->next == nullptr) list->tail = node->prev;
    else node->next->prev = node->prev;
    string value = node->value;
    delete node;
    list->size--;
    return value;
}

DoubleList* doubleCreate() { return new DoubleList; }
void doubleDestroy(DoubleList* list) {
    while (list->head != nullptr) {
        DoubleNode* next = list->head->next;
        delete list->head;
        list->head = next;
    }
    delete list;
}
int doubleSize(const DoubleList* list) { return list->size; }

void doubleInsertHead(DoubleList* list, const string& value) {
    DoubleNode* node = new DoubleNode{value, nullptr, list->head};
    if (list->head == nullptr) list->tail = node;
    else list->head->prev = node;
    list->head = node;
    list->size++;
}

void doubleInsertTail(DoubleList* list, const string& value) {
    DoubleNode* node = new DoubleNode{value, list->tail, nullptr};
    if (list->tail == nullptr) list->head = node;
    else list->tail->next = node;
    list->tail = node;
    list->size++;
}

void doublePush(DoubleList* list, const string& value) { doubleInsertTail(list, value); }

void doubleInsertBefore(DoubleList* list, const string& target, const string& value) {
    DoubleNode* after = findNode(list, target);
    if (after == nullptr) throw runtime_error("value not found");
    if (after == list->head) { doubleInsertHead(list, value); return; }
    DoubleNode* node = new DoubleNode{value, after->prev, after};
    after->prev->next = node;
    after->prev = node;
    list->size++;
}

void doubleInsertAfter(DoubleList* list, const string& target, const string& value) {
    DoubleNode* before = findNode(list, target);
    if (before == nullptr) throw runtime_error("value not found");
    if (before == list->tail) { doubleInsertTail(list, value); return; }
    DoubleNode* node = new DoubleNode{value, before, before->next};
    before->next->prev = node;
    before->next = node;
    list->size++;
}

string doubleDeleteHead(DoubleList* list) { return removeNode(list, list->head); }
string doubleDeleteTail(DoubleList* list) { return removeNode(list, list->tail); }
string doubleDelete(DoubleList* list, const string& target) {
    DoubleNode* node = findNode(list, target);
    if (node == nullptr) throw runtime_error("value not found");
    return removeNode(list, node);
}
string doubleDeleteBefore(DoubleList* list, const string& target) {
    DoubleNode* node = findNode(list, target);
    if (node == nullptr) throw runtime_error("value not found");
    if (node->prev == nullptr) throw runtime_error("no element before value");
    return removeNode(list, node->prev);
}
string doubleDeleteAfter(DoubleList* list, const string& target) {
    DoubleNode* node = findNode(list, target);
    if (node == nullptr) throw runtime_error("value not found");
    if (node->next == nullptr) throw runtime_error("no element after value");
    return removeNode(list, node->next);
}
string doubleRemove(DoubleList* list, const string& value) { return doubleDelete(list, value); }
string doubleGet(const DoubleList* list, const string& value) {
    DoubleNode* node = findNode(list, value);
    if (node == nullptr) throw runtime_error("value not found");
    return node->value;
}
int doubleFind(const DoubleList* list, const string& value) {
    int position = 0;
    for (DoubleNode* node = list->head; node != nullptr; node = node->next, position++)
        if (node->value == value) return position;
    return -1;
}
vector<string> doubleRead(const DoubleList* list, bool reverse) {
    vector<string> result;
    if (reverse) {
        for (DoubleNode* node = list->tail; node != nullptr; node = node->prev) result.push_back(node->value);
    } else {
        for (DoubleNode* node = list->head; node != nullptr; node = node->next) result.push_back(node->value);
    }
    return result;
}
