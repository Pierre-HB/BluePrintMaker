#pragma once

template<typename T>
class SMatrix {
private:
	int n;
	T value;
	SMatrix* blocks[4];
	/*
	* blocks idx :
	|0 1|
	|2 3|
	*/
public:
	SMatrix(int n);
	SMatrix(const SMatrix<T>& m);
	~SMatrix();

	int Get_n() const;

	T at(int line, int column) const;
};


template<typename T>
SMatrix<T>::SMatrix(int n) : n(n), value(T()), blocks{ nullptr, nullptr, nullptr, nullptr } {

}

template<typename T>
SMatrix<T>::SMatrix(const SMatrix<T>& m) : n(m.n), value(T(m.value)), blocks{ nullptr, nullptr, nullptr, nullptr } {
	for(int i = 0; i < 4; i++)
		if (m.blocks[i] != nullptr)
			blocks[i] = new SMatrix(&(m.blocks[i]));
}

template<typename T>
SMatrix<T>::~SMatrix() {

}

template<typename T>
int SMatrix<T>::Get_n() const {
	return n;
}

template<typename T>
T SMatrix<T>::at(int line, int column) const {
	if (n == 1)
		return value;
	if (blocks[0] == nullptr)
		return T();
	return 1;
}
