#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>
#include <utility>
#include <initializer_list>

#include "array_ptr.h"

struct ReserveProxyObj {
    ReserveProxyObj(size_t cpacity_to_reserve) : capacity(cpacity_to_reserve) {}

    size_t capacity;
};

template <typename Type>
class SimpleVector {
   public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по
    // умолчанию
    explicit SimpleVector(size_t size)
        : size_(size), capacity_(size), items_(size) {
        for (size_t i = 0; i < size; i++) {
            items_[i] = Type();
        }
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : size_(size), capacity_(size), items_(size) {
        for (size_t i = 0; i < size_; i++) {
            items_[i] = value;
        }
    }
    
    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : size_(init.size()), capacity_(init.size()), items_(init.size()) {
        std::copy(init.begin(), init.end(), begin());
    }

    // Копирующий конструткор
    SimpleVector(const SimpleVector& other)
        : size_(other.size_),
          capacity_(other.capacity_),
          items_(other.size_) {
        std::copy(other.begin(), other.end(), begin());
    }

    // Конструктор перемещения
    SimpleVector(SimpleVector&& moved) noexcept
        : size_(std::exchange(moved.size_, 0)),
          capacity_(std::exchange(moved.capacity_, 0)) {
        items_.swap(moved.items_);
    }

    // Конструктор резервирования
    SimpleVector(const ReserveProxyObj& reserved)
        : capacity_(reserved.capacity), items_(reserved.capacity) {}

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector<Type> temp(rhs);
            swap(temp);
        }
        
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs)  {
            SimpleVector<Type> temp(std::move(rhs));
            swap(temp);
        }

        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            Reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }

        *end() = item;
        ++size_;
    }

    // Move PushBack
    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            SimpleVector<Type> temp_vector(std::max(capacity_ + 1, capacity_ * 2));
            for (size_t i = 0; i < size_; i++) {
                temp_vector[i] = std::move(items_[i]);
            }
            std::swap(capacity_, temp_vector.capacity_);
            items_.swap(temp_vector.items_);
        }

        *end() = std::move(item);
        ++size_;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью
    // 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        size_t index = pos - begin();
        if (size_ == capacity_) {
            SimpleVector<Type> temp_vector(std::max(capacity_ + 1, capacity_ * 2));
            std::copy(begin(), begin() + index, temp_vector.begin());
            temp_vector[index] = value;
            std::copy(begin() + index, end(), temp_vector.begin() + index + 1);
            items_.swap(temp_vector.items_);
            std::swap(capacity_, temp_vector.capacity_);
        } else {
            std::copy_backward(begin() + index, end(), end() + 1);
            items_[index] = value;
        }
        ++size_;

        return Iterator(begin() + index);
    }

    // Move Insert
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        size_t index = pos - begin();
        if (size_ == capacity_) {
            SimpleVector<Type> temp_vector(std::max(capacity_ + 1, capacity_ * 2));
            std::move(begin(), begin() + index, temp_vector.begin());
            temp_vector[index] = std::move(value);
            std::move(begin() + index, end(), temp_vector.begin() + index + 1);
            items_.swap(temp_vector.items_);
            std::swap(capacity_, temp_vector.capacity_);
        } else {
            std::move_backward(begin() + index, end(), end() + 1);
            items_[index] = std::move(value);
        }
        ++size_;

        return Iterator(begin() + index);
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());

        size_t index = pos - begin();
        for (size_t i = index + 1; i < size_; ++i) {
            items_[i - 1] = std::move(items_[i]);
        }
        --size_;

        return Iterator(items_.Get() + index);
    }

    // Резервирует место. Повышает Capacity
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            SimpleVector<Type> temp_vector(new_capacity);
            std::copy(begin(), end(), temp_vector.begin());
            std::swap(capacity_, temp_vector.capacity_);
            items_.swap(temp_vector.items_);
        }
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        items_.swap(other.items_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept { return size_; }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept { return capacity_; }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept { return size_ == 0; }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Out of range");
        }

        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Out of range");
        }

        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept { size_ = 0; }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для
    // типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            SimpleVector<Type> temp_vec(std::max(new_size, capacity_ * 2));
            for (size_t i = 0; i < size_; i++) {
                temp_vec[i] = std::move(items_[i]);
            }
            items_.swap(temp_vec.items_);
            std::swap(capacity_, temp_vec.capacity_);
        }

        if (new_size > size_) {
            for (size_t i = size_; i < new_size; ++i) {
                items_[i] = Type();
            }
        }

        size_ = new_size;
    }

    // Итераторная область
    Iterator begin() noexcept { return items_.Get(); }


    Iterator end() noexcept { return items_.Get() + size_; }

    ConstIterator begin() const noexcept { return items_.Get(); }

    ConstIterator end() const noexcept { return items_.Get() + size_; }

    ConstIterator cbegin() const noexcept { return items_.Get(); }

    ConstIterator cend() const noexcept { return items_.Get() + size_; }

   private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> items_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs,
                       const SimpleVector<Type>& rhs) {
    return (&lhs == &rhs) ||
           (lhs.GetSize() == rhs.GetSize() &&
            std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()));
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs,
                       const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs,
                      const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                        rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs,
                       const SimpleVector<Type>& rhs) {
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs,
                      const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs,
                       const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}