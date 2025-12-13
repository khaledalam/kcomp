#!/usr/bin/env bash
set -euo pipefail

bin="${BIN:-./build/kcomp}"
test_dir="test_data"
passed=0
failed=0

mkdir -p "$test_dir"

green=$'\033[32m'
red=$'\033[31m'
blue=$'\033[34m'
reset=$'\033[0m'

run_test() {
  local name="$1"
  local input_file="$2"

  printf "%-30s " "$name..."

  if ! "$bin" c "$input_file" "$test_dir/output.kcomp" >/dev/null 2>&1; then
    echo "${red}FAIL${reset} (compression)"
    ((failed++))
    return
  fi

  if ! "$bin" d "$test_dir/output.kcomp" "$test_dir/restored.txt" >/dev/null 2>&1; then
    echo "${red}FAIL${reset} (decompression)"
    ((failed++))
    return
  fi

  if ! cmp -s "$input_file" "$test_dir/restored.txt"; then
    echo "${red}FAIL${reset} (mismatch)"
    ((failed++))
    return
  fi

  echo "${green}PASS${reset}"
  ((passed++))
  rm -f "$test_dir/output.kcomp" "$test_dir/restored.txt"
}

echo "${blue}kcomp test suite${reset}"
echo

echo "a" > "$test_dir/t1.txt"
run_test "single byte" "$test_dir/t1.txt"

printf "aaaaaaaaaa" > "$test_dir/t2.txt"
run_test "repeated bytes" "$test_dir/t2.txt"

echo "hello world" > "$test_dir/t3.txt"
run_test "simple text" "$test_dir/t3.txt"

echo "The quick brown fox jumps over the lazy dog" > "$test_dir/t4.txt"
run_test "pangram" "$test_dir/t4.txt"

printf 'line1\nline2\nline3\n' > "$test_dir/t5.txt"
run_test "newlines" "$test_dir/t5.txt"

printf 'x%.0s' {1..500} > "$test_dir/t6.txt"
run_test "500 bytes repetition" "$test_dir/t6.txt"

dd if=/dev/zero of="$test_dir/t7.bin" bs=512 count=1 2>/dev/null
run_test "512 zeros" "$test_dir/t7.bin"

if [ -f "enwik_10k" ]; then
  run_test "enwik_10k" "enwik_10k"
fi

echo
echo "────────────────────────────────"
echo "Total: ${green}$passed passed${reset}, ${red}$failed failed${reset}"

rm -rf "$test_dir"

[ $failed -eq 0 ]
