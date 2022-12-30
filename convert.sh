#!/bin/sh
txt_to_wav() {
  xxd -r -p $args > out.raw
  ffmpeg -f sbc -ac 1 -i ./out.raw output.wav
  rm ./out.raw
}

wav_to_txt() {
  ffmpeg -i $args -f sbc -ac 16000 -ac 1 -map_metadata -1 out.raw
  xxd -i ./out.raw | head -n -2 \
    | tail -n +2 | sed 's/ //g' \
    | tr --delete '\n' \
    | sed 's/,/\,\n/16; P; D' > SOUND.txt
  rm ./out.raw
}

[ ${1} = "-T" ] || [ ${1} = "--txt-to-wav" ] && shift 1 && args=$@ && txt_to_wav && exit
[ ${1} = "-W" ] || [ ${1} = "--wav-to-txt" ] && shift 1 && args=$@ &&  wav_to_txt && exit
echo "
Sound format converter:
Usage:
  ./convert.sh [option] [input-file]
Options:
-T or --txt-to-wav   Converts the text file to a wav audio file (output.wav)
-W or --wav-to-txt Converts a wav file to a file readable by the pinebuds firmware (SOUND.txt)
"
