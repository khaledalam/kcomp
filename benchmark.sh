#!/usr/bin/env bash
set -euo pipefail

input="${1:-enwik_10k}"
bin="${BIN:-./build/kcomp}"

mkdir -p outputs
rm -f outputs/out.* outputs/restored.* 2>/dev/null || true

bold=$'\033[1m'
green=$'\033[32m'
red=$'\033[31m'
yellow=$'\033[33m'
blue=$'\033[34m'
dim=$'\033[2m'
reset=$'\033[0m'

hr() {
  printf "${dim}%s${reset}\n" "────────────────────────────────────────────────────────────"
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

print_row() {
  local name="$1" out_file="$2" enc_s="$3" dec_s="$4" ok="$5"
  local in_sz out_sz rat
  in_sz=$(bytes "$input")
  out_sz=$(bytes "$out_file")
  rat=$(ratio "$in_sz" "$out_sz")

  printf "%-14s %10s  %8s  %10s  %10s  %s\n" \
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

in_sz=$(bytes "$input")

printf "${bold}kcomp benchmark${reset}\n"
printf "${dim}input:${reset} %s (%sB)\n" "$input" "$in_sz"
hr

printf "${bold}%-14s %10s  %8s  %10s  %10s  %s${reset}\n" "algo" "out" "ratio" "enc" "dec" "verify"

hr

printf "${blue}${bold}kcomp (PPM1)${reset}\n" > /dev/null
enc=$(run_time "\"$bin\" c \"$input\" outputs/out.kcomp")
dec=$(run_time "\"$bin\" d outputs/out.kcomp outputs/restored.kcomp")
ok=$(verify "$input" outputs/restored.kcomp)
print_row "kcomp" outputs/out.kcomp "$enc" "$dec" "$ok"

if need gzip; then
  enc=$(run_time "gzip -c -9 \"$input\" > outputs/out.gz")
  dec=$(run_time "gzip -cd outputs/out.gz > outputs/restored.gz")
  ok=$(verify "$input" outputs/restored.gz)
  print_row "gzip -9" outputs/out.gz "$enc" "$dec" "$ok"
else
  print_row "gzip -9" /dev/null "-" "-" "${yellow}skip${reset}"
fi

if need brotli; then
  enc=$(run_time "brotli -q 11 -c \"$input\" > outputs/out.br")
  dec=$(run_time "brotli -d -c outputs/out.br > outputs/restored.br")
  ok=$(verify "$input" outputs/restored.br)
  print_row "brotli 11" outputs/out.br "$enc" "$dec" "$ok"
else
  print_row "brotli 11" /dev/null "-" "-" "${yellow}skip${reset}"
fi

if need zstd; then
  enc=$(run_time "zstd -19 -c \"$input\" > outputs/out.zst")
  dec=$(run_time "zstd -d -c outputs/out.zst > outputs/restored.zst")
  ok=$(verify "$input" outputs/restored.zst)
  print_row "zstd -19" outputs/out.zst "$enc" "$dec" "$ok"
else
  print_row "zstd -19" /dev/null "-" "-" "${yellow}skip${reset}"
fi

if need xz; then
  enc=$(run_time "xz -9e -c \"$input\" > outputs/out.xz")
  dec=$(run_time "xz -d -c outputs/out.xz > outputs/restored.xz")
  ok=$(verify "$input" outputs/restored.xz)
  print_row "xz -9e" outputs/out.xz "$enc" "$dec" "$ok"
else
  print_row "xz -9e" /dev/null "-" "-" "${yellow}skip${reset}"
fi

hr
printf "${dim}tip:${reset} run with another file: ${bold}./benchmark.sh path/to/file${reset}\n"
printf "${dim}tip:${reset} override kcomp path: ${bold}BIN=./build/kcomp ./benchmark.sh${reset}\n"