package main

// SingleNode хранит значение и ссылку на следующий узел.
type SingleNode struct {
	value string
	next  *SingleNode
}

type SingleList struct {
	head *SingleNode
	tail *SingleNode
	size int
}

func singleCreate() *SingleList {
	return &SingleList{}
}

func singleSize(list *SingleList) int {
	return list.size
}

// Находит узел по индексу, последовательно проходя список.
func singleAt(list *SingleList, index int) *SingleNode {
	if index < 0 || index >= list.size {
		panic("index out of range")
	}
	node := list.head
	for i := 0; i < index; i++ {
		node = node.next
	}
	return node
}

// Вставляет новый узел в начало или после предыдущего узла.
func singleInsert(list *SingleList, index int, value string) {
	if index < 0 || index > list.size {
		panic("index out of range")
	}
	node := &SingleNode{value: value}
	if index == 0 {
		node.next = list.head
		list.head = node
		if list.tail == nil {
			list.tail = node
		}
	} else {
		before := singleAt(list, index-1)
		node.next = before.next
		before.next = node
		if node.next == nil {
			list.tail = node
		}
	}
	list.size++
}

func singleGet(list *SingleList, index int) string {
	return singleAt(list, index).value
}

// Удаляет узел и соединяет соседние элементы списка.
func singleDelete(list *SingleList, index int) string {
	if index < 0 || index >= list.size {
		panic("index out of range")
	}
	var node *SingleNode
	if index == 0 {
		node = list.head
		list.head = node.next
		if list.head == nil {
			list.tail = nil
		}
	} else {
		before := singleAt(list, index-1)
		node = before.next
		before.next = node.next
		if before.next == nil {
			list.tail = before
		}
	}
	list.size--
	return node.value
}

func singleFind(list *SingleList, value string) int {
	index := 0
	for node := list.head; node != nil; node = node.next {
		if node.value == value {
			return index
		}
		index++
	}
	return -1
}

func singleValues(list *SingleList) []string {
	result := []string{}
	for node := list.head; node != nil; node = node.next {
		result = append(result, node.value)
	}
	return result
}
