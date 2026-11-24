/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

template <typename ObjectType, size_t Capacity> class CStore {
public:
    size_t     m_nCount;
    ObjectType m_aObjects[Capacity];

public:
    CStore() {
        m_nCount = 0;
    }

    ObjectType& GetItemAtIndex(size_t index) {
        return m_aObjects[index];
    }

    bool IsFull() const {
        return m_nCount >= Capacity;
    }

    static constexpr size_t GetCapacity() {
        return Capacity;
    }

    size_t GetCount() const {
        return m_nCount;
    }

    // Без этой проверки переполнение затирало соседние статические массивы,
    // а падало потом в другом месте и по дикому адресу
    inline ObjectType& AddItem() {
        if (m_nCount >= Capacity) {
            return m_aObjects[Capacity - 1];
        }

        auto& obj = m_aObjects[m_nCount];
        ++m_nCount;
        return obj;
    }
};
