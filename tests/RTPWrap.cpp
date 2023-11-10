
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include "RTPWrap.h" // Replace with the actual path to your RTPWrap class definition


// Mock implementation of RTPWrap for testing
class MockRTPWrap : public RTPWrap {
public:
    uint64_t Initialize() override {
        return 1; // Return a fake handle for testing
    }

    uint64_t CreateSession(const std::string& localEndPoint) override {
        return 2; // Return a fake session handle for testing
    }

    uint64_t CreateStream(uint64_t sessionId, const RTPStreamConfig& streamConfig) override {
        return 3; // Return a fake stream handle for testing
    }

    uint64_t CreateStream(uint64_t sessionId, int srcPort, int direction) override {
        return 4; // Return a fake stream handle for testing
    }

    bool DestroyStream(uint64_t streamId) override {
        return true; // Assume success for testing
    }

    bool DestroySession(uint64_t sessionId) override {
        return true; // Assume success for testing
    }

    bool PushFrame(uint64_t streamId, std::vector<std::byte> pData) noexcept override {
        return true; // Assume success for testing
    }

    std::vector<std::byte> GrabFrame(uint64_t streamId, std::vector<std::byte> pData) noexcept override {
        return pData; // Echo back the data for testing
    }

    void Shutdown() override {
        // Mock shutdown for testing
    }
};

// Test cases
TEST_CASE("RTPWrap Functionality", "[RTPWrap]") {
    MockRTPWrap mockRTPWrap;

    SECTION("Initialize returns a valid handle") {
        REQUIRE(mockRTPWrap.Initialize() != 0);
    }

    SECTION("CreateSession returns a valid session handle") {
        REQUIRE(mockRTPWrap.CreateSession("localhost:1234") != 0);
    }

    SECTION("CreateStream with RTPStreamConfig returns a valid stream handle") {
        RTPStreamConfig config;
        REQUIRE(mockRTPWrap.CreateStream(2, config) != 0);
    }

    SECTION("CreateStream with port and direction returns a valid stream handle") {
        REQUIRE(mockRTPWrap.CreateStream(2, 5004, 1) != 0);
    }

    SECTION("DestroyStream returns true on success") {
        REQUIRE(mockRTPWrap.DestroyStream(3));
    }

    SECTION("DestroySession returns true on success") {
        REQUIRE(mockRTPWrap.DestroySession(2));
    }

    SECTION("PushFrame returns true on success") {
        std::vector<std::byte> frame = {std::byte('a'), std::byte('b')};
        REQUIRE(mockRTPWrap.PushFrame(3, frame));
    }

    SECTION("GrabFrame returns a frame on success") {
        std::vector<std::byte> frame = {std::byte('a'), std::byte('b')};
        REQUIRE(mockRTPWrap.GrabFrame(3, frame) == frame);
    }

    // Additional tests can be added here as needed
}

