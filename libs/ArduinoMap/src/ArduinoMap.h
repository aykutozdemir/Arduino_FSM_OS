/**
 * @file ArduinoMap.h
 * @brief Lightweight, templated map implementation for Arduino.
 *
 * This file defines a template-based map (key-value store) implementation
 * optimized for Arduino. It provides dynamic memory allocation, iterator
 * support, and memory-efficient linked list implementation.
 *
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#ifndef ARDUINO_MAP_H
#define ARDUINO_MAP_H

#include <Arduino.h>
#include "Pair.h"

/**
 * @brief Lightweight map implementation for Arduino
 *
 * This template class provides a dynamic key-value store that can work with
 * any data type. It uses a linked list implementation for memory efficiency
 * and supports iterator-based iteration.
 *
 * @tparam KeyType The type of keys stored in the map
 * @tparam ValueType The type of values stored in the map
 */
template <typename KeyType, typename ValueType>
class ArduinoMap
{
private:
    /**
     * @brief Node structure for linked list implementation
     */
    struct Node
    {
        KeyType key;        ///< The key
        ValueType value;    ///< The value
        Node *next;         ///< Pointer to next node

        /**
         * @brief Constructs a new node
         * @param k The key
         * @param v The value
         */
        Node(const KeyType &k, const ValueType &v) : key(k), value(v), next(nullptr) {}
    };

    Node *head;     ///< Pointer to the first node
    size_t mapSize; ///< Current number of elements in the map

public:
    /**
     * @brief Constructs an empty map
     */
    ArduinoMap() : head(nullptr), mapSize(0) {}

    /**
     * @brief Destructor - clears all nodes
     */
    ~ArduinoMap()
    {
        clear();
    }

    /**
     * @brief Copy constructor
     * @param other The map to copy from
     */
    ArduinoMap(const ArduinoMap &other) : head(nullptr), mapSize(0)
    {
        Node *current = other.head;
        while (current)
        {
            insert(current->key, current->value);
            current = current->next;
        }
    }

    /**
     * @brief Assignment operator
     * @param other The map to assign from
     * @return Reference to this map
     */
    ArduinoMap &operator=(const ArduinoMap &other)
    {
        if (this != &other)
        {
            clear();
            Node *current = other.head;
            while (current)
            {
                insert(current->key, current->value);
                current = current->next;
            }
        }
        return *this;
    }

    /**
     * @brief Insert a key-value pair into the map
     *
     * If the key already exists, the value is updated.
     *
     * @param key The key to insert
     * @param value The value to associate with the key
     * @return true if successful, false if memory allocation failed
     */
    bool insert(const KeyType &key, const ValueType &value)
    {
        // Check if key already exists
        Node *current = head;
        while (current)
        {
            if (current->key == key)
            {
                // Update existing value
                current->value = value;
                return true;
            }
            current = current->next;
        }

        // Create new node
        Node *new_node = new Node(key, value);
        if (!new_node)
            return false;

        // Insert at the beginning
        new_node->next = head;
        head = new_node;
        mapSize++;

        return true;
    }

    /**
     * @brief Get value by key
     *
     * @param key The key to search for
     * @return Pointer to the value if found, nullptr otherwise
     */
    ValueType *get(const KeyType &key)
    {
        Node *current = head;
        while (current)
        {
            if (current->key == key)
            {
                return &(current->value);
            }
            current = current->next;
        }
        return nullptr;
    }

    /**
     * @brief Remove a key-value pair from the map
     *
     * @param key The key to remove
     * @return true if the key was found and removed, false otherwise
     */
    bool remove(const KeyType &key)
    {
        Node *current = head;
        Node *prev = nullptr;

        while (current)
        {
            if (current->key == key)
            {
                if (prev)
                {
                    prev->next = current->next;
                }
                else
                {
                    head = current->next;
                }

                delete current;
                mapSize--;
                return true;
            }

            prev = current;
            current = current->next;
        }

        return false;
    }

    /**
     * @brief Clear all entries from the map
     */
    void clear()
    {
        Node *current = head;
        while (current)
        {
            Node *next = current->next;
            delete current;
            current = next;
        }
        head = nullptr;
        mapSize = 0;
    }

    /**
     * @brief Get the number of elements in the map
     * @return The number of key-value pairs
     */
    size_t size() const
    {
        return mapSize;
    }

    /**
     * @brief Check if the map is empty
     * @return true if the map is empty, false otherwise
     */
    bool empty() const
    {
        return mapSize == 0;
    }

    /**
     * @brief Iterator class for range-based for loops
     */
    class Iterator
    {
    private:
        Node *current;

    public:
        /**
         * @brief Constructs an iterator pointing to a node
         * @param node The node to point to
         */
        explicit Iterator(Node *node) : current(node) {}

        /**
         * @brief Pre-increment operator
         * @return Reference to this iterator
         */
        Iterator &operator++()
        {
            if (current)
                current = current->next;
            return *this;
        }

        /**
         * @brief Inequality operator
         * @param other The iterator to compare with
         * @return true if iterators point to different nodes
         */
        bool operator!=(const Iterator &other) const
        {
            return current != other.current;
        }

        /**
         * @brief Dereference operator
         * @return A Pair containing the key-value pair
         */
        Pair<KeyType, ValueType> operator*()
        {
            return Pair<KeyType, ValueType>(current->key, current->value);
        }
    };

    /**
     * @brief Get iterator to the beginning of the map
     * @return Iterator pointing to the first element
     */
    Iterator begin() { return Iterator(head); }

    /**
     * @brief Get iterator to the end of the map
     * @return Iterator pointing past the last element
     */
    Iterator end() { return Iterator(nullptr); }

    /**
     * @brief Const iterator class for range-based for loops
     */
    class ConstIterator
    {
    private:
        const Node *current;

    public:
        /**
         * @brief Constructs a const iterator pointing to a node
         * @param node The node to point to
         */
        explicit ConstIterator(const Node *node) : current(node) {}

        /**
         * @brief Pre-increment operator
         * @return Reference to this iterator
         */
        ConstIterator &operator++()
        {
            if (current)
                current = current->next;
            return *this;
        }

        /**
         * @brief Inequality operator
         * @param other The iterator to compare with
         * @return true if iterators point to different nodes
         */
        bool operator!=(const ConstIterator &other) const
        {
            return current != other.current;
        }

        /**
         * @brief Dereference operator
         * @return A Pair containing the key-value pair
         */
        Pair<KeyType, ValueType> operator*() const
        {
            return Pair<KeyType, ValueType>(current->key, current->value);
        }
    };

    /**
     * @brief Get const iterator to the beginning of the map
     * @return ConstIterator pointing to the first element
     */
    ConstIterator cbegin() const { return ConstIterator(head); }

    /**
     * @brief Get const iterator to the end of the map
     * @return ConstIterator pointing past the last element
     */
    ConstIterator cend() const { return ConstIterator(nullptr); }
};

#endif // ARDUINO_MAP_H