#!/usr/bin/env bash
set -euo pipefail

bin="${BIN:-./build/kcomp}"
datasets=("enwik_10k" "enwik_100k" "enwik_1m")

bold=$'\033[1m'
green=$'\033[32m'
red=$'\033[31m'
yellow=$'\033[33m'
blue=$'\033[34m'
dim=$'\033[2m'
reset=$'\033[0m'

mkdir -p outputs

hr() {
  printf "${dim}%s${reset}\n" "────────────────────────────────────────────────────────────────────────"
}

bytes() {
  stat -f "%z" "$1"
}

ratio() {
  awk -v a="$1" -v b="$2" 'BEGIN { if (a==0) { printf "0.00" } else { printf "%.2f", (b*100.0/a) } }'
}

run_time() {
  /usr/bin/time -p bash -c "$1" 1>/dev/null 2>&1 | awk '/^real /{print $2}'
}

verify() {
  if cmp -s "$1" "$2"; then
    printf "${green}OK${reset}"
  else
    printf "${red}BAD${reset}"
  fi
}

print_header() {
  printf "${bold}%-14s %12s  %8s  %10s  %10s  %s${reset}\n" \
    "algorithm" "compressed" "ratio" "encode" "decode" "verify"
}

print_row() {
  local name="$1" in_sz="$2" out_file="$3" enc_s="$4" dec_s="$5" ok="$6"
  local out_sz rat
  out_sz=$(bytes "$out_file" 2>/dev/null || echo "0")
  rat=$(ratio "$in_sz" "$out_sz")

  printf "%-14s %12s  %8s  %10s  %10s  %s\n" \
    "$name" \
    "${out_sz}B" \
    "${rat}%" \
    "${enc_s}s" \
    "${dec_s}s" \
    "$ok"
}

need() {
  command -v "$1" >/dev/null 2>&1
}

bench_dataset() {
  local input="$1"

  if [ ! -f "$input" ]; then
    echo "${red}Dataset not found: $input${reset}"
    return
  fi

  local in_sz=$(bytes "$input")

  printf "\n${bold}${blue}Dataset: $input${reset} ${dim}(${in_sz} bytes)${reset}\n"
  hr
  print_header
  hr

  rm -f outputs/out.* outputs/restored.* 2>/dev/null || true

  enc=$(run_time "\"$bin\" c \"$input\" outputs/out.kcomp")
  dec=$(run_time "\"$bin\" d outputs/out.kcomp outputs/restored.kcomp")
  ok=$(verify "$input" outputs/restored.kcomp)
  print_row "kcomp (PPM2)" "$in_sz" outputs/out.kcomp "$enc" "$dec" "$ok"

  if need gzip; then
    enc=$(run_time "gzip -c -9 \"$input\" > outputs/out.gz")
    dec=$(run_time "gzip -cd outputs/out.gz > outputs/restored.gz")
    ok=$(verify "$input" outputs/restored.gz)
    print_row "gzip -9" "$in_sz" outputs/out.gz "$enc" "$dec" "$ok"
  fi

  if need brotli; then
    enc=$(run_time "brotli -q 11 -c \"$input\" > outputs/out.br")
    dec=$(run_time "brotli -d -c outputs/out.br > outputs/restored.br")
    ok=$(verify "$input" outputs/restored.br)
    print_row "brotli -11" "$in_sz" outputs/out.br "$enc" "$dec" "$ok"
  fi

  if need zstd; then
    enc=$(run_time "zstd -19 -c \"$input\" > outputs/out.zst")
    dec=$(run_time "zstd -d -c outputs/out.zst > outputs/restored.zst")
    ok=$(verify "$input" outputs/restored.zst)
    print_row "zstd -19" "$in_sz" outputs/out.zst "$enc" "$dec" "$ok"
  fi

  if need xz; then
    enc=$(run_time "xz -9e -c \"$input\" > outputs/out.xz")
    dec=$(run_time "xz -d -c outputs/out.xz > outputs/restored.xz")
    ok=$(verify "$input" outputs/restored.xz)
    print_row "xz -9e" "$in_sz" outputs/out.xz "$enc" "$dec" "$ok"
  fi

  hr
}

printf "${bold}kcomp multi-dataset benchmark${reset}\n"

for dataset in "${datasets[@]}"; do
  bench_dataset "$dataset"
done

printf "\n${dim}tip:${reset} override kcomp path: ${bold}BIN=./build/kcomp ./benchmark_multi.sh${reset}\n"
