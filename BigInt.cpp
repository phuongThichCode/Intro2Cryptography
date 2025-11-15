#include "BigInt.h"

/*
	Toán tử gán từ kiểu long long sang BigInt.
	Cách làm: lấy dấu, chuyển số về dạng dương, rồi chia theo base để lưu vào vector z.
*/


BigInt& BigInt::operator=(long long v)
{
	sign = v < 0 ? -1 : 1;                  // Lưu dấu
	v *= sign;                              // Chuyển về số dương để dễ xử lý
	z.clear();
	while (v > 0)
    {
        z.push_back((int)(v % base));
        v /= base;
    }
	return *this;
}


/*
	Toán tử cộng gán +=
	Nếu 2 số cùng dấu: cộng từng “khối” trong z, nhớ xử lý carry.
	Nếu khác dấu: quy về phép trừ.
*/

BigInt& BigInt::operator+=(const BigInt& other)
{
	if (sign == other.sign)
	{
		// Hai số cùng dấu → cộng bình thường
		for (int i = 0, carry = 0; i < other.z.size() || carry; ++i)
		{
			if (i == (int)z.size())
				z.push_back(0);
			z[i] += carry + (i < (int)other.z.size() ? other.z[i] : 0);
			carry = z[i] >= base;
			if (carry)
				z[i] -= base;
		}
	}
	else if (other != 0)
	{
		// Khác dấu → a + (-b) = a - b
		*this -= -other;
	}
	return *this;
}

/*
	Toán tử cộng thường: dùng += cho đơn giản.
*/

BigInt operator+(BigInt a, const BigInt& b)
{
	a += b;
	return a;
}
/*
	Toán tử trừ gán -=
	Xử lý rất tương tự cộng nhưng phải so sánh độ lớn để biết số nào lớn hơn.
*/

BigInt& BigInt::operator-=(const BigInt& other)
{
	if (sign == other.sign)
	{
		// Hai số cùng dấu → kiểm tra độ lớn để trừ đúng hướng
		if ((sign == 1 && *this >= other) || (sign == -1 && *this <= other))
		{
			for (int i = 0, carry = 0; i < (int)other.z.size() || carry; ++i)
			{
				z[i] -= carry + (i < (int)other.z.size() ? other.z[i] : 0);
				carry = z[i] < 0;
				if (carry)
					z[i] += base;
			}
			trim();
		}
		else
		{
			// Nếu số bị trừ nhỏ hơn số trừ → đổi chiều và đổi dấu kết quả
			*this = other - *this;
			this->sign = -this->sign;
		}
	}
	else
	{
		// a - (-b) = a + b
		*this += -other;
	}
	return *this;
}

/*
	Toán tử trừ thường: dựa trên -=
*/

BigInt operator-(BigInt a, const BigInt& b)
{
	a -= b;
	return a;
}

/*
	Đổi dấu số (negate).
*/


BigInt operator-(BigInt v)
{
	if (!v.z.empty())
		v.sign = -v.sign;
	return v;
}

/*
	Nhân BigInt với số nguyên int.
	Cách làm: nhân từng block trong vector z, xử lý carry.
*/

BigInt& BigInt::operator*=(int v)
{
	if (v < 0)
		sign = -sign, v = -v;
	for (int i = 0, carry = 0; i < (int)z.size() || carry; ++i)
	{
		if (i == (int)z.size())
			z.push_back(0);
			long long cur = 1LL * z[i] * v + carry;
		carry = (int)(cur / base);
		z[i] = (int)(cur % base);
	}
	trim();
	return *this;
}
BigInt& BigInt::operator*=(const BigInt& v)
{
	*this = *this * v;
	return *this;
}

/*
	Toán tử * với int
*/

BigInt BigInt::operator*(int v) const
{
	return BigInt(*this) *= v;
}

/*
	Nhân 2 BigInt.
	Ngưỡng 150 block: nhỏ thì dùng nhân thường, lớn thì dùng FFT.
*/

BigInt BigInt::operator*(const BigInt& v) const
{
	if (min(z.size(), v.z.size()) < 150)
		return mul_simple(v);
	BigInt res;
	res.sign = sign * v.sign;
	res.z = multiply_bigint(convert_base(z, base_digits, fft_base_digits),
		convert_base(v.z, base_digits, fft_base_digits), fft_base);

		// Chuyển kết quả về base chuẩn

	res.z = convert_base(res.z, fft_base_digits, base_digits);
	res.trim();
	return res;
}
/*
	Chia BigInt cho int. Làm từ trái sang phải, luôn giữ remainder.
*/

BigInt& BigInt::operator/=(int v)
{
	if (v < 0)
		sign = -sign, v = -v;
	for (int i = (int)z.size() - 1, rem = 0; i >= 0; --i)
	{
		long long cur = z[i] + rem * 1LL * base;
		z[i] = (int)(cur / v);
		rem = (int)(cur % v);
	}
	trim();
	return *this;
}
BigInt& BigInt::operator/=(const BigInt& v)
{
	*this = *this / v;
	return *this;
}
BigInt& BigInt::operator%=(const BigInt& v)
{
	*this = *this % v;
	return *this;
}

/*
	Hàm chia lấy cả thương và dư.
	Thuật toán: chuẩn hóa (normalize) rồi chia long division.
*/

pair<BigInt, BigInt> divmod(const BigInt& a1, const BigInt& b1)
{
	int norm = base / (b1.z.back() + 1);
	BigInt a = a1.abs() * norm;
	BigInt b = b1.abs() * norm;
	BigInt q, r;
	q.z.resize(a.z.size());

	// Chia từ block lớn nhất xuống nhỏ nhất

	for (int i = (int)a.z.size() - 1; i >= 0; i--)
	{
		r *= base;
		r += a.z[i];
		int s1 = b.z.size() < r.z.size() ? r.z[b.z.size()] : 0;
		int s2 = b.z.size() - 1 < r.z.size() ? r.z[b.z.size() - 1] : 0;

		// Ước lượng chữ số chia 

		int d = (int)(((long long)s1 * base + s2) / b.z.back());
		r -= b * d;
		while (r < 0)
			r += b, --d;
		q.z[i] = d;
	}

	q.sign = a1.sign * b1.sign;
	r.sign = a1.sign;
	q.trim();
	r.trim();
	return { q, r / norm };
}

/*
	Nhân đơn giản O(n²). Dùng khi số không quá lớn.
*/


BigInt BigInt::mul_simple(const BigInt& v) const
{
	BigInt res;
	res.sign = sign * v.sign;
	res.z.resize(z.size() + v.z.size());
	for (int i = 0; i < (int)z.size(); ++i)
		if (z[i])
			for (int j = 0, carry = 0; j < (int)v.z.size() || carry; ++j)
			{
				long long cur = res.z[i + j] + (long long)z[i] * (j < v.z.size() ? v.z[j] : 0) + carry;
				carry = (int)(cur / base);
				res.z[i + j] = (int)(cur % base);
			}
	res.trim();
	return res;
}

/*
	Toán tử chia / và % dùng divmod()
*/


BigInt BigInt::operator/(const BigInt& v) const
{
	return divmod(*this, v).first;
}

BigInt BigInt::operator/(int v) const
{
	return BigInt(*this) /= v;
}

BigInt BigInt::operator%(const BigInt& v) const
{
	return divmod(*this, v).second;
}

/*
	Mod với số int
*/


int BigInt::operator%(int v) const
{
	if (v < 0)
		v = -v;
	int m = 0;
	for (int i = (int)z.size() - 1; i >= 0; --i)
		m = (int)((z[i] + m * 1LL*base) % v);
	return m * sign;
}

/*
	Các toán tử so sánh
*/

bool BigInt::operator<(const BigInt& v) const
{
	if (sign != v.sign)
		return sign < v.sign;
	if (z.size() != v.z.size())
		return z.size() * sign < v.z.size() * v.sign;
	for (int i = (int)z.size() - 1; i >= 0; i--)
		if (z[i] != v.z[i])
			return z[i] * sign < v.z[i] * sign;
	return false;
}

bool BigInt::operator>(const BigInt& v) const { return v < *this; }

bool BigInt::operator<=(const BigInt& v) const { return !(v < *this); }

bool BigInt::operator>=(const BigInt& v) const { return !(*this < v); }

bool BigInt::operator==(const BigInt& v) const { return sign == v.sign && z == v.z; }

bool BigInt::operator!=(const BigInt& v) const { return !(*this == v); }

/*
	Xóa các số 0 vô nghĩa cuối vector
*/

void BigInt::trim()
{
	while (!z.empty() && z.back() == 0)
		z.pop_back();
	if (z.empty())
		sign = 1;
}

bool BigInt::isZero() const { return z.empty(); }

BigInt BigInt::abs() const { return sign == 1 ? *this : -*this; }

long long BigInt::longValue() const
{
	/*
	Chuyển BigInt về long long (cẩn thận tràn!)
    */

	long long res = 0;
	for (int i = (int)z.size() - 1; i >= 0; i--)
		res = res * base + z[i];
	return res * sign;
}

/*
	Đọc BigInt từ string (nhập input).
*/


void BigInt::read(const string& s)
{
	sign = 1;
	z.clear();
	int pos = 0;
	while (pos < s.size() && (s[pos] == '-' || s[pos] == '+'))
	{
		if (s[pos] == '-')
			sign = -sign;
		++pos;
	}
	for (int i = (int)s.size() - 1; i >= pos; i -= base_digits)
	{
		int x = 0;
		for (int j = max(pos, i - base_digits + 1); j <= i; j++)
			x = x * 10 + s[j] - '0';
		z.push_back(x);
	}
	trim();
}

/*
	Nhập xuất BigInt qua stream
*/


istream& operator>>(istream& stream, BigInt& v)
{
	string s;
	stream >> s;
	v.read(s);
	return stream;
}

ostream& operator<<(ostream& stream, const BigInt& v)
{
	if (v.sign == -1)
		stream << '-';
	// In block cao nhất (không có số 0 đệm)
	stream << (v.z.empty() ? 0 : v.z.back());

	// In các block còn lại có đệm 0 cho đủ base_digits
	for (int i = (int)v.z.size() - 2; i >= 0; --i)
		stream << setw(base_digits) << setfill('0') << v.z[i];
	return stream;
}

/*
	Hàm chuyển đổi base (đổi số lượng chữ số mỗi block).
	Dùng khi muốn nhân FFT.
*/


vector<int> BigInt::convert_base(const vector<int>& a, int old_digits, int new_digits)
{
	vector<long long> p(max(old_digits, new_digits) + 1);
	p[0] = 1;
	for (int i = 1; i < p.size(); i++)
		p[i] = p[i - 1] * 10;
	vector<int> res;
	long long cur = 0;
	int cur_digits = 0;
	for (int v : a)
	{
		cur += v * p[cur_digits];
		cur_digits += old_digits;
		
		while (cur_digits >= new_digits)
		{
			res.push_back(int(cur % p[new_digits]));
			cur /= p[new_digits];
			cur_digits -= new_digits;
		}
	}
	res.push_back((int)cur);
	while (!res.empty() && res.back() == 0)
		res.pop_back();
	return res;
}