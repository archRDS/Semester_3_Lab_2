#include <algorithm>
#include <stdexcept>
#include "singleList.h"

using namespace std;

static SingleNode* findNode(const SingleList* list, const string& value) {
    for (SingleNode* node = list->head; node != nullptr; node = node->next)
        if (node->value == value) return node;
    return nullptr;
}

static string removeAfter(SingleList* list, SingleNode* before) {
    SingleNode* node = before == nullptr ? list->head : before->next;
    if (node == nullptr) throw runtime_error("no element to delete");
    if (before == nullptr) list->head = node->next;
    else before->next = node->next;
    if (node == list->tail) list->tail = before;
    string value = node->value;
    delete node;
    list->size--;
    return value;
}

SingleList* singleCreate() { return new SingleList; }
void singleDestroy(SingleList* list) {
    while (list->head != nullptr) {
        SingleNode* next = list->head->next;
        delete list->head;
        list->head = next;
    }
    delete list;
}
int singleSize(const SingleList* list) { return list->size; }

void singleInsertHead(SingleList* list, const string& value) {
    list->head = new SingleNode{value, list->head};
    if (list->tail == nullptr) list->tail = list->head;
    list->size++;
}

void singleInsertTail(SingleList* list, const string& value) {
    SingleNode* node = new SingleNode{value};
    if (list->tail == nullptr) list->head = node;
    else list->tail->next = node;
    list->tail = node;
    list->size++;
}

void singlePush(SingleList* list, const string& value) { singleInsertTail(list, value); }

void singleInsertBefore(SingleList* list, const string& target, const string& value) {
    SingleNode* before = nullptr;
    for (SingleNode* node = list->head; node != nullptr; node = node->next) {
        if (node->value == target) {
            if (before == nullptr) singleInsertHead(list, value);
            else { before->next = new SingleNode{value, node}; list->size++; }
            return;
        }
        before = node;
    }
    throw runtime_error("value not found");
}

void singleInsertAfter(SingleList* list, const string& target, const string& value) {
    SingleNode* node = findNode(list, target);
    if (node == nullptr) throw runtime_error("value not found");
    node->next = new SingleNode{value, node->next};
    if (list->tail == node) list->tail = node->next;
    list->size++;
}

string singleDeleteHead(SingleList* list) { return removeAfter(list, nullptr); }
string singleDeleteTail(SingleList* list) {
    if (list->head == nullptr) throw runtime_error("no element to delete");
    SingleNode* before = nullptr;
    for (SingleNode* node = list->head; node != list->tail; node = node->next) before = node;
    return removeAfter(list, before);
}

string singleDelete(SingleList* list, const string& target) {
    SingleNode* before = nullptr;
    for (SingleNode* node = list->head; node != nullptr; node = node->next) {
        if (node->value == target) return removeAfter(list, before);
        before = node;
    }
    throw runtime_error("value not found");
}

string singleDeleteBefore(SingleList* list, const string& target) {
    SingleNode* beforeBefore = nullptr;
    SingleNode* before = nullptr;
    for (SingleNode* node = list->head; node != nullptr; node = node->next) {
        if (node->value == target) {
            if (before == nullptr) throw runtime_error("no element before value");
            return removeAfter(list, beforeBefore);
        }
        beforeBefore = before;
        before = node;
    }
    throw runtime_error("value not found");
}

string singleDeleteAfter(SingleList* list, const string& target) {
    SingleNode* node = findNode(list, target);
    if (node == nullptr) throw runtime_error("value not found");
    return removeAfter(list, node);
}

string singleRemove(SingleList* list, const string& value) { return singleDelete(list, value); }
string singleGet(const SingleList* list, const string& value) {
    SingleNode* node = findNode(list, value);
    if (node == nullptr) throw runtime_error("value not found");
    return node->value;
}
int singleFind(const SingleList* list, const string& value) {
    int position = 0;
    for (SingleNode* node = list->head; node != nullptr; node = node->next, position++)
        if (node->value == value) return position;
    return -1;
}
vector<string> singleRead(const SingleList* list, bool reverse) {
    vector<string> result;
    for (SingleNode* node = list->head; node != nullptr; node = node->next) result.push_back(node->value);
    if (reverse) std::reverse(result.begin(), result.end());
    return result;
}
