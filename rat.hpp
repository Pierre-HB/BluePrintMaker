#pragma once

#include "num.hpp"

#define FLOAT_MANTISA_SIZE 23
#define FLOAT_EXPONANT_SIZE 8
#define FLOAT_SIGN_SIZE 1
#define FLOAT_EXPONANT_BIAS 127

typedef union {
    float f;
    struct {
        unsigned int mantisa : FLOAT_MANTISA_SIZE;
        unsigned int exponent : FLOAT_EXPONANT_SIZE;
        unsigned int sign : FLOAT_SIGN_SIZE;
    } parts;
} float_cast;


struct Rat {
    Num num, den;

    Rat() : num(0), den(1) {}

    Rat(int num) : num(num), den(1) {}

    Rat(float f): num(1), den(1){
        float_cast m_f = { f };

        int true_exp = m_f.parts.exponent - FLOAT_EXPONANT_BIAS - FLOAT_MANTISA_SIZE;
        int true_mantisse_abs = m_f.parts.mantisa | 1 << FLOAT_MANTISA_SIZE;

        if (true_exp >= 0)
            num = num << true_exp;
        else
            den = den << abs(true_exp);

        num *= true_mantisse_abs;

        if (m_f.parts.sign)
            num *= -1;

        reduce();
    }

    Rat(const Num &num): num(num), den(1){}

    Rat(const Num &num, const Num &den): num(num), den(den){
        reduce();
    }

    void reduce(){
        Num a = Num::gcd(num, den);
        if (a != 1){
            num /= a;
            den /= a;
        }
    }

    Rat inv() const {
        return Rat(den, num);
    }

    static Rat add(const Rat &a, const Rat &b){
        return Rat(a.num * b.den + a.den*b.num, a.den * b.den);
    }

    static Rat sub(const Rat &a, const Rat &b){
        return Rat(a.num * b.den - a.den*b.num, a.den * b.den);
    }

    static Rat mul(const Rat &a, const Rat &b){
        return Rat(a.num * b.num, a.den * b.den);
    }

    static Rat div(const Rat &a, const Rat &b){
        return Rat(a.num * b.den, a.den * b.num);
    }

    static int cmp(const Rat &a, const Rat &b){
        int result = Num::cmp(a.num * b.den, b.num * a.den);
        bool different_sign = (a.den < 0) != (b.den < 0);
        return different_sign ? -result : result;
    }

    double to_double() const {
        return num.to_double() / den.to_double();
    }

    void print(std::vector<char> &c){
        num.print(c);
        c.back() = '/';
        den.print(c);
    }

    Rat& operator += (const Rat &b){ return *this = add(*this, b); }
    Rat& operator -= (const Rat &b){ return *this = sub(*this, b); }
    Rat& operator *= (const Rat &b){ return *this = mul(*this, b); }
    Rat& operator /= (const Rat &b){ return *this = div(*this, b); }

    bool operator == (const Rat &b) const { return cmp(*this, b) == 0; }
    bool operator != (const Rat &b) const { return cmp(*this, b) != 0; }
    bool operator <= (const Rat &b) const { return cmp(*this, b) <= 0; }
    bool operator >= (const Rat &b) const { return cmp(*this, b) >= 0; }
    bool operator <  (const Rat &b) const { return cmp(*this, b) <  0; }
    bool operator >  (const Rat &b) const { return cmp(*this, b) >  0; }

    Rat operator + (const Rat &b) const { return add(*this, b); }
    Rat operator - (const Rat &b) const { return sub(*this, b); }
    Rat operator * (const Rat &b) const { return mul(*this, b); }
    Rat operator / (const Rat &b) const { return div(*this, b); }
    Rat operator - (            ) const { return Rat(-num, den); }

    friend Rat operator/(int i, const Rat& b);
};

Rat operator/(int i, const Rat& b) {
    return Rat(i) / b;
}