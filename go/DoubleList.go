package main

// DoubleNode хранит ссылки на предыдущий и следующий узлы.
type DoubleNode struct {
	value string
	prev  *DoubleNode
	next  *DoubleNode
}

type DoubleList struct {
	head *DoubleNode
	tail *DoubleNode
	size int
}

func doubleCreate() *DoubleList {
	return &DoubleList{}
}

func doubleSize(list *DoubleList) int {
	return list.size
}

func doubleAt(list *DoubleList, index int) *DoubleNode {
	if index < 0 || index >= list.size {
		panic("index out of range")
	}
	node := list.head
	for i := 0; i < index; i++ {
		node = node.next
	}
	return node
}

// Вставляет узел и обновляет связи с соседними узлами.
func doubleInsert(list *DoubleList, index int, value string) {
	if index < 0 || index > list.size {
		panic("index out of range")
	}
	node := &DoubleNode{value: value}
	if index == list.size {
		node.prev = list.tail
		if list.tail == nil {
			list.head = node
		} else {
			list.tail.next = node
		}
		list.tail = node
	} else {
		after := doubleAt(list, index)
		node.next = after
		node.prev = after.prev
		if after.prev == nil {
			list.head = node
		} else {
			after.prev.next = node
		}
		after.prev = node
	}
	list.size++
}

func doubleGet(list *DoubleList, index int) string {
	return doubleAt(list, index).value
}

// Удаляет узел, сохраняя прямые и обратные связи списка.
func doubleDelete(list *DoubleList, index int) string {
	node := doubleAt(list, index)
	if node.prev == nil {
		list.head = node.next
	} else {
		node.prev.next = node.next
	}
	if node.next == nil {
		list.tail = node.prev
	} else {
		node.next.prev = node.prev
	}
	list.size--
	return node.value
}

func doubleFind(list *DoubleList, value string) int {
	index := 0
	for node := list.head; node != nil; node = node.next {
		if node.value == value {
			return index
		}
		index++
	}
	return -1
}

// Читает список от начала или от конца.
func doubleValues(list *DoubleList, reverse bool) []string {
	result := []string{}
	if reverse {
		for node := list.tail; node != nil; node = node.prev {
			result = append(result, node.value)
		}
	} else {
		for node := list.head; node != nil; node = node.next {
			result = append(result, node.value)
		}
	}
	return result
}
