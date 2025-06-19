#pragma once

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
	SMatrix<T>& operator=(SMatrix<T>&& m) noexcept;//move assign
	SMatrix<T>& SMatrix<T>::operator=(const SMatrix<T>& other) noexcept;//move copy
	~SMatrix();

	int get_n() const;

	T at(int line, int column) const;
	void insert(const T& x, int line, int column);

	SMatrix<T> operator+=(const SMatrix<T>& other);
	SMatrix<T> operator+(const SMatrix<T>& other) const;

	SMatrix<T> operator-=(const SMatrix<T>& other);
	SMatrix<T> operator-(const SMatrix<T>& other) const;
	SMatrix<T> operator-() const;
	//friend SMatrix<T> operator-(SMatrix<T>&& m);
	//friend SMatrix<T> operator-(const SMatrix<T>& m);

	SMatrix<T> operator*(const SMatrix<T>& other) const;

	void transpose();
	SMatrix<T> transposed() const;

private:
	SMatrix<T>* get_block(int line, int column) const; //return the block containing the cell
	void split();
	void split(SMatrix<T>* const other[4]);//split and copy
	SMatrix<T>&& add_mult(const SMatrix<T>& other, SMatrix<T>& dest) const;
	void clean();//check if childs are 0 and so, delete them
	bool is_zero() const;
	SMatrix<T> neg(); //self negation
};


template<typename T>
SMatrix<T>::SMatrix(int n) : n_2(n/2), value(T()), blocks{ nullptr, nullptr, nullptr, nullptr } {
	assert(n == 1 || n % 2 == 0);
	std::cout << "created matrix " << n << "x" << n << std::endl;
	nb_creation++;
}

template<typename T>
SMatrix<T>::SMatrix(const SMatrix<T>& m) : n_2(m.n_2), value(T(m.value)), blocks{ nullptr, nullptr, nullptr, nullptr } {
	std::cout << "copied matrix " << get_n() << "x" << get_n() << std::endl;
	nb_copie++;
	if (m.blocks[0] != nullptr)
		for(int i = 0; i < 4; i++)
			blocks[i] = new SMatrix<T>(*(m.blocks[i]));
}

template<typename T>
SMatrix<T>::SMatrix(SMatrix<T>&& m) noexcept  : n_2(m.n_2), value(T(m.value)) {
	std::cout << "mooved matrix " << get_n() << "x" << get_n() << std::endl;
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
		*this = std::move(SMatrix<T>(other));
	}
	return *this;
}

template<typename T>
SMatrix<T>::~SMatrix() {
	std::cout << "destroyed matrix " << get_n() << "x" << get_n() << std::endl;
	nb_destroy++;
	if (blocks[0] != nullptr)
		for(int i = 0; i < 4; i++)
			delete blocks[i];
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
void SMatrix<T>::clean() {
	if (n_2 == 0)
		return;
	if (blocks[0] != nullptr && is_zero()) {
		for (int i = 0; i < 4; i++) {
			delete blocks[i];
			blocks[i] = nullptr;
		}
		std::cout << "CLEANED STUFFS" << std::endl;
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
	return get_block(line, column)->at(line % n_2, column % n_2);
}

template<typename T>
void SMatrix<T>::insert(const T& x, int line, int column) {
	if (n_2 == 0) {
		assert(line == 0 && column == 0);
		value = T(x);
	}
	else {
		if (blocks[0] == nullptr)
			split();
		get_block(line, column)->insert(x, line % n_2, column % n_2);
	}
}


template<typename T>
SMatrix<T>* SMatrix<T>::get_block(int line, int column) const {
	return blocks[(column < n_2) | ((line < n_2) << 1)];
}

template<typename T>
void SMatrix<T>::split() {
	assert(blocks[0] == nullptr);
	for (int i = 0; i < 4; i++)
		blocks[i] = new SMatrix<T>(n_2);
}

template<typename T>
void SMatrix<T>::split(SMatrix<T>* const other[4]) {
	assert(blocks[0] == nullptr);
	for (int i = 0; i < 4; i++)
		blocks[i] = new SMatrix<T>(*(other[i]));
}

template<typename T>
SMatrix<T> SMatrix<T>::operator+=(const SMatrix<T>& other) {
	assert(n_2 == other.n_2);
	if (n_2 == 0) {
		value += other.value;
		return *this;
	}
		
	if (other.blocks[0] != nullptr) {
		if (blocks[0] == nullptr) {
			//copy
			split(other.blocks);
		}
		else {
			//reccurse
			for (int i = 0; i < 4; i++)
				blocks[i]->operator+=(*(other.blocks[i]));
			clean();
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
SMatrix<T> SMatrix<T>::operator-=(const SMatrix<T>& other) {
	assert(n_2 == other.n_2);
	if (n_2 == 0) {
		value -= other.value;
		return *this;
	}

	if (other.blocks[0] != nullptr) {
		if (blocks[0] == nullptr) {
			//copy
			split(other.blocks);
		}
		else {
			//reccurse
			for (int i = 0; i < 4; i++)
				blocks[i]->operator-=(*(other.blocks[i]));
			clean();
		}
	}
	return *this;
}

template<typename T>
SMatrix<T> SMatrix<T>::operator-(const SMatrix<T>& other) const {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest -= other;
}

template<typename T>
SMatrix<T> SMatrix<T>::neg() {
	if (n_2 == 0)
		value = -value;
	else {
		if (blocks[0] != nullptr)
			for (int i = 0; i < 4; i++)
				blocks[i]->neg();
	}
	return *this;
}

template<typename T>
SMatrix<T> SMatrix<T>::operator-() const {
	SMatrix<T> dest = SMatrix<T>(*this);
	return dest.neg();
}

//template<typename T>
//SMatrix<T> operator-(SMatrix<T>&& m) {
//	return m.neg();
//}

//template<typename T>
//SMatrix<T> operator-(const SMatrix<T>& m) {
//	SMatrix<T> dest = SMatrix<T>(m);
//	return dest.neg();
//}

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

	dest.blocks[0] = new SMatrix<T>(blocks[1]->add_mult(*other.blocks[2], blocks[0]->operator*(*other.blocks[0])));
	dest.blocks[1] = new SMatrix<T>(blocks[1]->add_mult(*other.blocks[3], blocks[0]->operator*(*other.blocks[1])));
	dest.blocks[2] = new SMatrix<T>(blocks[3]->add_mult(*other.blocks[2], blocks[2]->operator*(*other.blocks[0])));
	dest.blocks[3] = new SMatrix<T>(blocks[3]->add_mult(*other.blocks[3], blocks[2]->operator*(*other.blocks[1])));

	dest.clean();

	return dest;
}

template<typename T>
SMatrix<T>&& SMatrix<T>::add_mult(const SMatrix<T>& other, SMatrix<T>& dest) const {
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

	blocks[0]->add_mult(*other.blocks[0], *dest.blocks[0]);
	blocks[1]->add_mult(*other.blocks[2], *dest.blocks[0]);

	blocks[0]->add_mult(*other.blocks[1], *dest.blocks[1]);
	blocks[1]->add_mult(*other.blocks[3], *dest.blocks[1]);

	blocks[2]->add_mult(*other.blocks[0], *dest.blocks[2]);
	blocks[3]->add_mult(*other.blocks[2], *dest.blocks[2]);

	blocks[2]->add_mult(*other.blocks[1], *dest.blocks[3]);
	blocks[3]->add_mult(*other.blocks[3], *dest.blocks[3]);

	dest.clean();

	return std::move(dest);
}