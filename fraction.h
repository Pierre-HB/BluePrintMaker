#pragma once

#include "int_inf.h"

static int PGCD(int a, int b) {
	assert(a >= 0);
	assert(b >= 0);
	int r;
	while (b != 0) {
		r = a % b;
		a = b;
		b = r;
	}
	return a;
}

static int64_t PGCD(int64_t a, int64_t b) {
	assert(a >= 0);
	assert(b >= 0);
	int64_t r;
	while (b != 0) {
		r = a % b;
		a = b;
		b = r;
	}
	return a;
}

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
//		
//											implicit first 1

typedef union {
	double d;
	struct {
		uint64_t mantisa : DOUBLE_MANTISA_SIZE;
		uint64_t exponent : DOUBLE_EXPONANT_SIZE;
		uint64_t sign : DOUBLE_SIGN_SIZE;
	} parts;
} double_cast_to_int64_t;
//													  1
//0000000000000000000000000000000000000000000000000000 000000000000
//+--------------------------------------------------+|+---------++
//										|			  |		|	  |
//									mantisse		  |	exponant  sign
//		

class Fraction {
private:
	int64_t num;// numerator
	int64_t den; //denominator
public:
	Fraction(); //zero
	Fraction(int64_t n); //integer
	Fraction(int64_t num, int64_t den);
	Fraction(int n); //integer
	Fraction(int num, int den);
	Fraction(float f);
	Fraction(double d);
	Fraction(const Fraction& f);
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

Fraction::Fraction(int64_t n) : num(n), den(1) {

}

Fraction::Fraction(int64_t num, int64_t den) : num(num), den(den) {
	if (den < 0) {
		this->den = abs(den);
		this->num *= -1;
	}
	_simplify();
}

Fraction::Fraction(int n) : Fraction(static_cast<int64_t>(n)) {

}

Fraction::Fraction(int num, int den) : num(static_cast<int64_t>(num)), den(static_cast<int64_t>(den)) {

}

Fraction::Fraction(const Fraction& f) : num(f.num), den(f.den) {

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
	/*double_cast m_d = { d };

	int64_t true_exp = m_d.parts.exponent - DOUBLE_EXPONANT_BIAS - DOUBLE_CROPED_MANTISA_SIZE;
	int64_t true_mantisse_abs = m_d.parts.mantisa | static_cast<uint64_t>(1) << DOUBLE_CROPED_MANTISA_SIZE;*/


	double_cast_to_int64_t m_d = { d };

	int64_t true_exp = m_d.parts.exponent - DOUBLE_EXPONANT_BIAS - DOUBLE_MANTISA_SIZE;
	int64_t true_mantisse_abs = m_d.parts.mantisa | static_cast<int64_t>(1) << DOUBLE_MANTISA_SIZE;
	

	if (true_exp >= 0)
		num = static_cast<int64_t>(1) << true_exp;
	else
		den = static_cast<int64_t>(1) << abs(true_exp);

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
	int64_t pgcd = PGCD(abs(num), den);
	num /= pgcd;
	den /= pgcd;
}

Fraction operator/(int i, const Fraction& f) {
	int64_t sign = f.num < 0 ? -1 : 1;
	int64_t den_1 = abs(f.num);
	int64_t num_1 = f.den * (int64_t)i * sign;
	return Fraction(num_1, den_1);
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
	std::cout << "multiplycation of : " << std::endl;
	dest.print();
	f.print();
	return dest *= f;
}

Fraction& Fraction::operator/=(const Fraction& f) {
	int64_t sign = f.num < 0 ? -1 : 1;
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
