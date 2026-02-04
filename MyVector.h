#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <type_traits>

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
		: data_(size)
		, size_(size) {
		//заполняем с помощью value-инициализации
		std::uninitialized_value_construct_n(data_.GetAdress(), size_);	
	}

	//конструктор копирования
	Vector(const Vector& other)
		//забронировать память размером как в other.size_
		:data_(other.size_)
		, size_(other.size_){
		//копирование из other по кол-ву элементов
		std::uninitialized_copy_n(other.data_.GetAdress(), size_, data_.GetAdress());

	}

	//Деструктор 
	~Vector() {
		//стандартная функция удаления из памяти
		std::destroy_n(data_.GetAdress(), size_);
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity < data_.Capacity()) {
			return;
		}
		RawMemory<T> new_data (new_capacity); //Если выбросит исключение то MyVector не изменится

		//Перемещайте элементы, только если соблюдается хотя бы одно из условий:
		// - конструктор перемещения типа T не выбрасывает исключений;
		// - тип T не имеет копирующего конструктора.

		//Шаблоны std::is_copy_constructible_v и std::is_nothrow_move_constructible_v 
		//помогают узнать, есть ли у типа копирующий конструктор и noexcept - конструктор 
		//перемещения.Выполняются эти шаблоны во время компиляции
		if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>) {
			std::uninitialized_move_n(data_.GetAdress(), size_, new_data.GetAdress());
		}
		else {
			std::uninitialized_copy_n(data_.GetAdress(), size_, new_data.GetAdress());
		}

		//затем свапаем через метод RawData
		data_.Swap(new_data);
		//и избавляемся от улик
		std::destroy_n(new_data.GetAdress(), size_);
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

private: //приватные поля
	RawMemory<T> data_{};
	size_t size_{};
};