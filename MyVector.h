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

	//конструкторы копировани€ удал€ем
	RawMemory(const RawMemory&) = delete;
	RawMemory& operator = (const RawMemory& rhs) = delete;

	//конструкторы перемещени€ пишем
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
		// –азрешаетс€ получать адрес €чейки пам€ти, следующей за последним элементом массива
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

private: //пол€
	T* buffer_{};
	size_t capacity_{};
};

template <typename T>
class Vector {
public:
	// онструктор по умолчанию
	Vector() = default;

	// онструктор вектора размером size с нулевыми значени€ми T
	explicit Vector(size_t size)
		: data_(size)
		, size_(size) {
		//заполн€ем с помощью value-инициализации
		std::uninitialized_value_construct_n(data_.GetAdress(), size_);
	}

	//конструктор копировани€
	Vector(const Vector& other)
		//забронировать пам€ть размером как в other.size_
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
				/* ѕрименить copy-and-swap */
				Vector rhs_copy(rhs);
				Swap(rhs_copy);
			}
			else {
				//–азмер вектора - источника меньше размера вектора - приЄмника
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

	//ƒеструктор 
	~Vector() {
		//стандартна€ функци€ удалени€ из пам€ти
		std::destroy_n(data_.GetAdress(), size_);
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity < data_.Capacity()) {
			return;
		}
		RawMemory<T> new_data(new_capacity); //≈сли выбросит исключение то MyVector не изменитс€

		//ѕеремещайте элементы, только если соблюдаетс€ хот€ бы одно из условий:
		// - конструктор перемещени€ типа T не выбрасывает исключений;
		// - тип T не имеет копирующего конструктора.

		//Ўаблоны std::is_copy_constructible_v и std::is_nothrow_move_constructible_v 
		//помогают узнать, есть ли у типа копирующий конструктор и noexcept - конструктор 
		//перемещени€.¬ыполн€ютс€ эти шаблоны во врем€ компил€ции
		if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>) {
			std::uninitialized_move_n(data_.GetAdress(), size_, new_data.GetAdress());
		}
		else {
			std::uninitialized_copy_n(data_.GetAdress(), size_, new_data.GetAdress());
		}

		//затем свапаем через метод RawData
		data_.Swap(new_data);
		//и избавл€емс€ от улик
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

private: //приватные пол€
	RawMemory<T> data_{};
	size_t size_{};
};