#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "BlockSizeAdapter.h"
#include <cstdlib>
#include <ctime>
#include <vector>

// Helper function to generate a vector of random floats of a given size
std::vector<float> generateRandomBuffer(size_t size) {
    std::vector<float> buffer(size);
    for (auto& sample : buffer) {
        sample = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // Random float between 0 and 1
    }
    return buffer;
}

// Seeding random number generator for all tests
unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
std::srand(seed);

TEST_CASE("BlockSizeAdapterTest - PushAndPop", "[BlockSizeAdapter]") {
    AudioBufferQueue q(5); // Assuming CODEC buffer size of 5 samples

    std::vector<float> input = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    std::vector<float> output;
    q.push(input);

    REQUIRE(q.pop(output));
    REQUIRE(output.size() == 5);
    REQUIRE(output[0] == Approx(1.0).epsilon(0.01));
    REQUIRE(output[4] == Approx(5.0).epsilon(0.01));

    REQUIRE_FALSE(q.pop(output)); // Not enough samples for another pop
}

TEST_CASE("BlockSizeAdapterTest - ReconstructKFromJBuffers", "[BlockSizeAdapter]") {
    const size_t K = 10 + std::rand() % 10; // Random size between 10 and 19 for the original buffer
    const size_t J = 5 + std::rand() % 5;   // Random size between 5 and 9 for the pop size

    BlockSizeAdapter bufferA(J);
    BlockSizeAdapter bufferB(K);

    std::vector<float> originalBuffer = generateRandomBuffer(K);
    bufferA.push(originalBuffer);

    std::vector<float> tempBuffer;
    while (bufferA.pop(tempBuffer)) {
        bufferB.push(tempBuffer);
    }

    std::vector<float> reconstructedBuffer;
    while (bufferB.pop(tempBuffer)) {
        reconstructedBuffer.insert(reconstructedBuffer.end(), tempBuffer.begin(), tempBuffer.end());
    }

    // Ensure reconstructed buffer matches original
    REQUIRE(reconstructedBuffer.size() == originalBuffer.size());
    for (size_t i = 0; i < originalBuffer.size(); ++i) {
        REQUIRE(reconstructedBuffer[i] == Approx(originalBuffer[i]).epsilon(0.01));
    }
}
