#pragma once

#include <type_traits>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <span>
#include <vector>
#include <ranges>
#include <algorithm>
#include <optional>

/**
 * @file    Utils.hpp
 * @brief   Funções utilitárias usadas no projeto.
 * @author  Pedro Itiro Nagao
 * @date    2026-06-14
 *
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

    /**
     * @brief Quebra um número pela metade para colocar em campos `lo` e `hi`.
     */
    template<typename Lo, typename Hi>
    void split(uint64_t value, Lo &lo, Hi &hi) {
        constexpr size_t lo_bits = sizeof(Lo) * 8;
        
        // O static_cast para um tipo menor descarta automaticamente os bits superiores de forma segura
        lo = static_cast<Lo>(value);
        
        // Deslocamos com segurança. Se lo_bits for >= 64, hi simplesmente recebe 0
        if constexpr (lo_bits < 64) {
            hi = static_cast<Hi>(value >> lo_bits);
        } else {
            hi = static_cast<Hi>(0);
        }
    }

    /**
     * @brief É um container
     */
    template <typename T>
    concept Container = requires(T &t) { t.data(); t.size(); };

    /**
     * @brief É um objeto (não uma coleção)
     */
    template <typename T>
    concept Object = !Container<T>;

    /**
     * @brief Lê uma estrutura inteira diretamente a partir de um container de bytes.
     * @param src O container
     * @param offset (Opcional) O offset (em bytes) para começar a escrever no container.
     * @param count (Opcional) O número de bytes que deve ser lido e copiado.
     */
    template <typename T, Container C>
    inline constexpr T copy(const C &src, 
        const std::optional<size_t> offset = std::nullopt,
        const std::optional<size_t> count  = std::nullopt) 
    {
        size_t off = offset.value_or(0);
        size_t to_copy = count.value_or(sizeof(T));

        if (src.size() < off + to_copy || sizeof(T) < to_copy)
            throw std::runtime_error("Erro de buffer: tamanho de cópia inválido ou dados insuficientes.");

        T obj{};
        std::copy_n(
            reinterpret_cast<const std::byte*>(std::to_address(src.data())) + off,
            to_copy,
            reinterpret_cast<std::byte*>(&obj)
        );
        return obj;
    }

    /**
     * @brief Escreve uma estrutura em um container de bytes.
     * @param dst Um container de destino.
     * @param src A estrutura/objeto a ser escrita no container.
     */
    template <Object O, Container C>
    inline constexpr void write_to(C &dst, const O &src) {
        if (dst.size() * sizeof(typename C::value_type) < sizeof(O))
            throw std::runtime_error("Buffer insuficiente para escrever a estrutura.");
            
        std::copy_n(reinterpret_cast<const std::byte*>(&src), sizeof(O), reinterpret_cast<std::byte*>(dst.data()));
    }

    /**
     * @brief Escreve uma estrutura em um container de bytes.
     * @param dst Um container de destino.
     * @param src A estrutura/objeto a ser escrita no container.
     * @param offset (opcional) O offset (em bytes) para começar a escrever no container.
     * @param count (opcional) Quantos bytes de src deverão ser escritos.
     */
    template <typename T>
        requires (Object<T> || std::same_as<T, std::string> || std::ranges::contiguous_range<T>)
    inline constexpr void write_to(std::span<std::byte> dst, const T &src,
        const std::optional<size_t> offset = std::nullopt,
        const std::optional<size_t> count  = std::nullopt)
    {
        size_t off = offset.value_or(0);

        const std::byte* src_ptr;
        size_t src_size;

        if constexpr (std::same_as<T, std::string>) {
            src_ptr  = reinterpret_cast<const std::byte*>(src.data());
            src_size = src.size();
        } else if constexpr (std::ranges::contiguous_range<T>) {
            src_ptr  = reinterpret_cast<const std::byte*>(std::ranges::data(src));
            src_size = std::ranges::size(src) * sizeof(std::ranges::range_value_t<T>);
        } else {
            src_ptr  = reinterpret_cast<const std::byte*>(&src);
            src_size = sizeof(T);
        }

        size_t to_write = count.value_or(src_size);
        if (off + to_write > dst.size())
            throw std::runtime_error("Buffer insuficiente para escrever.");

        std::memcpy(dst.data() + off, src_ptr, to_write);
    }

    /**
     * @brief Cria um `std::span<std::byte>` a partir de um container (será interpretado como bytes puros).
     * @param c O container.
     * @param offset (opcional) O offset (em elementos) para começar a ler do container.
     * @param count (opcional) Quantos bytes deverão ser incluidos no `std::span`.
     */
    template <Container C>
    inline constexpr auto as_byte_span(C &&c, 
        const std::optional<size_t> offset = std::nullopt, 
        const std::optional<size_t> count = std::nullopt
    ) {
        using Elem = std::conditional_t<std::is_const_v<std::remove_reference_t<C>>, const std::byte, std::byte>;
        size_t available = c.size() - offset.value_or(0);
        size_t to_copy = (count && *count > 0 ? *count : available) * sizeof(typename std::remove_cvref_t<C>::value_type);
        return std::span<Elem>(reinterpret_cast<Elem*>(c.data() + offset.value_or(0)), to_copy);
    }
    
    /**
     * @brief Cria um `std::span<std::byte>` a partir de uma referência para um objeto, garantindo que o tipo seja tratado como bytes.
     * @param o O objeto.
     * @param size (Opcional) O tamanho da estrutura (Estruturas como o grupo de descritores podem ter mais de um tamanho a depender da arquitetura 32 ou 64 bits por exemplo).
     * @returns Um `std::span<std::byte>` que abrange os dados especificados.
     */
    template <Object O> requires (!Container<O>)
    inline constexpr auto as_byte_span(O &&o, const std::optional<size_t> size = std::nullopt) {
        using Elem = std::conditional_t<std::is_const_v<std::remove_reference_t<O>>, const std::byte, std::byte>;
        return std::span<Elem>(reinterpret_cast<Elem*>(&o), size.value_or(sizeof(O)));
    }

    /**
     * @brief Cria um `std::span<T>` a partir de um container.
     * @param c O container.
     * @param offset (opcional) O offset (em elementos) para começar a ler do container.
     * @param count (opcional) Quantos elementos deverão ser incluidos no `std::span`.
     */
    template <typename T, Container C>
    inline constexpr auto as_span(C &&c, const std::optional<size_t> offset = std::nullopt, const std::optional<size_t> count = std::nullopt) {
        using Elem = std::conditional_t<std::is_const_v<std::remove_reference_t<C>>, const T, T>;
        size_t available = c.size() - offset.value_or(0);
        size_t to_copy = (count && *count > 0 ? *count : available) * sizeof(typename std::remove_cvref_t<C>::value_type) / sizeof(T);
        return std::span<Elem>(reinterpret_cast<Elem*>(c.data() + offset.value_or(0)), to_copy);
    }

    /**
     * @brief Cria um `std::span<T>` a partir de um objeto.
     * @param o O objeto.
     * @param size (Opcional) O tamanho real da estrutura em bytes (útil quando a estrutura
     *             pode ter tamanho variável, como inodes com tamanho menor que o alocado).
     */
    template <typename T, typename O> requires (!Container<O>)
    inline constexpr auto as_span(O &&o, const std::optional<size_t> size = std::nullopt) {
        using Elem = std::conditional_t<std::is_const_v<std::remove_reference_t<O>>, const T, T>;
        const std::size_t to_copy = size.value_or(sizeof(O)) / sizeof(T);
        return std::span<Elem>(reinterpret_cast<Elem*>(&o), to_copy);
    }

    /**
     * @brief "Desloca" um número para seu múltiplo de 4 mais próximo.
     * Truquezinho: Múltiplos de 4 sempre terminam com 00 à direita.
     * - Somar 3 a um número faz com que ele entre na "próxima" casa de um múltiplo de 4.
     * - Mascarar com o complemento de 1 do 3 resulta na limpeza dos primeiros 2 bits à direita.
     */
    template <std::integral T>
    inline constexpr T to_4bit_aligned(const T &to_align) {
        return (to_align + 3) & ~3;
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
        if(val) reinterpret_cast<uint8_t*>(bitmap_block.data())[idx/8] |= (1 << idx%8);  // Força o bit a virar 1
        else reinterpret_cast<uint8_t*>(bitmap_block.data())[idx/8] &= ~(1 << idx%8); // Força o bit a virar 0
    }

    /**
     * @brief Separa o nome do arquivo com o diretório. Útil caso o usuário passe um diretório junto.
     * @param str A string alvo.
     * @returns Um `std::pair<>{diretório, arquivo}`.
     */
    inline constexpr std::pair<std::string, std::string> split_path(const std::string& str) {
        size_t bar = str.find_last_of('/');

        if (bar == std::string::npos) return {"", str};

        if (bar == 0) return {"/", str.substr(1)};

        return {str.substr(0, bar), str.substr(bar + 1)};
    }
};