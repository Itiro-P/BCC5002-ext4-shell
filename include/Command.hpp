#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <print>

namespace Command {
    /**
     * @brief Estrutura para armazenar informações sobre os comandos disponíveis no shell.
     */
    extern std::vector<std::pair<std::string, std::string>> command_info;

    /**
     * @brief Exibe informações da imagem e do sistema de arquivos.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short info();

    /**
     * @brief Exibe o conteúdo de um arquivo no formato texto.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo a ser exibido.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short cat(const std::vector<std::string>& args);

    /**
     * @brief Exibe os atributos de um arquivo ou diretório.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo/diretório alvo.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short attr(const std::vector<std::string>& args);

    /**
     * @brief Muda o diretório atual para o especificado.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório destino.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short cd(const std::vector<std::string>& args);

    /**
     * @brief Lista o conteúdo do diretório atual.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório a ser visualizado.
     * Caso não seja fornecido um argumento, o conteúdo do diretório atual será listado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short ls(const std::vector<std::string>& args);

    /**
     * @brief Testa se um `inode` está livre ou ocupado.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o número do inode (em formato string) a ser testado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short test_inode(const std::vector<std::string>& args);

    /**
     * @brief Testa se um `bloco` está livre ou ocupado.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o número do bloco (em formato string) a ser testado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short test_block(const std::vector<std::string>& args);

    /**
     * @brief Copia um arquivo para outro local.
     * Na especificação do trabalho, o nome do comando deveria ser `export`. Mas, ao menos no `C++23`, `export` é uma palavra reservada, então o nome do comando foi alterado para `cp`.
     * @param args O vetor de argumentos, onde `args[1]` é o caminho do arquivo de origem e `args[2]` é o caminho do destino.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short cp(const std::vector<std::string>& args);
    
    /**
     * @brief Exibe o diretório atual.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short pwd();

    /**
     * @brief Cria um arquivo vazio.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo a ser criado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short touch(const std::vector<std::string>& args);

    /**
     * @brief Cria um diretório.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório a ser criado.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short mkdir(const std::vector<std::string>& args);

    /**
     * @brief Remove um arquivo.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do arquivo a ser removido.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short rm(const std::vector<std::string>& args);

    /**
     * @brief Remove um diretório vazio.
     * @param args O vetor de argumentos, onde `args[1]` deve ser o caminho do diretório a ser removido (que deve estar vazio).
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short rmdir(const std::vector<std::string>& args);

    /**
     * @brief Renomeia um arquivo.
     * @param args O vetor de argumentos, onde `args[1]` é o caminho atual do arquivo e `args[2]` é o novo nome/caminho.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short rename(const std::vector<std::string>& args);

    /**
     * @brief Sai do programa.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short exit();
    
    /**
     * @brief Limpa o terminal.
     * @returns Um código de saída indicando o resultado da execução do comando. Normalmente, 0 para sucesso e um valor diferente de zero para erros.
     */
    short clear();
}