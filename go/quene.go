package main

// Очередь хранит начало и конец; первым удаляется первый добавленный элемент.
type QueueNode struct {
	value string
	next  *QueueNode
}

type Queue struct {
	head *QueueNode
	tail *QueueNode
}

func queueCreate() *Queue {
	return &Queue{}
}

func queuePush(queue *Queue, value string) {
	node := &QueueNode{value: value}
	if queue.tail == nil {
		queue.head = node
	} else {
		queue.tail.next = node
	}
	queue.tail = node
}

func queueGet(queue *Queue) string {
	if queue.head == nil {
		panic("queue is empty")
	}
	return queue.head.value
}

func queuePop(queue *Queue) string {
	value := queueGet(queue)
	queue.head = queue.head.next
	if queue.head == nil {
		queue.tail = nil
	}
	return value
}

func queueValues(queue *Queue) []string {
	result := []string{}
	for node := queue.head; node != nil; node = node.next {
		result = append(result, node.value)
	}
	return result
}
