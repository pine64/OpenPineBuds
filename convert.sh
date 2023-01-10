#!/bin/sh
txt_to_wav() {
  xxd -r -p $arg1 > out.raw
  ffmpeg  \
    -v info         `# verbosity - other options are "quiet", "error", "panic"` \
    -f sbc          `# accept SBC format` \
    -ac 1           `# audio channel: #1` \
    -i ./out.raw    `# input file: out.raw` \
    $arg2           `# output to $arg2`
  rm ./out.raw
}

wav_to_txt() {
  ffmpeg \
    -v info            `# verbosity - other options are "quiet", "error", "panic"` \
    -i $arg1           `#input file: $arg1` \
    -f sbc             `#output format: SBC` \
    -ar 16000          `# audio rate: 16 kHz` \
    -ac 1              `# audio channel: #1` \
    -aq 16             `# audio quality: 16 (for SBC this means bitpool=16)` \
    -map_metadata -1   `# ????` \
    out.raw            `# output to out.raw`

  xxd -i ./out.raw            `# output in C include file style` \
    | head -n -2              `# skip last two xxd outline lines (skip C formatting)` \
    | tail -n +2              `# start output on line 2 of xxd output (skip C formatting)` \
    | sed 's/ //g' \          `# remove spaces` \
    | tr --delete '\n' \      `# remove newlines` \
    | sed 's/,/\,\n/16; P; D' `# collect into lines with the right length` \
    > $arg2
  rm ./out.raw
}

[ "${1}" = "-T" ] || [ "${1}" = "--txt-to-wav" ] && shift 1 && args=$@ && arg1=$(echo $args | cut -d" " -f1) && arg2=$(echo $args | cut -d" " -f2) && txt_to_wav && exit
[ "${1}" = "-W" ] || [ "${1}" = "--wav-to-txt" ] && shift 1 && args=$@ && arg1=$(echo $args | cut -d" " -f1) && arg2=$(echo $args | cut -d" " -f2) &&  wav_to_txt && exit
echo "
Sound format converter:
Usage:
  ./convert.sh [option] [input-file] [output-file]
Options:
-T or --txt-to-wav   Converts the text file to a wav audio file
-W or --wav-to-txt Converts a wav file to a file readable by the pinebuds firmware
"
