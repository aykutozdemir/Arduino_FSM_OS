/**
 * @file Pair.h
 * @brief Template implementation for Pair class
 *
 * This file defines a template Pair structure for storing two values together.
 * It is used by ArduinoMap for iterator operations.
 *
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#ifndef PAIR_H
#define PAIR_H

/**
 * @brief Template structure for storing a pair of values
 *
 * @tparam T1 Type of the first value
 * @tparam T2 Type of the second value
 */
template <typename T1, typename T2>
struct alignas(alignof(T1) > alignof(T2) ? alignof(T1) : alignof(T2)) Pair
{
    T1 first;  ///< The first value
    T2 second;  ///< The second value

    /**
     * @brief Default constructor
     */
    Pair() : first(), second() {}

    /**
     * @brief Constructor with initial values
     * @param a The first value
     * @param b The second value
     */
    Pair(const T1 a, const T2 b) : first(a), second(b) {}
};

/**
 * @brief Helper function to create a Pair
 *
 * @tparam T1 Type of the first value
 * @tparam T2 Type of the second value
 * @param a The first value
 * @param b The second value
 * @return Pair<T1, T2> A Pair containing the two values
 */
template <typename T1, typename T2>
Pair<T1, T2> make_pair(T1 a, T2 b)
{
    return Pair<T1, T2>(a, b);
}

#endif
