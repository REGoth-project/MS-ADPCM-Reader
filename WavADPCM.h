#pragma once
#include <stdint.h>
#include <cstdint>
#include <iostream>
#include "wavFormatTags.h"
#include <stdio.h>

namespace ms_adpcm
{


    template <int a, int b, int c, int d>
    struct FourCC
    {
        enum : uint32_t { value = (((((a << 8) | b) << 8) | c) << 8) | d };
    };

    enum class RIFFChunk
    {
        RIFF = FourCC<'R', 'I', 'F', 'F'>::value,
        FMT = FourCC<'f', 'm', 't', ' '>::value,
        DATA = FourCC<'d', 'a', 't', 'a'>::value,
    };

    /**
     * Read an integral type from the given pointer as little endian data
     * @tparam T Integral type
     * @param ptr Pointer to the data to convert
     * @return Integer at *ptr of type T read as little endian
     */
    template<typename T>
    T bigEndianRead(const uint8_t* ptr)
    {
        const std::size_t numBytes = sizeof(T);
        T result = 0;

        for(std::size_t i = 0;i < numBytes; i++)
        {
            result |= ptr[i] << (numBytes - i - 1) * 8;
        }

        return result;
    }

    /**
     * Read an integral type from the given pointer as little endian data
     * @tparam T Integral type
     * @param ptr Pointer to the data to convert
     * @return Integer at *ptr of type T read as little endian
     */
    template<typename T>
    T littleEndianRead(const uint8_t* ptr)
    {
        const std::size_t numBytes = sizeof(T);
        T result = 0;

        for(std::size_t i = 0;i < numBytes; i++)
        {
            result |= ptr[i] << i * 8;
        }

        return result;
    }

    template<typename T>
    T swapEndianess(T t)
    {
        const std::size_t numBytes = sizeof(T);
        T result = 0;

        for(std::size_t i = 0;i < numBytes; i++)
        {
            result |= (t & (0xFF << i)) >> (numBytes - i - 1) * 8;
        }

        return result;
    }

    struct WAVEFORMATEX
    {
        WAVEFORMATEX(){}
        WAVEFORMATEX(const uint8_t* data)
        {
            wFormatTag      = littleEndianRead<uint16_t>(data);
            nChannels       = littleEndianRead<uint16_t>(data + 2);
            nSamplesPerSec  = littleEndianRead<uint32_t>(data + 4);
            nAvgBytesPerSec = littleEndianRead<uint32_t>(data + 8);
            nBlockAlign     = littleEndianRead<uint16_t>(data + 12);
            wBitsPerSample  = littleEndianRead<uint16_t>(data + 14);
            cbSize          = littleEndianRead<uint16_t>(data + 16);
        }

        uint16_t wFormatTag;
        uint16_t nChannels;
        uint32_t nSamplesPerSec;
        uint32_t nAvgBytesPerSec;
        uint16_t nBlockAlign;
        uint16_t wBitsPerSample;
        uint16_t cbSize;
    };

    /**
     * Output some information about the WAVEFORMATEX-structure
     */
    std::ostream& operator<<(std::ostream& s, const WAVEFORMATEX& fmt)
    {
        s << "Format Tag:       " << fmt.wFormatTag << "\n"
          << "nChannels:        " << fmt.nChannels << "\n"
          << "nSamplesPerSec:   " << fmt.nSamplesPerSec << "\n"
          << "nAvgBytesPerSec:  " << fmt.nAvgBytesPerSec << "\n"
          << "nBlockAlign:      " << fmt.nBlockAlign << "\n"
          << "wBitsPerSample:   " << fmt.wBitsPerSample << "\n"
          << "cbSize:           " << fmt.cbSize << std::endl;

        return s;
    }

    void parse(const uint8_t* wavData, std::size_t size, WAVEFORMATEX& fmt, const uint8_t*& outRawData, std::size_t& outRawSize)
    {
        std::size_t offset = 0;

        uint32_t riffChunkId   = bigEndianRead<uint32_t>(wavData + offset);  offset += 4;
        uint32_t riffChunkSize = bigEndianRead<uint32_t>(wavData + offset);  offset += 4;
        uint32_t riffFormat    = bigEndianRead<uint32_t>(wavData + offset);  offset += 4;

        if(riffFormat != FourCC<'W', 'A', 'V', 'E'>::value)
            throw std::runtime_error("File is not a WAVE-File!");

        do{
            uint32_t subchunkId = bigEndianRead<uint32_t>(wavData + offset);
            offset += sizeof(uint32_t);

            uint32_t subchunkSize = littleEndianRead<uint32_t>(wavData + offset);
            offset += sizeof(uint32_t);

            switch(static_cast<RIFFChunk>(subchunkId))
            {
                case RIFFChunk::FMT:
                    fmt = WAVEFORMATEX(wavData + offset);
                    break;

                case RIFFChunk::DATA:
                    outRawSize = subchunkSize;
                    outRawData = wavData + offset;
                    break;

                default:
                    break;
            }

            offset += subchunkSize;

        }while(offset < size);
    }

    void writeWav(const std::string& path, const int16_t* samples, uint32_t numSamples, uint32_t sampleRate)
    {
        FILE* f = fopen(path.c_str(), "wb");

        if(!f)
            throw std::runtime_error("Failed to open file for writing: " + path);

        uint32_t riffChunkId   = static_cast<uint32_t>(FourCC<'F', 'F', 'I', 'R'>::value);
        uint32_t riffChunkSize = 36 + numSamples * 2;
        uint32_t riffFormat    = static_cast<uint32_t>(FourCC<'E', 'V', 'A', 'W'>::value);
        uint32_t fmtChunkId   = static_cast<uint32_t>(FourCC<' ', 't', 'm', 'f'>::value);
        uint32_t fmtChunkSize   = sizeof(WAVEFORMATEX);
        uint32_t dataChunkId   = static_cast<uint32_t>(FourCC<'a', 't', 'a', 'd'>::value);
        uint32_t dataChunkSize   = numSamples * sizeof(int16_t);

        fwrite(&riffChunkId, sizeof(riffChunkId), 1, f);
        fwrite(&riffChunkSize, sizeof(riffChunkSize), 1, f);
        fwrite(&riffFormat, sizeof(riffFormat), 1, f);
        fwrite(&fmtChunkId, sizeof(fmtChunkId), 1, f);
        fwrite(&fmtChunkSize, sizeof(fmtChunkSize), 1, f);

        WAVEFORMATEX fmt;
        fmt.wFormatTag = WAVE_FORMAT_PCM;
        fmt.nChannels = 1;
        fmt.nSamplesPerSec = sampleRate;
        fmt.wBitsPerSample = 16;
        fmt.nAvgBytesPerSec = sampleRate * fmt.nChannels * fmt.wBitsPerSample / 8;
        fmt.nBlockAlign = static_cast<uint16_t>(fmt.nChannels * fmt.wBitsPerSample / 8);
        fmt.cbSize = sizeof(WAVEFORMATEX);

        fwrite(&fmt, sizeof(fmt), 1, f);

        fwrite(&dataChunkId, sizeof(riffChunkId), 1, f);
        fwrite(&dataChunkSize, sizeof(dataChunkSize), 1, f);

        fwrite(samples, numSamples * sizeof(int16_t), 1, f);

        fclose(f);
    }

    static const int AdaptationTable [] = {
            230, 230, 230, 230, 307, 409, 512, 614,
            768, 614, 512, 409, 307, 230, 230, 230
    };

    static const int AdaptCoeff1 [] = { 256, 512, 0, 192, 240, 460, 392 } ;
    static const int AdaptCoeff2 [] = { 0, -256, 0, 64, 0, -208, -232 } ;

    struct ADPCMBlockHeader
    {
        ADPCMBlockHeader(const uint8_t* data)
        {
            predictor   = data[0];
            intialDelta = static_cast<int16_t>(data[1] | (data[2] << 8));
            sample1     = static_cast<int16_t>(data[3] | (data[4] << 8));
            sample2     = static_cast<int16_t>(data[5] | (data[6] << 8));
        }

        uint8_t predictor;  // Should be in Range 0..6
        int16_t intialDelta;
        int16_t sample1;
        int16_t sample2;
    };

    /**
     * Calculates how many samples there will be in a single ADPCM-Block
     * @param blockSize Size of a ADPCM-Block (BlockAlign-Field of WAVEFORMATEX)
     * @return Number of samples inside a single ADPCM-block
     */
    std::size_t calculateNumSamplesInBlock(std::size_t blockSize)
    {
        return 2 + (blockSize - sizeof(ADPCMBlockHeader)) * 2;
    }

    void decodeBlock(const uint8_t* blockData, std::size_t blockSize, int16_t* outSamples)
    {
        std::size_t offset = 0;
        std::size_t sampleIdx = 0;

        // Get Block information
        ADPCMBlockHeader block(blockData);
        offset += sizeof(block);

        // The first two samples go straight out to the buffer (reversed, though)
        outSamples[sampleIdx] = block.sample2; sampleIdx++;
        outSamples[sampleIdx] = block.sample1; sampleIdx++;

        // Init conversion
        int32_t predictor = 0;
        int32_t delta = block.intialDelta;
        int16_t sample1 = block.sample1;
        int16_t sample2 = block.sample2;

        // Run through all nibbles and convert them to samples
        while(offset < blockSize)
        {
            int8_t upper = static_cast<int8_t>( (blockData[offset] & 0xF0) >> 4 );
            int8_t lower = static_cast<int8_t>( (blockData[offset] & 0x0F) );

            for(int8_t nibble : {upper, lower})
            {
                predictor =  ((sample1 * AdaptCoeff1[block.predictor]) + (sample2 * AdaptCoeff2[block.predictor])) / 256;
                predictor += (nibble * delta);
                predictor =  predictor & 0xFFFF; // Clamp to int16_t range

                int16_t calculatedSample = static_cast<int16_t>(predictor);

                // Shuffle samples for the next run
                sample2 = sample1;
                sample1 = calculatedSample;

                // Calculate next adaptive scale factor
                delta = (AdaptationTable[static_cast<uint8_t>(nibble)] * delta) / 256;

                delta = delta < 16 ? delta : 16;

                // Output sample
                outSamples[sampleIdx] = calculatedSample; sampleIdx++;
            }

            offset++;
        }
    }
}
