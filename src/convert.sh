#!/usr/bin/env sh

txt_to_wav() {
  hash="$(md5sum "$1" | cut -d ' ' -f 1)"
  filename="out-$hash.sbc"

  xxd -r -p "$1" > "$filename"

  ffmpeg -y \
    -v quiet       `# verbosity - other options are "quiet", "error", "panic"` \
    -f sbc         `# accept SBC format` \
    -ac 1          `# audio channel: #1` \
    -i "$filename" `# input file: out-$hash.sbc` \
    "$2"           `# output to $2`

  rm "$filename"
}

audio_to_txt() {
  hash="$(md5sum "$1" | cut -d ' ' -f 1)"
  filename="out-$hash.sbc"

  ffmpeg -y \
    -v quiet                  `# verbosity - other options are "quiet", "error", "panic"` \
    -i "$1"                   `# input file: $1` \
    -f sbc                    `# output format: SBC` \
    -ar 16000                 `# audio rate: 16 kHz` \
    -ac 1                     `# audio channel: #1` \
    -aq 16                    `# audio quality: 16 (for SBC this means bitpool=16)` \
    -map_metadata -1          `# strip metadata` \
    "$filename"               `# output to out-$hash.sbc`

  xxd -i "$filename"          `# output in C include file style` \
    | head -n -2              `# skip last two xxd outline lines (skip C formatting)` \
    | tail -n +2              `# start output on line 2 of xxd output (skip C formatting)` \
    | sed 's/ //g'            `# remove spaces` \
    | tr --delete '\n'        `# remove newlines` \
    | sed 's/,/\,\n/16; P; D' `# collect into lines with the right length` \
    > "$2"

  rm "$filename"
}

audio_to_cpp() {
  varname="$(basename "$1" | rev | cut -f 2- -d '.' | rev | tr '[:lower:]/' '[:upper:]_')"

  ffmpeg -y \
    -v quiet         `# verbosity - other options are "quiet", "error", "panic"` \
    -i "$1"          `# input file: $1` \
    -f sbc           `# output format: SBC` \
    -ar 16000        `# audio rate: 16 kHz` \
    -ac 1            `# audio channel: #1` \
    -aq 16           `# audio quality: 16 (for SBC this means bitpool=16)` \
    -map_metadata -1 `# strip metadata` \
    "$varname"       `# output to something like EN_SOUND_POWER_ON`

  printf '#include <stdint.h>\n' > "$2"

  xxd -i "$varname"  `# output in C include file style` \
    >> "$2"

  sed -i 's/unsigned char/uint8_t/g' "$2"

  rm "$varname"
}

[ "$1" = '-T' ] || [ "$1" = '--txt-to-wav'   ] && shift 1 && txt_to_wav "$@" && exit
[ "$1" = '-A' ] || [ "$1" = '--audio-to-txt' ] && shift 1 && audio_to_txt "$@" && exit
[ "$1" = '-C' ] || [ "$1" = '--audio-to-cpp' ] && shift 1 && audio_to_cpp "$@" && exit

echo "
Sound format converter:
Usage:
  $0 [option] [input-file] [output-file]
Options:
  -T or --txt-to-wav   Converts the text file to a wav audio file
  -A or --audio-to-txt Converts an audio file to a file readable by the old pinebuds firmware
  -C or --audio-to-cpp Converts an audio file to a file compilable into the pinebuds firmware
"
