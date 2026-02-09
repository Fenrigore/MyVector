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
		, capacity_{ capacity } {
	}

	//конструкторы копирования удаляем
	RawMemory(const RawMemory&) = delete;
	RawMemory& operator = (const RawMemory& rhs) = delete;

	//конструкторы перемещения пишем
	RawMemory(RawMemory&& other) noexcept
		: buffer_{ other.buffer_ }
		, capacity_{ other.capacity_ } {
		other.buffer_ = nullptr;
		other.capacity_ = 0;
	}

	RawMemory& operator=(RawMemory&& rhs) noexcept {
		if (this != &rhs) {
			//нам без разницы что станет с правым, главное чтоб левый стал копией правого
			Swap(rhs);
		}
		return *this;
	}


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
		return n != 0 ? static_cast<T*>(operator new(sizeof(T) * n)) : nullptr;
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
		, size_(other.size_) {
		//копирование из other по кол-ву элементов
		std::uninitialized_copy_n(other.data_.GetAdress(), size_, data_.GetAdress());

	}

	Vector(Vector&& other) noexcept
		:data_{ std::move(other.data_) }
		, size_{ other.size_ } {
		other.size_ = 0;
	}

	Vector& operator=  (const Vector& rhs) {
		if (this != &rhs) {
			if (rhs.size_ > data_.Capacity()) {
				/* Применить copy-and-swap */
				Vector rhs_copy(rhs);
				Swap(rhs_copy);
			}
			else {
				//Размер вектора - источника меньше размера вектора - приёмника
				if (rhs.size_ < size_) {
					//копируем все элеиенты из rhs.data_	
					for (size_t i = 0; i < rhs.size_; ++i) {
						*(data_.GetAdress() + i) = *(rhs.data_.GetAdress() + i);
					}
					//не перезаписанные, а значит лишние, уничтожаем
					std::destroy_n(data_.GetAdress() + rhs.size_, size_ - rhs.size_);
				}//если больше или равен
				else {
					//тут минималка - размер this вектора, проходим по нему
					for (size_t i = 0; i < size_; ++i) {
						*(data_.GetAdress() + i) = *(rhs.data_.GetAdress() + i);
					}
					//а дальше копируем в свободную область
					std::uninitialized_copy((rhs.data_.GetAdress() + size_)
						, (rhs.data_.GetAdress() + rhs.size_), data_.GetAdress() + size_);

				}
				size_ = rhs.size_;
			}
		}
		return *this;
	}

	Vector& operator= (Vector&& rhs) noexcept {
		if (this != &rhs) {
			//как и с RawMemory, нам без разницы что станет с правым объектом, 
			// главное чтоб левый стал им.
			Swap(rhs);
		}
		return *this;
	}

	void Swap(Vector& other) noexcept {
		data_.Swap(other.data_);
		std::swap(size_, other.size_);
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
		RawMemory<T> new_data(new_capacity); //Если выбросит исключение то MyVector не изменится

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

	void Resize(size_t new_size) {
		//если уменьшаем
		if (new_size < size_) {
			//то удаляем элементы, у которых позиция больше нового размера
			for (size_t i = new_size; i < size_; ++i) {
				(data_.GetAdress() + i)->~T();
			}
		}//если увеличиваем
		else if (new_size > size_) {
			//бронируем больше памяти, если надо
			Reserve(new_size);
			//инициализируем новые объекты (T{})
			std::uninitialized_value_construct_n((data_.GetAdress() + size_), new_size - size_);
		}
		size_ = new_size;
	}


	void PushBack(const T& value) {
		if (size_ == Capacity()) {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			std::construct_at((new_data.GetAdress() + size_), value);
			try {
				if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>) {
					std::uninitialized_move_n(data_.GetAdress(), size_, new_data.GetAdress());
				}
				else {
					std::uninitialized_copy_n(data_.GetAdress(), size_, new_data.GetAdress());
				}
			}
			catch (...) {
				std::destroy_at((new_data.GetAdress() + size_));
				throw;
			}

			data_.Swap(new_data);
			std::destroy_n(new_data.GetAdress(), size_);
		}
		else {
			std::construct_at((data_.GetAdress() + size_), value);
		}
		++size_;
	}



	void PushBack(T&& value) {
		if (size_ == Capacity()) {
			RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
			std::construct_at((new_data.GetAdress() + size_), std::move(value));
			try {
				if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>) {
					std::uninitialized_move_n(data_.GetAdress(), size_, new_data.GetAdress());
				}
				else {
					std::uninitialized_copy_n(data_.GetAdress(), size_, new_data.GetAdress());
				}
			}
			catch (...) {
				std::destroy_at((new_data.GetAdress() + size_));
				throw;
			}

			data_.Swap(new_data);
			std::destroy_n(new_data.GetAdress(), size_);
		}
		else {
			std::construct_at((data_.GetAdress() + size_), std::move(value));
		}
		++size_;
	}



	void PopBack() noexcept {
		--size_;
		std::destroy_at(data_.GetAdress() + size_);
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