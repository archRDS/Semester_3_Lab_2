"""python verify.py path/to/lab2-cpp.exe path/to/lab2-go.exe"""
import random
import subprocess
import sys
import tempfile
from pathlib import Path

programs = [[sys.argv[1]], [sys.argv[2], "lab2"]]
checks = 0

def run(program, args, data="", success=True):
    global checks
    p = subprocess.run(program + args, input=data, capture_output=True, encoding="utf-8")
    assert (p.returncode == 0) == success, (args, p.stderr)
    checks += 1
    return p.stdout.strip()

for program in programs:
    for expression, answer in [("3 1 4 * +", "7"), ("314*+", "7"), ("-12 3 *", "-36")]:
        assert run(program, ["rpn", expression]) == answer
    for expression in ["1 +", "9223372036854775807 1 +"]:
        run(program, ["rpn", expression], success=False)
    for target in (10, 15):
        text = run(program, ["partition", str(target), "4", "10", "5", "1", "3", "7"])
        groups = [[int(x) for x in line.strip("{}").split()] for line in text.splitlines()]
        assert all(sum(g) == target for g in groups)
        assert sorted(x for g in groups for x in g) == [1, 3, 4, 5, 7, 10]
    run(program, ["partition", "4", "1", "2"], success=False)
    subsets = run(program, ["subsets", "x", "y", "z"]).splitlines()
    assert len(subsets) == len(set(subsets)) == 8 and "{}" in subsets
    assert len(run(program, ["subsets", "x", "x"]).splitlines()) == 3
    assert run(program, ["balanced", "7", "3", "2", "1", "9", "5", "4", "6", "8", "0"]) == "YES"
    assert run(program, ["balanced", "1", "2", "3", "0"]) == "NO"
    assert run(program, ["balanced", "0"]) == "YES"
    for text in ("abracadabra", "aaaa", "", "Привет, мир!"):
        archive = run(program, ["huffman", "encode", text])
        for decoder in programs:
            assert run(decoder, ["huffman", "decode", archive]) == text
    run(program, ["huffman", "decode", "61:2|1"], success=False)
    for a, b, answer in [("fall", "redd", "YES"), ("mad", "odd", "NO"), ("dad", "ada", "YES"), ("ab", "aa", "NO"), ("", "", "YES"), ("ааб", "яяю", "YES")]:
        assert run(program, ["isomorphic", a, b]) == answer
    rng = random.Random(42)
    for method in ("chain", "cuckoo"):
        reference, commands, expected = {}, [], []
        for i in range(2000):
            commands.append(f"SET k{i} {i}"); reference[f"k{i}"] = str(i); expected.append("OK")
        for _ in range(4000):
            key, op = f"k{rng.randrange(2500)}", rng.choice(["SET", "GET", "DEL"])
            if op == "SET":
                value = str(rng.randrange(10000)); commands.append(f"SET {key} {value}"); reference[key] = value; expected.append("OK")
            elif op == "GET":
                commands.append(f"GET {key}"); expected.append(reference.get(key, "-1"))
            else:
                commands.append(f"DEL {key}"); expected.append("TRUE" if key in reference else "FALSE"); reference.pop(key, None)
        assert run(program, ["hash", method], "\n".join(commands)).splitlines() == expected
    for capacity in (0, 1, 3):
        cache, commands, expected, tick = {}, [], [], 0
        for _ in range(500):
            key = str(rng.randrange(8)); tick += 1
            if rng.randrange(2):
                value = str(rng.randrange(100)); commands.append(f"SET {key} {value}")
                if not capacity: continue
                if key in cache: cache[key] = (value, cache[key][1] + 1, tick)
                else:
                    if len(cache) == capacity: del cache[min(cache, key=lambda k: cache[k][1:])]
                    cache[key] = (value, 1, tick)
            else:
                commands.append(f"GET {key}")
                if key in cache:
                    value, frequency, _ = cache[key]; expected.append(value); cache[key] = (value, frequency + 1, tick)
                else: expected.append("-1")
        assert run(program, ["lfu", str(capacity)], "\n".join(commands)).splitlines() == expected

with tempfile.TemporaryDirectory() as directory:
    path = str(Path(directory) / "sets.txt")
    for program, query, expected in [(programs[0], "SETADD s red", "TRUE"), (programs[1], "SET_AT s red", "TRUE"), (programs[1], "SETADD s red", "FALSE"), (programs[0], "SETDEL s red", "TRUE"), (programs[1], "SET_AT s red", "FALSE")]:
        assert run(program, ["--file", path, "--query", query]) == expected
print(f"PASS: {checks} runs, both languages, randomized hash/LFU, shared files and Huffman archives")
