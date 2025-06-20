#pragma once

#include <exception>

template<typename T>
class SMatrix {
private:
	int n_2; //store half of the dimension
	T value;
	SMatrix<T>* blocks[4];
	/*
	* blocks idx :
	|0 1|
	|2 3|
	*/
public:
	SMatrix(int n);
	SMatrix(const SMatrix<T>& m);//copy
	SMatrix(SMatrix<T>&& m) noexcept;//moove
	~SMatrix();

	int get_n() const;

	T at(int line, int column) const;
	void insert(const T& x, int line, int column);

	SMatrix<T>& operator=(SMatrix<T>&& m) noexcept;//move assign
	SMatrix<T>& operator=(const SMatrix<T>& m) noexcept;//copy assign

	SMatrix<T>& operator+=(const SMatrix<T>& other);
	SMatrix<T> operator+(const SMatrix<T>& other) const;
	SMatrix<T>&& operator+(SMatrix<T>&& other) const;

	SMatrix<T>& operator-=(const SMatrix<T>& other);
	SMatrix<T> operator-(const SMatrix<T>& other) const;
	SMatrix<T>&& operator-(SMatrix<T>&& other) const;

	SMatrix<T> operator-() const&;
	SMatrix<T> operator-() &;
	SMatrix<T>&& operator-() &&;

	SMatrix<T> operator*(const SMatrix<T>& other) const;

	void transpose();
	SMatrix<T> transposed() const;
	
	bool is_zero() const;

	SMatrix<T> inversed(bool* inverted = nullptr) const;
	
private:
	SMatrix<T>* _get_block(int line, int column) const; //return the block containing the cell
	void _split();
	void _split(SMatrix<T>* const other[4]);//_split and copy
	SMatrix<T>&& _add_mult(const SMatrix<T>& other, SMatrix<T>& dest) const;
	void _clean();//check if childs are 0 and so, delete them
	SMatrix<T>& _self_neg(); //self negation
	void _inverse();
};

//! emtpy constructor
template<typename T>
SMatrix<T>::SMatrix(int n) : n_2(n/2), value(T()), blocks{ nullptr, nullptr, nullptr, nullptr } {
	assert(n == 1 || n % 2 == 0);
}

//! copy constructor
template<typename T>
SMatrix<T>::SMatrix(const SMatrix<T>& m) : n_2(m.n_2), value(T(m.value)), blocks{ nullptr, nullptr, nullptr, nullptr } {
	if (m.blocks[0] != nullptr)
		for(int i = 0; i < 4; i++)
			blocks[i] = new SMatrix<T>(*(m.blocks[i]));
}

//! move constructor
template<typename T>
SMatrix<T>::SMatrix(SMatrix<T>&& m) noexcept  : n_2(m.n_2), value(T(m.value)) {
	for (int i = 0; i < 4; i++) {
		blocks[i] = m.blocks[i];
		m.blocks[i] = nullptr;
	}

	m.n_2 = 0;
	m.value = T();
}

//! move assignement
template<typename T>
SMatrix<T>& SMatrix<T>::operator=(SMatrix<T>&& other) noexcept {
	if (this != &other) {
		n_2 = other.n_2;
		value = other.value;
		other.n_2 = 0;
		other.value = T();

		for (int i = 0; i < 4; i++) {
			delete blocks[i];
			blocks[i] = other.blocks[i];
			other.blocks[i] = nullptr;
		}	
	}
	return *this;
}

//! copy assignement
template<typename T>
SMatrix<T>& SMatrix<T>::operator=(const SMatrix<T>& other) noexcept {
	if (this != &other) {
		n_2 = other.n_2;
		value = T(other.value);

		for (int i = 0; i < 4; i++)
			delete blocks[i];

		if(other.blocks[0] != nullptr)
			for (int i = 0; i < 4; i++)
				blocks[i] = new SMatrix<T>(*other.blocks[i]);
		else
			for (int i = 0; i < 4; i++)
				blocks[i] = nullptr;
	}
	return *this;
}

//! destructor
template<typename T>
SMatrix<T>::~SMatrix() {
	if (blocks[0] != nullptr)
		for(int i = 0; i < 4; i++)
			delete blocks[i];
}

//! true if the matrix is full of 0
template<typename T>
bool SMatrix<T>::is_zero() const {
	if (n_2 == 0)
		return value == T();
	if (n_2 == 1 && blocks[0] != nullptr)
		return blocks[0]->value == T() && blocks[1]->value == T() && blocks[2]->value == T() && blocks[3]->value == T();
	if (blocks[0] != nullptr)
		return blocks[0]->blocks[0] == nullptr && blocks[1]->blocks[0] == nullptr && blocks[2]->blocks[0] == nullptr && blocks[3]->blocks[0] == nullptr;
	return true;
}

//! delete childs if there are all 0
template<typename T>
void SMatrix<T>::_clean() {
	if (n_2 == 0)
		return;
	if (blocks[0] != nullptr && is_zero()) {
		for (int i = 0; i < 4; i++) {
			delete blocks[i];
			blocks[i] = nullptr;
		}
	}
}

//! return the dimension of the matrix
template<typename T>
int SMatrix<T>::get_n() const {
	return n_2 == 0 ? 1 : (n_2 << 1);
}

//! return a coeficient of the matrix given it's position
template<typename T>
T SMatrix<T>::at(int line, int column) const {
	if (n_2 == 0)
		return value;
	if (blocks[0] == nullptr)
		return T();
	return _get_block(line, column)->at(line % n_2, column % n_2);
}

//! write a coeficient of the matrix given it's position
template<typename T>
void SMatrix<T>::insert(const T& x, int line, int column) {
	if (n_2 == 0) {
		assert(line == 0 && column == 0);
		value = T(x);
	}
	else {
		if (blocks[0] == nullptr)
			_split();
		_get_block(line, column)->insert(x, line % n_2, column % n_2);
	}
}

//! internal function, return the sublock containing the position
template<typename T>
SMatrix<T>* SMatrix<T>::_get_block(int line, int column) const {
	return blocks[(column >= n_2) | ((line >= n_2) << 1)];
}

//! internal function, split the matrix with empty childs
template<typename T>
void SMatrix<T>::_split() {
	assert(blocks[0] == nullptr);
	for (int i = 0; i < 4; i++)
		blocks[i] = new SMatrix<T>(n_2);
}

//! internal function, split the matrix with copied childs
template<typename T>
void SMatrix<T>::_split(SMatrix<T>* const other[4]) {
	assert(blocks[0] == nullptr);
	for (int i = 0; i < 4; i++)
		blocks[i] = new SMatrix<T>(*(other[i]));
}

//! += operator
template<typename T>
SMatrix<T>& SMatrix<T>::operator+=(const SMatrix<T>& other) {
	assert(n_2 == other.n_2);
	if (n_2 == 0) {
		value += other.value;
		return *this;
	}
		
	if (other.blocks[0] != nullptr) {
		if (blocks[0] == nullptr) {
			//copy
			_split(other.blocks);
		}
		else {
			//reccurse
			for (int i = 0; i < 4; i++)
				blocks[i]->operator+=(*(other.blocks[i]));
			_clean();
		}
	}
	return *this;
}

//! addition with a constant matrix
template<typename T>
SMatrix<T> SMatrix<T>::operator+(const SMatrix<T>& other) const {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest += other;
}

//! addition with a rvalue matrix, result is writen in the 'other' matrix to save memory space and avoid copy
template<typename T>
SMatrix<T>&& SMatrix<T>::operator+(SMatrix<T>&& other) const {
	return std::move(other+= (*this));
}

//! operator -=
template<typename T>
SMatrix<T>& SMatrix<T>::operator-=(const SMatrix<T>& other) {
	assert(n_2 == other.n_2);
	if (n_2 == 0) {
		value -= other.value;
		return *this;
	}

	if (other.blocks[0] != nullptr) {
		if (blocks[0] == nullptr) {
			//copy
			_split(other.blocks);
		}
		else {
			//reccurse
			for (int i = 0; i < 4; i++)
				blocks[i]->operator-=(*(other.blocks[i]));
			_clean();
		}
	}
	return *this;
}

//! substraction with a constant matrix
template<typename T>
SMatrix<T> SMatrix<T>::operator-(const SMatrix<T>& other) const {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest -= other;
}

//! substraction with a rvalue matrix, result is writen in the 'other' matrix to save memory space and avoid copy
template<typename T>
SMatrix<T>&& SMatrix<T>::operator-(SMatrix<T>&& other) const {
	assert(n_2 == other.n_2);
	if (n_2 == 0) {
		other.value = value - other.value;
		return std::move(other);
	}

	if (blocks[0] == nullptr) {
		return std::move(other._self_neg());
	}

	else {
		if (other.blocks[0] == nullptr) {
			other._split(blocks);
		}
		else {
			for (int i = 0; i < 4; i++)
				blocks[i]->operator-(std::move(*(other.blocks[i])));
			other._clean();
		}
	}
	return std::move(other);
}

//! internal function, negate the matrix
template<typename T>
SMatrix<T>& SMatrix<T>::_self_neg() {
	if (n_2 == 0)
		value = -value;
	else {
		if (blocks[0] != nullptr)
			for (int i = 0; i < 4; i++)
				blocks[i]->_self_neg();
	}
	return *this;
}

//! operator - unary, negate a const matrix
template<typename T>
SMatrix<T> SMatrix<T>::operator-() const& {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest._self_neg();
}

//! operator - unary, negate a matrix
template<typename T>
SMatrix<T> SMatrix<T>::operator-() & {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest._self_neg();
}

//! operator - unary, negate a rvalue matrix. current matrix is overwriten to avoid copy.
template<typename T>
SMatrix<T>&& SMatrix<T>::operator-() && {
	return std::move(this->_self_neg());
}

//! transpose the current matrix in place
template<typename T>
void SMatrix<T>::transpose() {
	if (n_2 == 0)
		return;
	if (blocks[0] == nullptr)
		return;
	for (int i = 0; i < 4; i++)
		blocks[i]->transpose();
	std::swap(blocks[1], blocks[2]);
}

//! transpose return the transposed matrix of a const matrix
template<typename T>
SMatrix<T> SMatrix<T>::transposed() const {
	SMatrix<T> dest = SMatrix<T>(*this);
	dest.transpose();
	return dest;
}

//! matrix multiplication
template<typename T>
SMatrix<T> SMatrix<T>::operator*(const SMatrix<T>& other) const {
	assert(n_2 == other.n_2);
	SMatrix<T> dest = SMatrix<T>(get_n());
	if (n_2 == 0) {
		dest.value = value * other.value;
		return dest;
	}
	if (blocks[0] == nullptr || other.blocks[0] == nullptr)
		return dest;

	SMatrix<T>& A = *blocks[0];
	SMatrix<T>& B = *blocks[1];
	SMatrix<T>& C = *blocks[2];
	SMatrix<T>& D = *blocks[3];

	SMatrix<T>& E = *other.blocks[0];
	SMatrix<T>& F = *other.blocks[1];
	SMatrix<T>& G = *other.blocks[2];
	SMatrix<T>& H = *other.blocks[3];

	dest.blocks[0] = new SMatrix<T>(B._add_mult(G, A*E));
	dest.blocks[1] = new SMatrix<T>(B._add_mult(H, A*F));
	dest.blocks[2] = new SMatrix<T>(D._add_mult(G, C*E));
	dest.blocks[3] = new SMatrix<T>(D._add_mult(H, C*F));

	dest._clean();

	return dest;
}

//! internal function, perform the operation (*this)*other + dest whil using 'dest' memory space to avoid a copy.
template<typename T>
SMatrix<T>&& SMatrix<T>::_add_mult(const SMatrix<T>& other, SMatrix<T>& dest) const {
	assert(n_2 == other.n_2 && n_2 == dest.n_2);
	if (n_2 == 0) {
		dest.value += value * other.value;
		return std::move(dest);
	}

	if (blocks[0] == nullptr || other.blocks[0] == nullptr)
		return std::move(dest);

	if (dest.blocks[0] == nullptr) {
		dest = this->operator*(other);
		return std::move(dest);
	}

	SMatrix<T>& A = *blocks[0];
	SMatrix<T>& B = *blocks[1];
	SMatrix<T>& C = *blocks[2];
	SMatrix<T>& D = *blocks[3];

	SMatrix<T>& E = *other.blocks[0];
	SMatrix<T>& F = *other.blocks[1];
	SMatrix<T>& G = *other.blocks[2];
	SMatrix<T>& H = *other.blocks[3];

	A._add_mult(E, *dest.blocks[0]);
	B._add_mult(G, *dest.blocks[0]);

	A._add_mult(F, *dest.blocks[1]);
	B._add_mult(H, *dest.blocks[1]);

	C._add_mult(E, *dest.blocks[2]);
	D._add_mult(G, *dest.blocks[2]);

	C._add_mult(F, *dest.blocks[3]);
	D._add_mult(H, *dest.blocks[3]);

	dest._clean();

	return std::move(dest);
}

//! return the inverse of a const matrix. The parameter 'inverted' will tell if the operation was succesful. In case of failure, return a copy of the initial matrix
template<typename T>
SMatrix<T> SMatrix<T>::inversed(bool* inverted) const {
	const SMatrix<T> mT = transposed();
	SMatrix<T> tmp = mT * (*this); //create a positive semidefinite matrix for inversion
	try {
		tmp._inverse();
	}
	catch (std::runtime_error& e) {
		if (inverted)
			*inverted = false;
		return SMatrix<T>(*this);
	}
	if (inverted)
		*inverted = true;
	
	return tmp * mT; // (mT * m)^-1 * mT is the inverse of m
}

//! internal function, compute the inverse of a matrix. Throw a runtime_error if the matrix cannot be inverted
template<typename T>
void SMatrix<T>::_inverse() {
	if (n_2 == 0) {
		if (value == T())
			throw std::runtime_error("non invertible matrix");
		value = 1 / value;
		return;
	}

	if (blocks[0] == nullptr)
		throw std::runtime_error("non invertible matrix");

	SMatrix<T>& A = *blocks[0];
	SMatrix<T>& B = *blocks[1];
	SMatrix<T>& C = *blocks[2];
	SMatrix<T>& D = *blocks[3];

	SMatrix<T>& A_ = A;
	A_._inverse();

	const SMatrix<T> CA_ = C * A_;
	const SMatrix<T> A_B = A_ * B;

	SMatrix<T> M = D - C * A_ * B;
	SMatrix<T>& M_ = M;
	M_._inverse();

	SMatrix<T> M_CA_ = M_ * CA_;

	*blocks[0] = A_ + A_B * M_CA_;
	*blocks[1] = -(A_B * M_);
	*blocks[2] = -std::move(M_CA_); // M_CA_ is not used after, can move it
	*blocks[3] = M_;

	_clean();
}