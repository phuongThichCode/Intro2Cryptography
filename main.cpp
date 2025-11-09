#include <iostream>
#include <random>
#include <ctime>
#include <cstdlib>
#include <string>
#include <bitset>
#include <sstream>
#include "BigInt.h"

using namespace std;

// A: Modular exponentiation function
// Computes (base^exponent) % mod efficiently using binary exponentiation
// This handles large numbers using BigInt for 512+ bit arithmetic
BigInt modular_exponentiation(BigInt base, BigInt exponent, BigInt mod) {
    BigInt result = 1;
    base = base % mod;
    
    // Binary exponentiation algorithm
    while (exponent > 0) {
        // If exponent is odd, multiply base with result
        if (exponent % 2 == 1) {
            result = (result * base) % mod;
        }
        // Square the base and halve the exponent
        exponent = exponent / 2;
        base = (base * base) % mod;
    }
    
    return result;
}

// Helper function: Generate random BigInt with specified number of bits
BigInt generate_random_bits(int bits) {
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<unsigned long long> dis(0, ULLONG_MAX);
    
    // Build random number by concatenating random chunks
    string num_str = "";
    
    // Generate first digit to ensure it's in the right range (MSB = 1)
    int full_chunks = bits / 64;
    int remaining_bits = bits % 64;
    
    // Start with a 1 bit to ensure minimum size
    BigInt result = 1;
    for (int i = 0; i < bits - 1; i++) {
        result = result * 2;
        if (dis(gen) % 2 == 1) {
            result = result + 1;
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

// C: Generate a private key
// Private key should be in range [2, p-2]
BigInt generate_private_key(BigInt p) {
    // Generate random number in range [2, p-2]
    BigInt range = p - 3;  // p - 2 - 2 + 1
    BigInt private_key = generate_random_bits(256) % range + 2;
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
    
    cout << endl;
    cout << "Prime p (" << bit_size << "-bit) = " << p << endl;
    cout << "Generator g = " << g << endl;
    cout << endl;
    
    // 2. Generate private keys for Alice and Bob
    cout << "Step 2: Generating private keys" << endl;
    cout << "-------------------------------------------" << endl;
    BigInt a = generate_private_key(p);  // Alice's private key
    BigInt b = generate_private_key(p);  // Bob's private key
    
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
