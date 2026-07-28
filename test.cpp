#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int64_t beatbyte(int32_t t) {
    int64_t divisor = (3 & (t >> 14));
    if (divisor == 0) {
        divisor = 1;
    }

    int64_t period = ((t >> 16) % 6);
    int64_t repeat = ((t >> 10) % ((period / divisor) + 1) + 1);
    int64_t base = repeat * static_cast<int64_t>(t);

    int64_t shift = ((t & 16384) ? 4 : 3);
    int64_t branch_a = (base & ((-static_cast<int64_t>(t)) >> shift));
    int64_t branch_b = (base & (static_cast<int64_t>(t) * static_cast<int64_t>(t)));
    return ((t >> 14) & 3) ? branch_a : branch_b;
}

double secondPattern(int32_t t) {
    int32_t angle = ((-t * 2) % 32768);
    if (angle < 0) {
        angle += 32768;
    }
    int32_t magnitude = ((t * 2) % 3276);
    if (magnitude < 0) {
        magnitude += 3276;
    }
    return static_cast<double>(magnitude) * std::sin(static_cast<double>(angle) / 200.0);
}

void writeWav(const std::vector<int16_t>& samples, const std::string& path, uint32_t sampleRate) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Unable to create output WAV file");
    }

    const uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    const uint16_t audioFormat = 1;
    const uint16_t channels = 1;
    const uint16_t bitsPerSample = 16;
    const uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    const uint16_t blockAlign = channels * bitsPerSample / 8;

    out.write("RIFF", 4);
    uint32_t fileSize = 36 + dataSize;
    out.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    uint32_t fmtSize = 16;
    out.write(reinterpret_cast<const char*>(&fmtSize), sizeof(fmtSize));
    out.write(reinterpret_cast<const char*>(&audioFormat), sizeof(audioFormat));
    out.write(reinterpret_cast<const char*>(&channels), sizeof(channels));
    out.write(reinterpret_cast<const char*>(&sampleRate), sizeof(sampleRate));
    out.write(reinterpret_cast<const char*>(&byteRate), sizeof(byteRate));
    out.write(reinterpret_cast<const char*>(&blockAlign), sizeof(blockAlign));
    out.write(reinterpret_cast<const char*>(&bitsPerSample), sizeof(bitsPerSample));
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    for (int16_t sample : samples) {
        out.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
    }
}

std::vector<int16_t> makeBeatbyteWave(uint32_t sampleRate, int32_t durationSeconds) {
    std::vector<int16_t> pcm;
    pcm.reserve(durationSeconds * sampleRate);

    for (int32_t i = 0; i < durationSeconds * static_cast<int32_t>(sampleRate); ++i) {
        int64_t value = beatbyte(i);
        int32_t normalized = static_cast<int32_t>((value % 65536) - 32768);
        double scaled = normalized / 32768.0;
        scaled = std::clamp(scaled, -1.0, 1.0);
        pcm.push_back(static_cast<int16_t>(scaled * 32767.0));
    }

    return pcm;
}

std::vector<int16_t> makeSecondWave(uint32_t sampleRate, int32_t durationSeconds) {
    std::vector<int16_t> pcm;
    pcm.reserve(durationSeconds * sampleRate);

    for (int32_t i = 0; i < durationSeconds * static_cast<int32_t>(sampleRate); ++i) {
        double value = secondPattern(i) / 3276.0;
        value = std::clamp(value, -1.0, 1.0);
        pcm.push_back(static_cast<int16_t>(value * 32767.0));
    }

    return pcm;
}

}  // namespace

int main() {
    try {
        constexpr int32_t firstSeconds = 4 * 60;
        constexpr int32_t secondSeconds = 2 * 60;
        constexpr uint32_t firstRate = 1600;
        constexpr uint32_t secondRate = 8000;

        std::vector<int16_t> firstPcm = makeBeatbyteWave(firstRate, firstSeconds);
        writeWav(firstPcm, "beatbyte_1600.wav", firstRate);
        std::cout << "Created beatbyte_1600.wav with " << firstPcm.size() << " samples.\n";

        std::vector<int16_t> secondPcm = makeSecondWave(secondRate, secondSeconds);
        writeWav(secondPcm, "second_8000.wav", secondRate);
        std::cout << "Created second_8000.wav with " << secondPcm.size() << " samples.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
