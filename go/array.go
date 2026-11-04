package main

// Array хранит элементы, их количество и размер выделенной памяти.
type Array struct {
	data     []string
	size     int
	capacity int
}

func arrayCreate() *Array {
	return &Array{}
}

func arraySize(array *Array) int {
	return array.size
}

// При нехватке места увеличивает массив и вставляет значение по индексу.
func arrayInsert(array *Array, index int, value string) {
	if index < 0 || index > array.size {
		panic("index out of range")
	}
	if array.size == array.capacity {
		newCapacity := 4
		if array.capacity > 0 {
			newCapacity = array.capacity * 2
		}
		newData := make([]string, newCapacity)
		copy(newData, array.data)
		array.data = newData
		array.capacity = newCapacity
	}
	for i := array.size; i > index; i-- {
		array.data[i] = array.data[i-1]
	}
	array.data[index] = value
	array.size++
}

func arrayGet(array *Array, index int) string {
	if index < 0 || index >= array.size {
		panic("index out of range")
	}
	return array.data[index]
}

func arraySet(array *Array, index int, value string) {
	if index < 0 || index >= array.size {
		panic("index out of range")
	}
	array.data[index] = value
}

// Удаляет элемент и сдвигает оставшиеся значения влево.
func arrayDelete(array *Array, index int) string {
	value := arrayGet(array, index)
	for i := index; i+1 < array.size; i++ {
		array.data[i] = array.data[i+1]
	}
	array.size--
	array.data[array.size] = ""
	return value
}

func arrayValues(array *Array) []string {
	result := make([]string, array.size)
	copy(result, array.data[:array.size])
	return result
}
