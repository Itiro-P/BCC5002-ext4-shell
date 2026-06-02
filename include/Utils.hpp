#pragma once

#include <type_traits>
#include <cstdint>
#include <span>

/**
 * @brief Arquivo de cabeçalho que inclui definições e declarações de utilitários comuns usados em todo o projeto.
 * @author Pedro Itiro Nagao
 */
namespace Utils {
    /**
     * @brief Concatena duas metades de bits (_lo e _hi) em um tipo inteiro maior de 64 bits.
     * Usa conceitos do C++20 para garantir que apenas tipos inteiros sejam aceitos.
     * @param lo A parte inferior (bits 0-31 ou 0-15) do valor a ser concatenado.
     * @param hi A parte superior (bits 32-63 ou 16-31) do valor a ser concatenado.
     * @return O valor concatenado resultante, com os bits de `hi` deslocados para a posição correta e combinados com `lo`.
     */
    template<typename T>
    inline constexpr uint64_t concatenate(T lo, T hi) requires std::is_integral_v<T> {
        // Quantos bits a estrutura original 'T' possui? (Ex: se for uint32_t, bits = 32)
        constexpr std::size_t bits = sizeof(T) * 8;
            
        // Fazemos o cast do 'hi' para 64 bits ANTES do shift, 
        // para evitar que os bits saiam do limite do tipo original.
        return (static_cast<uint64_t>(hi) << bits) | static_cast<uint64_t>(lo);
    }

    /**
     * @brief Cria um `std::span` a partir de um ponteiro e um tamanho, garantindo que o tipo seja tratado como bytes.
     * @param ptr O ponteiro para o início dos dados.
     * @param size O número de bytes a incluir no span.
     * @return Um `std::span<std::byte>` que abrange os dados especificados.
     */
    template<typename T>
    inline constexpr std::span<std::byte> as_span(T* ptr, std::size_t size) {
        return std::span<std::byte>(reinterpret_cast<std::byte*>(ptr), size);
    }
};