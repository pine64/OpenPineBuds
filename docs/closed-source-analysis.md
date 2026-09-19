# What the prebuilt libraries hide

A community analysis of the closed `*.a` archives this tree links but does not ship sources for.
It documents *build metadata* only - what the compiler left inside the binaries - and is meant as
evidence for the relicensing effort (#92, #93) and as a map of what is implemented where.
No vendor source, no decompiled code and no binaries are included in this document.

Analysis and tooling: [@WebWire-NL](https://github.com/WebWire-NL), full working material in
<https://github.com/WebWire-NL/openpinebuds-integration>.

## The archives were built with debug info

23 archives, 619 objects, 3,158,128 bytes of `.text` across all library variants, GCC 9.2.1
(ARM 9-2019-q4-major) with `-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -g -O2
-std=gnu99 -ffunction-sections -fdata-sections -fstack-protector-strong`.

Because of `-g`, each archive still carries DWARF: compilation-unit source paths, function names
with declaration line numbers, line tables, type names and per-function section sizes. Four
different BES build trees are named in `DW_AT_comp_dir`, e.g.
`/home/wangzhiq/BES-project/GIT-project/xingqidian-1302-ibrt/mcu-sw-old-jiangxingbo/out/best2300p_ibrt_anc`.

## 498 source files are named by the blobs but absent from this tree

| files | directory |
|---|---:|
| 174 | `services/multimedia/audio/codec` |
| 130 | `services/multimedia/speech/src` |
| 34 | `services/ble_stack/hl/src` |
| 27 | `services/multimedia/audio/process` |
| 14 | `thirdparty/audio_codec_lib/ldac/src` |
| 9 | `thirdparty/audio_codec_lib/liblhdc-dec/dec_src` |
| 7 | `services/ble_stack/ke/src` |
| 6 | `services/bt_profiles_enhanced/protocols/btm` |
| 5 | `services/bt_profiles_enhanced/protocols/obex` |
| 4 | `services/ble_stack/hci/src` |
| 4 | `services/bt_profiles_enhanced/protocols/hshf` |
| 3 | `services/bt_profiles_enhanced/protocols/besaud` |
| 3 | `services/bt_profiles_enhanced/protocols/map` |
| 3 | `services/bt_profiles_enhanced/protocols/misc` |
| 3 | `services/bt_profiles_enhanced/protocols/pbap` |

The tree ships headers and configuration for these modules (`services/multimedia/speech/inc`,
`.../audio/process/*/include`, `.../anc/cfg`) but not the implementations.

## How much API the archive exposes

| | count |
|---|---:|
| exported functions / data objects | 4,723 / 1,003 |
| declared in a header that ships in this tree | 2,473 (52%) |
| referenced by open sources in this tree | 793 |
| in no shipped header and called by nothing | 2,186 |
| name-mangled C++ symbols (Fraunhofer FDK-AAC) | 1,282 |
| exported functions with **no** header anywhere in this tree | 301 (180 external, 121 static) |

The FDK-AAC decoder also exports its C API there (`aacDecoder_Open`, `aacDecoder_Fill`,
`aacDecoder_DecodeFrame`, `aacDecoder_SetParam`).

## What the closed code needs from the rest of the firmware

8,669 undefined references: 4,079 resolve to symbols this tree defines, 4,236 to other blobs, 354
to neither (ROM, hardware libraries, libgcc). The open-side contract is small - trace
(`hal_trace_printf`, 264 blobs), the heap (`med_malloc`/`med_calloc`/`med_free`), the RTOS
(`osapi_lock_stack`, `osTimerCreate`), `app_tws_ibrt_get_bt_ctrl_ctx` and libc.

## Interfaces can be recovered, implementations cannot

The DWARF turns back into declarations: 5,374 prototypes across the multimedia archive, and 593 in
the ANC/DSP subset, with no unresolvable return types - cross-checked against headers that do ship
(e.g. the reconstructed `adp_spx_*` prototypes match `audio/process/adp/include/adp_fftwrap.h`).

## Licensing boundary

Parts of the closed set are third-party code that cannot be relicensed by anyone: Fraunhofer
FDK-AAC (`services/multimedia/audio/codec/fdkaac_codec`), Savitech LHDC
(`BEST2300P_LibLHDC_V2_V3_3_1_0_SAVI_KEYPRO_UUID.a`) and Sony LDAC (`thirdparty/audio_codec_lib/ldac`).
Any licensing outcome has to account for those separately.

## Reproducing

```
scripts/blobmap.py        /path/to/OpenPineBuds   # inventory, DWARF, code size
scripts/apisurface.py     /path/to/OpenPineBuds   # exported and required symbol surface
scripts/dwarf_protos.py   lib.a [object-regex] [out.md]
scripts/publish_headerless.py /path/to/OpenPineBuds
```

Outputs, machine-readable inventory and generated declarations:
<https://github.com/WebWire-NL/openpinebuds-integration/releases/tag/v0.1.0>.
