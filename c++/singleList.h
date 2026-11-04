#pragma once
#include <string>
#include <vector>

struct SingleNode {
    std::string value;
    SingleNode* next = nullptr;
};

struct SingleList {
    SingleNode* head = nullptr;
    SingleNode* tail = nullptr;
    int size = 0;
};

SingleList* singleCreate();
void singleDestroy(SingleList* list);
int singleSize(const SingleList* list);
void singlePush(SingleList* list, const std::string& value);
void singleInsertHead(SingleList* list, const std::string& value);
void singleInsertTail(SingleList* list, const std::string& value);
void singleInsertBefore(SingleList* list, const std::string& target, const std::string& value);
void singleInsertAfter(SingleList* list, const std::string& target, const std::string& value);
std::string singleDelete(SingleList* list, const std::string& target);
std::string singleDeleteHead(SingleList* list);
std::string singleDeleteTail(SingleList* list);
std::string singleDeleteBefore(SingleList* list, const std::string& target);
std::string singleDeleteAfter(SingleList* list, const std::string& target);
std::string singleRemove(SingleList* list, const std::string& value);
std::string singleGet(const SingleList* list, const std::string& value);
int singleFind(const SingleList* list, const std::string& value);
std::vector<std::string> singleRead(const SingleList* list, bool reverse);
