package main

import (
	"bufio"
	"container/heap"
	"errors"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"
)

const labMaxInt = int64(1<<63 - 1)
const labMinInt = int64(-1 << 63)

func labInt(s string) (int64, error) {
	v, err := strconv.ParseInt(s, 10, 64)
	if err != nil {
		return 0, fmt.Errorf("invalid integer: %s", s)
	}
	return v, nil
}
func labUint(s string) (uint64, error) {
	v, err := strconv.ParseUint(s, 10, 64)
	if err != nil {
		return 0, fmt.Errorf("invalid uint64: %s", s)
	}
	return v, nil
}
func labRPN(expression string) (uint64, error) {
	tokens := strings.Fields(expression)
	if len(tokens) == 1 {
		if _, err := labUint(tokens[0]); err != nil && strings.ContainsAny(tokens[0], "+-*") {
			tokens = nil
			for _, c := range expression {
				if c != ' ' {
					tokens = append(tokens, string(c))
				}
			}
		}
	}
	stack := stackCreate() // Lab 1 linked stack
	for _, token := range tokens {
		if token == "+" || token == "-" || token == "*" {
			if stack.top == nil || stack.top.next == nil {
				return 0, errors.New("invalid RPN expression")
			}
			b, _ := labUint(stackPop(stack))
			a, _ := labUint(stackPop(stack))
			var value uint64
			switch token {
			case "+":
				if a > ^uint64(0)-b {
					return 0, errors.New("integer overflow")
				}
				value = a + b
			case "-":
				if a < b {
					return 0, errors.New("negative result is not uint64")
				}
				value = a - b
			case "*":
				if b != 0 && a > ^uint64(0)/b {
					return 0, errors.New("integer overflow")
				}
				value = a * b
			}
			stackPush(stack, strconv.FormatUint(value, 10))
		} else {
			v, err := labUint(token)
			if err != nil {
				return 0, err
			}
			stackPush(stack, strconv.FormatUint(v, 10))
		}
	}
	if stack.top == nil || stack.top.next != nil {
		return 0, errors.New("invalid RPN expression")
	}
	return labUint(stackGet(stack))
}

type hashEntry struct {
	key, value string
	used       bool
}
type labHash struct {
	cuckoo        bool
	chains        [][]hashEntry
	first, second []hashEntry
	count         int
	seed          uint64
}

func newLabHash(cuckoo bool) *labHash {
	h := &labHash{cuckoo: cuckoo}
	if cuckoo {
		h.first = make([]hashEntry, 11)
		h.second = make([]hashEntry, 11)
	} else {
		h.chains = make([][]hashEntry, 11)
	}
	return h
}
func labHashCode(s string, salt uint64) uint64 {
	h := uint64(1469598103934665603) ^ salt
	for i := 0; i < len(s); i++ {
		h ^= uint64(s[i])
		h *= 1099511628211
	}
	h ^= h >> 33
	h *= 0xff51afd7ed558ccd
	h ^= h >> 33
	return h
}
func (h *labHash) bucket(key string, table int) int {
	capacity := len(h.chains)
	if h.cuckoo {
		capacity = len(h.first)
	}
	salt := h.seed
	if table == 1 {
		salt += 0x9e3779b97f4a7c15
	}
	return int(labHashCode(key, salt) % uint64(capacity))
}
func (h *labHash) slot(key string, table int) *hashEntry {
	if table == 0 {
		return &h.first[h.bucket(key, 0)]
	}
	return &h.second[h.bucket(key, 1)]
}
func (h *labHash) capacity() int {
	if h.cuckoo {
		return len(h.first) * 2
	}
	return len(h.chains)
}
func (h *labHash) get(key string) (string, bool) {
	if h.cuckoo {
		for i := 0; i < 2; i++ {
			p := h.slot(key, i)
			if p.used && p.key == key {
				return p.value, true
			}
		}
	} else {
		for _, e := range h.chains[h.bucket(key, 0)] {
			if e.key == key {
				return e.value, true
			}
		}
	}
	return "", false
}
func (h *labHash) entries() []hashEntry {
	out := make([]hashEntry, 0, h.count)
	if h.cuckoo {
		for _, p := range h.first {
			if p.used {
				out = append(out, p)
			}
		}
		for _, p := range h.second {
			if p.used {
				out = append(out, p)
			}
		}
	} else {
		for _, b := range h.chains {
			out = append(out, b...)
		}
	}
	return out
}
func (h *labHash) insertRaw(e hashEntry) *hashEntry {
	table := 0
	for i := 0; i < 4*len(h.first); i++ {
		p := h.slot(e.key, table)
		if !p.used {
			*p = e
			h.count++
			return nil
		}
		e, *p = *p, e
		table ^= 1
	}
	return &e
}
func (h *labHash) rebuild(entries []hashEntry, capacity int) {
	for {
		h.first = make([]hashEntry, capacity)
		h.second = make([]hashEntry, capacity)
		h.count = 0
		h.seed++
		failed := false
		for i, e := range entries {
			if displaced := h.insertRaw(e); displaced != nil {
				retry := append([]hashEntry{}, entries[i+1:]...)
				retry = append(retry, *displaced)
				retry = append(retry, h.entries()...)
				entries = retry
				failed = true
				break
			}
		}
		if !failed {
			return
		}
		capacity = capacity*2 + 1
	}
}
func (h *labHash) put(key, value string) bool {
	if h.cuckoo {
		for i := 0; i < 2; i++ {
			p := h.slot(key, i)
			if p.used && p.key == key {
				p.value = value
				return false
			}
		}
		if (h.count+1)*2 > h.capacity() {
			all := append(h.entries(), hashEntry{key, value, true})
			h.rebuild(all, len(h.first)*2+1)
			return true
		}
		if displaced := h.insertRaw(hashEntry{key, value, true}); displaced != nil {
			all := append(h.entries(), *displaced)
			h.rebuild(all, len(h.first)*2+1)
		}
		return true
	}
	index := h.bucket(key, 0)
	for i := range h.chains[index] {
		if h.chains[index][i].key == key {
			h.chains[index][i].value = value
			return false
		}
	}
	if (h.count+1)*10 > len(h.chains)*9 {
		old := h.entries()
		h.chains = make([][]hashEntry, len(h.chains)*2+1)
		h.count = 0
		for _, e := range old {
			h.put(e.key, e.value)
		}
	}
	index = h.bucket(key, 0)
	h.chains[index] = append(h.chains[index], hashEntry{key, value, true})
	h.count++
	return true
}
func (h *labHash) erase(key string) bool {
	if h.cuckoo {
		for i := 0; i < 2; i++ {
			p := h.slot(key, i)
			if p.used && p.key == key {
				*p = hashEntry{}
				h.count--
				return true
			}
		}
	} else {
		index := h.bucket(key, 0)
		b := h.chains[index]
		for i, e := range b {
			if e.key == key {
				h.chains[index] = append(b[:i], b[i+1:]...)
				h.count--
				return true
			}
		}
	}
	return false
}
func (h *labHash) keys() []string {
	keys := make([]string, 0, h.count)
	for _, e := range h.entries() {
		keys = append(keys, e.key)
	}
	sort.Strings(keys)
	return keys
}

func labLoadSets(path string) (map[string]*labHash, error) {
	db := make(map[string]*labHash)
	data, err := os.ReadFile(path)
	if errors.Is(err, os.ErrNotExist) {
		return db, nil
	}
	if err != nil {
		return nil, err
	}
	in := strings.NewReader(string(data))
	var magic string
	var n int
	if _, err = fmt.Fscan(in, &magic, &n); err != nil || magic != "LAB2SETS" || n < 0 || n > 1000000 {
		return nil, errors.New("bad set file")
	}
	for i := 0; i < n; i++ {
		var name, key string
		if _, err = fmt.Fscan(in, &name, &key); err != nil {
			return nil, errors.New("bad set file")
		}
		if db[name] == nil {
			db[name] = newLabHash(false)
		}
		db[name].put(key, "1")
	}
	return db, nil
}
func labSaveSets(path string, db map[string]*labHash) error {
	names := make([]string, 0, len(db))
	n := 0
	for name, set := range db {
		names = append(names, name)
		n += set.count
	}
	sort.Strings(names)
	var out strings.Builder
	fmt.Fprintf(&out, "LAB2SETS %d\n", n)
	for _, name := range names {
		for _, key := range db[name].keys() {
			fmt.Fprintf(&out, "%s %s\n", name, key)
		}
	}
	return os.WriteFile(path, []byte(out.String()), 0666)
}
func labSetQuery(path, query string) (string, error) {
	w := strings.Fields(query)
	if len(w) != 3 {
		return "", errors.New("query must be SETADD/SETDEL/SET_AT name value")
	}
	db, err := labLoadSets(path)
	if err != nil {
		return "", err
	}
	set := db[w[1]]
	switch w[0] {
	case "SETADD":
		if set == nil {
			set = newLabHash(false)
			db[w[1]] = set
		}
		added := set.put(w[2], "1")
		if err = labSaveSets(path, db); err != nil {
			return "", err
		}
		if added {
			return "TRUE", nil
		}
		return "FALSE", nil
	case "SETDEL":
		if set == nil {
			return "FALSE", nil
		}
		deleted := set.erase(w[2])
		if deleted {
			if err = labSaveSets(path, db); err != nil {
				return "", err
			}
		}
		if deleted {
			return "TRUE", nil
		}
		return "FALSE", nil
	case "SET_AT":
		if set == nil {
			return "FALSE", nil
		}
		_, found := set.get(w[2])
		if found {
			return "TRUE", nil
		}
		return "FALSE", nil
	}
	return "", errors.New("unknown set operation")
}

func labPartition(target uint64, args []string) ([][]uint64, error) {
	if target == 0 {
		return nil, errors.New("sum must be positive")
	}
	unique := newLabHash(false)
	numbers := []uint64{}
	var total uint64
	for _, s := range args {
		v, err := labUint(s)
		if err != nil {
			return nil, err
		}
		if v == 0 {
			return nil, errors.New("elements must be natural numbers")
		}
		if unique.put(strconv.FormatUint(v, 10), "1") {
			if v > ^uint64(0)-total {
				return nil, errors.New("sum overflow")
			}
			numbers = append(numbers, v)
			total += v
		}
	}
	if len(numbers) == 0 || total%target != 0 {
		return nil, errors.New("partition impossible")
	}
	groups := int(total / target)
	if groups > len(numbers) {
		return nil, errors.New("partition impossible")
	}
	sort.Slice(numbers, func(i, j int) bool { return numbers[i] > numbers[j] })
	if numbers[0] > target {
		return nil, errors.New("partition impossible")
	}
	sums := make([]uint64, groups)
	result := make([][]uint64, groups)
	var search func(int) bool
	search = func(i int) bool {
		if i == len(numbers) {
			for _, x := range sums {
				if x != target {
					return false
				}
			}
			return true
		}
		tried := make(map[uint64]bool)
		for j := 0; j < groups; j++ {
			if numbers[i] > target-sums[j] || tried[sums[j]] {
				continue
			}
			tried[sums[j]] = true
			empty := sums[j] == 0
			sums[j] += numbers[i]
			result[j] = append(result[j], numbers[i])
			if search(i + 1) {
				return true
			}
			result[j] = result[j][:len(result[j])-1]
			sums[j] -= numbers[i]
			if empty {
				break
			}
		}
		return false
	}
	if !search(0) {
		return nil, errors.New("partition impossible")
	}
	return result, nil
}

func labSubsets(args []string) []string {
	array := arrayCreate()
	args = append([]string{}, args...)
	sort.Strings(args)
	for _, x := range args {
		arrayInsert(array, arraySize(array), x)
	}
	out := []string{}
	current := []string{}
	var generate func(int)
	generate = func(at int) {
		out = append(out, "{"+strings.Join(current, " ")+"}")
		for i := at; i < arraySize(array); i++ {
			if i > at && arrayGet(array, i) == arrayGet(array, i-1) {
				continue
			}
			current = append(current, arrayGet(array, i))
			generate(i + 1)
			current = current[:len(current)-1]
		}
	}
	generate(0)
	return out
}

type labTreeNode struct {
	key             int64
	left, right     *labTreeNode
	weight          uint64
	symbol, minimum int
}

func labBalanced(args []string) (bool, error) {
	var root *labTreeNode
	for _, s := range args {
		v, err := labInt(s)
		if err != nil {
			return false, err
		}
		if v == 0 {
			break
		}
		p := &root
		for *p != nil {
			if v == (*p).key {
				break
			}
			if v < (*p).key {
				p = &(*p).left
			} else {
				p = &(*p).right
			}
		}
		if *p == nil {
			*p = &labTreeNode{key: v}
		}
	}
	type frame struct {
		node *labTreeNode
		done bool
	}
	frames := []frame{}
	if root != nil {
		frames = append(frames, frame{root, false})
	}
	heights := make(map[*labTreeNode]int)
	okay := true
	for len(frames) > 0 {
		f := frames[len(frames)-1]
		frames = frames[:len(frames)-1]
		if !f.done {
			frames = append(frames, frame{f.node, true})
			if f.node.right != nil {
				frames = append(frames, frame{f.node.right, false})
			}
			if f.node.left != nil {
				frames = append(frames, frame{f.node.left, false})
			}
		} else {
			l, r := heights[f.node.left], heights[f.node.right]
			if l-r > 1 || r-l > 1 {
				okay = false
			}
			if l > r {
				heights[f.node] = l + 1
			} else {
				heights[f.node] = r + 1
			}
		}
	}
	return okay, nil
}

type labHuffNode = labTreeNode
type labHuffHeap []*labHuffNode

func (h labHuffHeap) Len() int { return len(h) }
func (h labHuffHeap) Less(i, j int) bool {
	if h[i].weight != h[j].weight {
		return h[i].weight < h[j].weight
	}
	return h[i].minimum < h[j].minimum
}
func (h labHuffHeap) Swap(i, j int)       { h[i], h[j] = h[j], h[i] }
func (h *labHuffHeap) Push(x interface{}) { *h = append(*h, x.(*labHuffNode)) }
func (h *labHuffHeap) Pop() interface{} {
	old := *h
	n := len(old)
	x := old[n-1]
	*h = old[:n-1]
	return x
}
func labHuffTree(counts [256]uint64) *labHuffNode {
	h := &labHuffHeap{}
	heap.Init(h)
	for i, n := range counts {
		if n > 0 {
			heap.Push(h, &labHuffNode{weight: n, symbol: i, minimum: i})
		}
	}
	for h.Len() > 1 {
		a := heap.Pop(h).(*labHuffNode)
		b := heap.Pop(h).(*labHuffNode)
		minimum := a.minimum
		if b.minimum < minimum {
			minimum = b.minimum
		}
		heap.Push(h, &labHuffNode{weight: a.weight + b.weight, symbol: -1, minimum: minimum, left: a, right: b})
	}
	if h.Len() == 0 {
		return nil
	}
	return heap.Pop(h).(*labHuffNode)
}
func labHuffCodes(node *labHuffNode, prefix string, codes *[256]string) {
	if node.symbol >= 0 {
		if prefix == "" {
			prefix = "0"
		}
		codes[node.symbol] = prefix
		return
	}
	labHuffCodes(node.left, prefix+"0", codes)
	labHuffCodes(node.right, prefix+"1", codes)
}
func labHuffEncode(input string) string {
	var counts [256]uint64
	for i := 0; i < len(input); i++ {
		counts[input[i]]++
	}
	root := labHuffTree(counts)
	var codes [256]string
	if root != nil {
		labHuffCodes(root, "", &codes)
	}
	var out strings.Builder
	first := true
	for i, n := range counts {
		if n > 0 {
			if !first {
				out.WriteByte(',')
			}
			first = false
			fmt.Fprintf(&out, "%02X:%d", i, n)
		}
	}
	out.WriteByte('|')
	for i := 0; i < len(input); i++ {
		out.WriteString(codes[input[i]])
	}
	return out.String()
}
func labHuffDecode(archive string) (string, error) {
	bar := strings.IndexByte(archive, '|')
	if bar < 0 {
		return "", errors.New("invalid Huffman archive")
	}
	var counts [256]uint64
	var total uint64
	if bar > 0 {
		for _, part := range strings.Split(archive[:bar], ",") {
			if len(part) < 4 || part[2] != ':' {
				return "", errors.New("invalid Huffman header")
			}
			b, err := strconv.ParseUint(part[:2], 16, 8)
			if err != nil || counts[b] != 0 {
				return "", errors.New("invalid Huffman header")
			}
			n, err := strconv.ParseUint(part[3:], 10, 64)
			if err != nil || n == 0 {
				return "", errors.New("invalid Huffman header")
			}
			counts[b] = n
			total += n
		}
	}
	root := labHuffTree(counts)
	bits := archive[bar+1:]
	if root == nil {
		if bits != "" {
			return "", errors.New("invalid Huffman bits")
		}
		return "", nil
	}
	var out strings.Builder
	p := root
	for _, bit := range bits {
		if bit != '0' && bit != '1' {
			return "", errors.New("invalid Huffman bits")
		}
		if root.symbol < 0 {
			if bit == '0' {
				p = p.left
			} else {
				p = p.right
			}
		} else if bit != '0' {
			return "", errors.New("invalid Huffman bits")
		}
		if p == nil {
			return "", errors.New("invalid Huffman bits")
		}
		if p.symbol >= 0 {
			out.WriteByte(byte(p.symbol))
			p = root
		}
	}
	if p != root || uint64(out.Len()) != total {
		return "", errors.New("incomplete Huffman bits")
	}
	for i := 0; i < out.Len(); i++ {
		counts[out.String()[i]]--
	}
	for _, n := range counts {
		if n != 0 {
			return "", errors.New("Huffman frequency mismatch")
		}
	}
	return out.String(), nil
}

func labIsomorphic(a, b string) bool {
	x, y := []rune(a), []rune(b)
	if len(x) != len(y) {
		return false
	}
	ab, ba := newLabHash(false), newLabHash(true)
	for i := range x {
		left, right := string(x[i]), string(y[i])
		if v, ok := ab.get(left); ok && v != right {
			return false
		}
		if v, ok := ba.get(right); ok && v != left {
			return false
		}
		ab.put(left, right)
		ba.put(right, left)
	}
	return true
}

type labLFUItem struct {
	key, value      string
	frequency, time uint64
}
type labLFU struct {
	limit int
	tick  uint64
	index *labHash
	items []labLFUItem
	order *DoubleList
}

func newLabLFU(limit int) *labLFU {
	return &labLFU{limit: limit, index: newLabHash(false), order: doubleCreate()}
}
func (c *labLFU) get(key string) string {
	id, ok := c.index.get(key)
	if !ok {
		return "-1"
	}
	position, _ := strconv.Atoi(id)
	e := &c.items[position]
	e.frequency++
	c.tick++
	e.time = c.tick
	index := doubleFind(c.order, key)
	doubleDelete(c.order, index)
	doubleInsert(c.order, doubleSize(c.order), key)
	return e.value
}
func (c *labLFU) set(key, value string) {
	if c.limit == 0 {
		return
	}
	if id, ok := c.index.get(key); ok {
		position, _ := strconv.Atoi(id)
		e := &c.items[position]
		e.value = value
		e.frequency++
		c.tick++
		e.time = c.tick
		index := doubleFind(c.order, key)
		doubleDelete(c.order, index)
		doubleInsert(c.order, doubleSize(c.order), key)
		return
	}
	if c.index.count == c.limit {
		victim := ""
		frequency := ^uint64(0)
		for node := c.order.head; node != nil; node = node.next {
			positionText, _ := c.index.get(node.value)
			position, _ := strconv.Atoi(positionText)
			if c.items[position].frequency < frequency {
				frequency = c.items[position].frequency
				victim = node.value
			}
		}
		id, _ := c.index.get(victim)
		reuse, _ := strconv.Atoi(id)
		c.index.erase(victim)
		doubleDelete(c.order, doubleFind(c.order, victim))
		c.tick++
		c.index.put(key, id)
		c.items[reuse] = labLFUItem{key, value, 1, c.tick}
		doubleInsert(c.order, doubleSize(c.order), key)
		return
	}
	c.tick++
	c.index.put(key, strconv.Itoa(len(c.items)))
	c.items = append(c.items, labLFUItem{key, value, 1, c.tick})
	doubleInsert(c.order, doubleSize(c.order), key)
}
func labHashSession(cuckoo bool) error {
	h := newLabHash(cuckoo)
	scanner := bufio.NewScanner(os.Stdin)
	for scanner.Scan() {
		w := strings.Fields(scanner.Text())
		if len(w) == 0 {
			continue
		}
		switch {
		case w[0] == "SET" && len(w) == 3:
			h.put(w[1], w[2])
			fmt.Println("OK")
		case w[0] == "GET" && len(w) == 2:
			if v, ok := h.get(w[1]); ok {
				fmt.Println(v)
			} else {
				fmt.Println("-1")
			}
		case w[0] == "DEL" && len(w) == 2:
			if h.erase(w[1]) {
				fmt.Println("TRUE")
			} else {
				fmt.Println("FALSE")
			}
		case w[0] == "STATS" && len(w) == 1:
			method := "chain"
			if cuckoo {
				method = "cuckoo"
			}
			fmt.Printf("%s size=%d capacity=%d\n", method, h.count, h.capacity())
		default:
			return errors.New("hash commands: SET key value, GET key, DEL key, STATS")
		}
	}
	return scanner.Err()
}
func labBenchmark(n int) error {
	for _, cuckoo := range []bool{false, true} {
		h := newLabHash(cuckoo)
		start := time.Now()
		for i := 0; i < n; i++ {
			h.put("key"+strconv.Itoa(i), strconv.Itoa(i))
		}
		inserted := time.Now()
		for i := 0; i < n; i++ {
			v, ok := h.get("key" + strconv.Itoa(i))
			if !ok || v != strconv.Itoa(i) {
				return errors.New("benchmark verification failed")
			}
		}
		found := time.Now()
		method := "chain"
		if cuckoo {
			method = "cuckoo"
		}
		fmt.Printf("%s n=%d insert_us=%d lookup_us=%d capacity=%d\n", method, n, inserted.Sub(start).Microseconds(), found.Sub(inserted).Microseconds(), h.capacity())
	}
	return nil
}
func labUsage() error {
	return errors.New("usage: go run . lab2 rpn EXPR | --file FILE --query 'SETADD name value' | partition SUM NUMBERS... | subsets VALUES... | balanced NUMBERS... 0 | huffman encode TEXT | huffman decode ARCHIVE | hash chain|cuckoo | benchmark N | isomorphic A B | lfu CAPACITY")
}
func runLab2(a []string) error {
	if len(a) == 0 {
		return labUsage()
	}
	switch {
	case len(a) == 4 && a[0] == "--file" && a[2] == "--query":
		result, err := labSetQuery(a[1], a[3])
		if err == nil {
			fmt.Println(result)
		}
		return err
	case a[0] == "rpn" && len(a) > 1:
		result, err := labRPN(strings.Join(a[1:], " "))
		if err == nil {
			fmt.Println(result)
		}
		return err
	case a[0] == "partition" && len(a) > 2:
		target, err := labUint(a[1])
		if err != nil {
			return err
		}
		groups, err := labPartition(target, a[2:])
		if err != nil {
			return err
		}
		for _, group := range groups {
			parts := make([]string, len(group))
			for i, n := range group {
				parts[i] = strconv.FormatUint(n, 10)
			}
			fmt.Println("{" + strings.Join(parts, " ") + "}")
		}
		return nil
	case a[0] == "subsets":
		for _, subset := range labSubsets(a[1:]) {
			fmt.Println(subset)
		}
		return nil
	case a[0] == "balanced":
		okay, err := labBalanced(a[1:])
		if err != nil {
			return err
		}
		if okay {
			fmt.Println("YES")
		} else {
			fmt.Println("NO")
		}
		return nil
	case len(a) == 3 && a[0] == "huffman" && a[1] == "encode":
		fmt.Println(labHuffEncode(a[2]))
		return nil
	case len(a) == 3 && a[0] == "huffman" && a[1] == "decode":
		result, err := labHuffDecode(a[2])
		if err == nil {
			fmt.Println(result)
		}
		return err
	case len(a) == 2 && a[0] == "hash" && (a[1] == "chain" || a[1] == "cuckoo"):
		return labHashSession(a[1] == "cuckoo")
	case len(a) == 2 && a[0] == "benchmark":
		n, err := labInt(a[1])
		if err != nil || n < 0 {
			return errors.New("N must be non-negative")
		}
		return labBenchmark(int(n))
	case len(a) == 3 && a[0] == "isomorphic":
		if labIsomorphic(a[1], a[2]) {
			fmt.Println("YES")
		} else {
			fmt.Println("NO")
		}
		return nil
	case len(a) == 2 && a[0] == "lfu":
		n, err := labInt(a[1])
		if err != nil || n < 0 {
			return errors.New("capacity must be non-negative")
		}
		cache := newLabLFU(int(n))
		scanner := bufio.NewScanner(os.Stdin)
		for scanner.Scan() {
			w := strings.Fields(scanner.Text())
			if len(w) == 0 {
				continue
			}
			if w[0] == "SET" && len(w) == 3 {
				cache.set(w[1], w[2])
			} else if w[0] == "GET" && len(w) == 2 {
				fmt.Println(cache.get(w[1]))
			} else {
				return errors.New("LFU commands: SET key value, GET key")
			}
		}
		return scanner.Err()
	}
	return labUsage()
}
