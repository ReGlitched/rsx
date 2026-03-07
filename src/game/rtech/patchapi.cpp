// This file is made available as part of the reSource Xtractor (RSX) asset extractor
// Licensed under AGPLv3. Details available at https://github.com/r-ex/rsx/blob/main/LICENSE

#include <pch.h>
#include <game/rtech/patchapi.h>

#if defined(PAKLOAD_PATCHING_ANY)
void CPakFile::SetPatchCommand(const int8_t cmd)
{
    assertm(cmd >= 0 && cmd < ePakPatchCommand_t::_CMD_COUNT, "CPakFile::SetPatchCommand: invalid patch command index.");
    this->p.patchFunc = g_pakPatchApi[cmd];
}

const int CPakFile::ResolvePointers()
{
    int pointerIdx = 0;
    for (pointerIdx = this->numResolvedPointers; pointerIdx < this->pointerCount(); ++pointerIdx)
    {
        PagePtr_t* const curPointer = &this->m_pPointerHeaders[pointerIdx];
        int curCount = curPointer->index - this->firstPageIdx;
        if (curCount < 0)  // get the index of the page in relation to the order that it will be loaded
            curCount += this->pageCount();

        if (curCount >= this->numProcessedPages)
            break;

        PagePtr_t* const ptr = reinterpret_cast<PagePtr_t*>(this->pageBuffers[curPointer->index] + curPointer->offset);
        assertm(ptr->ptr, "uh oh something went very wrong!!!! (shifted pointers are most likely wrong)");
        ptr->ptr = this->pageBuffers[ptr->index] + ptr->offset;
    }

    return pointerIdx;
}

bool PatchCmd_0(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    size_t numBytes = std::min(*numRemainingFileBufferBytes, pak->p.numBytesToPatch);

    if (pak->p.numBytesToSkip > 0)
    {
        if (numBytes <= pak->p.numBytesToSkip)
        {
            pak->p.offsetInFileBuffer += numBytes;
            pak->p.numBytesToSkip -= numBytes;
            pak->p.numBytesToPatch -= numBytes;
            *numRemainingFileBufferBytes -= numBytes;

            return pak->p.numBytesToPatch == 0;
        }

        pak->p.offsetInFileBuffer += pak->p.numBytesToSkip;
        pak->p.numBytesToPatch -= pak->p.numBytesToSkip;

        *numRemainingFileBufferBytes -= pak->p.numBytesToSkip;
        numBytes -= pak->p.numBytesToSkip;

        pak->p.numBytesToSkip = 0ull;

        if (numBytes == 0)
            return pak->p.numBytesToPatch == 0;
    }

#if (PAKLOAD_DEBUG == PAKLOAD_DEBUG_VERBOSE)
    Log("PTCH CMD 0: Sending some data to %p\n", pak->p.patchDestination);
#endif

    const size_t patchSrcSize = std::min(numBytes, pak->p.patchDestinationSize);
    memcpy(
        pak->p.patchDestination,
        pak->GetDecompressedBuffer().get() + pak->p.offsetInFileBuffer,
        patchSrcSize
    );

    pak->p.patchDestination += patchSrcSize;
    pak->p.offsetInFileBuffer += patchSrcSize;
    pak->p.patchDestinationSize -= patchSrcSize;
    pak->p.numBytesToPatch -= patchSrcSize;
    *numRemainingFileBufferBytes -= patchSrcSize;

    return pak->p.numBytesToPatch == 0;
}

bool PatchCmd_1(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    const size_t numBytes = std::min(*numRemainingFileBufferBytes, pak->p.numBytesToPatch);
    const bool ret = *numRemainingFileBufferBytes > pak->p.numBytesToPatch;

    *numRemainingFileBufferBytes -= numBytes;
    pak->p.offsetInFileBuffer += numBytes;
    pak->p.numBytesToPatch -= numBytes;

    return ret;
}

bool PatchCmd_2(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    UNUSED(numRemainingFileBufferBytes);

    if (pak->p.numBytesToSkip > 0)
    {
        if (pak->p.numBytesToPatch <= pak->p.numBytesToSkip)
        {
            pak->p.patchReplacementData += pak->p.numBytesToPatch;
            pak->p.numBytesToSkip -= pak->p.numBytesToPatch;
            pak->p.numBytesToPatch = 0ull;

            return true;
        }

        pak->p.patchReplacementData += pak->p.numBytesToSkip;
        pak->p.numBytesToPatch -= pak->p.numBytesToSkip;
        pak->p.numBytesToSkip = 0ull;
    }

    const size_t patchSrcSize = std::min(pak->p.numBytesToPatch, pak->p.patchDestinationSize);
    memcpy(pak->p.patchDestination, pak->p.patchReplacementData, patchSrcSize);

    pak->p.patchReplacementData += patchSrcSize;
    pak->p.patchDestination += patchSrcSize;
    pak->p.patchDestinationSize -= patchSrcSize;
    pak->p.numBytesToPatch -= patchSrcSize;

    return pak->p.numBytesToPatch == 0;
}

bool PatchCmd_3(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    const size_t v9 = std::min(*numRemainingFileBufferBytes, pak->p.numBytesToPatch);
    const size_t patchSrcSize = std::min(v9, pak->p.patchDestinationSize);

    memcpy(pak->p.patchDestination, pak->p.patchReplacementData, patchSrcSize);
    pak->p.patchReplacementData += patchSrcSize;
    pak->p.patchDestination += patchSrcSize;
    pak->p.offsetInFileBuffer += patchSrcSize;

    pak->p.patchDestinationSize -= patchSrcSize;
    pak->p.numBytesToPatch -= patchSrcSize;
    *numRemainingFileBufferBytes -= patchSrcSize;

    return pak->p.numBytesToPatch == 0;
}

bool PatchCmd_4_5(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    if (!*numRemainingFileBufferBytes)
        return false;

    *pak->p.patchDestination = *(char*)pak->p.patchReplacementData++;
    ++pak->p.offsetInFileBuffer;
    --pak->p.patchDestinationSize;
    ++pak->p.patchDestination;

    pak->SetPatchCommand(ePakPatchCommand_t::CMD_0);

    *numRemainingFileBufferBytes -= 1;

    return PatchCmd_0(pak, numRemainingFileBufferBytes);
}

bool PatchCmd_6(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    const size_t v2 = *numRemainingFileBufferBytes;
    size_t v3 = 2;

    if (*numRemainingFileBufferBytes < 2)
    {
        if (!*numRemainingFileBufferBytes)
            return false;

        v3 = *numRemainingFileBufferBytes;
    }

    const size_t patchSrcSize = pak->p.patchDestinationSize;
    if (v3 > patchSrcSize)
    {
        memcpy(pak->p.patchDestination, pak->p.patchReplacementData, patchSrcSize);
        pak->p.patchReplacementData += patchSrcSize;
        pak->p.offsetInFileBuffer += patchSrcSize;
        pak->p.patchDestinationSize -= patchSrcSize;
        pak->p.patchDestination += patchSrcSize;
        pak->SetPatchCommand(ePakPatchCommand_t::CMD_4);
        *numRemainingFileBufferBytes -= patchSrcSize;
    }
    else
    {
        memcpy(pak->p.patchDestination, pak->p.patchReplacementData, v3);
        pak->p.patchReplacementData += v3;
        pak->p.offsetInFileBuffer += v3;
        pak->p.patchDestinationSize -= v3;
        pak->p.patchDestination += v3;

        if (v2 >= 2)
        {
            pak->SetPatchCommand(ePakPatchCommand_t::CMD_0);
            *numRemainingFileBufferBytes -= v3;

            return PatchCmd_0(pak, numRemainingFileBufferBytes);
        }

        pak->SetPatchCommand(ePakPatchCommand_t::CMD_4);
        *numRemainingFileBufferBytes = 0;
    }

    return false;
}

const CPakFile::PatchFunc_t g_pakPatchApi[] = {
	PatchCmd_0,
    PatchCmd_1,
    PatchCmd_2,
    PatchCmd_3,
    PatchCmd_4_5,
    PatchCmd_4_5,
    PatchCmd_6
};
static_assert(sizeof(g_pakPatchApi) == 56);


// A lookup table to simplify reversing all of the bits in a byte
unsigned char s_ByteReverseLUT[UINT8_MAX + 1] =
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90,
  0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8,
  0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8,
  0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
  0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 0x0C, 0x8C,
  0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
  0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2,
  0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A,
  0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6,
  0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6,
  0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
  0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81,
  0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1,
  0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9,
  0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95,
  0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
  0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD,
  0x7D, 0xFD, 0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
  0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B,
  0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB,
  0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7,
  0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F,
  0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

#pragma warning( push, 0 )
#pragma warning( disable: 6297 )
// arrayBitDepth - number of bits used for array indexes
// (array is size 1 << arrayBitDepth)
// Return value is the number of bytes of "buffer" that were consumed
__int64 PakPatch_DecodeData(char* buffer, int arrayBitDepth, char* out_1, char* out_2)
{
    // NOTE: If looking back at this and comparing against IDA, I made a change to this code:
    //       The original version of this code took an extra argument in between arrayBitDepth and out_1, an array of integers
    //       I removed this array because in all instances that the code is used in engine and in RSX, array[i] == i, so the array
    //       access was completely pointless. I'm sure it could serve a purpose if used elsewhere, but RSX
    //       doesn't care about other usages.
    char numBitsForLowerComponent;

    unsigned long msbIndex;

    // Seems to be determining the number of bits used for the lower of two bitpacked components
    // For the calls made by RSX, numBitsForLowerComponent always seems to end up as 3
    if (!_BitScanReverse(&msbIndex, arrayBitDepth - 2))
        numBitsForLowerComponent = 0;
    else
        numBitsForLowerComponent = msbIndex + 1;

    uint32_t offset = 0;
    const uint32_t arraySize = 1 << arrayBitDepth;
    const uint32_t lowerCompMask = (1 << numBitsForLowerComponent) - 1;

    uint32_t v5 = 0;
    uint32_t numRemainingDecodes = arraySize;
    uint8_t prevLowerVal;
    for (uint8_t i = (lowerCompMask & buffer[offset]) + 1; numRemainingDecodes > 0; i = (lowerCompMask & buffer[offset]) + 1, v5 = (v5 + 1) << (i - prevLowerVal))
    {
        prevLowerVal = i;
        for (uint32_t j = s_ByteReverseLUT[v5 << (8 - i)]; j < arraySize; j += ((1ull << i)))
        {
            out_1[j] = ((unsigned __int8)buffer[offset] >> numBitsForLowerComponent);
            out_2[j] = i;

            numRemainingDecodes--;
        }
        offset++;
    }

    return offset;
}
#pragma warning( pop )
#endif // #if defined(PAKLOAD_PATCHING_ANY)