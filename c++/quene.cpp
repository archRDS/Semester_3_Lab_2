#include <stdexcept>
#include "quene.h"

using namespace std;

Queue* queueCreate() {
    return new Queue;
}

void queueDestroy(Queue* queue) {
    while (queue->head != nullptr) {
        QueueNode* next = queue->head->next;
        delete queue->head;
        queue->head = next;
    }
    delete queue;
}

// Добавляет новый элемент в конец очереди.
void queuePush(Queue* queue, const string& value) {
    QueueNode* node = new QueueNode{value};
    if (queue->tail == nullptr) queue->head = node;
    else queue->tail->next = node;
    queue->tail = node;
}

string queueGet(const Queue* queue) {
    if (queue->head == nullptr) throw runtime_error("queue is empty");
    return queue->head->value;
}

// Удаляет первый элемент очереди и возвращает его значение.
string queuePop(Queue* queue) {
    string value = queueGet(queue);
    QueueNode* old = queue->head;
    queue->head = old->next;
    if (queue->head == nullptr) queue->tail = nullptr;
    delete old;
    return value;
}

vector<string> queueRead(const Queue* queue) {
    vector<string> result;
    for (QueueNode* node = queue->head; node != nullptr; node = node->next) {
        result.push_back(node->value);
    }
    return result;
}
