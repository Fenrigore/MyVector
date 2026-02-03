#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>


template <typename T>
class RawMemory {
public:
	RawMemory() = default;

	explicit RawMemory(size_t capacity)
		:buffer_{ Allocate(capacity) }
		, capacity_{capacity} {}

	~RawMemory() {
		Deallocate(buffer_);
	}

	 T* operator+(size_t offset) noexcept {
		 // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
		 assert(offset <= capacity_);
		 return buffer_ + offset;
	 }

	 const T* operator+(size_t offset) const noexcept {
		 return const_cast<RawMemory&>(*this) + offset;
	 }

	 T& operator[](size_t index) noexcept {
		 assert(index < capacity_);
		 return buffer_[index];
	 }

	 const T& operator[](size_t index) const noexcept {
		 return const_cast<RawMemory&>(*this)[index];
	 }

	 void Swap(RawMemory& other) noexcept {
		 std::swap(buffer_, other.buffer_);
		 std::swap(capacity_, other.capacity_);
	 }

	 const T* GetAdress() const noexcept {
		 return buffer_;
	 }

	 T* GetAdress() noexcept {
		 return buffer_;
	 }

	 size_t Capacity() const noexcept {
		 return capacity_;
	 }

private: //методы
	static T* Allocate(size_t n) {
		return n !=0 ? static_cast<T*>(operator new(sizeof(T) * n)):nullptr;
	}

	static void Deallocate(T* buf) noexcept {
		operator delete(buf);
	}

private: //поля
	T* buffer_{};
	size_t capacity_{};
};

template <typename T>
class Vector {
public:
	//Конструктор по умолчанию
	Vector() = default;

	//Конструктор вектора размером size с нулевыми значениями T
	explicit Vector(size_t size) 
		:data_(size)
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
			DestroyN(data_.GetAdress(), i);
			//перевыбрасываем исключение
			throw;
		}
	}

	//конструктор копирования
	Vector(const Vector& other)
		//забронировать память размером как в other.size_
		:data_(other.size_)
		, size_(other.size_){
		
		//логика выбрасывания исключения такая же как и в конструкторе с size
		size_t i{};
		try {
			//скопировать в ячейку data_ + i объект из other.data_ по индексу i
			for (; i < size_; ++i) {
				CopyConstruct((data_.GetAdress() + i), other.data_[i]);
				//попробовать CopyConstruct(data_[i], other.data_[i]);
			}
		}
		catch (...) {
			DestroyN(data_.GetAdress(), i);
			throw;
		}
	}

	//Деструктор 
	~Vector() {
		DestroyN(data_.GetAdress(), size_);
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity < data_.Capacity()) {
			return;
		}
		RawMemory<T> new_data (new_capacity); //Если выбросит исключение то MyVector не изменится

		//а тут как в конструкторе копирования
		size_t i{};
		try {
			for (; i != size_; ++i) {
				CopyConstruct(new_data.GetAdress() + i, data_[i]);
			}
		}
		catch (...) {
			//тут уже уничтожаем во временной выделенной памяти
			DestroyN(new_data.GetAdress(), i);
			throw;
		}
		//если было брошено исключение то код ниже не отработает уже
		//затем свапаем через метод RawData
		data_.Swap(new_data);
		//и избавляемся от улик
		DestroyN(new_data.GetAdress(), size_);
	}

	size_t Size() noexcept {
		return size_;
	}

	size_t Capacity() noexcept {
		return data_.Capacity();
	}

	const T& operator [](size_t index) const noexcept {
		return const_cast<Vector&>(*this)[index];
	}

	T& operator [](size_t index) noexcept {
		assert(index < size_);
		return data_[index];
	}
private: // приватные методы
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
	RawMemory<T> data_{};
	size_t size_{};
};