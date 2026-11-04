package main

// Стек хранит вершину; первым удаляется последний добавленный элемент.
type StackNode struct {
	value string
	next  *StackNode
}

type Stack struct {
	top *StackNode
}

func stackCreate() *Stack {
	return &Stack{}
}

func stackPush(stack *Stack, value string) {
	stack.top = &StackNode{value: value, next: stack.top}
}

func stackGet(stack *Stack) string {
	if stack.top == nil {
		panic("stack is empty")
	}
	return stack.top.value
}

func stackPop(stack *Stack) string {
	value := stackGet(stack)
	stack.top = stack.top.next
	return value
}

func stackValues(stack *Stack) []string {
	result := []string{}
	for node := stack.top; node != nil; node = node.next {
		result = append(result, node.value)
	}
	return result
}
