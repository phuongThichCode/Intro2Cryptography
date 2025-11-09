// Tuần 8 - Nhóm 1
// 22120407_22120160_22120427_22120429_22120400
// MHUD CQ2022/22

#include "BigInt.h"
#include <random>

BigInt modPow(BigInt base, BigInt exponent, BigInt mod) {
    BigInt result(1);
    base = base % mod;
    while (exponent > 0) {
        if (exponent % 2 == 1) {
            result = (result * base) % mod;
        }
        base = (base * base) % mod;
        exponent = exponent / 2;
    }
    return result;
}

BigInt random_bigInt(BigInt lower, BigInt upper) {
    random_device rd;
    mt19937_64 gen(rd());

    BigInt range = upper - lower + 1;

    BigInt result;
    do {
        uint64_t r = gen();
        result = BigInt(r) % range + lower;
    } while (result < lower || result > upper);

    return result;
}


bool miller_rabin(BigInt n, int k) {

    if (n == 2 || n == 3)
        return true;
    if (n < 2 || n % 2 == 0)
        return false;

    BigInt d = n - 1;
    int s = 0;
    while (d % 2 == 0) {
        s += 1;
        d = d / 2;
    }

    for (int i = 0; i < k; ++i) {
        BigInt a = random_bigInt(2, n - 2);
        BigInt x = modPow(a, d, n);

        if (x == 1 || x == n - 1)
            continue;

        bool composite = true;
        for (int r = 1; r < s; r++) {
            x = (x * x) % n;
            if (x == n - 1) {
                composite = false;
                break;
            }
        }

        if (composite)
            return false;
    }

    return true;
}

int main() {

    BigInt n("2222959131164542537923671368246330146645013471945722985792071311");

    int k = 5;

    if (miller_rabin(n, k)) {
        cout << n << " is prime.\n";
    }
    else cout << n << " is not prime.\n";

    return 0;
}