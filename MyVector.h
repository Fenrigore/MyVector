#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>

template <typename T>
class Vector {
public:
	//Конструктор по умолчанию
	Vector() = default;

	//Конструктор вектора размером size с нулевыми значениями T
	explicit Vector(size_t size) 
		:data_(Allocate(size))
		, capacity_(size)
		, size_(size) {

		size_t i{};
		try {
			for (; i != size; ++i) {
				new (data_ + i) T();
			}
		}
		catch (...) {
			// i хранит количество созданных объектов
			//теперь их надо уничтожить
			DestroyN(data_, i);
			//и освободить память
			Deallocate(data_);
			//перевыбрасываем исключение
			throw;
		}
	}

	//конструктор копирования
	Vector(const Vector& other)
		//забронировать память размером как в other.size_
		:data_(Allocate(other.size_))
		, capacity_(other.size_)
		, size_(other.size_){
		
		//логика выбрасывания исключения такая же как и в конструкторе с size
		size_t i{};
		try {
			//скопировать в ячейку data_ + i объект из other.data_ по индексу i
			for (; i < size_; ++i) {
				CopyConstruct(data_ + i, other.data_[i]);
			}

		}
		catch (...) {
			DestroyN(data_, i);
			Deallocate(data_);
			throw;
		}
	}

	//Деструктор 
	~Vector() {
		DestroyN(data_, size_);
		Deallocate(data_);
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity < capacity_) {
			return;
		}
		T* new_data = Allocate(new_capacity); //Если выбросит исключение то MyVector не изменится

		//а тут как в конструкторе копирования
		size_t i{};
		try {
			for (; i != size_; ++i) {
				CopyConstruct(new_data + i, data_[i]);
			}
		}
		catch (...) {
			//тут уде уничтожаем во временной выделенной памяти
			DestroyN(new_data, i);
			Deallocate(new_data);
			throw;
		}
		DestroyN(data_, size_);
		Deallocate(data_);
		data_ = new_data;
		capacity_ = new_capacity;
	}

	size_t Size() noexcept {
		return size_;
	}

	size_t Capacity() noexcept {
		return capacity_;
	}

	const T& operator [](size_t index) const noexcept {
		return const_cast<Vector&>(*this)[index];
	}

	T& operator [](size_t index) noexcept {
		assert(index < size_);
		return data_[index];
	}
private: // приватные методы

	//Резервирует память размером sizeof(T) * n
	static T* Allocate(size_t n) {
		//Если n больше ноля, то создаю резерву битов в памяти 
		// и конвертирую этот указатель (void*) в указатель на шаблонный параметр T (T*)
		return n != 0 ? static_cast<T*>(operator new (sizeof(T) * n)) : nullptr;
	}

	static void Deallocate(T* buf) noexcept {
		operator delete(buf); //удаляю зарезервированную память (что с объектом?)
	}

	//Вызывает деструкторы n объектов массива по адресу buf
	static void DestroyN(T* buf, size_t n) noexcept {
		for (size_t i = 0; i < n; ++i) {
			Destroy(buf + i);
		}
	}

	// Создаёт копию объекта elem в сырой памяти по адресу buf
	static void CopyConstruct(T* buf, const T& elem) {
		new(buf) T(elem);
	}

	//вызывает деструктор объекта по адресу buf
	static void Destroy(T* buf) noexcept {
		buf->~T();
	}

private: //приватные поля
	T* data_{};
	size_t capacity_{};
	size_t size_{};
};