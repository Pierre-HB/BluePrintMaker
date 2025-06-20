#pragma once

#include <exception>
static int nb_creation = 0;
static int nb_copie = 0;
static int nb_move = 0;
static int nb_destroy = 0;

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
	//SMatrix<T>&& operator-(SMatrix<T>&& other) const;

	SMatrix<T> operator-() const;

	SMatrix<T> operator*(const SMatrix<T>& other) const;

	void transpose();
	SMatrix<T> transposed() const;
	
	bool is_zero() const;

	SMatrix<T> inversed(bool* inverted = nullptr) const;
	void _inverse();
	void print() const;
private:
	SMatrix<T>* _get_block(int line, int column) const; //return the block containing the cell
	void _split();
	void _split(SMatrix<T>* const other[4]);//_split and copy
	SMatrix<T>&& _add_mult(const SMatrix<T>& other, SMatrix<T>& dest) const;
	void _clean();//check if childs are 0 and so, delete them
	SMatrix<T> _self_neg(); //self negation
	
};


template<typename T>
SMatrix<T>::SMatrix(int n) : n_2(n/2), value(T()), blocks{ nullptr, nullptr, nullptr, nullptr } {
	assert(n == 1 || n % 2 == 0);
	//std::cout << "created matrix " << n << "x" << n << std::endl;
	nb_creation++;
}

template<typename T>
SMatrix<T>::SMatrix(const SMatrix<T>& m) : n_2(m.n_2), value(T(m.value)), blocks{ nullptr, nullptr, nullptr, nullptr } {
	//std::cout << "copied matrix " << get_n() << "x" << get_n() << std::endl;
	nb_copie++;
	if (m.blocks[0] != nullptr)
		for(int i = 0; i < 4; i++)
			blocks[i] = new SMatrix<T>(*(m.blocks[i]));
}

template<typename T>
SMatrix<T>::SMatrix(SMatrix<T>&& m) noexcept  : n_2(m.n_2), value(T(m.value)) {
	//std::cout << "mooved matrix " << get_n() << "x" << get_n() << std::endl;
	nb_move++;
	for (int i = 0; i < 4; i++) {
		blocks[i] = m.blocks[i];
		m.blocks[i] = nullptr;
	}

	m.n_2 = 0;
	m.value = T();
}

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

template<typename T>
SMatrix<T>::~SMatrix() {
	//std::cout << "destroyed matrix " << get_n() << "x" << get_n() << std::endl;
	nb_destroy++;
	if (blocks[0] != nullptr)
		for(int i = 0; i < 4; i++)
			delete blocks[i];
}

template<typename T>
void SMatrix<T>::print() const {
	int n = get_n();
	std::cout << std::endl;
	for (int col = 0; col < n; col++) {
		for (int line = 0; line < n; line++) {
			std::cout << at(line, col) << " ";
		}
		std::cout << std::endl;
	}
}

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

template<typename T>
int SMatrix<T>::get_n() const {
	return n_2 == 0 ? 1 : (n_2 << 1);
}

template<typename T>
T SMatrix<T>::at(int line, int column) const {
	if (n_2 == 0)
		return value;
	if (blocks[0] == nullptr)
		return T();
	return _get_block(line, column)->at(line % n_2, column % n_2);
}

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

template<typename T>
SMatrix<T>* SMatrix<T>::_get_block(int line, int column) const {
	return blocks[(column >= n_2) | ((line >= n_2) << 1)];
}

template<typename T>
void SMatrix<T>::_split() {
	assert(blocks[0] == nullptr);
	for (int i = 0; i < 4; i++)
		blocks[i] = new SMatrix<T>(n_2);
}

template<typename T>
void SMatrix<T>::_split(SMatrix<T>* const other[4]) {
	assert(blocks[0] == nullptr);
	for (int i = 0; i < 4; i++)
		blocks[i] = new SMatrix<T>(*(other[i]));
}

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

template<typename T>
SMatrix<T> SMatrix<T>::operator+(const SMatrix<T>& other) const {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest += other;
}

template<typename T>
SMatrix<T>&& SMatrix<T>::operator+(SMatrix<T>&& other) const {
	return std::move(other+= (*this));
}

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

template<typename T>
SMatrix<T> SMatrix<T>::operator-(const SMatrix<T>& other) const {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest -= other;
}

//template<typename T>
//SMatrix<T>&& SMatrix<T>::operator-(SMatrix<T>&& other) const {
//	assert(n_2 == other.n_2);
//	if (n_2 == 0) {
//		other.value = value - other.value;
//		return std::move(other);
//	}
//
//
//	if (blocks[0] != nullptr)
//	{
//		if (other.blocks[0] == nullptr)
//		{
//			//copy
//			_split(other.blocks);
//		}
//		else {
//			//reccurse
//			for (int i = 0; i < 4; i++)
//				blocks[i]->operator-=(*(other.blocks[i]));
//			_clean();
//		}
//	}
//	return *this;
//}

template<typename T>
SMatrix<T> SMatrix<T>::_self_neg() {
	if (n_2 == 0)
		value = -value;
	else {
		if (blocks[0] != nullptr)
			for (int i = 0; i < 4; i++)
				blocks[i]->_self_neg();
	}
	return *this;
}

template<typename T>
SMatrix<T> SMatrix<T>::operator-() const {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest._self_neg();
}

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

template<typename T>
SMatrix<T> SMatrix<T>::transposed() const {
	SMatrix<T> dest = SMatrix<T>(*this);
	dest.transpose();
	return dest;
}

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
	*blocks[1] = (A_B * M_)._self_neg();
	*blocks[2] = (M_CA_)._self_neg(); // M_CA_ is not used after, can self_neg it
	*blocks[3] = M_;

	_clean();
}