void test_surgefuzz_freq();
void test_surgefuzz_consec();
void test_surgefuzz_count();
void test_rfuzz();
void test_difuzzrtl();
void test_directfuzz();

int main() {
    test_surgefuzz_freq();
    test_surgefuzz_consec();
    test_surgefuzz_count();

    test_rfuzz();
    test_difuzzrtl();
    test_directfuzz();
    return 0;
}
