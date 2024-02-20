#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "BlockSizeAdapter.h"
#include <vector>
#include <cmath>
#include <algorithm>

using namespace Utilities::Buffer;

// Helper function for generating test data
std::vector<float> generateTestData(size_t size) {
    std::vector<float> data(size);
    float value = 0.1f; // Starting value for test data
    std::generate(data.begin(), data.end(), [&value]() { return value += 0.1f; });
    return data;
}

TEST_CASE("BlockSizeAdapter Constructor", "[BlockSizeAdapter]") {
    BlockSizeAdapter bsa(1024);
    REQUIRE(bsa.isEmpty() == true);
}

TEST_CASE("BlockSizeAdapter::push and BlockSizeAdapter::pop with vector", "[BlockSizeAdapter]") {
    BlockSizeAdapter bsa(5);
    std::vector<float> inputData = generateTestData(10); // Generate 10 float values
    bsa.push(inputData);

    SECTION("Data is ready after push") {
        REQUIRE(bsa.dataReady() == true);
    }

    std::vector<float> outputData;
    bsa.pop(outputData);

    SECTION("Pop retrieves correct block size of data") {
        REQUIRE(outputData.size() == 5);
    }

    SECTION("Pop retrieves correct data") {
        for (size_t i = 0; i < outputData.size(); ++i) {
            REQUIRE(outputData[i] == Approx(inputData[i]));
        }
    }

    SECTION("Adapter is not empty after partial pop") {
        REQUIRE(bsa.isEmpty() == false);
    }
}

TEST_CASE("BlockSizeAdapter::push and BlockSizeAdapter::pop with raw pointer", "[BlockSizeAdapter]") {
    BlockSizeAdapter bsa(5);
    auto inputData = generateTestData(10); // Generate 10 float values
    bsa.push(inputData.data(), inputData.size());

    float outputData[5];
    bsa.pop(outputData);

    SECTION("Pop retrieves correct data") {
        for (size_t i = 0; i < 5; ++i) {
            REQUIRE(outputData[i] == Approx(inputData[i]));
        }
    }
}

TEST_CASE("BlockSizeAdapter::isEmpty and BlockSizeAdapter::dataReady", "[BlockSizeAdapter]") {
    BlockSizeAdapter bsa(10);
    REQUIRE(bsa.isEmpty() == true);
    REQUIRE(bsa.dataReady() == false);

    std::vector<float> inputData = generateTestData(5); // Less than block size
    bsa.push(inputData);

    SECTION("Adapter is not empty but not ready with partial data") {
        REQUIRE(bsa.isEmpty() == false);
        REQUIRE(bsa.dataReady() == false);
    }

    bsa.push(inputData); // Push enough data to exceed block size

    SECTION("Adapter is ready when enough data is pushed") {
        REQUIRE(bsa.dataReady() == true);
    }
}

TEST_CASE("BlockSizeAdapter::setOutputBlockSize changes output block size", "[BlockSizeAdapter]") {
    BlockSizeAdapter bsa(5);
    std::vector<float> inputData = generateTestData(10); // Generate 10 float values
    bsa.push(inputData);

    bsa.setOutputBlockSize(10); // Change the output block size

    std::vector<float> outputData;
    bsa.pop(outputData);

    SECTION("Output data matches new block size") {
        REQUIRE(outputData.size() == 10);
    }
}

TEST_CASE("BlockSizeAdapter::monoSplit", "[BlockSizeAdapter]") {
    BlockSizeAdapter bsaLeft(2), bsaRight(2);
    std::vector<float> stereoLeft = {0.3f, 1.2f, 0.4f, 1.4f}; // Interleaved stereo data
    std::vector<float> stereoRight = {0.1f, 0.6f, 0.2f, 0.7f}; // Interleaved stereo data

    // Assuming a method to push stereo data for testing purpose
    bsaLeft.push(stereoSignal);
    bsaRight.push(stereoSignal);
    BlockSizeAdapter::monoSplit(bsaLeft, bsaRight);

    std::vector<float> block0, block1;
    bsaLeft.pop(block0);
    bsaLeft.pop(block1);

    SECTION("Left channel contains correct data") {
        REQUIRE(block0[0] == Approx(0.2f));
        REQUIRE(block0[1] == Approx(0.9f));
    }

    SECTION("Right channel contains correct data") {
        REQUIRE(block1[0] == Approx(0.3f));
        REQUIRE(block1[1] == Approx(10.5f));
    }
}
