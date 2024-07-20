use declio::ctx::Endian;
use declio::{Decode, Encode};
use std::fs::File;
use std::io::Read;
//Super experimental dumb tooling for parsing out the ANC data
// This is not production code, expect panics

const AUDIO_START: usize = 0x003EE000;
const AUDIO_LEN: usize = 16;
const AUD_IIR_NUM: usize = 8;
const AUD_COEF_LEN: usize = 0;

#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct AudSectionHead {
    #[declio(ctx = "Endian::Little")]
    magic: u16,
    #[declio(ctx = "Endian::Little")]
    vesrion: u16,
    #[declio(ctx = "Endian::Little")]
    crc: u32,
    #[declio(ctx = "Endian::Little")]
    reserved0: u32,
    #[declio(ctx = "Endian::Little")]
    reserved1: u32,
}
#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct AudSectionBodyIdent {
    anc_ver: [u8; 16], // These are actually char* but honestly doesnt matter as seem to always be 0x00
    batch_info: [u8; 16], // These are actually char* but honestly doesnt matter as seem to always be 0x00
    serial: [u8; 16], // These are actually char* but honestly doesnt matter as seem to always be 0x00
}

#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct anc_iir_coefs {
    #[declio(ctx = "Endian::Little")]
    coef_b: [i32; 3],
    #[declio(ctx = "Endian::Little")]
    coef_a: [i32; 3],
}
#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct aud_item {
    #[declio(ctx = "Endian::Little")]
    total_gain: u32,

    #[declio(ctx = "Endian::Little")]
    iir_bypass_flag: u16,

    #[declio(ctx = "Endian::Little")]
    iir_counter: u16,
    iir_coef: [anc_iir_coefs; AUD_IIR_NUM],

    // #if (AUD_SECTION_STRUCT_VERSION == 1)
    // #[declio(ctx = "Endian::Little")]
    // fir_bypass_flag: u16,
    // #[declio(ctx = "Endian::Little")]
    // fir_len: u16,
    // #[declio(ctx = "Endian::Little")]
    // fir_coef: [i16; AUD_COEF_LEN],
    // pos_tab: [i8; 16],
    // #elif (AUD_SECTION_STRUCT_VERSION == 2)
    #[declio(ctx = "Endian::Little")]
    reserved_for_drc: [u32; 32],
    // #elif (AUD_SECTION_STRUCT_VERSION == 3)

    // #endif
    #[declio(ctx = "Endian::Little")]
    reserved1: i16,
    dac_gain_offset: i8, // in qdb (quater of dB)
    adc_gain_offset: i8, // in qdb (quater of dB)
}

#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct struct_anc_cfg {
    //V1+
    anc_cfg_ff_l: aud_item,
    anc_cfg_ff_r: aud_item,
    anc_cfg_fb_l: aud_item,
    anc_cfg_fb_r: aud_item,
    //V2
    anc_cfg_tt_l: aud_item,
    anc_cfg_tt_r: aud_item,
    anc_cfg_mc_l: aud_item,
    anc_cfg_mc_r: aud_item,
}

#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct pctool_struct_anc_cfg {
    //
    anc_cfg: [struct_anc_cfg; 2], //0 == 41k, 2==44k
}

#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct AudSectionBodyConfig {
    anc_config_arr: [pctool_struct_anc_cfg; 1], // we may only use first of 4 entries
}
#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct AudSectionBody {
    anc_ident: AudSectionBodyIdent,
    anc_config: AudSectionBodyConfig,
}
#[derive(Debug, PartialEq, Encode, Decode, Copy, Clone, Default)]
struct PctoolAudSection {
    header: AudSectionHead,
    body: AudSectionBody,
}

fn main() {
    println!("Super dumb anc firmware details dumper");
    println!("By Ralim <ralim@ralimtek.com>");
    println!("Reading firmware.bin");
    let file_contents = get_firmware_file("firmware.bin");
    println!("Running decoder");

    let dummy_init: PctoolAudSection = PctoolAudSection::default();
    let dummy_serialised = declio::to_bytes(dummy_init).unwrap();

    let decoded: PctoolAudSection =
        declio::from_bytes(&file_contents[AUDIO_START..AUDIO_START + dummy_serialised.len()])
            .expect("decode failed");

    println!("Decoded {:#?}", decoded)
}

fn get_firmware_file(filename: &str) -> Vec<u8> {
    let mut f = File::open(&filename).expect("no file found");
    let mut buffer = Vec::new();

    // read the whole file
    f.read_to_end(&mut buffer).expect("could not read file");

    buffer
}
