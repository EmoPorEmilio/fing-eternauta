/**
 * @file MathTestFramework.h
 * @brief Minimal, self-contained unit testing framework for math operations
 *
 * ============================================================================
 * OVERVIEW
 * ============================================================================
 * This is a lightweight testing framework designed specifically for validating
 * mathematical operations in the ECS engine. It requires no external dependencies
 * and produces clear, readable output.
 *
 * ============================================================================
 * USAGE
 * ============================================================================
 *
 * 1. Define a test function:
 *
 *    TEST(MyTestSuite, MyTestName) {
 *        ASSERT_EQ(1 + 1, 2);
 *        ASSERT_TRUE(someCondition);
 *    }
 *
 * 2. Run all tests:
 *
 *    int main() {
 *        return TestRunner::runAll();
 *    }
 *
 * ============================================================================
 * AVAILABLE ASSERTIONS
 * ============================================================================
 *
 * ASSERT_TRUE(condition)      - Passes if condition is true
 * ASSERT_FALSE(condition)     - Passes if condition is false
 * ASSERT_EQ(a, b)             - Passes if a == b
 * ASSERT_NE(a, b)             - Passes if a != b
 * ASSERT_LT(a, b)             - Passes if a < b
 * ASSERT_LE(a, b)             - Passes if a <= b
 * ASSERT_GT(a, b)             - Passes if a > b
 * ASSERT_GE(a, b)             - Passes if a >= b
 * ASSERT_NEAR(a, b, epsilon)  - Passes if |a - b| <= epsilon
 * ASSERT_VEC3_NEAR(a, b, eps) - Passes if all components within epsilon
 * ASSERT_MAT4_NEAR(a, b, eps) - Passes if all matrix elements within epsilon
 *
 * ============================================================================
 */

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace test {

// =============================================================================
// Test Result Tracking
// =============================================================================

struct TestResult {
    std::string suiteName;
    std::string testName;
    bool passed;
    std::string failureMessage;
    std::string file;
    int line;
};

// =============================================================================
// Test Registry (Singleton)
// =============================================================================

class TestRegistry {
public:
    using TestFunction = std::function<void()>;

    struct TestCase {
        std::string suiteName;
        std::string testName;
        TestFunction function;
    };

    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void registerTest(const std::string& suite, const std::string& name, TestFunction fn) {
        tests.push_back({suite, name, fn});
    }

    std::vector<TestCase>& getTests() { return tests; }

    // Current test context (for assertion macros)
    std::string currentSuite;
    std::string currentTest;
    bool currentTestFailed = false;
    std::string currentFailureMessage;
    std::string currentFile;
    int currentLine = 0;

private:
    std::vector<TestCase> tests;
    TestRegistry() = default;
};

// =============================================================================
// Auto-Registration Helper
// =============================================================================

struct TestRegistrar {
    TestRegistrar(const std::string& suite, const std::string& name,
                  TestRegistry::TestFunction fn) {
        TestRegistry::instance().registerTest(suite, name, fn);
    }
};

// =============================================================================
// Test Runner
// =============================================================================

class TestRunner {
public:
    /**
     * @brief Run all registered tests and print results
     * @return 0 if all tests pass, 1 if any test fails
     */
    static int runAll() {
        auto& registry = TestRegistry::instance();
        auto& tests = registry.getTests();

        std::vector<TestResult> results;
        int passed = 0;
        int failed = 0;

        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                    MATH UNIT TEST SUITE                      ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";

        std::string lastSuite;

        for (auto& test : tests) {
            // Print suite header when suite changes
            if (test.suiteName != lastSuite) {
                if (!lastSuite.empty()) std::cout << "\n";
                std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
                std::cout << "│ Suite: " << std::left << std::setw(53) << test.suiteName << "│\n";
                std::cout << "└─────────────────────────────────────────────────────────────┘\n";
                lastSuite = test.suiteName;
            }

            // Run test
            registry.currentSuite = test.suiteName;
            registry.currentTest = test.testName;
            registry.currentTestFailed = false;
            registry.currentFailureMessage.clear();

            try {
                test.function();
            } catch (const std::exception& e) {
                registry.currentTestFailed = true;
                registry.currentFailureMessage = std::string("Exception: ") + e.what();
            }

            // Record result
            TestResult result;
            result.suiteName = test.suiteName;
            result.testName = test.testName;
            result.passed = !registry.currentTestFailed;
            result.failureMessage = registry.currentFailureMessage;
            result.file = registry.currentFile;
            result.line = registry.currentLine;
            results.push_back(result);

            // Print result
            if (result.passed) {
                std::cout << "  ✓ " << test.testName << "\n";
                passed++;
            } else {
                std::cout << "  ✗ " << test.testName << "\n";
                std::cout << "    └─ " << result.failureMessage << "\n";
                if (!result.file.empty()) {
                    std::cout << "       at " << result.file << ":" << result.line << "\n";
                }
                failed++;
            }
        }

        // Summary
        std::cout << "\n";
        std::cout << "══════════════════════════════════════════════════════════════\n";
        std::cout << "  SUMMARY: " << passed << " passed, " << failed << " failed, "
                  << (passed + failed) << " total\n";
        std::cout << "══════════════════════════════════════════════════════════════\n";

        if (failed == 0) {
            std::cout << "  ✓ All tests passed!\n";
        } else {
            std::cout << "  ✗ Some tests failed.\n";
        }
        std::cout << "\n";

        return (failed > 0) ? 1 : 0;
    }
};

// =============================================================================
// Assertion Helpers
// =============================================================================

inline void failTest(const std::string& message, const char* file, int line) {
    auto& registry = TestRegistry::instance();
    registry.currentTestFailed = true;
    registry.currentFailureMessage = message;
    registry.currentFile = file;
    registry.currentLine = line;
}

/**
 * @brief Format a floating-point value for display
 */
inline std::string formatFloat(float value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << value;
    return ss.str();
}

/**
 * @brief Format a vec3 for display
 */
inline std::string formatVec3(const glm::vec3& v) {
    std::ostringstream ss;
    ss << "(" << formatFloat(v.x) << ", " << formatFloat(v.y) << ", " << formatFloat(v.z) << ")";
    return ss.str();
}

/**
 * @brief Check if two floats are approximately equal
 */
inline bool nearlyEqual(float a, float b, float epsilon) {
    return std::abs(a - b) <= epsilon;
}

/**
 * @brief Check if two vec3s are approximately equal
 */
inline bool vec3NearlyEqual(const glm::vec3& a, const glm::vec3& b, float epsilon) {
    return nearlyEqual(a.x, b.x, epsilon) &&
           nearlyEqual(a.y, b.y, epsilon) &&
           nearlyEqual(a.z, b.z, epsilon);
}

/**
 * @brief Check if two mat4s are approximately equal
 */
inline bool mat4NearlyEqual(const glm::mat4& a, const glm::mat4& b, float epsilon) {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (!nearlyEqual(a[col][row], b[col][row], epsilon)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace test

// =============================================================================
// MACROS
// =============================================================================

/**
 * @brief Define a test case
 *
 * Usage:
 *   TEST(SuiteName, TestName) {
 *       // test code here
 *   }
 */
#define TEST(suite, name) \
    void suite##_##name##_impl(); \
    static test::TestRegistrar suite##_##name##_registrar(#suite, #name, suite##_##name##_impl); \
    void suite##_##name##_impl()

/**
 * @brief Assert that a condition is true
 *
 * Example:
 *   ASSERT_TRUE(value > 0);
 */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            test::failTest("Expected TRUE but got FALSE: " #condition, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that a condition is false
 *
 * Example:
 *   ASSERT_FALSE(isEmpty());
 */
#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            test::failTest("Expected FALSE but got TRUE: " #condition, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that two values are equal
 *
 * Example:
 *   ASSERT_EQ(computedValue, expectedValue);
 */
#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a == _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " << _a << " == " << _b; \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that two values are not equal
 */
#define ASSERT_NE(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (_a == _b) { \
            std::ostringstream _ss; \
            _ss << "Expected " << _a << " != " << _b; \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that a < b
 */
#define ASSERT_LT(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a < _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " << _a << " < " << _b; \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that a <= b
 */
#define ASSERT_LE(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a <= _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " << _a << " <= " << _b; \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that a > b
 */
#define ASSERT_GT(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a > _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " << _a << " > " << _b; \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that a >= b
 */
#define ASSERT_GE(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a >= _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " << _a << " >= " << _b; \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that two floats are approximately equal
 *
 * Example:
 *   ASSERT_NEAR(computedAngle, expectedAngle, 0.001f);
 *
 * @param a First value
 * @param b Second value
 * @param epsilon Maximum allowed difference
 */
#define ASSERT_NEAR(a, b, epsilon) \
    do { \
        float _a = (a); \
        float _b = (b); \
        float _eps = (epsilon); \
        if (!test::nearlyEqual(_a, _b, _eps)) { \
            std::ostringstream _ss; \
            _ss << "Expected |" << test::formatFloat(_a) << " - " << test::formatFloat(_b) \
                << "| <= " << test::formatFloat(_eps) << ", but difference is " \
                << test::formatFloat(std::abs(_a - _b)); \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that two vec3s are approximately equal (component-wise)
 *
 * Example:
 *   ASSERT_VEC3_NEAR(position, expectedPosition, 0.001f);
 *
 * @param a First vector
 * @param b Second vector
 * @param epsilon Maximum allowed difference per component
 */
#define ASSERT_VEC3_NEAR(a, b, epsilon) \
    do { \
        glm::vec3 _a = (a); \
        glm::vec3 _b = (b); \
        float _eps = (epsilon); \
        if (!test::vec3NearlyEqual(_a, _b, _eps)) { \
            std::ostringstream _ss; \
            _ss << "Expected vec3 " << test::formatVec3(_a) \
                << " to be near " << test::formatVec3(_b) \
                << " (epsilon=" << test::formatFloat(_eps) << ")"; \
            test::failTest(_ss.str(), __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert that two mat4s are approximately equal (element-wise)
 *
 * Example:
 *   ASSERT_MAT4_NEAR(modelMatrix, expectedMatrix, 0.001f);
 *
 * @param a First matrix
 * @param b Second matrix
 * @param epsilon Maximum allowed difference per element
 */
#define ASSERT_MAT4_NEAR(a, b, epsilon) \
    do { \
        glm::mat4 _a = (a); \
        glm::mat4 _b = (b); \
        float _eps = (epsilon); \
        if (!test::mat4NearlyEqual(_a, _b, _eps)) { \
            test::failTest("Matrices are not approximately equal", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)
