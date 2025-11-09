// Big Integer class for C++
// Reference: https://github.com/indy256/codelibrary/tree/main/cpp/numeric

#ifndef BigInt_H
#define BigInt_H

#include "fft.h"
#include <iomanip>

constexpr int digits(int base) noexcept {
    return base <= 1 ? 0 : 1 + digits(base / 10);
}
constexpr int base = 1000'000'000;
constexpr int base_digits = digits(base);

constexpr int fft_base = 10'000;  // fft_base^2 * n / fft_base_digits <= 10^15 for double
constexpr int fft_base_digits = digits(fft_base);

using namespace std;

class BigInt {
private:
    vector<int> z;  // Digits
    int sign;       // sign == 1 for positive, -1 for negative

public:
    BigInt(long long v = 0) { *this = v; }  // Constructor from long long
    BigInt(const string& s) { read(s); }  // Constructor from string

    BigInt& operator=(long long v);
    BigInt& operator+=(const BigInt& other);
    BigInt& operator-=(const BigInt& other);
    BigInt& operator*=(int v);
    BigInt& operator/=(int v);
    BigInt& operator*=(const BigInt&);
    BigInt& operator/=(const BigInt&);
    BigInt& operator%=(const BigInt&);

    // Helper methods for multiplication and division
    BigInt mul_simple(const BigInt& v) const;
    friend pair<BigInt, BigInt> divmod(const BigInt&, const BigInt&);

    friend BigInt operator+(BigInt, const BigInt&);
    friend BigInt operator-(BigInt, const BigInt&);
    friend BigInt operator-(BigInt v);
    BigInt operator*(int) const;
    BigInt operator*(const BigInt&) const;
    BigInt operator/(const BigInt&) const;
    BigInt operator/(int) const;
    BigInt operator%(const BigInt&) const;
    int operator%(int) const;

    bool operator<(const BigInt& v) const;
    bool operator>(const BigInt& v) const;
    bool operator<=(const BigInt& v) const;
    bool operator>=(const BigInt& v) const;
    bool operator==(const BigInt& v) const;
    bool operator!=(const BigInt& v) const;

    void trim();  // Remove leading zeros
    bool isZero() const;
    BigInt abs() const;
    long long longValue() const;

    void read(const string& s);

    friend istream& operator>>(istream& stream, BigInt& v);
    friend ostream& operator<<(ostream& stream, const BigInt& v);

    static vector<int> convert_base(const vector<int>& a, int old_digits, int new_digits);
};

#endif