#include <unity.h>
#include "em_string.h"

// Test for default constructor
void test_constructor_default() {
    EmString<10> s;
    TEST_ASSERT_EQUAL_STRING("", s.c_str());
    TEST_ASSERT_EQUAL_UINT(0, s.length());
    TEST_ASSERT_EQUAL_UINT(10, s.capacity());
}

// Test for constructor with const char*
void test_constructor_from_cstr() {
    // Within capacity
    EmString<10> s1("hello");
    TEST_ASSERT_EQUAL_STRING("hello", s1.c_str());
    TEST_ASSERT_EQUAL_UINT(5, s1.length());

    // Exceeding capacity (truncation)
    EmString<10> s2("hello world");
    TEST_ASSERT_EQUAL_STRING("hello worl", s2.c_str());
    TEST_ASSERT_EQUAL_UINT(10, s2.length());

    // Exact capacity
    EmString<10> s3("1234567890");
    TEST_ASSERT_EQUAL_STRING("1234567890", s3.c_str());
    TEST_ASSERT_EQUAL_UINT(10, s3.length());

    // Empty string
    EmString<10> s4("");
    TEST_ASSERT_EQUAL_STRING("", s4.c_str());
    TEST_ASSERT_EQUAL_UINT(0, s4.length());

    // Null pointer
    EmString<10> s5(nullptr);
    TEST_ASSERT_EQUAL_STRING("", s5.c_str());
    TEST_ASSERT_EQUAL_UINT(0, s5.length());
}

// Test for copy constructor
void test_constructor_copy() {
    EmString<10> s1("original");
    EmString<10> s2(s1);
    TEST_ASSERT_EQUAL_STRING("original", s2.c_str());

    EmString<10> s3("long string that will be truncated");
    EmString<10> s4(s3);
    TEST_ASSERT_EQUAL_STRING("long strin", s4.c_str());
}

// Test length() and capacity()
void test_length_and_capacity() {
    EmString<20> s;
    TEST_ASSERT_EQUAL_UINT(20, s.capacity());
    TEST_ASSERT_EQUAL_UINT(0, s.length());

    s.set("12345");
    TEST_ASSERT_EQUAL_UINT(20, s.capacity());
    TEST_ASSERT_EQUAL_UINT(5, s.length());
}

// Test set()
void test_set() {
    EmString<10> s;
    s.set("test");
    TEST_ASSERT_EQUAL_STRING("test", s.c_str());

    s.set("this is too long");
    TEST_ASSERT_EQUAL_STRING("this is to", s.c_str());

    s.set("");
    TEST_ASSERT_EQUAL_STRING("", s.c_str());

    s.set(nullptr);
    TEST_ASSERT_EQUAL_STRING("", s.c_str());
}

// Test format()
void test_format() {
    EmString<20> s;
    s.format("Hello %s, value: %d", "World", 42);
    TEST_ASSERT_EQUAL_STRING("Hello World, value: 4", s.c_str()); // Truncated

    EmString<30> s2;
    s2.format("Hello %s, value: %d", "World", 42);
    TEST_ASSERT_EQUAL_STRING("Hello World, value: 42", s2.c_str());

    s2.format("A very long format string that will surely be truncated %d %d %d", 1, 2, 3);
    TEST_ASSERT_EQUAL_STRING("A very long format string tha", s2.c_str());
}

// Test append()
void test_append() {
    EmString<15> s("start");
    s.append("-middle");
    TEST_ASSERT_EQUAL_STRING("start-middle", s.c_str());

    s.append("-end");
    TEST_ASSERT_EQUAL_STRING("start-middle-en", s.c_str());

    s.append("-more"); // Already full
    TEST_ASSERT_EQUAL_STRING("start-middle-en", s.c_str());

    EmString<10> s2;
    s2.append("first");
    TEST_ASSERT_EQUAL_STRING("first", s2.c_str());

    s2.append(nullptr);
    TEST_ASSERT_EQUAL_STRING("first", s2.c_str());

    s2.append("");
    TEST_ASSERT_EQUAL_STRING("first", s2.c_str());
}

// Test c_str() and buffer()
void test_getters() {
    EmString<10> s("test");
    const char* cstr = s.c_str();
    TEST_ASSERT_EQUAL_STRING("test", cstr);

    char* buf = s.buffer();
    buf[0] = 'B';
    TEST_ASSERT_EQUAL_STRING("Best", s.c_str());
}

// Test strcmp()
void test_strcmp() {
    EmString<10> s("abc");
    TEST_ASSERT_EQUAL(0, s.strcmp("abc"));
    TEST_ASSERT_TRUE(s.strcmp("abd") < 0);
    TEST_ASSERT_TRUE(s.strcmp("abb") > 0);
}

// Test startsWith()
void test_startsWith() {
    EmString<20> s("hello world");
    TEST_ASSERT_TRUE(s.startsWith("hello"));
    TEST_ASSERT_TRUE(s.startsWith("h"));
    TEST_ASSERT_TRUE(s.startsWith("hello world"));
    TEST_ASSERT_FALSE(s.startsWith("world"));
    TEST_ASSERT_FALSE(s.startsWith("hello world!"));
    TEST_ASSERT_FALSE(s.startsWith("")); // Standard strncmp behavior
    TEST_ASSERT_FALSE(s.startsWith(nullptr));

    EmString<10> empty_s;
    TEST_ASSERT_FALSE(empty_s.startsWith("a"));
}

// Test endsWith()
void test_endsWith() {
    EmString<20> s("hello world");
    TEST_ASSERT_TRUE(s.endsWith("world"));
    TEST_ASSERT_TRUE(s.endsWith("d"));
    TEST_ASSERT_TRUE(s.endsWith("hello world"));
    TEST_ASSERT_FALSE(s.endsWith("hello"));
    TEST_ASSERT_FALSE(s.endsWith("!hello world"));
    TEST_ASSERT_TRUE(s.endsWith(""));
    TEST_ASSERT_FALSE(s.endsWith(nullptr));

    EmString<10> empty_s;
    TEST_ASSERT_FALSE(empty_s.endsWith("a"));
}

// Test getToken()
void test_getToken() {
    EmString<30> s("token1,token2,token3");
    char token_buf[10];

    TEST_ASSERT_TRUE(s.getToken(0, ',', token_buf, sizeof(token_buf)));
    TEST_ASSERT_EQUAL_STRING("token1", token_buf);

    TEST_ASSERT_TRUE(s.getToken(1, ',', token_buf, sizeof(token_buf)));
    TEST_ASSERT_EQUAL_STRING("token2", token_buf);

    TEST_ASSERT_TRUE(s.getToken(2, ',', token_buf, sizeof(token_buf)));
    TEST_ASSERT_EQUAL_STRING("token3", token_buf);

    TEST_ASSERT_FALSE(s.getToken(3, ',', token_buf, sizeof(token_buf)));

    // Test with small buffer
    TEST_ASSERT_FALSE(s.getToken(0, ',', token_buf, 5));

    // Test with different separator
    EmString<30> s2("a|b|c");
    TEST_ASSERT_TRUE(s2.getToken(1, '|', token_buf, sizeof(token_buf)));
    TEST_ASSERT_EQUAL_STRING("b", token_buf);

    // Test with empty tokens
    EmString<30> s3("a,,c");
    TEST_ASSERT_TRUE(s3.getToken(1, ',', token_buf, sizeof(token_buf)));
    TEST_ASSERT_EQUAL_STRING("", token_buf);

    // Test with trailing separator
    EmString<30> s4("a,b,");
    TEST_ASSERT_TRUE(s4.getToken(2, ',', token_buf, sizeof(token_buf)));
    TEST_ASSERT_EQUAL_STRING("", token_buf);

    // Test with leading separator
    EmString<30> s5(",a,b");
    TEST_ASSERT_TRUE(s5.getToken(0, ',', token_buf, sizeof(token_buf)));
    TEST_ASSERT_EQUAL_STRING("", token_buf);

    // Test with null outToken
    TEST_ASSERT_FALSE(s.getToken(0, ',', nullptr, 10));

    // Test with zero outTokenSize
    TEST_ASSERT_FALSE(s.getToken(0, ',', token_buf, 0));
}

// Test isToken()
void test_isToken() {
    EmString<30> s("token1,token2,token3");
    TEST_ASSERT_TRUE(s.isToken(0, ',', "token1"));
    TEST_ASSERT_TRUE(s.isToken(1, ',', "token2"));
    TEST_ASSERT_TRUE(s.isToken(2, ',', "token3"));

    TEST_ASSERT_FALSE(s.isToken(0, ',', "token2"));
    TEST_ASSERT_FALSE(s.isToken(1, ',', "token1"));
    TEST_ASSERT_FALSE(s.isToken(3, ',', "token3")); // out of bounds
    TEST_ASSERT_FALSE(s.isToken(0, ',', nullptr));

    EmString<30> s2("a,,c");
    TEST_ASSERT_TRUE(s2.isToken(1, ',', ""));
    TEST_ASSERT_FALSE(s2.isToken(1, ',', "b"));

    EmString<30> s3("a,b,");
    TEST_ASSERT_TRUE(s3.isToken(2, ',', ""));
    TEST_ASSERT_FALSE(s3.isToken(3, ',', ""));

    EmString<30> s4(",a,b");
    TEST_ASSERT_TRUE(s4.isToken(0, ',', ""));
}

// Test substring()
void test_substring() {
    EmString<20> s("0123456789");
    EmString<20> sub;

    // Substring from index to end
    TEST_ASSERT_TRUE(s.substring(5, sub));
    TEST_ASSERT_EQUAL_STRING("56789", sub.c_str());

    // Substring from beginning
    TEST_ASSERT_TRUE(s.substring(0, sub));
    TEST_ASSERT_EQUAL_STRING("0123456789", sub.c_str());

    // Substring out of bounds
    TEST_ASSERT_FALSE(s.substring(10, sub));
    TEST_ASSERT_EQUAL_STRING("", sub.c_str());
    TEST_ASSERT_FALSE(s.substring(11, sub));
    TEST_ASSERT_EQUAL_STRING("", sub.c_str());

    // Substring with begin and end index
    TEST_ASSERT_TRUE(s.substring(2, 5, sub));
    TEST_ASSERT_EQUAL_STRING("234", sub.c_str());

    // Substring with end index out of bounds
    TEST_ASSERT_TRUE(s.substring(7, 20, sub));
    TEST_ASSERT_EQUAL_STRING("789", sub.c_str());

    // Substring with invalid indices
    TEST_ASSERT_FALSE(s.substring(5, 2, sub));
    TEST_ASSERT_EQUAL_STRING("", sub.c_str());
    TEST_ASSERT_FALSE(s.substring(5, 5, sub));
    TEST_ASSERT_EQUAL_STRING("", sub.c_str());

    // Substring into smaller EmString (truncation)
    EmString<3> small_sub;
    TEST_ASSERT_TRUE(s.substring(1, 8, small_sub));
    TEST_ASSERT_EQUAL_STRING("123", small_sub.c_str());
}

// Test indexOf()
void test_indexOf() {
    EmString<20> s("hello world, hello");

    // indexOf char
    TEST_ASSERT_EQUAL_INT(2, s.indexOf('l'));
    TEST_ASSERT_EQUAL_INT(3, s.indexOf('l', 3));
    TEST_ASSERT_EQUAL_INT(9, s.indexOf('l', 4));
    TEST_ASSERT_EQUAL_INT(15, s.indexOf('l', 10));
    TEST_ASSERT_EQUAL_INT(-1, s.indexOf('z'));
    TEST_ASSERT_EQUAL_INT(-1, s.indexOf('h', 20));

    // indexOf const char*
    TEST_ASSERT_EQUAL_INT(0, s.indexOf("hello"));
    TEST_ASSERT_EQUAL_INT(13, s.indexOf("hello", 1));
    TEST_ASSERT_EQUAL_INT(6, s.indexOf("world"));
    TEST_ASSERT_EQUAL_INT(-1, s.indexOf("goodbye"));
    TEST_ASSERT_EQUAL_INT(-1, s.indexOf("hello", 20));
    TEST_ASSERT_EQUAL_INT(-1, s.indexOf(nullptr));
}

// Test operators
void test_operators() {
    // operator const char*
    EmString<10> s1("test");
    const char* p = s1;
    TEST_ASSERT_EQUAL_STRING("test", p);

    // operator[]
    TEST_ASSERT_EQUAL_CHAR('t', s1[0]);
    TEST_ASSERT_EQUAL_CHAR('s', s1[2]);
    TEST_ASSERT_EQUAL_CHAR('t', s1[-1]); // last char
    TEST_ASSERT_EQUAL_CHAR('s', s1[-2]);
    TEST_ASSERT_EQUAL_CHAR(0, s1[4]); // out of bounds
    TEST_ASSERT_EQUAL_CHAR(0, s1[-5]); // out of bounds
    TEST_ASSERT_EQUAL_CHAR(0, s1[10]); // out of bounds

    // operator=
    EmString<10> s2;
    s2 = "assigned";
    TEST_ASSERT_EQUAL_STRING("assigned", s2.c_str());
    s2 = s1;
    TEST_ASSERT_EQUAL_STRING("test", s2.c_str());

    // operator== and operator!=
    EmString<10> s3("compare");
    TEST_ASSERT_TRUE(s3 == "compare");
    TEST_ASSERT_FALSE(s3 == "Compare");
    TEST_ASSERT_TRUE(s3 != "something else");
    TEST_ASSERT_FALSE(s3 != "compare");
}

void run_em_string_tests() {
    RUN_TEST(test_constructor_default);
    RUN_TEST(test_constructor_from_cstr);
    RUN_TEST(test_constructor_copy);
    RUN_TEST(test_length_and_capacity);
    RUN_TEST(test_set);
    RUN_TEST(test_format);
    RUN_TEST(test_append);
    RUN_TEST(test_getters);
    RUN_TEST(test_strcmp);
    RUN_TEST(test_startsWith);
    RUN_TEST(test_endsWith);
    RUN_TEST(test_getToken);
    RUN_TEST(test_isToken);
    RUN_TEST(test_substring);
    RUN_TEST(test_indexOf);
    RUN_TEST(test_operators);
}

// In your main test file (e.g., using PlatformIO):
/*
#include <Arduino.h>
#include <unity.h>

// Forward declaration
void run_em_string_tests();

void setUp(void) {
    // set up to run before each test
}

void tearDown(void) {
    // clean up to run after each test
}

void setup() {
    // Wait for >2 secs for the board to stabilize
    delay(2000);

    UNITY_BEGIN();
    run_em_string_tests();
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
*/
