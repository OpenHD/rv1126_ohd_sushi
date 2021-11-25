// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef MPP_JPEG_FD_INFO_H_
#define MPP_JPEG_FD_INFO_H_
#if 0
static IFD_S IFD0[] =
{
    {0x010e, 2, 32, 0x0C, {{.asv="thumbnail_test"}}}, //picture info
    {0x010f, 2, 32, 0x0C, {{.asv="rockchip"}}}, //manufact info
    {0x0110, 2, 32, 0x0C, {{.asv="rockchip IP Camrea"}}},  //module info
    {0x0112, 3, 2, 0x0C, {{.uwv=1}}}, // '1' means upper left, '3' lower right, '6' upper right, '8' lower left, '9' undefined.
    {0x011a, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // x resolution 1/72inch,
    {0x011b, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // y resolution 1/72inch
    {0x0128, 3, 2, 0x0C, {{.uwv=2}}}, // resolution unt, '1' means no-unit, '2' means inch, '3' means centimeter
    {0x0131, 2, 32, 0x0C, {{.asv="rkmedia v1.8"}}}, // software version
    {0x0132, 2, 20, 0x0C, {{.asv="2021:04:27 17:10:50"}}}, // date
    {0x013e, 5, 8 * 2, 0x0C, {{.uddwv=0x0000271000000C37}, {.uddwv=0x0000271000000C37}}}, // WhitePoint the values are '3127/10000,3290/10000'.
    {0x013f, 5, 8 * 6, 0x0C, {{.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280}}},  // PrimaryChromaticities values are '640/1000,330/1000,300/1000,600/1000,150/1000,0/1000'
    {0x0211, 5, 8 * 3, 0x0C, {{.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280}}}, // YCbCrCoefficients In usual, values are '0.299/0.587/0.114
    {0x0213, 3, 2, 0x0C, {{.uwv=1}}}, // YCbCrPositioning '1' means the center of pixel array, '2' means the datum point.
    {0x0214, 5, 8 * 6, 0x0C, {{.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280}}}, // ReferenceBlackWhite
    {0x8298, 2, 32, 0x0C, {{.asv="rockchip"}}}, // copyright
    {0x8769, 4, 4, 0x0C, {{.udwv=0}}}, // Offset to Exif Sub IFD
};

static IFD_S subIFD[] =
{
    {0x829a, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // ExposureTime
    {0x829d, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // FNumber
    {0x8822, 3, 2, 0x0C, {{.uwv=1}}}, // '1' means manual control, '2' program normal, '3' aperture priority, '4' shutter priority, '5' program creative (slow program), '6' program action(high-speed program), '7' portrait mode, '8' landscape mode.
    {0x8827, 3, 2 * 2, 0x0C, {{.uwv=1},{.uwv=1}}}, // ISOSpeedRatings CCD sensitivity equivalent to Ag-Hr film speedrate.
    {0x9000, 7, 1 * 4, 0x0C, {{.asv="0210"}}}, // Exif version number
    {0x9003, 2, 20, 0x0C, {{.asv="2021:04:27 17:10:50"}}}, // DateTimeOriginal
    {0x9004, 2, 20, 0x0C, {{.asv="2021:04:27 17:10:50"}}}, // DateTimeDigitized
    {0x9101, 7, 1 * 4, 0x0C, {{.dwv=0x00010203}}}, // ComponentConfiguration
    {0x9102, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // CompressedBitsPerPixel
    {0x9201, 10, 8, 0x0C, {{.ddwv=0x0000004800000001}}}, // ShutterSpeedValue
    {0x9202, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // ApertureValue
    {0x9203, 10, 8, 0x0C, {{.ddwv=0x0000004800000001}}}, // BrightnessValue
    {0x9204, 10, 8, 0x0C, {{.ddwv=0x0000004800000001}}}, // ExposureBiasValue
    {0x9205, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // MaxApertureValue
    {0x9206, 10, 8, 0x0C, {{.ddwv=0x0000004800000001}}}, // SubjectDistance
    {0x9207, 3, 2, 0x0C, {{.uwv=1}}}, // MeteringMode
    {0x9208, 3, 2, 0x0C, {{.uwv=1}}}, // LightSource
    {0x9209, 3, 2, 0x0C, {{.uwv=1}}}, // Flash
    {0x920a, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // FocalLength
    {0x927c, 7, 1, 0x0C, {{.asv="0210"}}}, // MakerNote
    {0x9286, 7, 1, 0x0C, {{.asv="0210"}}}, // UserComment
    {0xa000, 7, 1 * 4, 0x0C, {{.asv="0210"},{.asv="0210"},{.asv="0210"},{.asv="0210"}}}, // FlashPixVersion
    {0xa001, 3, 2, 0x0C, {{.uwv=1}}}, // ColorSpace
    {0xa002, 3, 2, 0x0C, {{.uwv=1}}}, // ExifImageWidth
    {0xa003, 3, 2, 0x0C, {{.uwv=1}}}, // ExifImageHeight
    {0xa004, 2, 32, 0x0C, {{.asv="null"}}}, // RelatedSoundFile
    {0xa005, 4, 4, 0x0C, {{.udwv=0}}}, // Extension of "ExifR98"
    {0xa20e, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // FocalPlaneXResolution
    {0xa20f, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // FocalPlaneYResolution
    {0xa210, 3, 2, 0x0C, {{.uwv=1}}}, // FocalPlaneResolutionUnit
    {0xa217, 3, 2, 0x0C, {{.uwv=1}}}, // SensingMethod
    {0xa300, 7, 1, 0x0C, {{.asv="0210"}}}, // FileSource
    {0xa301, 7, 1, 0x0C, {{.asv="0210"}}}, // SceneType
};

static IFD_S IFD1[] =
{
    {0x0100, 3, 2, 0x0C, {{.uwv=160}}}, // ImageWidth
    {0x0101, 3, 2, 0x0C, {{.uwv=120}}}, // ImageLength
    {0x0102, 3, 2 * 3, 0x0C, {{.uwv=1}, {.uwv=1},{.uwv=1}}}, //BitsPerSample
    {0x0103, 3, 2, 0x0C, {{.uwv=1}}}, // Compression
    {0x0106, 3, 2, 0x0C, {{.uwv=1}}}, // PhotometricInterpretation
    {0x0111, 3, 2, 0x0C, {{.uwv=1}}},  // StripOffsets
    {0x0115, 3, 2, 0x0C, {{.uwv=1}}}, // SamplesPerPixel
    {0x0116, 3, 2, 0x0C, {{.uwv=1}}}, // RowsPerStrip
    {0x0117, 3, 2, 0x0C, {{.uwv=1}}},  // StripByteConunts
    {0x011a, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // XResolution
    {0x011b, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // YResolution
    {0x011c, 3, 2, 0x0C, {{.uwv=1}}}, // PlanarConfiguration
    {0x0128, 3, 2, 0x0C, {{.uwv=1}}}, //PlanarConfiguration uint
    {0x0201, 4, 4, 0x0C, {{.udwv=0}}}, // JpegIFOffset
    {0x0202, 4, 4, 0x0C, {{.udwv=0}}}, // JpegIFByteCount
    {0x0211, 5, 8 * 3, 0x0C, {{.uddwv=0x0000004800000001},{.uddwv=0x0000004800000001},{.uddwv=0x0000004800000001}}}, // YCbCrCoefficients
    {0x0212, 3, 2 * 2, 0x0C, {{.uwv=1},{.uwv=1}}}, // YCbCrSubSampling
    {0x0213, 3, 2, 0x0C, {{.uwv=1}}}, // YCbCrPositioning
    {0x0214, 5, 8 * 6, 0x0C, {{.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280}, {.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280},{.uddwv=0x000003E800000280}}}, //  ReferenceBlackWhite
 };
 
static IFD_S miscTags[] =
{
    {0x00fe, 4, 4, 0x0C, {{.udwv=0}}}, // NewSubfileType
    {0x00ff, 3, 2, 0x0C,  {{.uwv=1}}}, // SubfileType
    {0x012d, 3, 2 * 3, 0x0C,  {{.uwv=1}, {.uwv=1}}}, // TransferFunction
    {0x013b, 2, 32, 0x0C, {{.asv="rockchip"}}}, // Artist
    {0x013d, 3, 2, 0x0C, {{.uwv=1}}}, // Predictor
    {0x0142, 3, 2, 0x0C, {{.uwv=1}}}, // TileWidth
    {0x0143, 3, 2, 0x0C, {{.uwv=1}}}, // TileLength
    {0x0144, 4, 4, 0x0C, {{.udwv=0}}},  // TileOffsets
    {0x0145, 3, 2, 0x0C, {{.uwv=1}}}, // TileByteCounts
    {0x014a, 4, 4, 0x0C, {{.udwv=0}}},  // SubIFDs
    {0x015b, 7, 1, 0x0C, {{.asv="0210"}}}, // JPEGTables
    {0x828d, 3, 2 * 2, 0x0C, {{.uwv=1}, {.uwv=1}}}, //CFARepeatPatternDim
    {0x828e, 1, 1, 0x0C, {{.ubv=1}}}, //CFAPattern
    {0x828f, 3, 2, 0x0C, {{.uwv=1}}}, //BatteryLevel
    {0x83bb, 5, 8, 0x0C,  {{.uddwv=0x0000004800000001}}},  // IPTC//NAA
    {0x8773, 7, 1, 0x0C, {{.asv="0210"}}},   // InterColorProfile
    {0x8824, 2, 32, 0x0C,{{.asv="SpectralSensitivity"}}}, // SpectralSensitivity
    {0x8825, 4, 4, 0x0C, {{.udwv=0}}}, //GPSInfo
    {0x8828, 7, 1, 0x0C, {{.asv="0210"}}},   // OECF
    {0x8829, 3, 2, 0x0C, {{.uwv=1}}}, // Interlace
    {0x882a, 3, 2, 0x0C, {{.uwv=1}}}, // TimeZoneOffset
    {0x882b, 3, 2, 0x0C, {{.uwv=1}}}, // SelfTimerMode
    {0x920b, 5, 8, 0x0C,  {{.uddwv=0x0000004800000001}}}, // FlashEnergy
    {0x920c, 7, 1, 0x0C, {{.asv="0210"}}}, // SpatialFrequencyResponse
    {0x920d, 7, 1, 0x0C, {{.asv="0210"}}},  // Noise
    {0x9211, 4, 4, 0x0C, {{.udwv=0}}}, // ImageNumber
    {0x9212, 2, 1, 0x0C, {{.asv="c"}}}, // SecurityClassification
    {0x9213, 2, 32, 0x0C,{{.asv="ImageHistory"}}}, //ImageHistory
    {0x9214, 3, 2 * 4, 0x0C, {{.uwv=1},{.uwv=1},{.uwv=1},{.uwv=1}}}, // SubjectLocation
    {0x9215, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // ExposureIndex
    {0x9216, 1, 1 * 4, 0x0C, {{.ubv=1},{.ubv=1},{.ubv=1},{.ubv=1}}}, //TIFF//EPStandardID
    {0x9290, 2, 32, 0x0C, {{.asv="SubSecTime"}}}, //SubSecTime
    {0x9291, 2, 32, 0x0C, {{.asv="SubSecTimeOriginal"}}}, //SubSecTimeOriginal
    {0x9292, 2, 32, 0x0C, {{.asv="SubSecTimeDigitized"}}}, //SubSecTimeDigitized
    {0xa20b, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // FlashEnergy
    {0xa20c, 3, 2, 0x0C, {{.uwv=1}}}, // SpatialFrequencyResponse
    {0xa214, 3, 2, 0x0C, {{.uwv=1}}}, // SubjectLocation
    {0xa215, 5, 8, 0x0C, {{.uddwv=0x0000004800000001}}}, // ExposureIndex
    {0xa302, 7, 1, 0x0C, {{.udwv=0}}}, //CFAPattern
};
#else
static IFD_S stIfd0[] = {
    {0x010e, 2, 32, 0x00, {{.asv = "thumbnail_test"}}},      // picture info
    {0x010f, 2, 32, 0x00, {{.asv = "rockchip"}}},            // manufact info
    {0x0110, 2, 32, 0x00, {{.asv = "rockchip IP Camrea"}}},  // module info
    {0x0131, 2, 32, 0x00, {{.asv = "rkmedia v1.8"}}},        // software version
    {0x0132, 2, 32, 0x00, {{.asv = "2021:04:27 17:10:50"}}}, // date
};
static IFD_S stIfd1[] = {
    {0x0100, 3, 1, 160, {{.uwv = 160}}}, // ImageWidth
    {0x0101, 3, 1, 120, {{.uwv = 120}}}, // ImageLength
    {0x0202, 4, 1, 0, {{.udwv = 0}}},    // JpegIFByteCount
    {0x0201, 4, 1, 0, {{.udwv = 0}}},    // JpegIFOffset
};
static IFD_S stMpfd[] = {
    {0xB000, 7, 4, 0X30303130, {{.udwv = 0}}}, // mp version
    {0xB001, 4, 1, 0x02, {{.udwv = 0x02}}},    // image cnt include main image
    {0xB002, 7, 32, 0, {{.udwv = 0}}},         // mp entry
    {0xB003, 7, 66, 0, {{.udwv = 0}}},         // imageuidlist
    {0xB004, 4, 1, 0x01, {{.udwv = 0}}},       // total frame
};
#endif
#endif