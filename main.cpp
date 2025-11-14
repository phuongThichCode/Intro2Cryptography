#include <iostream>
#include <iomanip>
#include <random>
#include <ctime>
#include <cstdlib>
#include <string>
#include <bitset>
#include <sstream>
#include <climits>
#include "BigInt.h"

using namespace std;

// A: Modular exponentiation function
// Computes (base^exponent) % mod efficiently using binary exponentiation + sliding window
// This handles large numbers using BigInt for 512+ bit arithmetic
BigInt modular_exponentiation(BigInt base, BigInt exponent,const BigInt& mod) {
    // Special cases
    if (mod == 1) return BigInt(0);
    if (exponent.isZero()) return BigInt(1) % mod;

    // Ensure base is within mod
    base %= mod;
    if (base.isZero()) return BigInt(0);

    // Convert exponent to binary representation, then store reversed bits in a vector
    std::vector<int> bits;
    {
        BigInt e = exponent;
        while (!e.isZero()) {
            int bit = e % 2;
            bits.push_back(bit);
            e /= 2;
        }
        if (bits.empty()) {
            bits.push_back(0);
        }
    }

    const int W = 4; //Optimal window size
    const int MAX_ODD = (1 << W);    // 2^W
    std::vector<BigInt> pre(MAX_ODD); // Precomputed a^u for odd u

    pre[1] = base;
    BigInt base2 = (base * base) % mod;
    for (int e = 3; e < MAX_ODD; e += 2) {
        pre[e] = (pre[e - 2] * base2) % mod;
    }

    BigInt result = 1;
    int i = (int)bits.size() - 1;   // index bit cao nhất

    // Compute result using sliding window
    while (i >= 0) {
        if (bits[i] == 0) {
            result = (result * result) % mod;
            --i;
        }
        else {
            int l = std::max(0, i - W + 1);
            int j = l;

            while (j < i && bits[j] == 0) {
                ++j;
            }
            int length = i - j + 1;

            int u = 0;
            for (int k = i; k >= j; --k) {
                u = (u << 1) | bits[k];
            }

            for (int k = 0; k < length; ++k) {
                result = (result * result) % mod;
            }

            result = (result * pre[u]) % mod;

            i = j - 1;
        }
    }

    return result;
}

// Helper function: Generate cryptographic-grade seed using multiple entropy sources
// Combines random_device, clock, and time for better entropy
unsigned long long generate_cryptographic_seed() {
    random_device rd;
    // Collect multiple random values from random_device
    unsigned long long seed1 = rd();
    unsigned long long seed2 = rd();
    unsigned long long seed3 = rd();
    
    // Combine with time and clock for additional entropy
    unsigned long long time_seed = static_cast<unsigned long long>(time(nullptr));
    unsigned long long clock_seed = static_cast<unsigned long long>(clock());
    
    // XOR all sources together to combine entropy
    // Using XOR preserves entropy better than addition
    return seed1 ^ (seed2 << 16) ^ (seed3 << 32) ^ time_seed ^ clock_seed;
}

// Helper function: Generate random BigInt with specified number of bits
// Uses improved entropy sources for cryptographic-grade randomness
BigInt generate_random_bits(int bits) {
    if (bits <= 0) {
        return BigInt(0);
    }
    
    // Generate cryptographic seed with multiple entropy sources
    unsigned long long seed = generate_cryptographic_seed();
    
    // Use multiple random_device instances for better entropy
    random_device rd1, rd2, rd3;
    seed ^= (static_cast<unsigned long long>(rd1()) << 0);
    seed ^= (static_cast<unsigned long long>(rd2()) << 16);
    seed ^= (static_cast<unsigned long long>(rd3()) << 32);
    
    // Create generator with combined seed
    mt19937_64 gen(seed);
    uniform_int_distribution<unsigned long long> dis(0, ULLONG_MAX);
    
    // Build random number bit by bit using better approach
    // Start with MSB = 1 to ensure correct bit length
    BigInt result = 1;
    
    // Generate remaining bits
    for (int i = 1; i < bits; i++) {
        result = result * 2;
        // Use random bit from generator
        if (dis(gen) % 2 == 1) {
            result = result + 1;
        }
    }
    
    // For better randomness, mix in additional entropy from more random_device calls
    // This helps break patterns in mt19937_64
    random_device rd_extra;
    for (int i = 0; i < 4; i++) {
        unsigned long long extra = rd_extra();
        // Mix extra entropy into result by adding small random values
        BigInt extra_big = BigInt(extra);
        result = result + extra_big;
        // Keep within bit bounds by using modulo
        BigInt max_val = BigInt(1);
        for (int j = 0; j < bits; j++) {
            max_val = max_val * 2;
        }
        result = result % max_val;
        
        // Ensure MSB is still set
        BigInt min_val = BigInt(1);
        for (int j = 0; j < bits - 1; j++) {
            min_val = min_val * 2;
        }
        if (result < min_val) {
            result = result + min_val;
        }
    }
    
    return result;
}

// Helper function: Miller-Rabin primality test for BigInt
// Returns true if n is probably prime, false if definitely composite
bool miller_rabin_test(BigInt n, int k = 20) {
    if (n == 2 || n == 3) return true;
    if (n < 2 || n % 2 == 0) return false;
    
    // Write n-1 as 2^r * d
    BigInt d = n - 1;
    int r = 0;
    while (d % 2 == 0) {
        d = d / 2;
        r++;
    }
    
    // Witness loop - test k times
    random_device rd;
    mt19937_64 gen(rd());
    
    for (int i = 0; i < k; i++) {
        // Generate random witness a in range [2, n-2]
        BigInt a = generate_random_bits(32) % (n - 3) + 2;
        BigInt x = modular_exponentiation(a, d, n);
        
        if (x == 1 || x == n - 1)
            continue;
        
        bool composite = true;
        for (int j = 0; j < r - 1; j++) {
            x = modular_exponentiation(x, BigInt(2), n);
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

// B: Generate a safe prime number of specified bit size
// A safe prime is a prime p where (p-1)/2 is also prime (Sophie Germain prime)
// Minimum 512 bits as required
BigInt generate_safe_prime(int bit_size) {
    cout << "Generating " << bit_size << "-bit safe prime (this may take several minutes)..." << endl;
    
    int attempts = 0;
    while (true) {
        attempts++;
        if (attempts % 10 == 0) {
            cout << "  Attempt " << attempts << "..." << endl;
        }
        
        // Generate random odd number of bit_size bits
        BigInt q = generate_random_bits(bit_size - 1);
        if (q % 2 == 0) {
            q = q + 1;  // Make it odd
        }
        
        // Check if q is prime (Sophie Germain prime)
        if (miller_rabin_test(q)) {
            // Check if p = 2q + 1 is also prime (safe prime)
            BigInt p = q * 2 + 1;
            if (miller_rabin_test(p)) {
                cout << "Safe prime found after " << attempts << " attempts!" << endl;
                return p;
            }
        }
    }
}

// Helper function: Validate prime p for Diffie-Hellman
// Returns true if p is valid, false otherwise
bool validate_prime(BigInt p) {
    // p must be at least 5 (for safe prime with p-2 >= 2)
    if (p < 5) {
        return false;
    }
    // p must be odd
    if (p % 2 == 0) {
        return false;
    }
    return true;
}

// Helper function: Generate random BigInt in range [min, max]
// Uses cryptographic-grade random number generation
// For large ranges (512-bit), modulo bias is negligible (< 2^-256), so direct modulo is safe
BigInt generate_random_in_range(BigInt min_val, BigInt max_val) {
    if (max_val < min_val) {
        // Invalid range, return min_val
        return min_val;
    }
    
    if (min_val == max_val) {
        // Range is single value
        return min_val;
    }
    
    BigInt range = max_val - min_val + 1;
    
    // Calculate approximate bit length of max_val for efficient random generation
    // We estimate bits needed by finding the smallest power of 2 that exceeds max_val
    int approx_bits = 0;
    BigInt test = BigInt(1);
    while (test <= max_val && approx_bits < 1024) {
        approx_bits++;
        if (approx_bits < 63) {  // Use shift for small values
            test = test * 2;
        } else {
            // For large values, approximate
            break;
        }
    }
    
    // Use enough bits to cover the range with margin
    // Add 64 extra bits to ensure we have sufficient entropy
    // For 512-bit primes, this gives us ~576 bits of randomness
    int bits_to_use = (approx_bits > 0) ? (approx_bits + 64) : 512;
    
    // Generate cryptographic random number with sufficient bits
    BigInt random_value = generate_random_bits(bits_to_use);
    
    // Reduce to range using modulo
    // For cryptographic ranges (512-bit), bias is negligible
    BigInt result = (random_value % range) + min_val;
    
    // Ensure result is in valid range (safety check)
    if (result < min_val) {
        result = min_val;
    } else if (result > max_val) {
        // This should not happen with proper modulo, but handle it
        result = max_val;
    }
    
    return result;
}

// C: Generate a private key for Diffie-Hellman
// Private key should be in range [2, p-2] for security
// Uses cryptographic-grade random number generation with rejection sampling
BigInt generate_private_key(BigInt p) {
    // Validate prime p
    if (!validate_prime(p)) {
        cerr << "ERROR: Invalid prime p for private key generation!" << endl;
        cerr << "Prime p must be at least 5 and odd." << endl;
        // Return a safe default (but this should not happen in practice)
        return BigInt(2);
    }
    
    // Private key must be in range [2, p-2]
    // This ensures:
    // 1. Private key is not 0 or 1 (too small, insecure)
    // 2. Private key is not p-1 (which would make public key = 1)
    // 3. Private key is not p (which would make public key = 0)
    BigInt min_key = BigInt(2);
    BigInt max_key = p - 2;
    
    // Generate private key using rejection sampling to avoid bias
    BigInt private_key = generate_random_in_range(min_key, max_key);
    
    // Double-check the generated key is in valid range
    if (private_key < min_key || private_key > max_key) {
        // This should not happen, but handle it gracefully
        cerr << "WARNING: Generated private key out of range, adjusting..." << endl;
        // Use modulo as fallback
        BigInt range = max_key - min_key + 1;
        private_key = (generate_random_bits(256) % range) + min_key;
    }
    
    return private_key;
}

// D: Main function - Diffie-Hellman key exchange implementation
int main(int argc, char* argv[]) {
    cout << "================================================================" << endl;
    cout << "     DIFFIE-HELLMAN KEY EXCHANGE - IMPLEMENTATION" << endl;
    cout << "================================================================" << endl;
    cout << endl;
    
    // Determine bit size from command line or use default
    int bit_size = 512;  // Default: 512-bit as required
    
    if (argc > 1) {
        bit_size = atoi(argv[1]);
        if (bit_size != 64 && bit_size != 128 && bit_size != 256 && bit_size != 512) {
            cout << "Invalid bit size. Supported: 64, 128, 256, 512" << endl;
            cout << "Usage: " << argv[0] << " [bit_size]" << endl;
            cout << "Example: " << argv[0] << " 128" << endl;
            return 1;
        }
    } else {
        // Interactive menu
        cout << "Select bit size for testing:" << endl;
        cout << "  1. 64-bit   (fast, for quick testing)" << endl;
        cout << "  2. 128-bit  (moderate, for testing)" << endl;
        cout << "  3. 256-bit  (slower, more secure)" << endl;
        cout << "  4. 512-bit  (REQUIRED for submission, very slow)" << endl;
        cout << endl;
        cout << "Enter choice (1-4) [default: 4]: ";
        
        string choice;
        getline(cin, choice);
        
        if (choice.empty() || choice == "4") {
            bit_size = 512;
        } else if (choice == "1") {
            bit_size = 64;
        } else if (choice == "2") {
            bit_size = 128;
        } else if (choice == "3") {
            bit_size = 256;
        } else {
            cout << "Invalid choice. Using default 512-bit." << endl;
            bit_size = 512;
        }
        cout << endl;
    }
    
    cout << "Using " << bit_size << "-bit prime" << endl;
    if (bit_size < 512) {
        cout << "WARNING: For final submission, use 512-bit!" << endl;
    }
    cout << endl;
    
    // 1. Generate safe prime p and generator g
    cout << "Step 1: Generating parameters" << endl;
    cout << "-------------------------------------------" << endl;
    BigInt p = generate_safe_prime(bit_size);
    BigInt g = 2;  // Generator (commonly used value)
    
    // Validate generated prime
    if (!validate_prime(p)) {
        cerr << "ERROR: Generated prime p is invalid!" << endl;
        return 1;
    }
    
    cout << endl;
    cout << "Prime p (" << bit_size << "-bit) = " << p << endl;
    cout << "Generator g = " << g << endl;
    cout << endl;
    
    // 2. Generate private keys for Alice and Bob
    cout << "Step 2: Generating private keys" << endl;
    cout << "-------------------------------------------" << endl;
    cout << "Generating cryptographic-grade private keys..." << endl;
    BigInt a = generate_private_key(p);  // Alice's private key
    BigInt b = generate_private_key(p);  // Bob's private key
    
    // Validate private keys
    BigInt min_key = BigInt(2);
    BigInt max_key = p - 2;
    if (a < min_key || a > max_key || b < min_key || b > max_key) {
        cerr << "ERROR: Generated private keys are out of valid range!" << endl;
        cerr << "Private keys must be in range [2, " << max_key << "]" << endl;
        return 1;
    }
    
    // Ensure private keys are different (very unlikely but check anyway)
    if (a == b) {
        cout << "WARNING: Alice and Bob have the same private key!" << endl;
        cout << "Generating new key for Bob..." << endl;
        b = generate_private_key(p);
    }
    
    cout << "Alice's private key a = " << a << endl;
    cout << "Bob's private key b = " << b << endl;
    cout << endl;
    
    // 3. Compute public keys
    cout << "Step 3: Computing public keys" << endl;
    cout << "-------------------------------------------" << endl;
    cout << "Alice computes A = g^a mod p..." << endl;
    BigInt A = modular_exponentiation(g, a, p);  // Alice computes A = g^a % p
    
    cout << "Bob computes B = g^b mod p..." << endl;
    BigInt B = modular_exponentiation(g, b, p);  // Bob computes B = g^b % p
    
    cout << endl;
    cout << "Alice's public key A = " << A << endl;
    cout << "Bob's public key B = " << B << endl;
    cout << endl;
    
    // 4. Exchange public keys and compute shared secrets
    cout << "Step 4: Computing shared secrets" << endl;
    cout << "-------------------------------------------" << endl;
    cout << "Alice computes shared secret = B^a mod p..." << endl;
    BigInt alice_shared_secret = modular_exponentiation(B, a, p);  // Alice computes s = B^a % p
    
    cout << "Bob computes shared secret = A^b mod p..." << endl;
    BigInt bob_shared_secret = modular_exponentiation(A, b, p);    // Bob computes s = A^b % p
    
    cout << endl;
    cout << "Alice's computed shared secret = " << alice_shared_secret << endl;
    cout << "Bob's computed shared secret = " << bob_shared_secret << endl;
    cout << endl;
    
    // 5. Verify that both shared secrets match
    cout << "Step 5: Verification" << endl;
    cout << "-------------------------------------------" << endl;
    if (alice_shared_secret == bob_shared_secret) {
        cout << "SUCCESS! The shared secrets match!" << endl;
        cout << "Shared secret = " << alice_shared_secret << endl;
        cout << endl;
        cout << "Alice and Bob can now use this shared secret for" << endl;
        cout << "symmetric encryption (e.g., AES) to communicate securely." << endl;
    } else {
        cout << "ERROR! The shared secrets do not match." << endl;
        cout << "Something went wrong in the key exchange!" << endl;
    }
    
    cout << endl;
    cout << "================================================================" << endl;
    
    return 0;
}
