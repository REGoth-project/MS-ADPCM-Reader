#include <iostream>
#include <stdio.h>
#include <vector>
#include "WavADPCM.h"

/**
 * Reads the data of the given file into a std::vector
 * @param path File to load
 * @param target Target vector, which will be filled with the data
 */
void readFile(const std::string& path, std::vector<uint8_t>& target)
{
    FILE* f = fopen(path.c_str(), "rb");
    if(!f)
        throw std::runtime_error("Failed to open file: " + path);

    fseek(f, 0, SEEK_END);
    std::size_t s = ftell(f);
    fseek(f, 0, SEEK_SET);

    if(s == 0)
        throw std::runtime_error("File '" + path + "' is empty!");

    target.resize(s);

    fread(&target[0], s, 1, f);

    fclose(f);
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "Usage: ms_adpcm <in-file> <out-file>";
        return 0;
    }

    try {
        std::vector<uint8_t> data;
        readFile(argv[1], data);

        std::cout << "File size: " << data.size() / 1024 << " kb" << std::endl;

        ms_adpcm::WAVEFORMATEX fmt;
        const uint8_t* rawData = nullptr;
        std::size_t rawSize;
        ms_adpcm::parse(data.data(), data.size(), fmt, rawData, rawSize);

        std::size_t numBlocks = rawSize / fmt.nBlockAlign;
        std::size_t samplesPerBlock = ms_adpcm::calculateNumSamplesInBlock(fmt.nBlockAlign);
        std::size_t numSamples = numBlocks * samplesPerBlock;

        std::size_t sampleIdx = 0;
        std::vector<int16_t> samples;
        samples.resize(numSamples);

        for(std::size_t i=0; i < numBlocks; i++)
        {
            std::size_t offset = i * fmt.nBlockAlign;
            ms_adpcm::decodeBlock(rawData + offset, fmt.nBlockAlign, &samples[sampleIdx]);

            sampleIdx += samplesPerBlock;
        }

        std::cout << fmt;

        ms_adpcm::writeWav(argv[2], samples.data(), samples.size(), fmt.nSamplesPerSec);

    }catch(const std::runtime_error& e)
    {
        std::cout << "Conversion failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}