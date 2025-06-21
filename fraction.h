#pragma once

#define FLOAT_MANTISA_SIZE 23
#define FLOAT_EXPONANT_SIZE 8
#define FLOAT_SIGN_SIZE 1
#define FLOAT_EXPONANT_BIAS 127
#define DOUBLE_MANTISA_SIZE 52
#define DOUBLE_CROPED_MANTISA_SIZE 30 // = 30 - 1 - 1	retrieve two bit to ass the implicit and the sign bit
#define DOUBLE_EXPONANT_SIZE 11
#define DOUBLE_SIGN_SIZE 1
#define DOUBLE_EXPONANT_BIAS 1023

typedef union {
	float f;
	struct {
		unsigned int mantisa : FLOAT_MANTISA_SIZE;
		unsigned int exponent : FLOAT_EXPONANT_SIZE;
		unsigned int sign : FLOAT_SIGN_SIZE;
	} parts;
} float_cast;

//						 1
//00000000000000000000000 000000000
//+---------------------+|+------++
//			|			 |	  |	  |
//		mantisse		 |exponant sign	
//						implicit bit

typedef union {
	double d;
	struct {
		uint64_t : DOUBLE_MANTISA_SIZE - DOUBLE_CROPED_MANTISA_SIZE; //padding to remove bits not fitting in a int
		uint64_t mantisa : DOUBLE_CROPED_MANTISA_SIZE; //int size minus bit sign and minus implicit first 1
		uint64_t exponent : DOUBLE_EXPONANT_SIZE;
		uint64_t sign : DOUBLE_SIGN_SIZE;
	} parts;
} double_cast;
//													  1
//0000000000000000000000000000000000000000000000000000 000000000000
//+--------------------++----------------------------+|+---------++
//			|							|			  |		|	  |
//		padding						mantisse		  |	exponant  sign
//													implicit first 1

class Fraction {
private:
	int num;// numerator
	unsigned int den; //denominator
public:
	Fraction(); //zero
	Fraction(int n); //integer
	Fraction(int num, unsigned int den);
	Fraction(int num, int den);
	Fraction(float f);
	Fraction(double d);
	float toFloat() const;
	double toDouble() const;

	void print() const;

	bool operator==(const Fraction& f) const;
	bool operator!=(const Fraction& f) const;

	Fraction& operator+=(const Fraction& f);
	Fraction operator+(const Fraction& f) const;

	Fraction& operator-=(const Fraction& f);
	Fraction operator-(const Fraction& f) const;

	Fraction operator-() const&;
	Fraction operator-() &;
	Fraction&& operator-() &&;

	Fraction& operator*=(const Fraction& f);
	Fraction operator*(const Fraction& f) const;

	Fraction& operator/=(const Fraction& f);
	Fraction operator/(const Fraction& f) const;

	friend Fraction operator/(int i, const Fraction& f);

private:
	void _simplify();
};

Fraction::Fraction() : num(0), den(1) {

}

Fraction::Fraction(int n) : num(n), den(1) {

}

Fraction::Fraction(int num, unsigned int den) : num(num), den(den) {
	_simplify();
}

Fraction::Fraction(int num, int den) : num(num), den(den) {
	if (den < 0) {
		this->den = abs(den);
		this->num *= -1;
	}
	_simplify();
}

Fraction::Fraction(float f) : num(1), den(1) {
	float_cast m_f = { f };

	int true_exp = m_f.parts.exponent - FLOAT_EXPONANT_BIAS - FLOAT_MANTISA_SIZE;
	int true_mantisse_abs = m_f.parts.mantisa | 1 << FLOAT_MANTISA_SIZE;

	if (true_exp >= 0)
		num = 1 << true_exp;
	else
		den = 1 << abs(true_exp);

	num *= true_mantisse_abs;
	if (m_f.parts.sign)
		num *= -1;

	_simplify();
}

Fraction::Fraction(double d) : num(1), den(1) {
	double_cast m_d = { d };

	int64_t true_exp = m_d.parts.exponent - DOUBLE_EXPONANT_BIAS - DOUBLE_CROPED_MANTISA_SIZE;
	uint64_t true_mantisse_abs = m_d.parts.mantisa | static_cast<uint64_t>(1) << DOUBLE_CROPED_MANTISA_SIZE;

	if (true_exp >= 0)
		num = 1 << true_exp;
	else
		den = 1 << abs(true_exp);

	num *= true_mantisse_abs;
	if (m_d.parts.sign)
		num *= -1;

	_simplify();
}

void Fraction::print() const {
	std::cout << num << "/" << den << std::endl;
}

//! internal function, use euclid algorithm to simplify the fraction
void Fraction::_simplify() {
	unsigned int a = abs(num);
	unsigned int b = den;
	unsigned int r;

	while (b != 0) {
		r = a % b;
		a = b;
		b = r;
	}
	num /= a;
	den /= a;
}

Fraction operator/(int i, const Fraction& f) {
	int sign = f.num < 0 ? -1 : 1;
	unsigned int den = abs(f.num);
	int num = f.den * i * sign;
	return Fraction(num, den);
}

bool Fraction::operator==(const Fraction& f) const {
	return num * f.den == f.num * den;
}

bool Fraction::operator!=(const Fraction& f) const {
	return !operator==(f);
}

Fraction& Fraction::operator+=(const Fraction& f) {
	num = num * f.den + f.num * den;
	den *= f.den;
	_simplify();
	return *this;
}

Fraction Fraction::operator+(const Fraction& f) const {
	Fraction dest = Fraction(*this);
	return dest += f;
}

Fraction& Fraction::operator-=(const Fraction& f) {
	num = num * f.den - f.num * den;
	den *= f.den;
	_simplify();
	return *this;
}

Fraction Fraction::operator-(const Fraction& f) const {
	Fraction dest = Fraction(*this);
	return dest -= f;
}

Fraction Fraction::operator-() const& {
	Fraction dest = Fraction(*this);
	dest.num *= -1;
	return dest;
}

Fraction Fraction::operator-()&{
	Fraction dest = Fraction(*this);
	dest.num *= -1;
	return dest;
}

Fraction&& Fraction::operator-()&& {
	num *= -1;
	return std::move(*this);
}

Fraction& Fraction::operator*=(const Fraction& f) {
	num *= f.num;
	den *= f.den;
	_simplify();
	return *this;
}

Fraction Fraction::operator*(const Fraction& f) const {
	Fraction dest = Fraction(*this);
	return dest *= f;
}

Fraction& Fraction::operator /= (const Fraction& f) {
	int sign = f.num < 0 ? -1 : 1;
	num *= f.den * sign;
	den *= abs(f.num);
	_simplify();
	return *this;
}

Fraction Fraction::operator/(const Fraction& f) const {
	Fraction dest = Fraction(*this);
	return dest /= f;
}

float Fraction::toFloat() const {
	return float(num) / float(den);
}
double Fraction::toDouble() const{
	return double(num) / double(den);
}
