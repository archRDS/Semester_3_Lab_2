#pragma once

#include <string>
#include <vector>

struct QueueNode {
    std::string value;
    QueueNode* next = nullptr;
};

struct Queue {
    QueueNode* head = nullptr;
    QueueNode* tail = nullptr;
};

Queue* queueCreate();
void queueDestroy(Queue* queue);
void queuePush(Queue* queue, const std::string& value);
std::string queueGet(const Queue* queue);
std::string queuePop(Queue* queue);
std::vector<std::string> queueRead(const Queue* queue);
