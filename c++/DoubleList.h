#pragma once
#include <string>
#include <vector>

struct DoubleNode {
    std::string value;
    DoubleNode* prev = nullptr;
    DoubleNode* next = nullptr;
};

struct DoubleList {
    DoubleNode* head = nullptr;
    DoubleNode* tail = nullptr;
    int size = 0;
};

DoubleList* doubleCreate();
void doubleDestroy(DoubleList* list);
int doubleSize(const DoubleList* list);
void doublePush(DoubleList* list, const std::string& value);
void doubleInsertHead(DoubleList* list, const std::string& value);
void doubleInsertTail(DoubleList* list, const std::string& value);
void doubleInsertBefore(DoubleList* list, const std::string& target, const std::string& value);
void doubleInsertAfter(DoubleList* list, const std::string& target, const std::string& value);
std::string doubleDelete(DoubleList* list, const std::string& target);
std::string doubleDeleteHead(DoubleList* list);
std::string doubleDeleteTail(DoubleList* list);
std::string doubleDeleteBefore(DoubleList* list, const std::string& target);
std::string doubleDeleteAfter(DoubleList* list, const std::string& target);
std::string doubleRemove(DoubleList* list, const std::string& value);
std::string doubleGet(const DoubleList* list, const std::string& value);
int doubleFind(const DoubleList* list, const std::string& value);
std::vector<std::string> doubleRead(const DoubleList* list, bool reverse);
