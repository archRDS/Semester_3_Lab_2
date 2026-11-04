package main

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
)

// Data хранит одну структуру данных выбранного типа.
type Data struct {
	kind   byte
	array  *Array
	single *SingleList
	doubly *DoubleList
	stack  *Stack
	queue  *Queue
}

type Database map[string]*Data

func getData(db Database, name string, kind byte, create bool) *Data {
	data, exists := db[name]
	if !exists {
		if !create {
			panic("container does not exist")
		}
		data = &Data{kind: kind}
		switch kind {
		case 'M':
			data.array = arrayCreate()
		case 'F':
			data.single = singleCreate()
		case 'L':
			data.doubly = doubleCreate()
		case 'S':
			data.stack = stackCreate()
		case 'Q':
			data.queue = queueCreate()
		}
		db[name] = data
	}
	if data.kind != kind {
		panic("wrong container type")
	}
	return data
}

func listSize(data *Data) int {
	if data.kind == 'F' {
		return singleSize(data.single)
	}
	return doubleSize(data.doubly)
}

func listInsert(data *Data, index int, value string) {
	if data.kind == 'F' {
		singleInsert(data.single, index, value)
	} else {
		doubleInsert(data.doubly, index, value)
	}
}

func listGet(data *Data, index int) string {
	if data.kind == 'F' {
		return singleGet(data.single, index)
	}
	return doubleGet(data.doubly, index)
}

func listDelete(data *Data, index int) string {
	if data.kind == 'F' {
		return singleDelete(data.single, index)
	}
	return doubleDelete(data.doubly, index)
}

func listFind(data *Data, value string) int {
	if data.kind == 'F' {
		return singleFind(data.single, value)
	}
	return doubleFind(data.doubly, value)
}

func values(data *Data, reverse bool) []string {
	var result []string
	switch data.kind {
	case 'M':
		result = arrayValues(data.array)
	case 'F':
		result = singleValues(data.single)
	case 'L':
		result = doubleValues(data.doubly, reverse)
	case 'S':
		result = stackValues(data.stack)
	case 'Q':
		result = queueValues(data.queue)
	}
	if reverse && data.kind != 'L' {
		for left, right := 0, len(result)-1; left < right; left, right = left+1, right-1 {
			result[left], result[right] = result[right], result[left]
		}
	}
	return result
}

func show(data *Data, reverse bool) string {
	items := values(data, reverse)
	if len(items) == 0 {
		return "(empty)"
	}
	return strings.Join(items, " ")
}

// Читает сохраненные структуры перед выполнением команды.
func loadFile(path string) Database {
	db := make(Database)
	file, err := os.Open(path)
	if errors.Is(err, os.ErrNotExist) {
		return db
	}
	if err != nil {
		panic(err)
	}
	defer file.Close()
	input := bufio.NewReader(file)
	var count int
	if _, err := fmt.Fscan(input, &count); err != nil || count < 0 || count > 1000000 {
		panic("bad data file")
	}
	for i := 0; i < count; i++ {
		var kindText, name string
		var size int
		if _, err := fmt.Fscan(input, &kindText, &name, &size); err != nil || size < 0 || size > 1000000 {
			panic("bad data file")
		}
		if len(kindText) != 1 || !strings.Contains("MFLSQ", kindText) {
			panic("bad data file")
		}
		if _, exists := db[name]; exists {
			panic("bad data file")
		}
		kind := kindText[0]
		data := getData(db, name, kind, true)
		items := make([]string, size)
		for j := 0; j < size; j++ {
			if _, err := fmt.Fscan(input, &items[j]); err != nil {
				panic("bad data file")
			}
		}
		if kind == 'S' {
			for j := size - 1; j >= 0; j-- {
				stackPush(data.stack, items[j])
			}
		} else {
			for _, value := range items {
				switch kind {
				case 'M':
					arrayInsert(data.array, arraySize(data.array), value)
				case 'F':
					singleInsert(data.single, singleSize(data.single), value)
				case 'L':
					doubleInsert(data.doubly, doubleSize(data.doubly), value)
				case 'Q':
					queuePush(data.queue, value)
				}
			}
		}
	}
	return db
}

// Сохраняет структуры после команды, изменившей данные.
func saveFile(db Database, path string) {
	names := make([]string, 0, len(db))
	for name := range db {
		names = append(names, name)
	}
	sort.Strings(names)
	var output strings.Builder
	fmt.Fprintln(&output, len(db))
	for _, name := range names {
		data := db[name]
		items := values(data, false)
		fmt.Fprintf(&output, "%c %s %d\n", data.kind, name, len(items))
		for _, value := range items {
			fmt.Fprintln(&output, value)
		}
	}
	if err := os.WriteFile(path, []byte(output.String()), 0666); err != nil {
		panic("cannot write data file")
	}
}

func need(words []string, count int) {
	if len(words) != count {
		panic("wrong number of arguments")
	}
}

func number(text string) int {
	value, err := strconv.ParseInt(text, 10, 32)
	if err != nil || value < 0 {
		panic("index must be a non-negative integer")
	}
	return int(value)
}

// Выполняет команды массива.
func runArray(db Database, words []string, changed *bool) string {
	op := words[0]
	data := getData(db, words[1], 'M', op == "MPUSH" || op == "MINSERT")
	switch op {
	case "MPUSH":
		need(words, 3)
		arrayInsert(data.array, arraySize(data.array), words[2])
		*changed = true
		return words[2]
	case "MINSERT":
		need(words, 4)
		arrayInsert(data.array, number(words[2]), words[3])
		*changed = true
		return words[3]
	case "MGET":
		need(words, 3)
		return arrayGet(data.array, number(words[2]))
	case "MSET":
		need(words, 4)
		arraySet(data.array, number(words[2]), words[3])
		*changed = true
		return words[3]
	case "MDEL":
		need(words, 3)
		result := arrayDelete(data.array, number(words[2]))
		*changed = true
		return result
	case "MLEN":
		need(words, 2)
		return strconv.Itoa(arraySize(data.array))
	case "MREAD":
		need(words, 2)
		return show(data, false)
	}
	panic("unknown command")
}

// Выполняет команды односвязного и двусвязного списков.
func runList(db Database, words []string, changed *bool) string {
	op := words[0]
	action := op[1:]
	adding := action == "PUSH" || action == "INSERTHEAD" || action == "INSERTTAIL" ||
		action == "INSERTBEFORE" || action == "INSERTAFTER"
	data := getData(db, words[1], op[0], adding)
	size := listSize(data)
	switch action {
	case "PUSH", "INSERTHEAD", "INSERTTAIL":
		need(words, 3)
		index := size
		if action == "INSERTHEAD" {
			index = 0
		}
		listInsert(data, index, words[2])
		*changed = true
		return words[2]
	case "INSERTBEFORE", "INSERTAFTER":
		need(words, 4)
		index := listFind(data, words[2])
		if index == -1 {
			panic("value not found")
		}
		if action == "INSERTAFTER" {
			index++
		}
		listInsert(data, index, words[3])
		*changed = true
		return words[3]
	case "DEL", "DELBEFORE", "DELAFTER":
		need(words, 3)
		index := listFind(data, words[2])
		if index == -1 {
			panic("value not found")
		}
		if action == "DELBEFORE" {
			index--
		}
		if action == "DELAFTER" {
			index++
		}
		result := listDelete(data, index)
		*changed = true
		return result
	case "DELHEAD", "DELTAIL":
		need(words, 2)
		index := size - 1
		if action == "DELHEAD" {
			index = 0
		}
		result := listDelete(data, index)
		*changed = true
		return result
	case "REMOVE":
		need(words, 3)
		index := listFind(data, words[2])
		if index == -1 {
			panic("value not found")
		}
		result := listDelete(data, index)
		*changed = true
		return result
	case "FIND":
		need(words, 3)
		return strconv.Itoa(listFind(data, words[2]))
	case "GET":
		need(words, 3)
		index := listFind(data, words[2])
		if index == -1 {
			panic("value not found")
		}
		return listGet(data, index)
	case "READ":
		if len(words) != 2 && len(words) != 3 {
			panic("wrong number of arguments")
		}
		if len(words) == 3 && words[2] != "FORWARD" && words[2] != "REVERSE" {
			panic("use FORWARD or REVERSE")
		}
		return show(data, len(words) == 3 && words[2] == "REVERSE")
	}
	panic("unknown command")
}

// Выполняет команды стека и очереди.
func runStackQueue(db Database, words []string, changed *bool) string {
	op := words[0]
	action := op[1:]
	data := getData(db, words[1], op[0], action == "PUSH")
	switch action {
	case "PUSH":
		need(words, 3)
		if data.kind == 'S' {
			stackPush(data.stack, words[2])
		} else {
			queuePush(data.queue, words[2])
		}
		*changed = true
		return words[2]
	case "POP":
		need(words, 2)
		var result string
		if data.kind == 'S' {
			result = stackPop(data.stack)
		} else {
			result = queuePop(data.queue)
		}
		*changed = true
		return result
	case "GET":
		need(words, 2)
		if data.kind == 'S' {
			return stackGet(data.stack)
		}
		return queueGet(data.queue)
	case "READ":
		need(words, 2)
		return show(data, false)
	}
	panic("unknown command")
}

// Разбирает строку команды и выбирает нужную структуру данных.
func run(db Database, query string, changed *bool) string {
	words := strings.Fields(query)
	if len(words) == 0 {
		panic("empty command")
	}
	op := words[0]
	if op == "HELP" {
		need(words, 1)
		return "M: CREATE PUSH INSERT GET SET DEL LEN READ; F/L: CREATE PUSH INSERTHEAD INSERTTAIL INSERTBEFORE INSERTAFTER DEL DELHEAD DELTAIL DELBEFORE DELAFTER REMOVE FIND GET READ (list arguments are values; FIND returns an index); S/Q: CREATE PUSH POP GET READ; PRINT name"
	}
	if len(words) < 2 {
		panic("container name required")
	}
	if op == "PRINT" {
		need(words, 2)
		data, exists := db[words[1]]
		if !exists {
			panic("container does not exist")
		}
		return show(data, false)
	}
	if len(op) < 2 || !strings.ContainsRune("MFLSQ", rune(op[0])) {
		panic("unknown command")
	}
	kind := op[0]
	if op == string(kind)+"CREATE" {
		need(words, 2)
		if _, exists := db[words[1]]; exists {
			panic("container already exists")
		}
		getData(db, words[1], kind, true)
		*changed = true
		return "OK"
	}
	if kind == 'M' {
		return runArray(db, words, changed)
	}
	if kind == 'F' || kind == 'L' {
		return runList(db, words, changed)
	}
	return runStackQueue(db, words, changed)
}

func execute(file, query string) (failure error) {
	defer func() {
		if value := recover(); value != nil {
			failure = fmt.Errorf("%v", value)
		}
	}()
	db := loadFile(file)
	changed := false
	result := run(db, query, &changed)
	if changed {
		saveFile(db, file)
	}
	fmt.Println(result)
	return nil
}

// Точка входа: читает --file и --query, затем выполняет одну команду.
func main() {
	if len(os.Args) > 1 && os.Args[1] == "lab2" {
		if err := runLab2(os.Args[2:]); err != nil {
			fmt.Fprintln(os.Stderr, "error:", err)
			os.Exit(1)
		}
		return
	}
	var file, query string
	for i := 1; i+1 < len(os.Args); i += 2 {
		option := os.Args[i]
		if option == "--file" {
			file = os.Args[i+1]
		} else if option == "--query" {
			query = os.Args[i+1]
		} else {
			fmt.Fprintln(os.Stderr, "unknown option")
			os.Exit(1)
		}
	}
	if len(os.Args) != 5 || file == "" || query == "" {
		fmt.Fprintln(os.Stderr, "usage: dbms --file data.txt --query \"COMMAND name arguments\"")
		os.Exit(1)
	}
	if err := execute(file, query); err != nil {
		fmt.Fprintln(os.Stderr, "error:", err)
		os.Exit(1)
	}
}
