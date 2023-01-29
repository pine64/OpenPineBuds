#!/bin/sh
txt_to_wav() {
  hash=$(echo $arg1 | md5sum | cut -d" " -f1)
  xxd -r -p $arg1 > out-$hash.sbc
  # sbcinfo out-$hash.sbc
  ffmpeg -y \
    -v info         `# verbosity - other options are "quiet", "error", "panic"` \
    -f sbc          `# accept SBC format` \
    -ac 1           `# audio channel: #1` \
    -i ./out-$hash.sbc    `# input file: out.raw` \
    $arg2           `# output to $arg2`
  rm out-$hash.sbc
}

audio_to_txt() {
  hash=$(echo $arg1 | md5sum | cut -d" " -f1)
  ffmpeg -y \
    -v info            `# verbosity - other options are "quiet", "error", "panic"` \
    -i $arg1           `#input file: $arg1` \
    -f sbc             `#output format: SBC` \
    -ar 16000          `# audio rate: 16 kHz` \
    -ac 1              `# audio channel: #1` \
    -aq 16             `# audio quality: 16 (for SBC this means bitpool=16)` \
    -map_metadata -1   `# ????` \
    out-$hash.sbc            `# output to out.raw`

  # sbcinfo out-$hash.sbc
  xxd -i out-$hash.sbc            `# output in C include file style` \
    | head -n -2              `# skip last two xxd outline lines (skip C formatting)` \
    | tail -n +2              `# start output on line 2 of xxd output (skip C formatting)` \
    | sed 's/ //g'            `# remove spaces` \
    | tr --delete '\n'        `# remove newlines` \
    | sed 's/,/\,\n/16; P; D' `# collect into lines with the right length` \
    > $arg2
  rm out-$hash.sbc
}

audio_to_cpp() {
  hash=$(echo $arg1 | md5sum | cut -d" " -f1)
  filename=$(out-$hash.sbc)
  varname=$(echo $arg1 | rev | cut -c5- | cut -d"/" -f1,2 | rev | tr '[:lower:]' '[:upper:]' | tr "/" _)
  ffmpeg -y \
    -v info            `# verbosity - other options are "quiet", "error", "panic"` \
    -i $arg1           `#input file: $arg1` \
    -f sbc             `#output format: SBC` \
    -ar 16000          `# audio rate: 16 kHz` \
    -ac 1              `# audio channel: #1` \
    -aq 16             `# audio quality: 16 (for SBC this means bitpool=16)` \
    -map_metadata -1   `# ????` \
    $varname            `# output to something like EN_SOUND_POWER_ON`

  echo "#include <stdint.h>\n" > $arg2

  # sbcinfo out-$hash.sbc
  xxd -i $varname            `# output in C include file style` \
    >> $arg2

  sed -i "s/unsigned char/uint8_t/g" $arg2
  
  #rm $filename
}

[ "${1}" = "-T" ] || [ "${1}" = "--txt-to-wav"   ] && shift 1 && args=$@ && arg1=$(echo $args | cut -d" " -f1) && arg2=$(echo $args | cut -d" " -f2) && txt_to_wav && exit
[ "${1}" = "-A" ] || [ "${1}" = "--audio-to-txt" ] && shift 1 && args=$@ && arg1=$(echo $args | cut -d" " -f1) && arg2=$(echo $args | cut -d" " -f2) && audio_to_txt && exit
[ "${1}" = "-C" ] || [ "${1}" = "--audio-to-cpp" ] && shift 1 && args=$@ && arg1=$(echo $args | cut -d" " -f1) && arg2=$(echo $args | cut -d" " -f2) && audio_to_cpp && exit
echo "
Sound format converter:
Usage:
  ./convert.sh [option] [input-file] [output-file]
Options:
-T or --txt-to-wav   Converts the text file to a wav audio file
-A or --audio-to-txt Converts a wav file to a file readable by the old pinebuds firmware
-C or --audio-to-cpp Converts a wav file to a file compilable into the pinebuds firmware
"
