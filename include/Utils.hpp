#pragma once

#include <type_traits>
#include <cstdint>
#include <stdexcept>
#include <span>
#include <vector>
#include <ranges>
#include <algorithm>

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
     * @returns O valor concatenado resultante, com os bits de `hi` deslocados para a posição correta e combinados com `lo`.
     */
    template<typename T>
    inline constexpr uint64_t concatenate(T lo, T hi) requires std::is_integral_v<T> {
        // Quantos bits a estrutura original 'T' possui? (Ex: se for uint32_t, bits = 32)
        constexpr std::size_t bits = sizeof(T) * 8;
            
        // Fazemos o cast do 'hi' para 64 bits ANTES do shift, 
        // para evitar que os bits saiam do limite do tipo original.
        return (static_cast<uint64_t>(hi) << bits) | static_cast<uint64_t>(lo);
    }

    template <typename T>
    concept Container = requires(T &t) { t.data(); t.size(); };

    template <typename T>
    concept Object = !Container<T>;

    /**
     * @brief Lê uma estrutura inteira diretamente a partir de um container de bytes de forma segura.
     */
    template <typename T, Container C>
    inline constexpr T copy(const C &src) {
        if (src.size() < sizeof(T)) {
            throw std::runtime_error("Erro de buffer: dados insuficientes para ler a estrutura.");
        }
        
        T obj{};
        std::copy_n(std::to_address(src.data()), sizeof(T), reinterpret_cast<std::byte*>(&obj));
        return obj;
    }

    /**
     * @brief Lê uma estrutura copiando apenas 'n' bytes a partir de um container.
     */
    template <typename T, Container C, typename S = std::size_t>
    inline constexpr T copy(const C &src, const S n) {
        const std::size_t bytes_to_copy = static_cast<std::size_t>(n);

        if (src.size() < bytes_to_copy || sizeof(T) < bytes_to_copy) {
            throw std::runtime_error("Erro de buffer: tamanho de cópia inválido ou dados insuficientes.");
        }
        
        T obj{}; // Garante que bytes não copiados fiquem zerados
        
        std::copy_n(std::to_address(src.data()), bytes_to_copy, reinterpret_cast<std::byte*>(&obj));
        
        return obj;
    }

    /**
     * @brief Lê uma estrutura copiando apenas 'n' bytes a partir de um span.
     */
    template <typename T, typename S = std::size_t>
    inline constexpr T copy(std::span<const std::byte> src, const S n) {
        if (src.size() < static_cast<std::size_t>(n) || sizeof(T) < static_cast<std::size_t>(n)) {
            throw std::runtime_error("Erro de buffer: tamanho de cópia inválido ou dados insuficientes.");
        }
        
        T obj{};
        
        std::copy_n(std::to_address(src.data()), n, reinterpret_cast<std::byte*>(&obj));
        return obj;
    }

    /**
     * @brief Lê uma estrutura a partir de um container, limitando a cópia automaticamente 
     * ao tamanho da estrutura ou a um limite máximo informado (o que for menor).
     */
    template <typename T, Container C>
    inline constexpr T copy_bounded(const C &src, std::size_t max_size) {
        std::size_t bytes_to_copy = std::min(sizeof(T), max_size);
        
        return copy<T>(src, bytes_to_copy);
    }

    /**
     * @brief Cria um `std::span<std::byte>` a partir de uma referência para um container, garantindo que o tipo seja tratado como bytes.
     * @param c O container.
     * @returns Um `std::span<std::byte>` que abrange os dados especificados.
     */
    inline constexpr auto as_span(Container auto &c) {
        using ContainerType = std::remove_cvref_t<decltype(c)>;
        return std::span(reinterpret_cast<std::byte*>(c.data()), c.size() * sizeof(typename ContainerType::value_type));
    }

    /**
     * @brief Cria um `std::span<const std::byte>` a partir de uma referência para um container, garantindo que o tipo seja tratado como bytes.
     * @param c Ocontainer.
     * @returns Um `std::span<const std::byte>` que abrange os dados especificados.
     */
    inline constexpr auto as_span(const Container auto &c) {
        using ContainerType = std::remove_cvref_t<decltype(c)>;
        return std::span(reinterpret_cast<const std::byte*>(c.data()), c.size() * sizeof(typename ContainerType::value_type));
    }

    /**
     * @brief Cria um `std::span<std::byte>` a partir de uma referência para um objeto, garantindo que o tipo seja tratado como bytes.
     * @param c O objeto.
     * @returns Um `std::span<std::byte>` que abrange os dados especificados.
     */
    inline constexpr auto as_span(Object auto &o) {
        return std::span(reinterpret_cast<std::byte*>(&o), sizeof(o));
    }

    /**
     * @brief Cria um `std::span<const std::byte>` a partir de uma referência para um objeto, garantindo que o tipo seja tratado como bytes.
     * @param c O objeto.
     * @returns Um `std::span<const std::byte>` que abrange os dados especificados.
     */
    inline constexpr auto as_span(const Object auto &o) {
        return std::span(reinterpret_cast<const std::byte*>(&o), sizeof(o));
    }

    /**
     * @brief Separa (tokeniza) um `std::string` em um vetor de strings usando como base um delimitador.
     * @param str A string alvo.
     * @param delimiter O delimitador, por padrão é um espaço (` `).
     */
    inline constexpr std::vector<std::string> filter_split(const std::string &str, const std::string& delimiter = " ") {
        // Usamos ranges para dividir a string em pedaços menores, usando delimiter como delimitador.
        // Colocamos a string alvo em um std::string_view para evitar cópias desnecessárias, e depois transformamos cada argumento em std::string.
        return str | 
        // Quebramos a linha em tokens
        std::views::split(delimiter) | 
        // Eliminamos (filtramos) as strings vazias ou que contenham apenas espaços.
        std::views::filter([](auto&& arg) { 
            return !arg.empty() && std::none_of(arg.begin(), arg.end(), isspace); 
        }) | 
        // Transformamos cada argumento em uma string normal.
        std::views::transform([](auto&& arg) { 
            return std::string(arg.begin(), arg.end()); 
        }) |
        // E colocamos em um vetor de strings.
        std::ranges::to<std::vector>();
    }

    /**
     * @brief Tokeniza uma string considerando que substrings com aspas devem ficar juntas.
     * @param s A string.
     * @returns Um vetor de strings tokenizadas.
     */
    inline constexpr std::vector<std::string> tokenize(std::string_view s) {
        std::vector<std::string> tokens;
        std::string current;
        char inQuote = 0;

        for (const auto &ch : s) {
            if (inQuote && ch == inQuote) {
                inQuote = 0;
            } else if (!inQuote && (ch == '\'' || ch == '"')) {
                inQuote = ch;
            } else if (!inQuote && ch == ' ') {
                if (!current.empty()) {
                    tokens.push_back(std::move(current));
                    current.clear();
                }
            } else {
                current += ch;
            }
        }

        if (!current.empty()) tokens.push_back(std::move(current));

        return tokens;
    }

    /**
     * @brief Retorna `true` se o bit está ativo. `false` caos contrário.
     */
    inline constexpr bool test_bit(const std::span<const std::byte> bitmap_block, const uint32_t idx) {
        return ((reinterpret_cast<const uint8_t*>(bitmap_block.data())[idx/8] >> (idx % 8)) & 1) == 1;
    }

    /**
     * @brief Seta o bit de um bitmap na posição `idx` como 0 ou 1.
     * @param bitmap_block O bloco de bitmap.
     * @param idx O índice do bit.
     * @param val `true` se forçar ativação. `false` caso contrário.
     */
    inline constexpr void set_bit(std::span<std::byte> bitmap_block, const uint32_t idx, const bool val) {       
        if (val) reinterpret_cast<uint8_t*>(bitmap_block.data())[idx/8] |= (1 << idx%8);  // Força o bit a virar 1
        else reinterpret_cast<uint8_t*>(bitmap_block.data())[idx/8] &= ~(1 << idx%8); // Força o bit a virar 0
    }
};