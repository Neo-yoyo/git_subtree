#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>


template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
            : buffer_(Allocate(capacity))
            , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Deallocate(buffer_);
            capacity_ = 0;
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

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};



template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    /// Конструктор по умолчанию.
    /// Инициализирует вектор нулевого размера и вместимости
    Vector() = default;

    /// Конструктор, который создаёт вектор заданного размера.
    /// Вместимость созданного вектора равна его размеру,
    /// а элементы проинициализированы значением по умолчанию для типа T
    explicit Vector(size_t size);

    /// Копирующий конструктор. Создаёт копию элементов исходного вектора.
    /// Имеет вместимость, равную размеру исходного вектора,
    /// то есть выделяет память без запаса.
    Vector(const Vector& other);

    Vector& operator=(const Vector& rhs);

    Vector(Vector&& other) noexcept;

    Vector& operator=(Vector&& rhs) noexcept;

    ~Vector();

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args);
    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>);
    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);



    [[nodiscard]] size_t Size() const noexcept;
    [[nodiscard]] size_t Capacity() const noexcept;
    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;
    /// Резервирует достаточно места, чтобы вместить количество capacity
    void Reserve(size_t new_capacity);
    /// выполняющий обмен содержимого вектора с другим вектором
    void Swap(Vector& other) noexcept;
    void Resize(size_t new_size);

    template <typename Type>
    void PushBack(Type&& value);

    void PopBack() noexcept;
    [[nodiscard]] bool IsEmpty() const noexcept;

    template <typename... Args>
    T& EmplaceBack(Args&&... args);

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    /// вставка элемента в этот же вектор, когда вместимость достаточна
    template <typename... Args>
    void EmplaceShift(size_t index, Args&&... args);

    /// выполняет реаллокацию памяти при полностью заполненном векторе
    template <typename... Args>
    void EmplaceReallocate(size_t index, Args&&... args);

}; // class Vector

template<typename T>
Vector<T>::Vector(size_t size)
        : data_(size)
        , size_(size)  //
{
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template<typename T>
Vector<T>::Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
{
    std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
}

template<typename T>
Vector<T>& Vector<T>::operator=(const Vector& rhs) {
    if (this != &rhs) {
        if (rhs.size_ > data_.Capacity()) {
            /* Применить copy-and-swap */
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
        } else {
            /* Скопировать элементы из rhs, создав при необходимости новые
               или удалив существующие */
            if(rhs.size_ < size_) {
                std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
            }
            else {
                std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
            }
            size_ = rhs.size_;
        }
    }
    return *this;
}

template<typename T>
Vector<T>::Vector(Vector&& other) noexcept {
    Swap(other);
}

template<typename T>
Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept {
    if (this != &rhs) {
        Swap(rhs);
    }
    return *this;
}

template<typename T>
Vector<T>::~Vector() {
    std::destroy_n(data_.GetAddress(), size_);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept{
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept{
    return data_.GetAddress() + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept{
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept{
    return data_.GetAddress() + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept{
    return begin();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept{
    return end();
}

template<typename T>
template <typename... Args>
typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args&&... args){
    size_t index = pos - begin();

    if (size_ < data_.Capacity()) {
        if (index == size_) {
            EmplaceBack(std::forward<Args>(args)...);
            return iterator(data_.GetAddress() + index);
        } else {
            EmplaceShift(index, std::forward<Args>(args)...);
            return iterator(data_.GetAddress() + index);
        }
    } else {
        EmplaceReallocate(index, std::forward<Args>(args)...);
        return iterator(data_.GetAddress() + index);
    }
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T& value){
    return Emplace(pos, value);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T&& value){
    return Emplace(pos, std::move(value));
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>){
    assert(pos >= begin() && pos < end());
    size_t index = pos - begin();
    /// Сначала на место удаляемого элемента нужно переместить следующие за ним элементы
    std::move(begin() + index + 1, end(), begin() + index);
    PopBack();
    return begin() + index;
}

template<typename T>
size_t Vector<T>::Size() const noexcept {
    return size_;
}

template<typename T>
size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}

template<typename T>
const T& Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector&>(*this)[index];
}

template<typename T>
T& Vector<T>::operator[](size_t index) noexcept {
    assert(index < size_);
    return data_[index];
}

template<typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }
    RawMemory<T> new_data(new_capacity);
    // constexpr оператор if будет вычислен во время компиляции
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    else {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template<typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}

template<typename T>
void Vector<T>::Resize(size_t new_size) {
    /// уменьшение размера вектора
    if (new_size < size_) {
        /// удалить лишние элементы вектора, вызвав их деструкторы
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
    }
        /// увеличение размера вектора
    else if (new_size > size_) {
        /// сначала нужно убедиться, что вектору достаточно памяти для новых элементов
        if (new_size > data_.Capacity()) {
            Reserve(new_size);
        }
        /// новые элементы нужно проинициализировать
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
    }
    /// обновить размер вектора
    size_ = new_size;
}

template<typename T>
template<typename Type>
void Vector<T>::PushBack(Type&& value) {
    EmplaceBack(std::forward<Type>(value));
}

template<typename T>
void Vector<T>::PopBack() noexcept {
    assert(!IsEmpty());
    std::destroy_at(data_.GetAddress() + size_ - 1);
    --size_;
}

template<typename T>
bool Vector<T>::IsEmpty() const noexcept {
    return size_ == 0;
}

template<typename T>
template<typename ...Args>
T& Vector<T>::EmplaceBack(Args && ...args) {
    if (size_ == Capacity()) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        new (new_data + size_) T(std::forward<Args>(args)...);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    } else {
        new (data_ + size_) T(std::forward<Args>(args)...);
    }

    ++size_;
    return data_[size_ - 1];
}

template<typename T>
template<typename ...Args>
void Vector<T>::EmplaceShift(size_t index, Args&&... args) {
    /// Создаем временный объект, чтобы избежать перезаписи при вставке из этого же вектора
    T temp_value(std::forward<Args>(args)...);
    /// Создаем копию или перемещаем последний элемент вектора в неинициализированную область
    new (data_ + size_) T(std::move(data_[size_ - 1]));
    /// Перемещаем элементы вправо с использованием std::move_backward
    std::move_backward(data_ + index, data_ + size_ - 1, data_ + size_);
    /// Перемещаем временный элемент в позицию вставки
    data_[index] = std::move(temp_value);
    ++size_;
}

template<typename T>
template<typename ...Args>
void Vector<T>::EmplaceReallocate(size_t index, Args&&... args) {
    RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);

    /// Конструируем вставляемый элемент в новом блоке
    new (new_data + index) T(std::forward<Args>(args)...);

    try {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            /// перемещаем элементы до позиции в новый блок
            std::uninitialized_move_n(data_.GetAddress(), index, new_data.GetAddress());
            /// перемещаем элементы после позиции в новый блок
            std::uninitialized_move_n(data_.GetAddress() + index, size_ - index,
                                      new_data.GetAddress() + index + 1);
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), index, new_data.GetAddress());
            std::uninitialized_copy_n(data_.GetAddress() + index, size_ - index,
                                      new_data.GetAddress() + index + 1);
        }
    }
    catch (...) {
        std::destroy_n(new_data.GetAddress() + index, 1);
        throw;
    }

    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
    ++size_;
}